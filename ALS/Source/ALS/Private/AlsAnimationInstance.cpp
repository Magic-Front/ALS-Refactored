#include "AlsAnimationInstance.h"

#include "AlsCharacter.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Utility/AlsMath.h"
#include "Utility/AlsUtility.h"

const FCollisionObjectQueryParams UAlsAnimationInstance::GroundPredictionObjectQueryParameters{
	ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic) | ECC_TO_BITFIELD(ECC_Destructible)
};

void UAlsAnimationInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	AlsCharacter = Cast<AAlsCharacter>(GetOwningActor());
}

void UAlsAnimationInstance::NativeUpdateAnimation(const float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (DeltaTime <= SMALL_NUMBER || !IsValid(AlsCharacter))
	{
		return;
	}

	const auto PreviousLocomotionMode{LocomotionMode};

	Stance = AlsCharacter->GetStance();
	Gait = AlsCharacter->GetGait();
	RotationMode = AlsCharacter->GetRotationMode();
	ViewMode = AlsCharacter->GetViewMode();
	OverlayMode = AlsCharacter->GetOverlayMode();
	LocomotionMode = AlsCharacter->GetLocomotionMode();
	LocomotionAction = AlsCharacter->GetLocomotionAction();

	RefreshLocomotion(DeltaTime);
	RefreshLayering();
	RefreshAiming(DeltaTime);
	RefreshFeet(DeltaTime);

	if (PreviousLocomotionMode.IsRagdolling() && !LocomotionMode.IsRagdolling())
	{
		StopRagdolling();
	}

	if (LocomotionMode.IsGrounded())
	{
		if (LocomotionState.bMoving)
		{
			MovementDirection = CalculateMovementDirection();

			RefreshMovement(DeltaTime);
		}

		RefreshDynamicTransitions();

		RefreshRotateInPlace();
		RefreshTurnInPlace(DeltaTime);

		return;
	}

	RefreshInAir(DeltaTime);
	RefreshRagdolling();
}

void UAlsAnimationInstance::RefreshLocomotion(const float DeltaTime)
{
	LocomotionState.bHasInput = AlsCharacter->GetLocomotionState().bHasInput;

	if (LocomotionState.bHasInput && RotationMode.IsVelocityDirection())
	{
		// Get the delta between the current input yaw angle and character rotation and map it to a range
		// from 0 to 1. This value is used in the aiming to make the character look toward the current input.

		const auto InputYawAngle{
			FRotator::NormalizeAxis(
				AlsCharacter->GetLocomotionState().InputYawAngle - AlsCharacter->GetLocomotionState().SmoothRotation.Yaw)
		};

		const auto InputYawAmount{(InputYawAngle / 180.0f + 1.0f) * 0.5f};

		LocomotionState.InputYawAmount = UAlsMath::ExponentialDecay(LocomotionState.InputYawAmount, InputYawAmount,
		                                                            GeneralSettings.InputYawAmountInterpolationSpeed, DeltaTime);
	}

	LocomotionState.bHasSpeed = AlsCharacter->GetLocomotionState().bHasSpeed;
	LocomotionState.Speed = AlsCharacter->GetLocomotionState().Speed;
	LocomotionState.Velocity = AlsCharacter->GetLocomotionState().Velocity;
	LocomotionState.VelocityYawAngle = AlsCharacter->GetLocomotionState().VelocityYawAngle;

	LocomotionState.bMoving = AlsCharacter->GetLocomotionState().bMoving;

	LocomotionState.GaitAmount = GetCurveValue(Constants.GaitAmountCurve);
	LocomotionState.GaitWalkingAmount = UAlsMath::Clamp01(LocomotionState.GaitAmount);
	LocomotionState.GaitRunningAmount = UAlsMath::Clamp01(LocomotionState.GaitAmount - 1.0f);
	LocomotionState.GaitSprintingAmount = UAlsMath::Clamp01(LocomotionState.GaitAmount - 2.0f);

	// The allow transitions curve is modified within certain states, so that allow transition will be true while in those states.

	LocomotionState.bAllowTransitions = GetCurveValue(Constants.AllowTransitionsCurve) >= 0.99f;

	// Allow movement animations if character is moving.

	if (!LocomotionState.bMoving || !LocomotionMode.IsGrounded())
	{
		return;
	}

	// Calculate the relative acceleration amount. This value represents the current amount of acceleration / deceleration
	// relative to the actor rotation. It is normalized to a range of -1 to 1 so that -1 equals the max
	// braking deceleration, and 1 equals the max acceleration of the character movement component.

	const auto Acceleration{AlsCharacter->GetLocomotionState().Acceleration};

	const auto* CharacterMovement{AlsCharacter->GetCharacterMovement()};

	if ((Acceleration | LocomotionState.Velocity) > 0.0f)
	{
		LocomotionState.RelativeAccelerationAmount = AlsCharacter->GetLocomotionState().SmoothRotation.UnrotateVector(
			UAlsMath::ClampMagnitude01(Acceleration / CharacterMovement->GetMaxAcceleration()));
	}
	else
	{
		LocomotionState.RelativeAccelerationAmount = AlsCharacter->GetLocomotionState().SmoothRotation.UnrotateVector(
			UAlsMath::ClampMagnitude01(Acceleration / CharacterMovement->GetMaxBrakingDeceleration()));
	}

	// Set the rotation yaw offsets. These values influence the rotation yaw offset curve in the
	// animation graph and are used to offset the character's rotation for more natural movement.
	// The curves allow for fine control over how the offset behaves for each movement direction.

	const auto RotationYawOffset{
		FRotator::NormalizeAxis(LocomotionState.VelocityYawAngle - AlsCharacter->GetAimingState().SmoothRotation.Yaw)
	};

	LocomotionState.RotationYawOffsetForward = GeneralSettings.RotationYawOffsetForwardCurve->GetFloatValue(RotationYawOffset);
	LocomotionState.RotationYawOffsetBackward = GeneralSettings.RotationYawOffsetBackwardCurve->GetFloatValue(RotationYawOffset);
	LocomotionState.RotationYawOffsetLeft = GeneralSettings.RotationYawOffsetLeftCurve->GetFloatValue(RotationYawOffset);
	LocomotionState.RotationYawOffsetRight = GeneralSettings.RotationYawOffsetRightCurve->GetFloatValue(RotationYawOffset);

	// Use the relative acceleration as the sprint relative acceleration if less than 0.5 seconds has elapsed
	// since the start of the sprint, otherwise set the sprint relative acceleration to zero.This is
	// necessary in order to apply the acceleration animation only at the beginning of the sprint.

	if (Gait.IsSprinting())
	{
		LocomotionState.SprintTime += DeltaTime;
		LocomotionState.SprintRelativeAccelerationAmount = LocomotionState.SprintTime < 0.5f
			                                                   ? LocomotionState.RelativeAccelerationAmount.X
			                                                   : 0.0f;
	}
	else
	{
		LocomotionState.SprintTime = 0.0f;
		LocomotionState.SprintRelativeAccelerationAmount = 0.0f;
	}
}

void UAlsAnimationInstance::RefreshLayering()
{
	LayeringState.HeadBlendAmount = GetCurveValueClamped01(Constants.LayerHeadCurve);
	LayeringState.HeadAdditiveBlendAmount = GetCurveValueClamped01(Constants.LayerHeadAdditiveCurve);

	// The mesh space blend will always be 1 unless the local space blend is 1.

	LayeringState.ArmLeftBlendAmount = GetCurveValueClamped01(Constants.LayerArmLeftCurve);
	LayeringState.ArmLeftAdditiveBlendAmount = GetCurveValueClamped01(Constants.LayerArmLeftAdditiveCurve);
	LayeringState.ArmLeftLocalSpaceBlendAmount = GetCurveValueClamped01(Constants.LayerArmLeftLocalSpaceCurve);
	LayeringState.ArmLeftMeshSpaceBlendAmount = 1.0f - FMath::FloorToInt(LayeringState.ArmLeftLocalSpaceBlendAmount);

	// The mesh space blend will always be 1 unless the local space blend is 1.

	LayeringState.ArmRightBlendAmount = GetCurveValueClamped01(Constants.LayerArmRightCurve);
	LayeringState.ArmRightAdditiveBlendAmount = GetCurveValueClamped01(Constants.LayerArmRightAdditiveCurve);
	LayeringState.ArmRightLocalSpaceBlendAmount = GetCurveValueClamped01(Constants.LayerArmRightLocalSpaceCurve);
	LayeringState.ArmRightMeshSpaceBlendAmount = 1.0f - FMath::FloorToInt(LayeringState.ArmRightLocalSpaceBlendAmount);

	LayeringState.HandLeftBlendAmount = GetCurveValueClamped01(Constants.LayerHandLeftCurve);
	LayeringState.HandRightBlendAmount = GetCurveValueClamped01(Constants.LayerHandRightCurve);

	LayeringState.SpineBlendAmount = GetCurveValueClamped01(Constants.LayerSpineCurve);
	LayeringState.SpineAdditiveBlendAmount = GetCurveValueClamped01(Constants.LayerSpineAdditiveCurve);

	LayeringState.PelvisBlendAmount = GetCurveValueClamped01(Constants.LayerPelvisCurve);
	LayeringState.LegsBlendAmount = GetCurveValueClamped01(Constants.LayerLegsCurve);

	LayeringState.PoseStandingBlendAmount = GetCurveValueClamped01(Constants.PoseStandCurve);
	LayeringState.PoseCrouchingBlendAmount = GetCurveValueClamped01(Constants.PoseCrouchCurve);
}

void UAlsAnimationInstance::RefreshAiming(const float DeltaTime)
{
	if (LocomotionAction.IsNone())
	{
		AimingState.Rotation = AlsCharacter->GetAimingState().SmoothRotation;

		const auto AimingRotation{AimingState.Rotation - AlsCharacter->GetLocomotionState().SmoothRotation};

		AimingState.YawAngle = FRotator::NormalizeAxis(AimingRotation.Yaw);
		AimingState.PitchAngle = FRotator::NormalizeAxis(AimingRotation.Pitch);
	}

	AimingState.YawSpeed = AlsCharacter->GetAimingState().YawSpeed;

	// Interpolate the aiming rotation value to achieve smooth aiming rotation changes. Interpolating
	// the rotation before calculating the angle ensures the value is not affected by changes in
	// actor rotation, allowing slow aiming rotation changes with fast actor rotation changes.

	AimingState.SmoothRotation = UAlsMath::ExponentialDecay(AimingState.SmoothRotation, AimingState.Rotation,
	                                                        GeneralSettings.AimingSmoothRotationInterpolationSpeed, DeltaTime);

	const auto AimingSmoothRotation{AimingState.SmoothRotation - AlsCharacter->GetLocomotionState().SmoothRotation};

	AimingState.SmoothYawAngle = FRotator::NormalizeAxis(AimingSmoothRotation.Yaw);
	AimingState.SmoothPitchAngle = FRotator::NormalizeAxis(AimingSmoothRotation.Pitch);

	// Separate the Smooth aiming yaw angle into 3 separate values. These 3 values are used to
	// improve the blending of the aiming when rotating completely around the character. This allows
	// you to keep the aiming responsive but still smoothly blend from left to right or right to left.

	AimingState.SmoothYawAmount = (AimingState.SmoothYawAngle / 180.0f + 1.0f) * 0.5f;
	AimingState.SmoothYawLeftAmount = FMath::GetMappedRangeValueClamped({0.0f, 180.0f}, {0.5f, 0.0f},
	                                                                    FMath::Abs(AimingState.SmoothYawAngle));
	AimingState.SmoothYawRightAmount = FMath::GetMappedRangeValueClamped({0.0f, 180.0f}, {0.5f, 1.0f},
	                                                                     FMath::Abs(AimingState.SmoothYawAngle));

	if (!RotationMode.IsVelocityDirection())
	{
		AimingState.PitchAmount = FMath::GetMappedRangeValueClamped({-90.0f, 90.0f}, {1.0f, 0.0f}, AimingState.PitchAngle);
	}

	AimingState.LookAmount = (1.0f - GetCurveValueClamped01(Constants.AimManualCurve)) *
	                         (1.0f - GetCurveValueClamped01(Constants.AimBlockCurve));
}

void UAlsAnimationInstance::RefreshFeet(const float DeltaTime)
{
	FeetState.FootPlanted = FMath::Clamp(GetCurveValue(Constants.FootPlantedCurve), -1.0f, 1.0f);

	FeetState.Left.IkAmount = GetCurveValueClamped01(Constants.FootLeftIkCurve);
	FeetState.Right.IkAmount = GetCurveValueClamped01(Constants.FootRightIkCurve);

	RefreshFootLock(FeetState.Left, Constants.FootLeftVirtualBone, Constants.FootLeftLockCurve, DeltaTime);
	RefreshFootLock(FeetState.Right, Constants.FootRightVirtualBone, Constants.FootRightLockCurve, DeltaTime);

	if (LocomotionMode.IsInAir())
	{
		ResetFootOffset(FeetState.Left, DeltaTime);
		ResetFootOffset(FeetState.Right, DeltaTime);

		RefreshPelvisOffset(DeltaTime, FVector::ZeroVector, FVector::ZeroVector);
		return;
	}

	if (LocomotionMode.IsRagdolling())
	{
		return;
	}

	auto TargetFootLeftLocationOffset{FVector::ZeroVector};
	RefreshFootOffset(FeetState.Left, TargetFootLeftLocationOffset, DeltaTime);

	auto TargetFootRightLocationOffset{FVector::ZeroVector};
	RefreshFootOffset(FeetState.Right, TargetFootRightLocationOffset, DeltaTime);

	RefreshPelvisOffset(DeltaTime, TargetFootLeftLocationOffset, TargetFootRightLocationOffset);
}

void UAlsAnimationInstance::RefreshFootLock(FAlsFootState& FootState, const FName& FootBoneName,
                                            const FName& FootLockCurveName, const float DeltaTime) const
{
	if (FootState.IkAmount <= SMALL_NUMBER)
	{
		return;
	}

	const auto FootTransform{GetSkelMeshComponent()->GetSocketTransform(FootBoneName)};
	const auto NewFootLockAmount{GetCurveValueClamped01(FootLockCurveName)};

	if (FeetSettings.bDisableFootLock || NewFootLockAmount <= 0.0f)
	{
		FootState.LockAmount = 0.0f;

		FootState.LockLocation = FootTransform.GetLocation();
		FootState.LockRotation = FootTransform.GetRotation();

		FootState.FinalLocation = FootState.LockLocation;
		FootState.FinalRotation = FootState.LockRotation;
		return;
	}

	const auto& ComponentTransform{GetSkelMeshComponent()->GetComponentTransform()};

	const auto bNewAmountIsEqualOne{NewFootLockAmount >= 1.0f};
	const auto bNewAmountIsLessThanPrevious{NewFootLockAmount <= FootState.LockAmount};

	// Only update the foot lock amount if the new value is less than the current, or it equals 1. This makes it
	// so that the foot can only blend out of the locked position or lock to a new position, and never blend in.

	if (bNewAmountIsEqualOne || bNewAmountIsLessThanPrevious)
	{
		FootState.LockAmount = NewFootLockAmount;

		// If the new foot lock amount equals 1 and the previous is less than 1, save the new lock location and rotation.

		if (bNewAmountIsEqualOne && !bNewAmountIsLessThanPrevious)
		{
			const auto FootRelativeTransform{FootTransform.GetRelativeTransform(ComponentTransform)};

			FootState.LockRelativeLocation = FootRelativeTransform.GetLocation();
			FootState.LockRelativeRotation = FootRelativeTransform.GetRotation();
		}
	}

	// Get the distance traveled between frames relative to the mesh rotation to determine how much the foot should be offset
	// to remain planted on the ground, and subtract it from the current relative location to get the new relative location.

	FootState.LockRelativeLocation -= ComponentTransform.InverseTransformVector(LocomotionState.Velocity * DeltaTime);

	// Use the difference between the current and previous rotations to determine
	// how much the foot should be rotated to remain planted on the ground.

	if (LocomotionMode.IsGrounded())
	{
		const auto RotationDifference{
			(AlsCharacter->GetLocomotionState().PreviousSmoothRotation - AlsCharacter->GetLocomotionState().SmoothRotation).Quaternion()
		};

		// Subtract the rotation difference from the current relative rotation to get the new relative rotation.

		FootState.LockRelativeRotation = RotationDifference * FootState.LockRelativeRotation;

		// Rotate the relative location by the rotation difference to keep the foot planted in component space.

		FootState.LockRelativeLocation = RotationDifference.RotateVector(FootState.LockRelativeLocation);
	}

	FootState.LockLocation = ComponentTransform.TransformPosition(FootState.LockRelativeLocation);
	FootState.LockRotation = ComponentTransform.TransformRotation(FootState.LockRelativeRotation);

	FootState.FinalLocation = FMath::Lerp(FootTransform.GetLocation(), FootState.LockLocation, FootState.LockAmount);
	FootState.FinalRotation = FQuat::Slerp(FootTransform.GetRotation(), FootState.LockRotation, FootState.LockAmount);
}

void UAlsAnimationInstance::RefreshFootOffset(FAlsFootState& FootState, FVector& TargetLocationOffset, const float DeltaTime) const
{
	if (FootState.IkAmount <= SMALL_NUMBER)
	{
		TargetLocationOffset = FVector::ZeroVector;
		return;
	}

	// Trace downward from the foot location to find the geometry. If the surface is walkable, save the impact location and normal.

	auto FootLocation{FootState.FinalLocation};
	FootLocation.Z = GetSkelMeshComponent()->GetSocketLocation(Constants.RootBone).Z;

	FHitResult Hit;
	GetWorld()->LineTraceSingleByChannel(Hit,
	                                     FootLocation + FVector{0.0f, 0.0f, FeetSettings.IkTraceDistanceUpward},
	                                     FootLocation - FVector{0.0f, 0.0f, FeetSettings.IkTraceDistanceDownward},
	                                     UEngineTypes::ConvertToCollisionChannel(FeetSettings.IkTraceChannel),
	                                     {__FUNCTION__, true, AlsCharacter});

#if ENABLE_DRAW_DEBUG
	if (UAlsUtility::ShouldDisplayDebug(AlsCharacter, UAlsConstants::TracesDisplayName()))
	{
		UAlsUtility::DrawDebugLineTraceSingle(GetWorld(), Hit.TraceStart, Hit.TraceEnd,
		                                      AlsCharacter->GetCharacterMovement()->IsWalkable(Hit),
		                                      Hit, {0.0f, 0.25f, 1.0f}, {0.0f, 0.75f, 1.0f});
	}
#endif

	FQuat TargetRotationOffset;

	if (AlsCharacter->GetCharacterMovement()->IsWalkable(Hit))
	{
		// Find the difference in location from the impact location and the expected (flat) floor location. These values
		// are offset by the impact normal multiplied by the foot height to get better behavior on angled surfaces.

		TargetLocationOffset = Hit.ImpactPoint +
		                       Hit.ImpactNormal * FeetSettings.FootHeight -
		                       FootLocation;

		TargetLocationOffset.Z -= FeetSettings.FootHeight;

		// Calculate the rotation offset.

		TargetRotationOffset = FRotator{
			-UAlsMath::DirectionToAngle({Hit.ImpactNormal.Z, Hit.ImpactNormal.X}),
			0.0f,
			UAlsMath::DirectionToAngle({Hit.ImpactNormal.Z, Hit.ImpactNormal.Y})
		}.Quaternion();
	}
	else
	{
		TargetRotationOffset = FQuat::Identity;
	}

	// Interpolate the current location offset to the new target value. Interpolate at
	// different speeds based on whether the new target is above or below the current one.

	FootState.OffsetLocation = FMath::VInterpTo(FootState.OffsetLocation, TargetLocationOffset, DeltaTime,
	                                            FootState.OffsetLocation.Z > TargetLocationOffset.Z ? 30.0f : 15.0f);

	// Interpolate the current rotation offset to the new target value.

	FootState.OffsetRotation = FMath::QInterpTo(FootState.OffsetRotation, TargetRotationOffset, DeltaTime, 30.0f);

	FootState.FinalLocation += FootState.OffsetLocation;
	FootState.FinalRotation = FootState.OffsetRotation * FootState.FinalRotation;
}

void UAlsAnimationInstance::ResetFootOffset(FAlsFootState& FootState, const float DeltaTime)
{
	FootState.OffsetLocation = FMath::VInterpTo(FootState.OffsetLocation, FVector::ZeroVector, DeltaTime, 15.0f);
	FootState.OffsetRotation = FMath::QInterpTo(FootState.OffsetRotation, FQuat::Identity, DeltaTime, 15.0f);

	FootState.FinalLocation += FootState.OffsetLocation;
	FootState.FinalRotation = FootState.OffsetRotation * FootState.FinalRotation;
}

void UAlsAnimationInstance::RefreshPelvisOffset(const float DeltaTime, const FVector& TargetFootLeftLocationOffset,
                                                const FVector& TargetFootRightLocationOffset)
{
	// Calculate the pelvis offset amount by finding the average foot ik weight. If the amount is 0, clear the offset.

	FeetState.PelvisOffsetAmount = (FeetState.Left.IkAmount + FeetState.Right.IkAmount) * 0.5f;

	if (FeetState.PelvisOffsetAmount <= SMALL_NUMBER)
	{
		FeetState.PelvisOffsetLocation = FVector::ZeroVector;
		return;
	}

	// Set the new location offset to be the lowest foot offset.

	const auto TargetOffset{
		TargetFootLeftLocationOffset.Z < TargetFootRightLocationOffset.Z
			? TargetFootLeftLocationOffset
			: TargetFootRightLocationOffset
	};

	// Interpolate the current location offset to the new target value. Interpolate at
	// different speeds based on whether the new target is above or below the current one.

	FeetState.PelvisOffsetLocation = FMath::VInterpTo(FeetState.PelvisOffsetLocation, TargetOffset, DeltaTime,
	                                                  TargetOffset.Z > FeetState.PelvisOffsetLocation.Z ? 10.0f : 15.0f);
}

EAlsMovementDirection UAlsAnimationInstance::CalculateMovementDirection() const
{
	// Calculate the movement direction. This value represents the direction the character is moving relative to the camera
	// during the looking direction / aiming modes, and is used in the cycle blending to blend to the appropriate directional states.

	if (Gait.IsSprinting() || RotationMode.IsVelocityDirection())
	{
		return EAlsMovementDirection::Forward;
	}

	return UAlsMath::CalculateMovementDirection(
		FRotator::NormalizeAxis(LocomotionState.VelocityYawAngle - AimingState.Rotation.Yaw),
		70.0f, 5.0f);
}

void UAlsAnimationInstance::RefreshMovement(const float DeltaTime)
{
	RefreshVelocityBlend(DeltaTime);

	MovementState.StrideBlendAmount = CalculateStrideBlendAmount();
	MovementState.WalkRunBlendAmount = CalculateWalkRunBlendAmount();
	MovementState.StandingPlayRate = CalculateStandingPlayRate();
	MovementState.CrouchingPlayRate = CalculateCrouchingPlayRate();

	// Interpolate the lean amount.

	LeanState.RightAmount = FMath::FInterpTo(LeanState.RightAmount, LocomotionState.RelativeAccelerationAmount.Y, DeltaTime,
	                                         GeneralSettings.LeanInterpolationSpeed);

	LeanState.ForwardAmount = FMath::FInterpTo(LeanState.ForwardAmount, LocomotionState.RelativeAccelerationAmount.X, DeltaTime,
	                                           GeneralSettings.LeanInterpolationSpeed);
}

void UAlsAnimationInstance::RefreshVelocityBlend(const float DeltaTime)
{
	// Calculate and interpolate the velocity blend. This value represents the velocity amount of the
	// actor in each direction (normalized so that diagonals equal 0.5 for each direction), and is
	// used in a blend multi node to produce better directional blending than a standard blend space.

	const auto RelativeVelocityDirection{
		AlsCharacter->GetLocomotionState().SmoothRotation.UnrotateVector(LocomotionState.Velocity.GetSafeNormal())
	};

	const auto RelativeDirection{
		RelativeVelocityDirection /
		(FMath::Abs(RelativeVelocityDirection.X) +
		 FMath::Abs(RelativeVelocityDirection.Y) +
		 FMath::Abs(RelativeVelocityDirection.Z))
	};

	MovementState.VelocityBlend.ForwardAmount = FMath::FInterpTo(MovementState.VelocityBlend.ForwardAmount,
	                                                             UAlsMath::Clamp01(RelativeDirection.X), DeltaTime,
	                                                             MovementSettings.VelocityBlendInterpolationSpeed);

	MovementState.VelocityBlend.BackwardAmount = FMath::FInterpTo(MovementState.VelocityBlend.BackwardAmount,
	                                                              FMath::Abs(FMath::Clamp(RelativeDirection.X, -1.0f, 0.0f)), DeltaTime,
	                                                              MovementSettings.VelocityBlendInterpolationSpeed);

	MovementState.VelocityBlend.LeftAmount = FMath::FInterpTo(MovementState.VelocityBlend.LeftAmount,
	                                                          FMath::Abs(FMath::Clamp(RelativeDirection.Y, -1.0f, 0.0f)), DeltaTime,
	                                                          MovementSettings.VelocityBlendInterpolationSpeed);

	MovementState.VelocityBlend.RightAmount = FMath::FInterpTo(MovementState.VelocityBlend.RightAmount,
	                                                           UAlsMath::Clamp01(RelativeDirection.Y), DeltaTime,
	                                                           MovementSettings.VelocityBlendInterpolationSpeed);
}

float UAlsAnimationInstance::CalculateStrideBlendAmount() const
{
	// Calculate the stride blend. This value is used within the blend spaces to scale the stride (distance feet travel) so
	// that the character can walk or run at different movement speeds. It also allows the walk or run gait animations to
	// blend independently while still matching the animation speed to the movement speed, preventing the character from needing
	// to play a half walk + half run blend. The curves are used to map the stride amount to the speed for maximum control.

	const auto Speed{LocomotionState.Speed / GetSkelMeshComponent()->GetComponentScale().Z};

	const auto StandingStrideBlend{
		FMath::Lerp(MovementSettings.StrideBlendAmountWalkCurve->GetFloatValue(Speed),
		            MovementSettings.StrideBlendAmountRunCurve->GetFloatValue(Speed),
		            LocomotionState.GaitRunningAmount)
	};

	// Crouching stride blends.

	return FMath::Lerp(StandingStrideBlend,
	                   MovementSettings.StrideBlendAmountWalkCurve->GetFloatValue(Speed),
	                   LayeringState.PoseCrouchingBlendAmount);
}

float UAlsAnimationInstance::CalculateWalkRunBlendAmount() const
{
	// Calculate the walk run blend. This value is used within the blend spaces to blend between walking and running.

	return Gait.IsWalking() ? 0.0f : 1.0f;
}

float UAlsAnimationInstance::CalculateStandingPlayRate() const
{
	// Calculate the standing play rate by dividing the character's speed by the animated speed for each gait.
	// The interpolation are determined by the gait amount curve that exists on every locomotion cycle so that the
	// play rate is always in sync with the currently blended animation. The value is also divided by the
	// stride blend and the mesh scale so that the play rate increases as the stride or scale gets smaller.

	const auto WalkRunSpeedAmount{
		FMath::Lerp(LocomotionState.Speed / MovementSettings.AnimatedWalkSpeed,
		            LocomotionState.Speed / MovementSettings.AnimatedRunSpeed,
		            LocomotionState.GaitRunningAmount)
	};

	const auto WalkRunSprintSpeedAmount{
		FMath::Lerp(WalkRunSpeedAmount,
		            LocomotionState.Speed / MovementSettings.AnimatedSprintSpeed,
		            LocomotionState.GaitSprintingAmount)
	};

	return FMath::Clamp(WalkRunSprintSpeedAmount / MovementState.StrideBlendAmount / GetSkelMeshComponent()->GetComponentScale().Z, 0.0f,
	                    3.0f);
}

float UAlsAnimationInstance::CalculateCrouchingPlayRate() const
{
	// Calculate the crouching play rate by dividing the character's speed by the animated speed. This value needs
	// to be separate from the standing play rate to improve the blend from crouching to standing while in motion.

	return FMath::Clamp(LocomotionState.Speed / MovementSettings.AnimatedCrouchSpeed /
	                    MovementState.StrideBlendAmount / GetSkelMeshComponent()->GetComponentScale().Z,
	                    0.0f, 2.0f);
}

void UAlsAnimationInstance::ActivatePivot()
{
	MovementState.bPivotActive = LocomotionState.Speed < MovementSettings.PivotActivationSpeedThreshold;

	if (MovementState.bPivotActive)
	{
		GetWorld()->GetTimerManager().SetTimer(PivotResetTimer, this, &ThisClass::OnPivotResetTimerEnded, 0.1f, false);
	}
}

void UAlsAnimationInstance::OnPivotResetTimerEnded()
{
	MovementState.bPivotActive = false;
}

void UAlsAnimationInstance::PlayTransition(UAnimSequenceBase* Animation, const float BlendInTime, const float BlendOutTime,
                                           const float PlayRate, const float StartTime)
{
	PlaySlotAnimationAsDynamicMontage(Animation, Constants.TransitionSlot, BlendInTime, BlendOutTime, PlayRate, 1, 0.0f, StartTime);
}

void UAlsAnimationInstance::PlayTransitionFromStandingIdleOnly(UAnimSequenceBase* Animation, const float BlendInTime,
                                                               const float BlendOutTime, const float PlayRate, const float StartTime)
{
	if (!LocomotionState.bMoving && Stance.IsStanding())
	{
		PlayTransition(Animation, BlendInTime, BlendOutTime, PlayRate, StartTime);
	}
}

void UAlsAnimationInstance::StopTransitionAndTurnInPlaceSlotAnimations(const float BlendOutTime)
{
	StopSlotAnimation(BlendOutTime, Constants.TransitionSlot);
	StopSlotAnimation(BlendOutTime, Constants.TurnInPlaceSlot);
}

void UAlsAnimationInstance::RefreshDynamicTransitions()
{
	if (LocomotionState.bMoving || !LocomotionState.bAllowTransitions)
	{
		return;
	}

	// Check each foot to see if the location difference between the foot look and its desired / target location
	// (determined via a virtual bone) exceeds a threshold. If it does, play an additive transition animation on
	// that foot. The currently set transition plays the second half of a 2 foot transition animation, so that only
	// a single foot moves. The separate virtual bone allows the system to know its desired location when locked.

	const auto SkeletalMesh{GetSkelMeshComponent()};

	if (FVector::DistSquared(SkeletalMesh->GetSocketLocation(Constants.FootLeftVirtualBone), FeetState.Left.LockLocation) >
	    FMath::Square(DynamicTransitionSettings.FootIkDistanceThreshold))
	{
		PlayDynamicTransition(DynamicTransitionSettings.TransitionRightAnimation, 0.2f, 0.2f, 1.5f, 0.8f, 0.1f);
		return;
	}

	if (FVector::DistSquared(SkeletalMesh->GetSocketLocation(Constants.FootRightVirtualBone), FeetState.Right.LockLocation) >
	    FMath::Square(DynamicTransitionSettings.FootIkDistanceThreshold))
	{
		PlayDynamicTransition(DynamicTransitionSettings.TransitionLeftAnimation, 0.2f, 0.2f, 1.5f, 0.8f, 0.1f);
	}
}

void UAlsAnimationInstance::PlayDynamicTransition(UAnimSequenceBase* Animation, const float BlendInTime, const float BlendOutTime,
                                                  const float PlayRate, const float StartTime, const float AllowanceDelayTime)
{
	if (bAllowDynamicTransitions)
	{
		bAllowDynamicTransitions = false;

		GetWorld()->GetTimerManager().SetTimer(DynamicTransitionsAllowanceTimer, this,
		                                       &ThisClass::OnDynamicTransitionAllowanceTimerEnded, AllowanceDelayTime, false);

		PlayTransition(Animation, BlendInTime, BlendOutTime, PlayRate, StartTime);
	}
}

void UAlsAnimationInstance::OnDynamicTransitionAllowanceTimerEnded()
{
	bAllowDynamicTransitions = true;
}

void UAlsAnimationInstance::RefreshRotateInPlace()
{
	// Rotate in place is allowed only if the character is standing still and aiming or in first-person view mode.

	if (LocomotionState.bMoving || !RotationMode.IsAiming() && ViewMode != EAlsViewMode::FirstPerson)
	{
		RotateInPlaceState.bRotatingLeft = false;
		RotateInPlaceState.bRotatingRight = false;
		return;
	}

	// Check if the character should rotate left or right by checking if the aiming yaw angle exceeds the threshold.

	const auto bRotatingLeftPrevious{RotateInPlaceState.bRotatingLeft};
	const auto bRotatingRightPrevious{RotateInPlaceState.bRotatingRight};

	RotateInPlaceState.bRotatingLeft = AimingState.YawAngle < -RotateInPlaceSettings.AimingYawAngleThreshold;
	RotateInPlaceState.bRotatingRight = AimingState.YawAngle > RotateInPlaceSettings.AimingYawAngleThreshold;

	if (!bRotatingLeftPrevious && RotateInPlaceState.bRotatingLeft ||
	    !bRotatingRightPrevious && RotateInPlaceState.bRotatingRight)
	{
		RotateInPlaceState.InitialPlayRate = FMath::Lerp(RotateInPlaceSettings.PlayRate.X, RotateInPlaceSettings.PlayRate.Y,
		                                                 UAlsMath::Clamp01(FMath::Abs(AimingState.YawAngle / 180.0f)));
	}

	if (!RotateInPlaceState.bRotatingLeft && !RotateInPlaceState.bRotatingRight)
	{
		return;
	}

	// If the character should be rotating, set the play rate to scale with the aiming yaw speed.
	// This makes the character rotate faster when moving the camera faster.

	const auto PlayRateFromYawSpeed{
		FMath::GetMappedRangeValueClamped(RotateInPlaceSettings.ReferenceAimingYawSpeed,
		                                  RotateInPlaceSettings.PlayRate,
		                                  AimingState.YawSpeed)
	};

	if (RotateInPlaceState.InitialPlayRate > PlayRateFromYawSpeed)
	{
		RotateInPlaceState.PlayRate = RotateInPlaceState.InitialPlayRate;
	}
	else
	{
		RotateInPlaceState.InitialPlayRate = 0.0f;
		RotateInPlaceState.PlayRate = PlayRateFromYawSpeed;
	}

	// If rotating too fast, then disable the foot lock, or else the legs begin to twist into a spiral.

	RotateInPlaceState.bDisableFootLock = RotateInPlaceSettings.bDisableFootLock ||
	                                      AimingState.YawSpeed > RotateInPlaceSettings.MaxFootLockAimingYawSpeed;
}

void UAlsAnimationInstance::RefreshTurnInPlace(const float DeltaTime)
{
	// Turn in place is allowed only if transitions are allowed, the character
	// standing still and looking at the camera and not in first-person mode.

	if (LocomotionState.bMoving || !RotationMode.IsLookingDirection() || ViewMode == EAlsViewMode::FirstPerson)
	{
		TurnInPlaceState.ActivationDelayTime = 0.0f;
		TurnInPlaceState.bDisableFootLock = false;
		return;
	}

	if (!LocomotionState.bAllowTransitions)
	{
		TurnInPlaceState.ActivationDelayTime = 0.0f;
		return;
	}

	// Check if the aiming yaw speed is below the threshold, and if aiming yaw angle is outside of the
	// threshold. If so, begin counting the activation delay time. If not, reset the activation delay time.
	// This ensures the conditions remain true for a sustained period of time before turning in place.

	if (AimingState.YawSpeed >= TurnInPlaceSettings.AimingYawSpeedThreshold ||
	    FMath::Abs(AimingState.YawAngle) <= TurnInPlaceSettings.AimingYawAngleThreshold)
	{
		TurnInPlaceState.ActivationDelayTime = 0.0f;
		TurnInPlaceState.bDisableFootLock = false;
		return;
	}

	TurnInPlaceState.ActivationDelayTime += DeltaTime;

	const auto ActivationDelay{
		FMath::GetMappedRangeValueClamped({TurnInPlaceSettings.AimingYawAngleThreshold, 180.0f},
		                                  TurnInPlaceSettings.AimingYawAngleToActivationDelay,
		                                  FMath::Abs(AimingState.YawAngle))
	};

	// Check if the activation delay time exceeds the set delay (mapped to the aiming yaw angle). If so, start a turn in place.

	if (TurnInPlaceState.ActivationDelayTime <= ActivationDelay)
	{
		return;
	}

	StartTurnInPlace(AimingState.Rotation.Yaw);
}

void UAlsAnimationInstance::StartTurnInPlace(const float TargetYawAngle, const float PlayRateScale, const float StartTime,
                                             const bool bAllowRestartIfPlaying)
{
	const auto TurnAngle{FRotator::NormalizeAxis(TargetYawAngle - AlsCharacter->GetLocomotionState().SmoothRotation.Yaw)};

	// Choose settings on the turn angle and stance.

	FAlsTurnInPlaceSettings Settings;
	if (Stance.IsStanding())
	{
		if (FMath::Abs(TurnAngle) < TurnInPlaceSettings.Turn180AngleThreshold)
		{
			Settings = TurnAngle < 0.0f
				           ? TurnInPlaceSettings.StandingTurn90Left
				           : TurnInPlaceSettings.StandingTurn90Right;
		}
		else
		{
			Settings = TurnAngle < 0.0f
				           ? TurnInPlaceSettings.StandingTurn180Left
				           : TurnInPlaceSettings.StandingTurn180Right;
		}
	}
	else
	{
		if (FMath::Abs(TurnAngle) < TurnInPlaceSettings.Turn180AngleThreshold)
		{
			Settings = TurnAngle < 0.0f
				           ? TurnInPlaceSettings.CrouchingTurn90Left
				           : TurnInPlaceSettings.CrouchingTurn90Right;
		}
		else
		{
			Settings = TurnAngle < 0.0f
				           ? TurnInPlaceSettings.CrouchingTurn180Left
				           : TurnInPlaceSettings.CrouchingTurn180Right;
		}
	}

	// If the animation is not playing or set to be overriden, play the turn animation as a dynamic montage.

	if (!bAllowRestartIfPlaying && IsPlayingSlotAnimation(Settings.Animation, Constants.TurnInPlaceSlot))
	{
		return;
	}

	PlaySlotAnimationAsDynamicMontage(Settings.Animation, Constants.TurnInPlaceSlot, 0.2f, 0.2f, Settings.PlayRate * PlayRateScale, 1,
	                                  0.0f, StartTime);

	// Scale the rotation yaw delta (gets scaled in animation graph) to compensate for play rate and turn angle (if allowed).

	if (Settings.bScalePlayRateByAnimatedTurnAngle)
	{
		TurnInPlaceState.PlayRate = Settings.PlayRate * PlayRateScale * TurnAngle / Settings.AnimatedTurnAngle;
	}
	else
	{
		TurnInPlaceState.PlayRate = Settings.PlayRate * PlayRateScale;
	}

	TurnInPlaceState.bDisableFootLock = TurnInPlaceSettings.bDisableFootLock;
}

void UAlsAnimationInstance::Jump()
{
	InAirState.bJumped = true;
	InAirState.JumpPlayRate = UAlsMath::LerpClamped(1.2f, 1.5f, LocomotionState.Speed / 600.0f);

	GetWorld()->GetTimerManager().SetTimer(JumpResetTimer, this, &ThisClass::OnJumpResetTimerEnded, 0.1f, false);
}

void UAlsAnimationInstance::OnJumpResetTimerEnded()
{
	InAirState.bJumped = false;
}

void UAlsAnimationInstance::RefreshInAir(const float DeltaTime)
{
	if (!LocomotionMode.IsInAir())
	{
		return;
	}

	// Update the vertical velocity. Setting this value only while in the air allows you to use it within the
	// animation graph for the landing strength. If not, the vertical velocity would return to 0 on landing.

	InAirState.VerticalVelocity = LocomotionState.Velocity.Z;

	// Set the ground prediction amount.

	InAirState.GroundPredictionAmount = CalculateGroundPredictionAmount();

	// Interpolate the lean amount.

	const auto TargetLeanAmount{CalculateInAirLeanAmount()};

	LeanState.RightAmount = FMath::FInterpTo(LeanState.RightAmount, TargetLeanAmount.RightAmount,
	                                         DeltaTime, GeneralSettings.LeanInterpolationSpeed);
	LeanState.ForwardAmount = FMath::FInterpTo(LeanState.ForwardAmount, TargetLeanAmount.ForwardAmount,
	                                           DeltaTime, GeneralSettings.LeanInterpolationSpeed);
}

float UAlsAnimationInstance::CalculateGroundPredictionAmount() const
{
	// Calculate the ground prediction weight by tracing in the velocity direction to find a walkable surface the character
	// is falling toward, and getting the "time" (range from 0 to 1, 1 being maximum, 0 being about to ground) till impact.
	// The ground prediction amount curve is used to control how the time affects the final amount for a smooth blend.

	if (InAirState.VerticalVelocity >= -200.0f)
	{
		return 0.0f;
	}

	const auto* Capsule{AlsCharacter->GetCapsuleComponent()};
	const auto SweepStartLocation{AlsCharacter->GetLocomotionState().SmoothLocation};

	auto VelocityDirection{LocomotionState.Velocity};
	VelocityDirection.Z = FMath::Clamp(InAirState.VerticalVelocity, -4000.0f, -200.0f);
	VelocityDirection.Normalize();

	const auto SweepVector{
		VelocityDirection * FMath::GetMappedRangeValueClamped({0.0f, -4000.0f}, {50.0f, 2000.0f}, InAirState.VerticalVelocity)
	};

	FHitResult Hit;
	GetWorld()->SweepSingleByObjectType(Hit, SweepStartLocation, SweepStartLocation + SweepVector, FQuat::Identity,
	                                    GroundPredictionObjectQueryParameters,
	                                    FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),
	                                                                 Capsule->GetScaledCapsuleHalfHeight()),
	                                    {__FUNCTION__, false, AlsCharacter});

#if ENABLE_DRAW_DEBUG
	if (UAlsUtility::ShouldDisplayDebug(AlsCharacter, UAlsConstants::TracesDisplayName()))
	{
		UAlsUtility::DrawDebugSweepSingleCapsule(GetWorld(), Hit.TraceStart, Hit.TraceEnd, FRotator::ZeroRotator,
		                                         Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(),
		                                         AlsCharacter->GetCharacterMovement()->IsWalkable(Hit),
		                                         Hit, {0.25f, 0.0f, 1.0f}, {0.75f, 0.0f, 1.0f});
	}
#endif

	if (!AlsCharacter->GetCharacterMovement()->IsWalkable(Hit))
	{
		return 0.0f;
	}

	return FMath::Lerp(InAirSettings.GroundPredictionAmountCurve->GetFloatValue(Hit.Time), 0.0f,
	                   GetCurveValueClamped01(Constants.GroundPredictionBlockCurve));
}

FAlsLeanState UAlsAnimationInstance::CalculateInAirLeanAmount() const
{
	// Use the relative velocity direction and amount to determine how much the character should lean
	// while in air. The lean amount curve gets the vertical velocity and is used as a multiplier to
	// smoothly reverse the leaning direction when transitioning from moving upwards to moving downwards.

	const auto RelativeVelocity{
		AlsCharacter->GetLocomotionState().SmoothRotation.UnrotateVector(LocomotionState.Velocity) / 350.0f *
		InAirSettings.LeanAmountCurve->GetFloatValue(InAirState.VerticalVelocity)
	};

	return {RelativeVelocity.Y, RelativeVelocity.X};
}

void UAlsAnimationInstance::RefreshRagdolling()
{
	if (LocomotionMode != EAlsLocomotionMode::Ragdolling)
	{
		return;
	}

	// Scale the flail play rate by the root speed. The faster the ragdoll moves, the faster the character will flail.

	RagdollingState.FlailPlayRate = UAlsMath::Clamp01(
		GetSkelMeshComponent()->GetPhysicsLinearVelocity(Constants.RootBone).Size() / 1000.0f
	);
}

void UAlsAnimationInstance::StopRagdolling()
{
	// Save a snapshot of the current ragdoll pose for use in animation graph to blend out of the ragdoll.

	SnapshotPose(RagdollingState.FinalRagdollPose);
}

float UAlsAnimationInstance::GetCurveValueClamped01(const FName& CurveName) const
{
	return UAlsMath::Clamp01(GetCurveValue(CurveName));
}
