// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class LotA : ModuleRules
{
	public LotA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"HTTP", 
			"Json", 
			"JsonUtilities", 
			"Slate", 
			"SlateCore",
			"UMG",
			"ApplicationCore",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Add private modules here if needed
		});

		// Explicitly add Slate include paths
		PublicIncludePaths.AddRange(new string[]
		{
			Path.Combine(EngineDirectory, "Source", "Runtime", "Slate", "Public"),
			Path.Combine(EngineDirectory, "Source", "Runtime", "SlateCore", "Public"),
			Path.Combine(EngineDirectory, "Source", "Runtime", "ApplicationCore", "Public")
		});

		PublicIncludePaths.Add(Path.Combine(EngineDirectory, "Source", "Runtime", "UMG", "Public"));
        
		// Add module include paths
		PublicIncludePaths.AddRange(new string[] { "LotA/Public" });
		PrivateIncludePaths.AddRange(new string[] { "LotA/Private" });
	}
}