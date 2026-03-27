// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Structures/EffectStructuresCDE.h"

#include "EffectLogicBaseCDE.generated.h"

class UActorCDE;

/**
 * Base class for effect logic implementations.
 * Provides Blueprintable hooks for custom effect behavior while maintaining separation from core CDE architecture.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class COMPONENTDYNAMICEFFECTS_API UEffectLogicBaseCDE : public UObject
{
	GENERATED_BODY()

public:
	UEffectLogicBaseCDE();

	/**
	 * Initializes runtime context with an explicit owning CDE component.
	 * Called immediately after creation.
	 */
	void Initialize(UActorCDE *InOwningComponent, AActor *InTarget, AActor *InInstigator, const FStatusEffectDefinitionCDE &InDefinition);

	/**
	 * Refreshes the runtime context snapshot before invoking Blueprint callbacks.
	 */
	void RefreshRuntimeContext(const FActiveStatusEffectCDE &InRuntimeEffect, float InDeltaTime = 0.0f, EEffectRemoveReason InRemoveReason = EEffectRemoveReason::Removed);

	/**
	 * Called when effect is applied.
	 * Executed after validation but before state change.
	 *
	 * @param Context Current runtime context for this effect instance
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	void OnApply(const FEffectExecutionContextCDE &Context);
	virtual void OnApply_Implementation(const FEffectExecutionContextCDE &Context);

	/**
	 * Called when effect is removed.
	 * Executed before cleanup.
	 *
	 * @param Context Current runtime context for this effect instance
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	void OnRemove(const FEffectExecutionContextCDE &Context);
	virtual void OnRemove_Implementation(const FEffectExecutionContextCDE &Context);

	/**
	 * Called every frame if RequiresTick() returns true.
	 * Executed during the tick phase.
	 *
	 * @param Context Current runtime context for this effect instance
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	void OnTick(const FEffectExecutionContextCDE &Context);
	virtual void OnTick_Implementation(const FEffectExecutionContextCDE &Context);

	/**
	 * Whether this logic needs ticking.
	 * Determines if OnTick will be called.
	 *
	 * @return True if this effect requires ticking
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	bool RequiresTick(const FEffectExecutionContextCDE &Context) const;
	virtual bool RequiresTick_Implementation(const FEffectExecutionContextCDE &Context) const;

	/**
	 * Whether this effect should expire (used for Conditional policy).
	 * Checked during expiration evaluation.
	 *
	 * @return True if this effect should expire
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	bool ShouldExpire(const FEffectExecutionContextCDE &Context) const;
	virtual bool ShouldExpire_Implementation(const FEffectExecutionContextCDE &Context) const;

	/**
	 * Called when the runtime effect instance changes without being removed.
	 * Triggered on reapply, duration refresh, instigator change, and other runtime updates.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	void OnEffectUpdated(const FEffectExecutionContextCDE &Context);
	virtual void OnEffectUpdated_Implementation(const FEffectExecutionContextCDE &Context);

	/**
	 * Called when the active stack count changes.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Effect Logic CDE")
	void OnStackChanged(const FEffectExecutionContextCDE &Context);
	virtual void OnStackChanged_Implementation(const FEffectExecutionContextCDE &Context);

	/**
	 * Access modifier value.
	 * Retrieves the value of a specific modifier tag.
	 *
	 * @param ModifierTag The tag of the modifier to retrieve
	 * @param OutValue Output parameter for the modifier value
	 * @return True if the modifier was found and value retrieved
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	bool TryGetModifierValue(FGameplayTag ModifierTag, float &OutValue) const;

	/**
	 * Get the target actor of this effect logic.
	 *
	 * @return Target actor
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	AActor *GetTargetActor() const { return TargetActor.Get(); }

	/**
	 * Get the instigator actor of this effect logic.
	 *
	 * @return Instigator actor
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	AActor *GetInstigatorActor() const { return InstigatorActor.Get(); }

	/**
	 * Get the CDE component that owns this effect logic.
	 *
	 * @return CDE component
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	UActorCDE *GetCDEComponent() const;

	/**
	 * Get the latest runtime context snapshot for this logic instance.
	 */
	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	FEffectExecutionContextCDE GetEffectContext() const { return RuntimeContext; }

	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	FGameplayTag GetEffectTag() const { return RuntimeContext.EffectTag; }

	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	int32 GetCurrentStacks() const { return RuntimeContext.CurrentStacks; }

	UFUNCTION(BlueprintPure, Category = "Effect Logic CDE")
	float GetTimeRemaining() const { return RuntimeContext.TimeRemaining; }

	/**
	 * Broadcast custom event.
	 * Triggers both native and Blueprint delegates.
	 *
	 * @param EventTag The tag of the event to broadcast
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect Logic CDE")
	void BroadcastEffectEvent(FGameplayTag EventTag);

public:
	/** Native delegate for effect logic events */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnEffectLogicEventCDE, FGameplayTag);
	FOnEffectLogicEventCDE OnEffectLogicEvent;

	/** Blueprint delegate for effect logic events */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEffectLogicEventBP, FGameplayTag, EventTag);

	UPROPERTY(BlueprintAssignable, Category = "Effect Logic CDE")
	FOnEffectLogicEventBP OnEffectLogicEventBP;

protected:
	// Cached reference to the owning CDE component
	UPROPERTY(Transient)
	TWeakObjectPtr<UActorCDE> OwningComponent;

	// Cached reference to the target actor
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> TargetActor;

	// Cached reference to the instigator actor
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> InstigatorActor;

	// Runtime copy of the effect definition
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Effect Logic CDE")
	FStatusEffectDefinitionCDE RuntimeDefinition;

	// Flattened runtime context used by Blueprint callbacks.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Effect Logic CDE")
	FEffectExecutionContextCDE RuntimeContext;

private:
	// Lookup table for modifier values
	TMap<FGameplayTag, float> RuntimeModifierLookup;
};
