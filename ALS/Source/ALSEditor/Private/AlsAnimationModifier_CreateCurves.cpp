﻿#include "AlsAnimationModifier_CreateCurves.h"

#include "Animation/AnimSequence.h"

void UAlsAnimationModifier_CreateCurves::OnApply_Implementation(UAnimSequence* Sequence)
{
	for (const auto& Curve : Curves)
	{
		if (UAnimationBlueprintLibrary::DoesCurveExist(Sequence, Curve.Name, ERawCurveTrackTypes::RCT_Float))
		{
			UAnimationBlueprintLibrary::RemoveCurve(Sequence, Curve.Name);
		}

		UAnimationBlueprintLibrary::AddCurve(Sequence, Curve.Name);

		if (Curve.bAddKeyOnEachFrame)
		{
			for (auto i{0}; i < Sequence->GetNumberOfFrames(); i++)
			{
				UAnimationBlueprintLibrary::AddFloatCurveKey(Sequence, Curve.Name, Sequence->GetTimeAtFrame(i), 0);
			}
		}
		else
		{
			for (const auto& CurveKey : Curve.Keys)
			{
				UAnimationBlueprintLibrary::AddFloatCurveKey(Sequence, Curve.Name, Sequence->GetTimeAtFrame(CurveKey.Frame),
				                                             CurveKey.Value);
			}
		}
	}
}
