using UnrealBuildTool;

public class GunnerEditor : ModuleRules
{
    public GunnerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new[] {
            "Gunner", "UnrealEd", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph",
            "KismetCompiler", "AssetRegistry", "ControlRig"
        });
    }
}
