using UnrealBuildTool;
public class GunnerEditorTarget : TargetRules
{
    public GunnerEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("Gunner");
    }
}
