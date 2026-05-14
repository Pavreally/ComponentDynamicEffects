// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StatusEffectsDataAssetCDE.h"

#include "StatusEffectCollectionDataAssetCDE.generated.h"

UCLASS(BlueprintType)
class COMPONENTDYNAMICEFFECTS_API UStatusEffectCollectionDataAssetCDE : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status Effects")
	TArray<TObjectPtr<UStatusEffectsDataAssetCDE>> Effects;
};
