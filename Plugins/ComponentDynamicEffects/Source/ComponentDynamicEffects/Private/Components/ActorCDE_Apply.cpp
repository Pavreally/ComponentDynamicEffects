// Pavel Gornostaev <https://github.com/Pavreally>

#include "Components/ActorCDE.h"
#include "DataAsset/EffectLogicBaseCDE.h"

#include "ComponentDynamicEffects.h"
#include "GameFramework/Actor.h"

namespace
{
bool IsModifierEqual(const FStatusEffectModifierCDE& A, const FStatusEffectModifierCDE& B)
{
	return A.ModifierTag == B.ModifierTag
		&& FMath::IsNearlyEqual(A.Value, B.Value);
}

bool AreModifiersEqual(const TArray<FStatusEffectModifierCDE>& A, const TArray<FStatusEffectModifierCDE>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < A.Num(); ++Index)
	{
		if (!IsModifierEqual(A[Index], B[Index]))
		{
			return false;
		}
	}

	return true;
}

bool IsEffectDefinitionEqual(const FStatusEffectDefinitionCDE& A, const FStatusEffectDefinitionCDE& B)
{
	const FStatusEffectDataCDE& AData = A.Data;
	const FStatusEffectDataCDE& BData = B.Data;

	return AData.EffectTag == BData.EffectTag
		&& AData.DurationPolicy == BData.DurationPolicy
		&& FMath::IsNearlyEqual(AData.Duration, BData.Duration)
		&& AData.StackingPolicy == BData.StackingPolicy
		&& AData.MaxStacks == BData.MaxStacks
		&& AData.Priority == BData.Priority
		&& AData.CancelEffectsWithLowerPriority == BData.CancelEffectsWithLowerPriority
		&& AData.RemoveOnApply == BData.RemoveOnApply
		&& A.EffectLogicClass.Get() == B.EffectLogicClass.Get()
		&& AreModifiersEqual(A.Modifiers, B.Modifiers);
}
}

bool UActorCDE::ApplyEffect(FGameplayTag EffectTag, AActor* Instigator)
{
	EnsureRegisteredEffectsInitialized();

	// Prevent application if any currently active effect explicitly blocks this effect tag.
	for (const TPair<FGameplayTag, FActiveStatusEffectCDE>& Pair : ActiveEffects)
	{
		const FStatusEffectDataCDE& ActiveData = Pair.Value.Definition.Data;
		if (ActiveData.BlockedEffects.HasTagExact(EffectTag))
		{
			return false;
		}
	}

	const FStatusEffectDefinitionCDE* EffectDefinition = FindRegisteredEffectDefinition(EffectTag);
	if (!EffectDefinition)
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("ActorCDE::ApplyEffect - Effect '%s' is not registered."), *EffectTag.ToString());
		return false;
	}

	const FStatusEffectDataCDE& DefinitionData = EffectDefinition->Data;

	for (const FGameplayTag& TagToRemove : DefinitionData.RemoveOnApply)
	{
		if (TagToRemove.IsValid())
		{
			RemoveEffect(TagToRemove);
		}
	}

	if (!DefinitionData.CancelEffectsWithLowerPriority.IsEmpty())
	{
		TArray<FGameplayTag> PriorityCancelledTags;

		for (const FGameplayTag& TagToCancel : DefinitionData.CancelEffectsWithLowerPriority)
		{
			if (!TagToCancel.IsValid())
			{
				continue;
			}

			if (const FActiveStatusEffectCDE* ActiveEffect = ActiveEffects.Find(TagToCancel))
			{
				if (DefinitionData.Priority > ActiveEffect->Definition.Data.Priority)
				{
					PriorityCancelledTags.Add(TagToCancel);
				}
			}
		}

		for (const FGameplayTag& TagToCancel : PriorityCancelledTags)
		{
			RemoveEffectInternal(TagToCancel, EEffectRemoveReason::Replaced);
		}
	}

	FActiveStatusEffectCDE* ExistingEffect = ActiveEffects.Find(DefinitionData.EffectTag);
	if (ExistingEffect && DefinitionData.Priority < ExistingEffect->Definition.Data.Priority)
	{
		return false;
	}

	if (ExistingEffect && DefinitionData.Priority > ExistingEffect->Definition.Data.Priority)
	{
		RemoveEffectInternal(DefinitionData.EffectTag, EEffectRemoveReason::Replaced);
		ExistingEffect = nullptr;
	}

	if (DefinitionData.DurationPolicy == EEffectDurationPolicy::Instant)
	{
		FActiveStatusEffectCDE InstantEffect;
		BuildRuntimeEffect(InstantEffect, *EffectDefinition, Instigator, true);

		BroadcastEffectAdded(InstantEffect);
		CleanupRuntimeLogic(InstantEffect, EEffectRemoveReason::Removed);
		BroadcastEffectRemoved(DefinitionData.EffectTag, EEffectRemoveReason::Removed);
		return true;
	}

	if (ExistingEffect)
	{
		if (DefinitionData.StackingPolicy == EStackingPolicy::Ignore)
		{
			return false;
		}

		if (DefinitionData.StackingPolicy == EStackingPolicy::Replace)
		{
			RemoveEffectInternal(DefinitionData.EffectTag, EEffectRemoveReason::Replaced);
			ExistingEffect = nullptr;
		}
	}

	if (ExistingEffect)
	{
		bool bEffectChanged = false;
		bool bStacksChanged = false;
		const bool bDefinitionChanged = !IsEffectDefinitionEqual(ExistingEffect->Definition, *EffectDefinition);
		const bool bInstigatorChanged = ExistingEffect->Instigator != Instigator;
		bEffectChanged = bDefinitionChanged || bInstigatorChanged;

		ExistingEffect->Definition = *EffectDefinition;
		ExistingEffect->Instigator = Instigator;

		if (DefinitionData.StackingPolicy == EStackingPolicy::RefreshDuration)
		{
			const float NewTimeRemaining = (DefinitionData.DurationPolicy == EEffectDurationPolicy::Timed)
				? FMath::Max(0.0f, DefinitionData.Duration)
				: 0.0f;

			if (!FMath::IsNearlyEqual(ExistingEffect->TimeRemaining, NewTimeRemaining))
			{
				ExistingEffect->TimeRemaining = NewTimeRemaining;
				bEffectChanged = true;
			}
		}
		else if (DefinitionData.StackingPolicy == EStackingPolicy::AddStack)
		{
			const int32 MaxStacks = FMath::Max(1, DefinitionData.MaxStacks);
			const int32 NewStacks = FMath::Min(ExistingEffect->CurrentStacks + 1, MaxStacks);
			if (NewStacks != ExistingEffect->CurrentStacks)
			{
				ExistingEffect->CurrentStacks = NewStacks;
				bStacksChanged = true;
				bEffectChanged = true;
			}
		}

		bool bLogicChanged = false;
		bool bAppliedNewLogic = false;

		if (EffectDefinition->EffectLogicClass.Get())
		{
			const bool bNeedsNewLogic = !IsValid(ExistingEffect->LogicInstance) || !ExistingEffect->LogicInstance->IsA(EffectDefinition->EffectLogicClass.Get());
			if (bNeedsNewLogic)
			{
				CleanupRuntimeLogic(*ExistingEffect, EEffectRemoveReason::Replaced);
				UObject* LogicOuter = this;
				ExistingEffect->LogicInstance = NewObject<UEffectLogicBaseCDE>(LogicOuter, EffectDefinition->EffectLogicClass.Get());

				if (IsValid(ExistingEffect->LogicInstance))
				{
					ExistingEffect->LogicInstance->Initialize(this, GetOwner(), Instigator, ExistingEffect->Definition);
					RefreshLogicRuntimeContext(*ExistingEffect);
					ExistingEffect->LogicInstance->OnApply(ExistingEffect->LogicInstance->GetEffectContext());
					bAppliedNewLogic = true;
				}

				bLogicChanged = true;
			}
			else
			{
				ExistingEffect->LogicInstance->Initialize(this, GetOwner(), Instigator, ExistingEffect->Definition);
				RefreshLogicRuntimeContext(*ExistingEffect);
			}
		}
		else if (IsValid(ExistingEffect->LogicInstance))
		{
			CleanupRuntimeLogic(*ExistingEffect, EEffectRemoveReason::Replaced);
			bLogicChanged = true;
		}

		if (IsValid(ExistingEffect->LogicInstance) && !bAppliedNewLogic && bEffectChanged)
		{
			RefreshLogicRuntimeContext(*ExistingEffect);
			if (bStacksChanged)
			{
				ExistingEffect->LogicInstance->OnStackChanged(ExistingEffect->LogicInstance->GetEffectContext());
			}

			ExistingEffect->LogicInstance->OnEffectUpdated(ExistingEffect->LogicInstance->GetEffectContext());
		}

		bEffectChanged = bEffectChanged || bLogicChanged;
		if (!bEffectChanged)
		{
			return true;
		}

		RebuildAggregatedTags();
		UpdateComponentTickState();

		const FActiveStatusEffectCDE UpdatedSnapshot = *ExistingEffect;
		if (bStacksChanged)
		{
			BroadcastEffectStackChanged(UpdatedSnapshot);
		}

		BroadcastEffectUpdated(UpdatedSnapshot);
		return true;
	}

	FActiveStatusEffectCDE RuntimeEffect;
	BuildRuntimeEffect(RuntimeEffect, *EffectDefinition, Instigator, true);

	FActiveStatusEffectCDE& AddedEffect = ActiveEffects.Add(DefinitionData.EffectTag, RuntimeEffect);
	RebuildAggregatedTags();
	UpdateComponentTickState();

	const FActiveStatusEffectCDE AddedSnapshot = AddedEffect;
	BroadcastEffectAdded(AddedSnapshot);
	return true;
}

void UActorCDE::BuildRuntimeEffect(FActiveStatusEffectCDE& OutRuntimeEffect, const FStatusEffectDefinitionCDE& EffectDefinition, AActor* Instigator, bool bCallOnApply)
{
	OutRuntimeEffect = FActiveStatusEffectCDE{};
	OutRuntimeEffect.Definition = EffectDefinition;
	OutRuntimeEffect.Instigator = Instigator;
	OutRuntimeEffect.CurrentStacks = 1;
	OutRuntimeEffect.TimeRemaining = (EffectDefinition.Data.DurationPolicy == EEffectDurationPolicy::Timed)
		? FMath::Max(0.0f, EffectDefinition.Data.Duration)
		: 0.0f;

	if (!EffectDefinition.EffectLogicClass.Get())
	{
		return;
	}

	UObject* LogicOuter = this;
	OutRuntimeEffect.LogicInstance = NewObject<UEffectLogicBaseCDE>(LogicOuter, EffectDefinition.EffectLogicClass.Get());
	if (!IsValid(OutRuntimeEffect.LogicInstance))
	{
		return;
	}

	OutRuntimeEffect.LogicInstance->Initialize(this, GetOwner(), Instigator, EffectDefinition);
	RefreshLogicRuntimeContext(OutRuntimeEffect);
	if (bCallOnApply)
	{
		OutRuntimeEffect.LogicInstance->OnApply(OutRuntimeEffect.LogicInstance->GetEffectContext());
	}
}

void UActorCDE::CleanupRuntimeLogic(FActiveStatusEffectCDE& RuntimeEffect, EEffectRemoveReason RemoveReason) const
{
	if (!IsValid(RuntimeEffect.LogicInstance))
	{
		return;
	}

	RuntimeEffect.LogicInstance->RefreshRuntimeContext(RuntimeEffect, 0.0f, RemoveReason);
	RuntimeEffect.LogicInstance->OnRemove(RuntimeEffect.LogicInstance->GetEffectContext());
	RuntimeEffect.LogicInstance = nullptr;
}

void UActorCDE::RefreshLogicRuntimeContext(FActiveStatusEffectCDE& RuntimeEffect, float DeltaTime, EEffectRemoveReason RemoveReason) const
{
	if (!IsValid(RuntimeEffect.LogicInstance))
	{
		return;
	}

	RuntimeEffect.LogicInstance->RefreshRuntimeContext(RuntimeEffect, DeltaTime, RemoveReason);
}
