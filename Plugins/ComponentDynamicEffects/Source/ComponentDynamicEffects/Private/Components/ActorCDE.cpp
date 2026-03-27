// Pavel Gornostaev <https://github.com/Pavreally>

#include "Components/ActorCDE.h"

UActorCDE::UActorCDE()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
}

void UActorCDE::OnRegister()
{
	Super::OnRegister();

	bRuntimeRegisteredEffectsRefreshed = false;
	InitializeRegisteredEffectsFromAssets();
}

void UActorCDE::BeginPlay()
{
	Super::BeginPlay();

	EnsureRegisteredEffectsInitialized();
	RebuildAggregatedTags();
	UpdateComponentTickState();
}

void UActorCDE::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!ActiveEffects.IsEmpty())
	{
		TArray<FGameplayTag> EffectTags;
		ActiveEffects.GetKeys(EffectTags);

		for (const FGameplayTag& EffectTag : EffectTags)
		{
			RemoveEffect(EffectTag);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UActorCDE::BroadcastEffectAdded(const FActiveStatusEffectCDE& Effect)
{
	if (OnEffectAddedCDE.IsBound())
	{
		OnEffectAddedCDE.Broadcast(Effect);
	}

	if (OnEffectAddedBP.IsBound())
	{
		OnEffectAddedBP.Broadcast(Effect);
	}
}

void UActorCDE::BroadcastEffectUpdated(const FActiveStatusEffectCDE& Effect)
{
	if (OnEffectUpdatedCDE.IsBound())
	{
		OnEffectUpdatedCDE.Broadcast(Effect);
	}

	if (OnEffectUpdatedBP.IsBound())
	{
		OnEffectUpdatedBP.Broadcast(Effect);
	}
}

void UActorCDE::BroadcastEffectStackChanged(const FActiveStatusEffectCDE& Effect)
{
	if (OnEffectStackChangedCDE.IsBound())
	{
		OnEffectStackChangedCDE.Broadcast(Effect);
	}

	if (OnEffectStackChangedBP.IsBound())
	{
		OnEffectStackChangedBP.Broadcast(Effect);
	}
}

void UActorCDE::BroadcastEffectRemoved(FGameplayTag EffectTag, EEffectRemoveReason Reason)
{
	if (OnEffectRemovedCDE.IsBound())
	{
		OnEffectRemovedCDE.Broadcast(EffectTag, Reason);
	}

	if (OnEffectRemovedBP.IsBound())
	{
		OnEffectRemovedBP.Broadcast(EffectTag, Reason);
	}
}
