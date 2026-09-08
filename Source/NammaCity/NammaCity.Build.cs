using UnrealBuildTool;

public class NammaCity : ModuleRules
{
    public NammaCity(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput",
                "AnimationCore"
            }
        );
    }
}
