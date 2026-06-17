// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class MelodiaMelusina_PROD : ModuleRules
{
	public MelodiaMelusina_PROD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AudioMixer", "UMG", "Slate", "SlateCore", "PCG" });

		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "PCG"));
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "BuildMode"));

		PrivateDependencyModuleNames.AddRange(new string[] { "ImageWrapper", "AssetRegistry" });

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "EditorScriptingUtilities", "AssetTools", "EditorSubsystem" });
		}
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
