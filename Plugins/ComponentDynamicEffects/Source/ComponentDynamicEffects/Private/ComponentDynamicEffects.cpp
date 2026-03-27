// Pavel Gornostaev <https://github.com/Pavreally>

#include "ComponentDynamicEffects.h"
#include "GameplayTagsManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "FComponentDynamicEffectsModule"

// Define the log category.
DEFINE_LOG_CATEGORY(LogComponentDynamicEffects);

void FComponentDynamicEffectsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	// Adding a path to plugin tags
	UGameplayTagsManager::Get().AddTagIniSearchPath(
		FPaths::ProjectPluginsDir() / TEXT("ComponentDynamicEffects/Config/Tags"));
}

void FComponentDynamicEffectsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FComponentDynamicEffectsModule, ComponentDynamicEffects)
