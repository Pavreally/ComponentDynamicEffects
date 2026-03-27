// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Structures/EffectStructuresCDE.h"
#include "DataAsset/EffectLogicBaseCDE.h"
#include "DataAsset/StatusEffectsDataAssetCDE.h"
#include "ActorCDE.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectAddedCDE, const FActiveStatusEffectCDE&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectUpdatedCDE, const FActiveStatusEffectCDE&);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectStackChangedCDE, const FActiveStatusEffectCDE&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEffectRemovedCDE, FGameplayTag, EEffectRemoveReason);
// Blueprint-assignable versions of the above delegates.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectAddedCDEBP, FActiveStatusEffectCDE, Effect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectUpdatedCDEBP, FActiveStatusEffectCDE, Effect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectStackChangedCDEBP, FActiveStatusEffectCDE, Effect);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEffectRemovedCDEBP, FGameplayTag, EffectTag, EEffectRemoveReason, Reason);

/**
 * Component Dynamic Effects (CDE).
 * Stores registered and active status effects and exposes gameplay-tag based queries for other systems.
 * 
 * Effects are defined as static data in FStatusEffectDefinitionCDE and registered to the component, which manages their runtime state in FActiveStatusEffectCDE.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COMPONENTDYNAMICEFFECTS_API UActorCDE : public UActorComponent
{
	GENERATED_BODY()

public:
	UActorCDE();

	/**
	 * Register or replace an effect definition available for tag-based application.
	 *
	 * @param EffectDefinition Definition struct describing the effect to register
	 * @return true if the definition was accepted and registered
	 */
	UFUNCTION(BlueprintCallable, Category = "Status Effects CDE|Registration", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool RegisterEffect(FStatusEffectDefinitionCDE EffectDefinition);

	/**
	 * Unregister an effect definition from the component library.
	 *
	 * @param EffectTag Tag of the registered effect to remove
	 * @param bRemoveActiveEffect If true, any active instances will also be removed
	 * @return true if a definition was unregistered
	 */
	UFUNCTION(BlueprintCallable, Category = "Status Effects CDE|Registration", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool UnregisterEffect(FGameplayTag EffectTag, bool bRemoveActiveEffect = false);

	/**
	 * Apply a registered effect definition by gameplay tag.
	 *
	 * @param EffectTag Tag of the effect definition to apply
	 * @param Instigator Actor responsible for the application (may be nullptr)
	 * @return true if an effect instance was applied
	 */
	UFUNCTION(BlueprintCallable, Category = "Status Effects CDE", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool ApplyEffect(FGameplayTag EffectTag, AActor* Instigator);

	/**
	 * Remove an effect by its gameplay tag.
	 *
	 * @param EffectTag Tag of the active effect to remove
	 * @return true if an active effect was found and removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Status Effects CDE", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool RemoveEffect(FGameplayTag EffectTag);

	/**
	 * Remove stacks from an active effect without forcing full removal.
	 *
	 * @param EffectTag Tag of the active effect
	 * @param StacksToRemove Number of stacks to remove (defaults to 1)
	 * @return true if stacks were removed
	 */
	UFUNCTION(BlueprintCallable, Category = "Status Effects CDE", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool RemoveEffectStack(FGameplayTag EffectTag, int32 StacksToRemove = 1);

	/**
	 * Returns true if the component has a registered effect definition for this tag.
	 *
	 * @param EffectTag Tag to query
	 * @return true if a definition for the tag exists in the registration cache
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE|Registration", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool HasRegisteredEffect(FGameplayTag EffectTag) const;

	/**
	 * Returns the registered base effect data for a single tag.
	 *
	 * @param EffectTag Tag to query
	 * @param OutEffectData Filled with the effect data if found
	 * @return true if effect data was found
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE|Registration", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool TryGetRegisteredEffectDataByTag(FGameplayTag EffectTag, FStatusEffectDataCDE& OutEffectData) const;

	/**
	 * Returns the full registered effect definition for a single tag.
	 *
	 * @param EffectTag Tag to query
	 * @param OutEffectDefinition Filled with the full definition if found
	 * @return true if a definition was found
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE|Registration", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool TryGetRegisteredEffectDefinitionByTag(FGameplayTag EffectTag, FStatusEffectDefinitionCDE& OutEffectDefinition) const;

	/**
	 * Returns the active runtime effect instance for a single tag.
	 *
	 * @param EffectTag Tag of the active effect to query
	 * @param OutActiveEffect Filled with the active runtime instance if present
	 * @return true if an active effect instance was found
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool TryGetActiveEffectByTag(FGameplayTag EffectTag, FActiveStatusEffectCDE& OutActiveEffect) const;

	/**
	 * Returns all currently registered effect tags.
	 *
	 * @return Container of registered effect tags
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE|Registration", meta = (Keywords = "CDE, Component Dynamic Effects"))
	FGameplayTagContainer GetRegisteredEffectTags() const;

	/**
	 * Check whether an effect is currently active.
	 *
	 * @param EffectTag Tag to query
	 * @return true if an active effect with the tag exists
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool HasEffect(FGameplayTag EffectTag) const;

	/**
	 * Returns false if any configured blocking effect for ActionTag is currently active.
	 *
	 * @param ActionTag Action tag to check against configured blockers
	 * @return true if the action can be performed (no blocking effects active)
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE|Actions", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool CanPerformAction(FGameplayTag ActionTag) const;

	/**
	 * Returns true if ActionTag has at least one active blocking effect.
	 *
	 * @param ActionTag Action tag to query
	 * @return true if there is at least one blocking effect active for the action
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE|Actions", meta = (Keywords = "CDE, Component Dynamic Effects"))
	bool HasBlockingEffect(FGameplayTag ActionTag) const;

	/**
	 * Fast external query surface aggregating all active effect tags.
	 *
	 * @return Container of currently active tags aggregated from running effects
	 */
	UFUNCTION(BlueprintPure, Category = "Status Effects CDE", meta = (Keywords = "CDE, Component Dynamic Effects"))
	FGameplayTagContainer GetAggregatedTags() const;

	const TMap<FGameplayTag, FActiveStatusEffectCDE>& GetActiveEffects() const
	{
		return ActiveEffects;
	}

	// Native delegates for systems that listen to effect state changes.
	FOnEffectAddedCDE OnEffectAddedCDE;
	FOnEffectUpdatedCDE OnEffectUpdatedCDE;
	FOnEffectStackChangedCDE OnEffectStackChangedCDE;
	FOnEffectRemovedCDE OnEffectRemovedCDE;

	// Blueprint-assignable delegates mirroring the native effect lifecycle events.
	UPROPERTY(BlueprintAssignable, Category = "Status Effects CDE", meta = (DisplayName = "OnEffectAdded", Keywords = "CDE, Component Dynamic Effects"))
	FOnEffectAddedCDEBP OnEffectAddedBP;

	UPROPERTY(BlueprintAssignable, Category = "Status Effects CDE", meta = (DisplayName = "OnEffectUpdated", Keywords = "CDE, Component Dynamic Effects"))
	FOnEffectUpdatedCDEBP OnEffectUpdatedBP;

	UPROPERTY(BlueprintAssignable, Category = "Status Effects CDE", meta = (DisplayName = "OnEffectStackChanged", Keywords = "CDE, Component Dynamic Effects"))
	FOnEffectStackChangedCDEBP OnEffectStackChangedBP;

	UPROPERTY(BlueprintAssignable, Category = "Status Effects CDE", meta = (DisplayName = "OnEffectRemoved", Keywords = "CDE, Component Dynamic Effects"))
	FOnEffectRemovedCDEBP OnEffectRemovedBP;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	 * Builds runtime state from a static effect definition and optionally creates runtime logic.
	 *
	 * @param OutRuntimeEffect Reference to the runtime struct to populate
	 * @param EffectDefinition Source static definition used to build runtime state
	 * @param Instigator The actor responsible for the effect application (may be nullptr)
	 * @param bCallOnApply If true, call the runtime OnApply hook after building
	 */
	void BuildRuntimeEffect(FActiveStatusEffectCDE& OutRuntimeEffect, const FStatusEffectDefinitionCDE& EffectDefinition, AActor* Instigator, bool bCallOnApply);

	/**
	 * Calls OnRemove on runtime logic and clears the instance pointer.
	 *
	 * @param RuntimeEffect The runtime effect instance to cleanup
	 * @param RemoveReason The reason that is forwarded to the logic callback context
	 */
	void CleanupRuntimeLogic(FActiveStatusEffectCDE& RuntimeEffect, EEffectRemoveReason RemoveReason = EEffectRemoveReason::Removed) const;

	/**
	 * Synchronizes the cached runtime context stored in a logic instance.
	 *
	 * @param RuntimeEffect The effect whose logic context should be refreshed
	 * @param DeltaTime Delta time for the current update pass
	 * @param RemoveReason Removal reason associated with the current callback
	 */
	void RefreshLogicRuntimeContext(FActiveStatusEffectCDE& RuntimeEffect, float DeltaTime = 0.0f, EEffectRemoveReason RemoveReason = EEffectRemoveReason::Removed) const;

	/**
	 * Removes an active effect with an explicit internal reason, used by delegates and tick logic.
	 *
	 * @param EffectTag Tag identifying the active effect to remove
	 * @param Reason Internal reason for removal (reported to delegates)
	 * @return true if an active effect was found and removed
	 */
	bool RemoveEffectInternal(FGameplayTag EffectTag, EEffectRemoveReason Reason);

	/**
	 * Loads hard-referenced effect assets into the registered effect library.
	 * Called during initialization to populate the registration cache.
	 */
	void InitializeRegisteredEffectsFromAssets();

	/**
	 * Ensures registered effects are initialized before public API access.
	 * Safe to call multiple times; performs lazy initialization.
	 */
	void EnsureRegisteredEffectsInitialized();

	/**
	 * Returns the full registered effect definition for a tag, preferring runtime registrations.
	 *
	 * @param EffectTag Tag to query
	 * @return Pointer to the found definition or nullptr if not found
	 */
	const FStatusEffectDefinitionCDE* FindRegisteredEffectDefinition(FGameplayTag EffectTag) const;

	/**
	 * Validates that an effect definition can be registered and applied safely.
	 *
	 * @param EffectDefinition Candidate definition to validate
	 * @param Context Human-readable context string for validation messages
	 * @param SourceObject Optional source object to attribute validation context
	 * @return true if the definition is valid
	 */
	bool ValidateEffectDefinition(const FStatusEffectDefinitionCDE& EffectDefinition, const TCHAR* Context, const UObject* SourceObject = nullptr) const;

	/**
	 * Rebuilds the registered effect tag cache from asset and runtime registrations.
	 */
	void RebuildRegisteredEffectTags();

	/**
	 * Rebuilds the aggregated active tag cache from currently active runtime effects.
	 */
	void RebuildAggregatedTags();

	/**
	 * Enables or disables component ticking based on whether any active effect requires per-frame updates.
	 */
	void UpdateComponentTickState();

	/**
	 * Returns true if any active effect requires TickComponent updates.
	 *
	 * @return true if ticking is required
	 */
	bool IsTickRequired();

	/**
	 * Broadcasts effect addition notifications to native and Blueprint subscribers.
	 *
	 * @param Effect The active effect instance that was added
	 */
	void BroadcastEffectAdded(const FActiveStatusEffectCDE& Effect);

	/**
	 * Broadcasts effect update notifications to native and Blueprint subscribers.
	 *
	 * @param Effect The active effect instance that was updated
	 */
	void BroadcastEffectUpdated(const FActiveStatusEffectCDE& Effect);

	/**
	 * Broadcasts stack change notifications to native and Blueprint subscribers.
	 *
	 * @param Effect The active effect instance whose stacks changed
	 */
	void BroadcastEffectStackChanged(const FActiveStatusEffectCDE& Effect);

	/**
	 * Broadcasts effect removal notifications to native and Blueprint subscribers.
	 *
	 * @param EffectTag The tag of the effect that was removed
	 * @param Reason The removal reason passed to listeners
	 */
	void BroadcastEffectRemoved(FGameplayTag EffectTag, EEffectRemoveReason Reason);

	/**
	 * Hard-referenced effect data assets loaded into the component library during initialization.
	 * These assets are scanned to populate the registration cache.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects CDE|Registration", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UStatusEffectsDataAssetCDE>> RegisteredEffectAssets;

	/**
	 * Registered effect definitions sourced from configured hard references.
	 * Populated during initialization from `RegisteredEffectAssets`.
	 */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Status Effects CDE|Registration")
	TMap<FGameplayTag, FStatusEffectDefinitionCDE> AssetRegisteredEffects;

	/**
	 * Effect definitions registered procedurally at runtime.
	 * These override or supplement asset-registered definitions.
	 */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Status Effects CDE|Registration")
	TMap<FGameplayTag, FStatusEffectDefinitionCDE> RuntimeRegisteredEffects;

	/**
	 * Cached set of all currently registered effect tags for quick queries.
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status Effects CDE|Registration", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RegisteredEffectTags;

	/**
	 * Runtime storage keyed by effect tag. Contains active effect instances and runtime state.
	 */
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "Status Effects CDE")
	TMap<FGameplayTag, FActiveStatusEffectCDE> ActiveEffects;

	/**
	 * Maps an action tag to effect tags that block that action. Configurable in editor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects CDE|Actions", meta = (AllowPrivateAccess = "true"))
	TMap<FGameplayTag, FGameplayTagContainer> ActionBlockingMap;

	/**
	 * Cached set of all currently active effect tags for fast read-only queries exposed to clients.
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Status Effects CDE", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer AggregatedTags;

	/**
	 * Asset-sourced tags explicitly unregistered at runtime (suppressed from registration cache).
	 */
	UPROPERTY(Transient)
	FGameplayTagContainer SuppressedAssetEffectTags;

	/**
	 * Tracks whether hard-referenced effect assets were already loaded into the registration cache.
	 */
	bool bRegisteredEffectsInitialized = false;

	/**
	 * Tracks whether a game-world refresh was performed after any editor-time initialization.
	 */
	bool bRuntimeRegisteredEffectsRefreshed = false;
};
