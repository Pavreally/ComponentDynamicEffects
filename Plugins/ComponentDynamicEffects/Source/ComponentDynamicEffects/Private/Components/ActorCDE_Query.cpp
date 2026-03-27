// Pavel Gornostaev <https://github.com/Pavreally>

#include "Components/ActorCDE.h"

bool UActorCDE::RemoveEffect(FGameplayTag EffectTag)
{
	return RemoveEffectInternal(EffectTag, EEffectRemoveReason::Removed);
}

bool UActorCDE::RemoveEffectInternal(FGameplayTag EffectTag, EEffectRemoveReason Reason)
{
	if (!EffectTag.IsValid())
	{
		return false;
	}

	FActiveStatusEffectCDE RemovedEffect;
	if (!ActiveEffects.RemoveAndCopyValue(EffectTag, RemovedEffect))
	{
		return false;
	}

	CleanupRuntimeLogic(RemovedEffect, Reason);
	RebuildAggregatedTags();
	UpdateComponentTickState();
	BroadcastEffectRemoved(EffectTag, Reason);
	return true;
}

bool UActorCDE::RemoveEffectStack(FGameplayTag EffectTag, int32 StacksToRemove)
{
	if (!EffectTag.IsValid() || StacksToRemove <= 0)
	{
		return false;
	}

	FActiveStatusEffectCDE* RuntimeEffect = ActiveEffects.Find(EffectTag);
	if (!RuntimeEffect)
	{
		return false;
	}

	if (RuntimeEffect->Definition.Data.StackingPolicy != EStackingPolicy::AddStack)
	{
		return RemoveEffectInternal(EffectTag, EEffectRemoveReason::Removed);
	}

	RuntimeEffect->CurrentStacks = FMath::Max(RuntimeEffect->CurrentStacks - StacksToRemove, 0);
	if (RuntimeEffect->CurrentStacks == 0)
	{
		return RemoveEffectInternal(EffectTag, EEffectRemoveReason::Removed);
	}

	if (IsValid(RuntimeEffect->LogicInstance))
	{
		RefreshLogicRuntimeContext(*RuntimeEffect);
		RuntimeEffect->LogicInstance->OnStackChanged(RuntimeEffect->LogicInstance->GetEffectContext());
		RuntimeEffect->LogicInstance->OnEffectUpdated(RuntimeEffect->LogicInstance->GetEffectContext());
	}

	UpdateComponentTickState();

	const FActiveStatusEffectCDE UpdatedSnapshot = *RuntimeEffect;
	BroadcastEffectStackChanged(UpdatedSnapshot);
	BroadcastEffectUpdated(UpdatedSnapshot);
	return true;
}

bool UActorCDE::HasRegisteredEffect(FGameplayTag EffectTag) const
{
	const_cast<UActorCDE*>(this)->EnsureRegisteredEffectsInitialized();
	return FindRegisteredEffectDefinition(EffectTag) != nullptr;
}

bool UActorCDE::TryGetRegisteredEffectDataByTag(FGameplayTag EffectTag, FStatusEffectDataCDE& OutEffectData) const
{
	const_cast<UActorCDE*>(this)->EnsureRegisteredEffectsInitialized();

	OutEffectData = FStatusEffectDataCDE{};
	if (const FStatusEffectDefinitionCDE* EffectDefinition = FindRegisteredEffectDefinition(EffectTag))
	{
		OutEffectData = EffectDefinition->Data;
		return true;
	}

	return false;
}

bool UActorCDE::TryGetRegisteredEffectDefinitionByTag(FGameplayTag EffectTag, FStatusEffectDefinitionCDE& OutEffectDefinition) const
{
	const_cast<UActorCDE*>(this)->EnsureRegisteredEffectsInitialized();

	OutEffectDefinition = FStatusEffectDefinitionCDE{};
	if (const FStatusEffectDefinitionCDE* EffectDefinition = FindRegisteredEffectDefinition(EffectTag))
	{
		OutEffectDefinition = *EffectDefinition;
		return true;
	}

	return false;
}

bool UActorCDE::TryGetActiveEffectByTag(FGameplayTag EffectTag, FActiveStatusEffectCDE& OutActiveEffect) const
{
	OutActiveEffect = FActiveStatusEffectCDE{};
	if (const FActiveStatusEffectCDE* ActiveEffect = ActiveEffects.Find(EffectTag))
	{
		OutActiveEffect = *ActiveEffect;
		return true;
	}

	return false;
}

FGameplayTagContainer UActorCDE::GetRegisteredEffectTags() const
{
	const_cast<UActorCDE*>(this)->EnsureRegisteredEffectsInitialized();
	return RegisteredEffectTags;
}

bool UActorCDE::HasEffect(FGameplayTag EffectTag) const
{
	if (!EffectTag.IsValid())
	{
		return false;
	}

	return ActiveEffects.Contains(EffectTag);
}

bool UActorCDE::CanPerformAction(FGameplayTag ActionTag) const
{
	return !HasBlockingEffect(ActionTag);
}

bool UActorCDE::HasBlockingEffect(FGameplayTag ActionTag) const
{
	if (!ActionTag.IsValid())
	{
		return false;
	}

	const FGameplayTagContainer* BlockingEffects = ActionBlockingMap.Find(ActionTag);
	if (!BlockingEffects || BlockingEffects->IsEmpty())
	{
		return false;
	}

	return AggregatedTags.HasAny(*BlockingEffects);
}

FGameplayTagContainer UActorCDE::GetAggregatedTags() const
{
	return AggregatedTags;
}

void UActorCDE::RebuildAggregatedTags()
{
	AggregatedTags.Reset();

	for (const TPair<FGameplayTag, FActiveStatusEffectCDE>& Pair : ActiveEffects)
	{
		if (Pair.Key.IsValid())
		{
			AggregatedTags.AddTag(Pair.Key);
		}
	}
}
