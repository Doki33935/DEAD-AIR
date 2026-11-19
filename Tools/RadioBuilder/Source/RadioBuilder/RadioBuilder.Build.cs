using UnrealBuildTool;
public class RadioBuilder : ModuleRules {
 public RadioBuilder(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage = PCHUsageMode.NoPCHs;
 PublicDefinitions.Add("__has_feature(x)=0");
 PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","UMG","Slate","SlateCore","UnrealEd","Kismet","KismetCompiler","BlueprintGraph","UMGEditor","AssetRegistry","AssetTools","MediaAssets"});
 }
}
