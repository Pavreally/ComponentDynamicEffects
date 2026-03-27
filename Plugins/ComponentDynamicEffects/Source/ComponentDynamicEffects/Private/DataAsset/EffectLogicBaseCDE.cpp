// Pavel Gornostaev <https://github.com/Pavreally>

#include "DataAsset/EffectLogicBaseCDE.h"
#include "Components/ActorCDE.h"
#include "GameFramework/Actor.h"

namespace
{
void RebuildModifierLookup(const FStatusEffectDefinitionCDE& InDefinition, TMap<FGameplayTag, float>& OutLookup)
{
	OutLookup.Reset();

	for (const FStatusEffectModifierCDE& Modifier : InDefinition.Modifiers)
	{
		if (Modifier.ModifierTag.IsValid())
		{
			OutLookup.Add(Modifier.ModifierTag, Modifier.Value);
		}
	}
}
}

UEffectLogicBaseCDE::UEffectLogicBaseCDE()
{
}

void UEffectLogicBaseCDE::Initialize(UActorCDE *InOwningComponent, AActor *InTarget, AActor *InInstigator, const FStatusEffectDefinitionCDE &InDefinition)
{
	OwningComponent = InOwningComponent;
	TargetActor = InTarget;
	InstigatorActor = InInstigator;
	RuntimeDefinition = InDefinition;
	RuntimeContext = FEffectExecutionContextCDE{};
	RuntimeContext.EffectComponent = InOwningComponent;
	RuntimeContext.Target = InTarget;
	RuntimeContext.Instigator = InInstigator;
	RuntimeContext.EffectTag = InDefinition.Data.EffectTag;
	RuntimeContext.DurationPolicy = InDefinition.Data.DurationPolicy;
	RuntimeContext.StackingPolicy = InDefinition.Data.StackingPolicy;
	RuntimeContext.Duration = InDefinition.Data.Duration;
	RuntimeContext.MaxStacks = FMath::Max(1, InDefinition.Data.MaxStacks);
	RuntimeContext.Priority = InDefinition.Data.Priority;
	RuntimeContext.CurrentStacks = 1;
	RuntimeContext.TimeRemaining = (InDefinition.Data.DurationPolicy == EEffectDurationPolicy::Timed)
		? FMath::Max(0.0f, InDefinition.Data.Duration)
		: 0.0f;
	RuntimeContext.Definition = InDefinition;

	RebuildModifierLookup(RuntimeDefinition, RuntimeModifierLookup);
}

void UEffectLogicBaseCDE::RefreshRuntimeContext(const FActiveStatusEffectCDE &InRuntimeEffect, float InDeltaTime, EEffectRemoveReason InRemoveReason)
{
	InstigatorActor = InRuntimeEffect.Instigator;
	RuntimeDefinition = InRuntimeEffect.Definition;

	RuntimeContext.EffectComponent = OwningComponent.Get();
	RuntimeContext.Target = TargetActor.Get();
	RuntimeContext.Instigator = InRuntimeEffect.Instigator;
	RuntimeContext.EffectTag = InRuntimeEffect.Definition.Data.EffectTag;
	RuntimeContext.DurationPolicy = InRuntimeEffect.Definition.Data.DurationPolicy;
	RuntimeContext.StackingPolicy = InRuntimeEffect.Definition.Data.StackingPolicy;
	RuntimeContext.Duration = InRuntimeEffect.Definition.Data.Duration;
	RuntimeContext.MaxStacks = FMath::Max(1, InRuntimeEffect.Definition.Data.MaxStacks);
	RuntimeContext.Priority = InRuntimeEffect.Definition.Data.Priority;
	RuntimeContext.CurrentStacks = InRuntimeEffect.CurrentStacks;
	RuntimeContext.TimeRemaining = InRuntimeEffect.TimeRemaining;
	RuntimeContext.DeltaTime = InDeltaTime;
	RuntimeContext.RemoveReason = InRemoveReason;
	RuntimeContext.Definition = InRuntimeEffect.Definition;

	RebuildModifierLookup(RuntimeDefinition, RuntimeModifierLookup);
}

void UEffectLogicBaseCDE::OnApply_Implementation(const FEffectExecutionContextCDE &Context)
{
	(void)Context;
}

void UEffectLogicBaseCDE::OnRemove_Implementation(const FEffectExecutionContextCDE &Context)
{
	(void)Context;
}

void UEffectLogicBaseCDE::OnTick_Implementation(const FEffectExecutionContextCDE &Context)
{
	(void)Context;
}

bool UEffectLogicBaseCDE::RequiresTick_Implementation(const FEffectExecutionContextCDE &Context) const
{
	(void)Context;
	return false;
}

bool UEffectLogicBaseCDE::ShouldExpire_Implementation(const FEffectExecutionContextCDE &Context) const
{
	(void)Context;
	return false;
}

void UEffectLogicBaseCDE::OnEffectUpdated_Implementation(const FEffectExecutionContextCDE &Context)
{
	(void)Context;
}

void UEffectLogicBaseCDE::OnStackChanged_Implementation(const FEffectExecutionContextCDE &Context)
{
	(void)Context;
}

bool UEffectLogicBaseCDE::TryGetModifierValue(FGameplayTag ModifierTag, float &OutValue) const
{
	OutValue = 0.0f;

	if (!ModifierTag.IsValid())
	{
		return false;
	}

	if (const float *Value = RuntimeModifierLookup.Find(ModifierTag))
	{
		OutValue = *Value;
		return true;
	}

	return false;
}

UActorCDE *UEffectLogicBaseCDE::GetCDEComponent() const
{
	return OwningComponent.Get();
}

void UEffectLogicBaseCDE::BroadcastEffectEvent(FGameplayTag EventTag)
{
	OnEffectLogicEvent.Broadcast(EventTag);
	OnEffectLogicEventBP.Broadcast(EventTag);
}
