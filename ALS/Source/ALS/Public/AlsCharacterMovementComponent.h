#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "Settings/AlsMovementCharacterSettings.h"

#include "AlsCharacterMovementComponent.generated.h"

class ALS_API FAlsSavedMove final : public FSavedMove_Character
{
private:
	using Super = FSavedMove_Character;

	EAlsStance Stance{EAlsStance::Standing};

	EAlsRotationMode RotationMode{EAlsRotationMode::LookingDirection};

	EAlsGait MaxAllowedGait{EAlsGait::Walking};

protected:
	virtual void Clear() override;

	virtual void SetMoveFor(ACharacter* Character, float NewDeltaTime, const FVector& NewAcceleration,
	                        FNetworkPredictionData_Client_Character& PredictionData) override;

	virtual bool CanCombineWith(const FSavedMovePtr& NewMovePtr, ACharacter* Character, float MaxDelta) const override;

	virtual void PrepMoveFor(ACharacter* Character) override;
};

class ALS_API FAlsNetworkPredictionData final : public FNetworkPredictionData_Client_Character
{
private:
	using Super = FNetworkPredictionData_Client_Character;

public:
	FAlsNetworkPredictionData(const UCharacterMovementComponent& MovementComponent);

protected:
	virtual FSavedMovePtr AllocateNewMove() override;
};

UCLASS()
class ALS_API UAlsCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	friend FAlsSavedMove;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (AllowPrivateAccess))
	UAlsMovementCharacterSettings* MovementSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (AllowPrivateAccess))
	FAlsMovementGaitSettings GaitSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (AllowPrivateAccess))
	EAlsStance Stance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (AllowPrivateAccess))
	EAlsRotationMode RotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (AllowPrivateAccess))
	EAlsGait MaxAllowedGait;

public:
	UAlsCharacterMovementComponent();

	virtual float GetMaxAcceleration() const override;

	virtual float GetMaxBrakingDeceleration() const override;

protected:
	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;

public:
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

public:
	void SetMovementSettings(UAlsMovementCharacterSettings* NewMovementSettings);

	const FAlsMovementGaitSettings& GetGaitSettings() const;

private:
	void RefreshGaitSettings();

public:
	void SetStance(EAlsStance NewStance);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetStance(EAlsStance NewStance);

public:
	void SetRotationMode(EAlsRotationMode NewMode);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetRotationMode(EAlsRotationMode NewMode);

public:
	void SetMaxAllowedGait(EAlsGait NewGait);

private:
	UFUNCTION(Server, Reliable)
	void ServerSetMaxAllowedGait(EAlsGait NewGait);

	void RefreshMaxWalkSpeed();

public:
	float CalculateGaitAmount() const;
};

inline const FAlsMovementGaitSettings& UAlsCharacterMovementComponent::GetGaitSettings() const
{
	return GaitSettings;
}
