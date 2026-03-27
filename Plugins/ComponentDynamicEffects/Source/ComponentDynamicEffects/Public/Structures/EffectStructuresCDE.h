// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

#include "EffectStructuresCDE.generated.h"

class UEffectLogicBaseCDE;
class UActorCDE;

class AActor;

USTRUCT(BlueprintType)
struct FStatusEffectModifierCDE
{
	GENERATED_BODY()

public:
	// Unique gameplay tag used to query this modifier at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect|Modifiers", meta = (ToolTip = "Identifier for this modifier, used to query modifier values at runtime"))
	FGameplayTag ModifierTag;

	// Numeric value assigned to ModifierTag.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect|Modifiers", meta = (ToolTip = "Numeric value applied by this modifier"))
	float Value = 0.0f;
};

/**
 * Describes how long a status effect should remain active.
 */
UENUM(BlueprintType)
enum class EEffectDurationPolicy : uint8
{
	Instant,
	Timed,
	Infinite,
	Conditional
};

UENUM(BlueprintType)
enum class EStackingPolicy : uint8
{
	RefreshDuration,
	AddStack,
	Replace,
	Ignore
};

UENUM(BlueprintType)
enum class EEffectRemoveReason : uint8
{
	Expired,
	Removed,
	Replaced,
	ConditionMet
};

/**
 * Static data definition for a status effect.
 * This struct is authored in data assets and copied into runtime effect instances.
 */
USTRUCT(BlueprintType)
struct FStatusEffectDataCDE
{
	GENERATED_BODY()

public:
	// Primary tag used to register, query, apply and remove this effect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ToolTip = "Primary gameplay tag identifying this effect"))
	FGameplayTag EffectTag;

	// Determines how this effect ends its lifetime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ToolTip = "Policy describing effect lifetime (Instant, Timed, Infinite, Conditional)"))
	EEffectDurationPolicy DurationPolicy = EEffectDurationPolicy::Infinite;

	// Duration in seconds when DurationPolicy is Timed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ClampMin = "0.0", EditCondition = "DurationPolicy == EEffectDurationPolicy::Timed", EditConditionHides, ToolTip = "Duration in seconds when using Timed duration policy"))
	float Duration = 0.0f;

	// Defines how reapplying the same effect behaves.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect|Stacking", meta = (ToolTip = "How reapplication of the same effect is handled (RefreshDuration, AddStack, etc.)"))
	EStackingPolicy StackingPolicy = EStackingPolicy::RefreshDuration;

	// Maximum number of stacks when StackingPolicy is AddStack.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect|Stacking", meta = (ClampMin = "1", EditCondition = "StackingPolicy == EStackingPolicy::AddStack", EditConditionHides, ToolTip = "Maximum stacks allowed when using AddStack policy"))
	int32 MaxStacks = 1;

	// Used to reject or replace lower-priority active effects.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ToolTip = "Priority used to resolve conflicts when multiple effects interact") )
	int32 Priority = 0;

	/** Active effects from this list will be replaced when this effect has higher priority. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect|Priority", meta = (ToolTip = "Tags of effects that will be canceled if this effect has higher priority"))
	FGameplayTagContainer CancelEffectsWithLowerPriority;

	// Active effects removed immediately before this effect is applied.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ToolTip = "Tags of effects to remove before applying this effect"))
	FGameplayTagContainer RemoveOnApply;
};

USTRUCT(BlueprintType)
struct FStatusEffectDefinitionCDE
{
	GENERATED_BODY()

public:
	// Core behavior settings for the effect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ToolTip = "Core, author-authored settings for the effect"))
	FStatusEffectDataCDE Data;

	// Optional runtime logic object instantiated while the effect is active.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect", meta = (ToolTip = "Optional runtime logic class instantiated for the effect"))
	TSubclassOf<UEffectLogicBaseCDE> EffectLogicClass;

	// Custom author-defined parameters available to the effect logic at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effect|Modifiers", meta = (ToolTip = "List of custom modifiers available to runtime logic"))
	TArray<FStatusEffectModifierCDE> Modifiers;

	bool TryGetModifierValue(FGameplayTag ModifierTag, float& OutValue) const
	{
		OutValue = 0.0f;
		if (!ModifierTag.IsValid())
		{
			return false;
		}

		for (const FStatusEffectModifierCDE& Modifier : Modifiers)
		{
			if (Modifier.ModifierTag == ModifierTag)
			{
				OutValue = Modifier.Value;
				return true;
			}
		}

		return false;
	}
};

/**
 * Runtime context passed into effect-logic callbacks.
 * Mirrors the most commonly used runtime values so Blueprint logic does not need to unpack nested structs every frame.
 */
USTRUCT(BlueprintType)
struct FEffectExecutionContextCDE
{
	GENERATED_BODY()

public:
	// Owning CDE component managing this effect instance.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	TObjectPtr<UActorCDE> EffectComponent = nullptr;

	// Actor currently affected by this effect.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	TObjectPtr<AActor> Target = nullptr;

	// Actor responsible for applying or refreshing this effect instance.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	TObjectPtr<AActor> Instigator = nullptr;

	// Primary runtime tag of this effect for direct Blueprint access.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	FGameplayTag EffectTag;

	// Cached duration policy to avoid drilling into Definition.Data.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	EEffectDurationPolicy DurationPolicy = EEffectDurationPolicy::Infinite;

	// Cached stacking policy to avoid drilling into Definition.Data.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	EStackingPolicy StackingPolicy = EStackingPolicy::RefreshDuration;

	// Authored duration in seconds when DurationPolicy is Timed.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	float Duration = 0.0f;

	// Maximum number of stacks allowed for this effect definition.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	int32 MaxStacks = 1;

	// Authored priority of this effect definition.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	int32 Priority = 0;

	// Current active stack count of the runtime instance.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	int32 CurrentStacks = 1;

	// Remaining time for timed effects. Zero for non-timed effects.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	float TimeRemaining = 0.0f;

	// Delta time for the current tick/update pass.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	float DeltaTime = 0.0f;

	// Removal reason associated with the current callback.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	EEffectRemoveReason RemoveReason = EEffectRemoveReason::Removed;

	// Full runtime definition snapshot for advanced cases.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect Context")
	FStatusEffectDefinitionCDE Definition;
};

/**
 * Runtime representation of an active effect instance.
 */
USTRUCT(BlueprintType)
struct FActiveStatusEffectCDE
{
	GENERATED_BODY()

public:
	// Runtime snapshot of the definition used to build this active effect.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect", meta = (ToolTip = "Runtime copy of the definition used to create this active instance"))
	FStatusEffectDefinitionCDE Definition;

	// Remaining duration in seconds for timed effects.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect", meta = (ToolTip = "Remaining time (seconds) for timed effects"))
	float TimeRemaining = 0.0f;

	// Current stack count for stackable effects.
	UPROPERTY(BlueprintReadOnly, Category = "Status Effect", meta = (ToolTip = "Current stack count for this active effect"))
	int32 CurrentStacks = 1;

	// Actor that applied this active effect instance.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Status Effect", meta = (ToolTip = "Actor that instigated this effect"))
	TObjectPtr<AActor> Instigator = nullptr;

	// Optional runtime behavior object created from FStatusEffectDefinitionCDE::EffectLogicClass.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Status Effect", meta = (ToolTip = "Optional runtime behavior instance for this effect"))
	TObjectPtr<UEffectLogicBaseCDE> LogicInstance = nullptr;
};
