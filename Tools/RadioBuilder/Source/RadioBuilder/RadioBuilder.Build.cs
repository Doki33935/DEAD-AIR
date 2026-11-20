using UnrealBuildTool;
public class RadioBuilder : ModuleRules {
 public RadioBuilder(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage = PCHUsageMode.NoPCHs;
 // This editor-only graph authoring module is never shipped. Optimizing its
 // large generated-node functions adds minutes to every authoring/check build.
 OptimizeCode = CodeOptimization.Never;
 PublicDefinitions.Add("__has_feature(x)=0");
 PublicDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","InputCore","UMG","Slate","SlateCore","UnrealEd","Kismet","KismetCompiler","BlueprintGraph","UMGEditor","AssetRegistry","AssetTools","MediaAssets","MeshDescription","StaticMeshDescription"});
 }
}
