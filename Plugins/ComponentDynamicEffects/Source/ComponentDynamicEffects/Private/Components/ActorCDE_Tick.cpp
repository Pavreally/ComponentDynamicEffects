// Pavel Gornostaev <https://github.com/Pavreally>

#include "Components/ActorCDE.h"

void UActorCDE::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveEffects.IsEmpty())
	{
		UpdateComponentTickState();
		return;
	}

	struct FPendingRemove
	{
		FGameplayTag Tag;
		EEffectRemoveReason Reason = EEffectRemoveReason::Removed;
	};

	TArray<FPendingRemove> PendingRemovals;
	PendingRemovals.Reserve(ActiveEffects.Num());

	for (TPair<FGameplayTag, FActiveStatusEffectCDE>& Pair : ActiveEffects)
	{
		FActiveStatusEffectCDE& RuntimeEffect = Pair.Value;

		if (IsValid(RuntimeEffect.LogicInstance))
		{
			RefreshLogicRuntimeContext(RuntimeEffect, DeltaTime);
			if (RuntimeEffect.LogicInstance->RequiresTick(RuntimeEffect.LogicInstance->GetEffectContext()))
			{
				RuntimeEffect.LogicInstance->OnTick(RuntimeEffect.LogicInstance->GetEffectContext());
			}
		}

		if (RuntimeEffect.Definition.Data.DurationPolicy == EEffectDurationPolicy::Timed)
		{
			RuntimeEffect.TimeRemaining -= DeltaTime;
			if (RuntimeEffect.TimeRemaining <= 0.0f)
			{
				PendingRemovals.Add({ Pair.Key, EEffectRemoveReason::Expired });
			}
		}
		else if (RuntimeEffect.Definition.Data.DurationPolicy == EEffectDurationPolicy::Conditional)
		{
			if (!IsValid(RuntimeEffect.LogicInstance))
			{
				ensureMsgf(false, TEXT("Conditional effect '%s' has no LogicInstance."), *Pair.Key.ToString());
				PendingRemovals.Add({ Pair.Key, EEffectRemoveReason::ConditionMet });
			}
			else if (RuntimeEffect.LogicInstance->ShouldExpire(RuntimeEffect.LogicInstance->GetEffectContext()))
			{
				PendingRemovals.Add({ Pair.Key, EEffectRemoveReason::ConditionMet });
			}
		}
	}

	for (const FPendingRemove& Entry : PendingRemovals)
	{
		RemoveEffectInternal(Entry.Tag, Entry.Reason);
	}
}

void UActorCDE::UpdateComponentTickState()
{
	SetComponentTickEnabled(IsTickRequired());
}

bool UActorCDE::IsTickRequired()
{
	for (TPair<FGameplayTag, FActiveStatusEffectCDE>& Pair : ActiveEffects)
	{
		FActiveStatusEffectCDE& RuntimeEffect = Pair.Value;
		if (RuntimeEffect.Definition.Data.DurationPolicy == EEffectDurationPolicy::Timed
			|| RuntimeEffect.Definition.Data.DurationPolicy == EEffectDurationPolicy::Conditional)
		{
			return true;
		}

		if (IsValid(RuntimeEffect.LogicInstance))
		{
			RefreshLogicRuntimeContext(RuntimeEffect);
			if (RuntimeEffect.LogicInstance->RequiresTick(RuntimeEffect.LogicInstance->GetEffectContext()))
			{
				return true;
			}
		}
	}

	return false;
}
