using UnrealBuildTool;

public class NammaCityEditorTarget : TargetRules
{
    public NammaCityEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("NammaCity");
    }
}
