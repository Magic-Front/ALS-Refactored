#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "State/Enumerations/AlsMovementDirection.h"

#include "AlsMath.generated.h"

class UCapsuleComponent;

UCLASS()
class ALS_API UAlsMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr float TwoPi{6.2831853071795864769252867665590057683943387987502116419498891846f};

public:
	UFUNCTION(BlueprintPure, Category = "ALS|Als Math")
	static float Clamp01(float Value);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math")
	static float LerpClamped(float A, float B, float Alpha);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math")
	static float Damp(const float Smoothing, const float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math")
	static float ExponentialDecay(const float Lambda, const float DeltaTime);

	template <class T>
	static T Damp(const T& A, const T& B, const float Smoothing, const float DeltaTime);

	template <class T>
	static T ExponentialDecay(const T& A, const T& B, const float Lambda, const float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math")
	static float InterpolateAngleConstant(float CurrentAngle, float TargetAngle, float DeltaTime, float InterpolationSpeed);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math|Vector", Meta = (AutoCreateRefTerm = "Vector"))
	static FVector ClampMagnitude01(const FVector& Vector);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math|Vector")
	static FVector2D RadianToDirection(float Radian);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math|Vector")
	static FVector2D AngleToDirection(float Angle);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math|Vector", Meta = (AutoCreateRefTerm = "Direction"))
	static float DirectionToAngle(const FVector2D& Direction);

	UFUNCTION(BlueprintPure, Category = "ALS|Als Math|Vector", Meta = (DisplayName = "Angle Between (Skip Normalization)", AutoCreateRefTerm = "From, To"))
	static float AngleBetweenSkipNormalization(const FVector& From, const FVector& To);

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Math|Transform")
	static FTransform AddTransforms(const FTransform& Left, const FTransform& Right);

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Math|Transform")
	static FTransform SubtractTransforms(const FTransform& Left, const FTransform& Right);

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Math|Input")
	static float FixGamepadDiagonalValues(float AxisValue, float OtherAxisValue);

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Math|Input")
	static EAlsMovementDirection CalculateMovementDirection(float Angle, float ForwardHalfAngle, float AngleOffset);
};

inline float UAlsMath::Clamp01(const float Value)
{
	return Value <= 0.0f ? 0.0f : Value >= 1.0f ? 1.0f : Value;
}

inline float UAlsMath::LerpClamped(const float A, const float B, const float Alpha)
{
	return A + Clamp01(Alpha) * (B - A);
}

inline float UAlsMath::Damp(const float Smoothing, const float DeltaTime)
{
	// https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/

	return 1 - FMath::Pow(Smoothing, DeltaTime);
}

inline float UAlsMath::ExponentialDecay(const float Lambda, const float DeltaTime)
{
	// https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/

	return 1 - FMath::Exp(-Lambda * DeltaTime);
}

template <class T>
T UAlsMath::Damp(const T& A, const T& B, const float Smoothing, const float DeltaTime)
{
	return FMath::Lerp(A, B, Damp(Smoothing, DeltaTime));
}

template <class T>
T UAlsMath::ExponentialDecay(const T& A, const T& B, const float Lambda, const float DeltaTime)
{
	return FMath::Lerp(A, B, ExponentialDecay(Lambda, DeltaTime));
}

inline FVector UAlsMath::ClampMagnitude01(const FVector& Vector)
{
	const auto MagnitudeSquared{Vector.SizeSquared()};
	if (MagnitudeSquared <= 1.0f)
	{
		return Vector;
	}

	const auto Scale{FMath::InvSqrt(MagnitudeSquared)};
	return {Vector.X * Scale, Vector.Y * Scale, Vector.Z * Scale};
}

inline FVector2D UAlsMath::RadianToDirection(const float Radian)
{
	float Sin, Cos;
	FMath::SinCos(&Sin, &Cos, Radian);

	return {Cos, Sin};
}

inline FVector2D UAlsMath::AngleToDirection(const float Angle)
{
	return RadianToDirection(FMath::DegreesToRadians(Angle));
}

inline float UAlsMath::DirectionToAngle(const FVector2D& Direction)
{
	return FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
}

inline float UAlsMath::AngleBetweenSkipNormalization(const FVector& From, const FVector& To)
{
	return FMath::RadiansToDegrees(FMath::Acos(From | To));
}

inline FTransform UAlsMath::AddTransforms(const FTransform& Left, const FTransform& Right)
{
	return {
		Left.Rotator() + Right.Rotator(),
		Left.GetLocation() + Right.GetLocation(),
		Left.GetScale3D() + Right.GetScale3D()
	};
}

inline FTransform UAlsMath::SubtractTransforms(const FTransform& Left, const FTransform& Right)
{
	return {
		Left.Rotator() - Right.Rotator(),
		Left.GetLocation() - Right.GetLocation(),
		Left.GetScale3D() - Right.GetScale3D()
	};
}
