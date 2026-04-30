/*
 * MIT License — same as ArenaGenerator plugin
 */

#include "ArenaGeneratorEditorModule.h"
#include "ArenaGeneratorSettings.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "ArenaGeneratorEditorModule"

void FArenaGeneratorEditorModule::StartupModule()
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings(
			"Project",
			"Plugins",
			"Arena Generator",
			LOCTEXT("ArenaGeneratorSettingsName", "Arena Generator"),
			LOCTEXT("ArenaGeneratorSettingsDescription", "Configuration for the Arena Generator plugin."),
			GetMutableDefault<UArenaGeneratorSettings>()
		);

		if (SettingsSection.IsValid())
		{
			SettingsSection->OnModified().BindRaw(this, &FArenaGeneratorEditorModule::HandleSettingsSaved);
		}
	}
}

void FArenaGeneratorEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Project", "Plugins", "Arena Generator");
		}
	}
}

bool FArenaGeneratorEditorModule::HandleSettingsSaved()
{
	UArenaGeneratorSettings* Settings = GetMutableDefault<UArenaGeneratorSettings>();
	bool ResaveSettings = false;

	if (ResaveSettings && Settings)
	{
		Settings->SaveConfig();
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FArenaGeneratorEditorModule, ArenaGeneratorEditor)
