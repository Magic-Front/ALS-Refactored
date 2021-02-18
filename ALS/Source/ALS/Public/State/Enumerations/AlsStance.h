﻿#pragma once

#include "AlsStance.generated.h"

UENUM(BlueprintType)
enum class EAlsStance : uint8
{
	Standing,
	Crouching
};

USTRUCT(BlueprintType)
struct FAlsStance
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
	EAlsStance Stance{EAlsStance::Standing};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
	bool bStanding{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
	bool bCrouching{false};

public:
	FAlsStance() {}

	FAlsStance(const EAlsStance InitialStance)
	{
		*this = InitialStance;
	}

	FORCEINLINE bool IsStanding() const
	{
		return bStanding;
	}

	FORCEINLINE bool IsCrouching() const
	{
		return bCrouching;
	}

	FORCEINLINE operator EAlsStance() const
	{
		return Stance;
	}

	FORCEINLINE void operator=(const EAlsStance NewStance)
	{
		Stance = NewStance;

		bStanding = Stance == EAlsStance::Standing;
		bCrouching = Stance == EAlsStance::Crouching;
	}
};