using UnrealBuildTool;

public class XRacing : ModuleRules
{
    public XRacing(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "PhysicsCore",
            "Chaos"
        });

        PrivateDependencyModuleNames.AddRange(new string[] 
        { 
            "Slate",
            "SlateCore"
        });

        // Link with the existing C++ simulation library
        PublicAdditionalLibraries.Add("D:/x-racing/build/engine/Release/project0_engine.lib");
    }
}
