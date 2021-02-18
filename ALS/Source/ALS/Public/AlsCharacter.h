#pragma once

#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Settings/AlsMantlingSettings.h"
#include "Settings/AlsMovementCharacterSettings.h"
#include "Settings/AlsRagdollingSettings.h"
#include "Settings/AlsRollingSettings.h"
#include "State/Enumerations/AlsGait.h"
#include "State/Enumerations/AlsLocomotionAction.h"
#include "State/Enumerations/AlsLocomotionMode.h"
#include "State/Enumerations/AlsMantlingType.h"
#include "State/Enumerations/AlsOverlayMode.h"
#include "State/Enumerations/AlsRotationMode.h"
#include "State/Enumerations/AlsStance.h"
#include "State/Structures/AlsAimingCharacterState.h"
#include "State/Structures/AlsInAirCharacterState.h"
#include "State/Structures/AlsLocomotionCharacterState.h"
#include "State/Structures/AlsMantlingState.h"
#include "State/Structures/AlsRagdollingCharacterState.h"
#include "State/Structures/AlsRollingState.h"

#include "AlsCharacter.generated.h"

class UTimelineComponent;

UCLASS()
class ALS_API AAlsCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess))
	UTimelineComponent* MantlingTimeline;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Als Character", meta = (AllowPrivateAccess))
	bool bRotateToVelocityWhenSprinting;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Als Character", meta = (AllowPrivateAccess))
	FAlsGeneralMantlingSettings GeneralMantlingSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Als Character", meta = (AllowPrivateAccess))
	FAlsRagdollingSettings RagdollingSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Als Character", meta = (AllowPrivateAccess))
	FAlsRollingSettings RollingSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings|Als Character", meta = (AllowPrivateAccess))
	UAlsMovementCharacterSettings* MovementSettings;

	// We won't use curve based movement and a few other features on networked games.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	bool bEnableNetworkOptimizations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	EAlsStance DesiredStance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	EAlsStance Stance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	EAlsGait DesiredGait{EAlsGait::Running};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	EAlsGait Gait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	EAlsRotationMode DesiredRotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	EAlsRotationMode RotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, ReplicatedUsing = "OnReplicate_OverlayMode",
		meta = (AllowPrivateAccess))
	EAlsOverlayMode OverlayMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	EAlsLocomotionMode LocomotionMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	EAlsLocomotionAction LocomotionAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	FVector InputAcceleration;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	FAlsLocomotionCharacterState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	bool bAiming;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	FRotator AimingRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	FAlsAimingCharacterState AimingState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	FAlsInAirCharacterState InAirState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	FAlsMantlingState MantlingState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated, meta = (AllowPrivateAccess))
	FVector RagdollTargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	FAlsRagdollingCharacterState RagdollingState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, meta = (AllowPrivateAccess))
	FAlsRollingState RollingState;

	FTimerHandle LandedGroundFrictionResetTimer;

public:
	AAlsCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;

	virtual void AddMovementInput(FVector Direction, float Scale = 1.0f, bool bForce = false) override;

	virtual void Jump() override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMode, uint8 PreviousCustomMode = 0) override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnJumped_Implementation() override;

	virtual void Landed(const FHitResult& Hit) override;

	// Desired Stance

public:
	EAlsStance GetDesiredStance() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void SetDesiredStance(EAlsStance NewStance);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredStance(EAlsStance NewStance);

	void RefreshDesiredStance();

	// Stance

public:
	EAlsStance GetStance() const;

private:
	void SetStance(EAlsStance NewStance);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnStanceChanged(EAlsStance PreviousStance);

	// Desired Gait

public:
	EAlsGait GetDesiredGait() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void SetDesiredGait(EAlsGait NewGait);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredGait(EAlsGait NewGait);

	// Gait

public:
	EAlsGait GetGait() const;

private:
	const FAlsMovementGaitSettings& GetGaitSettings() const;

	void SetGait(EAlsGait NewGait);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnGaitChanged(EAlsGait PreviousGait);

private:
	void RefreshGait(const FAlsMovementGaitSettings& GaitSettings);

	bool CanSprint() const;

	EAlsGait CalculateMaxAllowedGait() const;

	EAlsGait CalculateActualGait(EAlsGait MaxAllowedGait, const FAlsMovementGaitSettings& GaitSettings) const;

	float CalculateGaitAmount(const FAlsMovementGaitSettings& GaitSettings) const;

	void RefreshGaitSettingsNetworked(EAlsGait MaxAllowedGait, const FAlsMovementGaitSettings& GaitSettings) const;

	void RefreshGaitSettingsStandalone(EAlsGait MaxAllowedGait, const FAlsMovementGaitSettings& GaitSettings) const;

	// Desired Rotation Mode

public:
	EAlsRotationMode GetDesiredRotationMode() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void SetDesiredRotationMode(EAlsRotationMode NewMode);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetDesiredRotationMode(EAlsRotationMode NewMode);

	// Rotation Mode

public:
	EAlsRotationMode GetRotationMode() const;

private:
	void SetRotationMode(EAlsRotationMode NewMode);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnRotationModeChanged(EAlsRotationMode PreviousMode);

	void RefreshRotationMode();

	// Overlay Mode

public:
	EAlsOverlayMode GetOverlayMode() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void SetOverlayMode(EAlsOverlayMode NewMode);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetOverlayMode(EAlsOverlayMode NewMode);

	UFUNCTION()
	void OnReplicate_OverlayMode(EAlsOverlayMode PreviousMode);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnOverlayModeChanged(EAlsOverlayMode PreviousMode);

	// Locomotion Mode

public:
	EAlsLocomotionMode GetLocomotionMode() const;

private:
	void SetLocomotionMode(EAlsLocomotionMode NewMode);

	void NotifyLocomotionModeChanged(EAlsLocomotionMode PreviousMode);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnLocomotionModeChanged(EAlsLocomotionMode PreviousMode);

	// Locomotion Action

public:
	EAlsLocomotionAction GetLocomotionAction() const;

	void SetLocomotionAction(EAlsLocomotionAction NewAction);

	void NotifyLocomotionActionChanged(EAlsLocomotionAction PreviousAction);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnLocomotionActionChanged(EAlsLocomotionAction PreviousAction);

	// Locomotion

public:
	const FVector& GetInputAcceleration() const;

	const FAlsLocomotionCharacterState& GetLocomotionState() const;

private:
	void SetInputAcceleration(const FVector& NewInputAcceleration);

	void RefreshLocomotion(float DeltaTime);

	// Aiming

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void SetAiming(bool bNewAiming);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

public:
	const FRotator& GetAimingRotation() const;

private:
	void SetAimingRotation(const FRotator& NewAimingRotation);

public:
	const FAlsAimingCharacterState& GetAimingState() const;

private:
	void RefreshAiming(float DeltaTime);

	// Rotation

private:
	void RefreshGroundedActorRotation(float DeltaTime, const FAlsMovementGaitSettings& GaitSettings);

	void RefreshAimingActorRotation(float DeltaTime);

	void RefreshInAirActorRotation(float DeltaTime);

	float CalculateActorRotationSpeed(const FAlsMovementGaitSettings& GaitSettings) const;

	void RefreshActorRotation(const float TargetYawAngle, const float DeltaTime, const float RotationSpeed);

	void RefreshActorRotationExtraSmooth(const float TargetYawAngle, const float DeltaTime,
	                                     const float TargetRotationSpeed, const float ActorRotationSpeed);

	// Jumping

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnJumpedNetworked();

	void OnJumpedNetworked();

	// Landing

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnLandedNetworked();

	void OnLandedNetworked();

	void OnLandedGroundFrictionReset() const;

	// Mantling

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	bool TryStartMantlingGrounded();

private:
	bool TryStartMantlingInAir();

	bool TryStartMantling(const FAlsMantlingTraceSettings& TraceSettings);

	UFUNCTION(Server, Reliable)
	void ServerStartMantling(UPrimitiveComponent* TargetPrimitive, const FVector& TargetLocation, const FRotator& TargetRotation,
	                         float MantlingHeight, EAlsMantlingType MantlingType);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartMantling(UPrimitiveComponent* TargetPrimitive, const FVector& TargetLocation, const FRotator& TargetRotation,
	                            float MantlingHeight, EAlsMantlingType MantlingType);

	void StartMantling(UPrimitiveComponent* TargetPrimitive, const FVector& TargetLocation, const FRotator& TargetRotation,
	                   float MantlingHeight, EAlsMantlingType MantlingType);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	FAlsMantlingSettings SelectMantlingSettings(EAlsMantlingType MantlingType);

	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnMantlingStarted(UPrimitiveComponent* TargetPrimitive, const FVector& TargetLocation, const FRotator& TargetRotation,
	                       float MantlingHeight, EAlsMantlingType MantlingType);

private:
	UFUNCTION()
	void OnMantlingTimelineUpdated(float BlendInTime);

	UFUNCTION()
	void OnMantlingTimelineEnded();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnMantlingEnded();

	// Ragdolling

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void StartRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling();

	void StartRagdollingImplementation();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnRagdollingStarted();

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	bool TryStopRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void StopRagdollingImplementation();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UAnimMontage* SelectGetUpMontage(bool bRagdollFacedUpward);

	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	void OnRagdollingEnded();

private:
	void SetRagdollTargetLocation(const FVector& NewLocation);

	UFUNCTION(Server, Unreliable)
	void ServerSetRagdollTargetLocation(const FVector& NewLocation);

	void RefreshRagdolling(float DeltaTime);

	void RefreshRagdollingActorTransform(float DeltaTime);

	// Rolling

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void TryStartRolling(float PlayRate = 1);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UAnimMontage* SelectRollMontage();

private:
	void StartRolling(float PlayRate, float TargetYawAngle);

	UFUNCTION(Server, Reliable)
	void ServerStartRolling(UAnimMontage* Montage, float PlayRate, float TargetYawAngle);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRolling(UAnimMontage* Montage, float PlayRate, float TargetYawAngle);
};

inline const FVector& AAlsCharacter::GetInputAcceleration() const
{
	return InputAcceleration;
}

inline const FAlsLocomotionCharacterState& AAlsCharacter::GetLocomotionState() const
{
	return LocomotionState;
}

inline const FRotator& AAlsCharacter::GetAimingRotation() const
{
	return AimingRotation;
}

inline const FAlsAimingCharacterState& AAlsCharacter::GetAimingState() const
{
	return AimingState;
}

inline EAlsStance AAlsCharacter::GetDesiredStance() const
{
	return DesiredStance;
}

inline EAlsStance AAlsCharacter::GetStance() const
{
	return Stance;
}

inline EAlsGait AAlsCharacter::GetDesiredGait() const
{
	return DesiredGait;
}

inline EAlsGait AAlsCharacter::GetGait() const
{
	return Gait;
}

inline EAlsRotationMode AAlsCharacter::GetDesiredRotationMode() const
{
	return DesiredRotationMode;
}

inline EAlsRotationMode AAlsCharacter::GetRotationMode() const
{
	return RotationMode;
}

inline EAlsOverlayMode AAlsCharacter::GetOverlayMode() const
{
	return OverlayMode;
}

inline EAlsLocomotionMode AAlsCharacter::GetLocomotionMode() const
{
	return LocomotionMode;
}

inline EAlsLocomotionAction AAlsCharacter::GetLocomotionAction() const
{
	return LocomotionAction;
}