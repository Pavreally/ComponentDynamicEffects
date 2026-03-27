// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Structures/EffectStructuresCDE.h"

#include "StatusEffectsDataAssetCDE.generated.h"

/**
 * Data asset containing a single effect definition and optional runtime logic class.
 */
UCLASS(BlueprintType)
class COMPONENTDYNAMICEFFECTS_API UStatusEffectsDataAssetCDE : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Full authoring definition for a single effect asset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status Effect", meta = (ToolTip = "Authoring definition stored in this data asset"))
	FStatusEffectDefinitionCDE EffectDefinition;
};
