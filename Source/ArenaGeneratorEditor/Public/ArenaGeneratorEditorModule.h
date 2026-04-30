/*
 * MIT License — same as ArenaGenerator plugin
 */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FArenaGeneratorEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool HandleSettingsSaved();
};
