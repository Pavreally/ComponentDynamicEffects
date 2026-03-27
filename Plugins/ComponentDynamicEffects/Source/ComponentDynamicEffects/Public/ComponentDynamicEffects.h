// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/** Log category for ComponentDynamicEffects plugin. */
COMPONENTDYNAMICEFFECTS_API DECLARE_LOG_CATEGORY_EXTERN(LogComponentDynamicEffects, Log, All);

class FComponentDynamicEffectsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
