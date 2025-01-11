// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

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
			"UMG" // Added for UMG support
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Add private modules here if needed
		});

		// Optional: Add include paths if required
		PublicIncludePaths.AddRange(new string[] { "LotA/Public" });
		PrivateIncludePaths.AddRange(new string[] { "LotA/Private" });
	}
}
