// Pavel Gornostaev <https://github.com/Pavreally>

#include "Components/ActorCDE.h"

#include "ComponentDynamicEffects.h"
#include "Engine/World.h"

bool UActorCDE::RegisterEffect(FStatusEffectDefinitionCDE EffectDefinition)
{
	EnsureRegisteredEffectsInitialized();

	if (!ValidateEffectDefinition(EffectDefinition, TEXT("ActorCDE::RegisterEffect")))
	{
		return false;
	}

	const FGameplayTag EffectTag = EffectDefinition.Data.EffectTag;
	if (RuntimeRegisteredEffects.Contains(EffectTag))
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("ActorCDE::RegisterEffect - Replacing runtime effect definition for tag '%s'."), *EffectTag.ToString());
	}
	else if (AssetRegisteredEffects.Contains(EffectTag) && !SuppressedAssetEffectTags.HasTagExact(EffectTag))
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("ActorCDE::RegisterEffect - Runtime definition overrides asset-backed effect '%s'."), *EffectTag.ToString());
	}

	RuntimeRegisteredEffects.FindOrAdd(EffectTag) = EffectDefinition;
	SuppressedAssetEffectTags.RemoveTag(EffectTag);
	RebuildRegisteredEffectTags();
	return true;
}

bool UActorCDE::UnregisterEffect(FGameplayTag EffectTag, bool bRemoveActiveEffect)
{
	EnsureRegisteredEffectsInitialized();

	if (!EffectTag.IsValid())
	{
		return false;
	}

	bool bWasRegistered = RuntimeRegisteredEffects.Remove(EffectTag) > 0;
	if (AssetRegisteredEffects.Contains(EffectTag) && !SuppressedAssetEffectTags.HasTagExact(EffectTag))
	{
		SuppressedAssetEffectTags.AddTag(EffectTag);
		bWasRegistered = true;
	}

	if (!bWasRegistered)
	{
		return false;
	}

	RebuildRegisteredEffectTags();

	if (bRemoveActiveEffect)
	{
		RemoveEffect(EffectTag);
	}

	return true;
}

void UActorCDE::InitializeRegisteredEffectsFromAssets()
{
	AssetRegisteredEffects.Reset();

	for (const UStatusEffectsDataAssetCDE* EffectAsset : RegisteredEffectAssets)
	{
		if (!IsValid(EffectAsset))
		{
			UE_LOG(LogComponentDynamicEffects, Warning, TEXT("ActorCDE::InitializeRegisteredEffectsFromAssets - Encountered invalid StatusEffectsDataAsset."));
			continue;
		}

		const FStatusEffectDefinitionCDE& EffectDefinition = EffectAsset->EffectDefinition;
		if (!ValidateEffectDefinition(EffectDefinition, TEXT("ActorCDE::InitializeRegisteredEffectsFromAssets"), EffectAsset))
		{
			continue;
		}

		const FGameplayTag EffectTag = EffectDefinition.Data.EffectTag;
		if (AssetRegisteredEffects.Contains(EffectTag))
		{
			UE_LOG(LogComponentDynamicEffects, Warning, TEXT("ActorCDE::InitializeRegisteredEffectsFromAssets - Duplicate asset registration for '%s'. Later asset overrides earlier one."), *EffectTag.ToString());
		}

		AssetRegisteredEffects.FindOrAdd(EffectTag) = EffectDefinition;
	}

	TArray<FGameplayTag> SuppressedTags;
	for (const FGameplayTag& SuppressedTag : SuppressedAssetEffectTags)
	{
		SuppressedTags.Add(SuppressedTag);
	}

	for (const FGameplayTag& SuppressedTag : SuppressedTags)
	{
		if (!AssetRegisteredEffects.Contains(SuppressedTag))
		{
			SuppressedAssetEffectTags.RemoveTag(SuppressedTag);
		}
	}

	bRegisteredEffectsInitialized = true;
	RebuildRegisteredEffectTags();
}

void UActorCDE::EnsureRegisteredEffectsInitialized()
{
	UWorld* World = GetWorld();
	const bool bIsGameWorld = World && World->IsGameWorld();
	const bool bNeedsRuntimeRefresh = bIsGameWorld && !bRuntimeRegisteredEffectsRefreshed;

	if (!bRegisteredEffectsInitialized || bNeedsRuntimeRefresh)
	{
		InitializeRegisteredEffectsFromAssets();
		bRuntimeRegisteredEffectsRefreshed = bIsGameWorld;
	}
}

const FStatusEffectDefinitionCDE* UActorCDE::FindRegisteredEffectDefinition(FGameplayTag EffectTag) const
{
	if (!EffectTag.IsValid())
	{
		return nullptr;
	}

	if (const FStatusEffectDefinitionCDE* RuntimeDefinition = RuntimeRegisteredEffects.Find(EffectTag))
	{
		return RuntimeDefinition;
	}

	if (SuppressedAssetEffectTags.HasTagExact(EffectTag))
	{
		return nullptr;
	}

	return AssetRegisteredEffects.Find(EffectTag);
}

bool UActorCDE::ValidateEffectDefinition(const FStatusEffectDefinitionCDE& EffectDefinition, const TCHAR* Context, const UObject* SourceObject) const
{
	const FStatusEffectDataCDE& EffectData = EffectDefinition.Data;
	const FString SourceName = IsValid(SourceObject) ? GetNameSafe(SourceObject) : TEXT("Runtime");

	if (!EffectData.EffectTag.IsValid())
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Invalid EffectTag. Source: %s"), Context, *SourceName);
		return false;
	}

	if (EffectData.DurationPolicy == EEffectDurationPolicy::Conditional && EffectDefinition.EffectLogicClass.Get() == nullptr)
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Conditional effect '%s' requires EffectLogicClass. Source: %s"), Context, *EffectData.EffectTag.ToString(), *SourceName);
		return false;
	}

	if (EffectData.Duration < 0.0f)
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Effect '%s' has negative duration. Source: %s"), Context, *EffectData.EffectTag.ToString(), *SourceName);
		return false;
	}

	if (EffectData.MaxStacks <= 0)
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Effect '%s' must have MaxStacks > 0. Source: %s"), Context, *EffectData.EffectTag.ToString(), *SourceName);
		return false;
	}

	if (EffectData.StackingPolicy != EStackingPolicy::AddStack && EffectData.MaxStacks != 1)
	{
		UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Effect '%s' has MaxStacks > 1 but is not using AddStack. Source: %s"), Context, *EffectData.EffectTag.ToString(), *SourceName);
		return false;
	}

	TSet<FGameplayTag> UniqueModifierTags;
	for (const FStatusEffectModifierCDE& Modifier : EffectDefinition.Modifiers)
	{
		if (!Modifier.ModifierTag.IsValid())
		{
			UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Effect '%s' contains an invalid modifier tag. Source: %s"), Context, *EffectData.EffectTag.ToString(), *SourceName);
			return false;
		}

		if (UniqueModifierTags.Contains(Modifier.ModifierTag))
		{
			UE_LOG(LogComponentDynamicEffects, Warning, TEXT("%s - Effect '%s' contains duplicate modifier tag '%s'. Source: %s"), Context, *EffectData.EffectTag.ToString(), *Modifier.ModifierTag.ToString(), *SourceName);
			return false;
		}

		UniqueModifierTags.Add(Modifier.ModifierTag);
	}

	return true;
}

void UActorCDE::RebuildRegisteredEffectTags()
{
	RegisteredEffectTags.Reset();

	for (const TPair<FGameplayTag, FStatusEffectDefinitionCDE>& Pair : AssetRegisteredEffects)
	{
		if (Pair.Key.IsValid() && !SuppressedAssetEffectTags.HasTagExact(Pair.Key))
		{
			RegisteredEffectTags.AddTag(Pair.Key);
		}
	}

	for (const TPair<FGameplayTag, FStatusEffectDefinitionCDE>& Pair : RuntimeRegisteredEffects)
	{
		if (Pair.Key.IsValid())
		{
			RegisteredEffectTags.AddTag(Pair.Key);
		}
	}
}
