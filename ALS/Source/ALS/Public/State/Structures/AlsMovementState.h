﻿#pragma once

#include "AlsMovementState.generated.h"

UENUM(BlueprintType)
enum class EAlsHipsDirection : uint8
{
	Forward,
	Backward,
	LeftForward,
	LeftBackward,
	RightForward,
	RightBackward,
};

USTRUCT(BlueprintType)
struct ALS_API FAlsVelocityBlendState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, ClampMax = 1))
	float ForwardAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, ClampMax = 1))
	float BackwardAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, ClampMax = 1))
	float LeftAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, ClampMax = 1))
	float RightAmount{0.0f};
};

USTRUCT(BlueprintType)
struct ALS_API FAlsMovementState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAlsHipsDirection HipsDirection = EAlsHipsDirection::Forward;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FAlsVelocityBlendState VelocityBlend;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, ClampMax = 1))
	float StrideBlendAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0, ClampMax = 1))
	float WalkRunBlendAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0))
	float StandingPlayRate{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ClampMin = 0))
	float CrouchingPlayRate{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPivotActive{false};
};
