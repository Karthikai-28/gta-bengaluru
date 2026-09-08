using UnrealBuildTool;

public class NammaCityTarget : TargetRules
{
    public NammaCityTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("NammaCity");
    }
}
