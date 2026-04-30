// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ArenaGeneratorEditor : ModuleRules
{
	public ArenaGeneratorEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ArenaGenerator",
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Settings",
				"DeveloperSettings",
				"Slate",
				"SlateCore",
			});
	}
}
