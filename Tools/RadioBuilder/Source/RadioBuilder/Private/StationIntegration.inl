// Editor-only inspection/authoring. Runtime assets use stock Blueprint classes.
namespace StationIntegration {
using namespace RadioBuild;
FString Report;
void Line(const FString& S){Report+=S+TEXT("\n");}
void MeshInfo(UStaticMesh* M){
 if(!M)return;auto B=M->GetBoundingBox();Line(FString::Printf(TEXT("MESH %s min=%s max=%s materials=%d"),*M->GetPathName(),*B.Min.ToString(),*B.Max.ToString(),M->GetStaticMaterials().Num()));
 if(auto D=M->GetMeshDescription(0))Line(FString::Printf(TEXT(" vertices=%d triangles=%d"),D->Vertices().Num(),D->Triangles().Num()));
 for(auto Mat:M->GetStaticMaterials())Line(FString::Printf(TEXT(" material slot=%s interface=%s"),*Mat.MaterialSlotName.ToString(),*GetPathNameSafe(Mat.MaterialInterface)));
}
void BlueprintInfo(const TCHAR* Path){
 auto B=LoadObject<UBlueprint>(nullptr,Path);if(!B){Line(FString(TEXT("MISSING "))+Path);return;}
 FCompilerResultsLog Results;FKismetEditorUtilities::CompileBlueprint(B,EBlueprintCompileOptions::SkipGarbageCollection,&Results);Line(FString::Printf(TEXT("BLUEPRINT %s parent=%s status=%d errors=%d warnings=%d"),Path,*GetPathNameSafe(B->ParentClass),int(B->Status),Results.NumErrors,Results.NumWarnings));
 for(auto V:B->NewVariables)Line(FString::Printf(TEXT(" VAR %s %s.%s object=%s default=%s"),*V.VarName.ToString(),*V.VarType.PinCategory.ToString(),*V.VarType.PinSubCategory.ToString(),*GetPathNameSafe(V.VarType.PinSubCategoryObject.Get()),*V.DefaultValue));
 if(B->SimpleConstructionScript)for(auto N:B->SimpleConstructionScript->GetAllNodes()){auto C=N->ComponentTemplate;Line(FString::Printf(TEXT(" COMPONENT %s class=%s parent=%s"),*N->GetVariableName().ToString(),*C->GetClass()->GetName(),*N->ParentComponentOrVariableName.ToString()));if(auto Scene=Cast<USceneComponent>(C))Line(TEXT("  transform ")+Scene->GetRelativeTransform().ToHumanReadableString());if(auto SM=Cast<UStaticMeshComponent>(C)){MeshInfo(SM->GetStaticMesh());Line(FString::Printf(TEXT("  visibility collision=%d response=%d"),int(SM->GetCollisionEnabled()),int(SM->GetCollisionResponseToChannel(ECC_Visibility))));}}
 TArray<UEdGraph*> Graphs;B->GetAllGraphs(Graphs);for(auto G:Graphs){Line(FString(TEXT(" GRAPH "))+G->GetName());for(auto N:G->Nodes){Line(FString::Printf(TEXT("  NODE %s %s title=%s"),*N->NodeGuid.ToString(),*N->GetClass()->GetName(),*N->GetNodeTitle(ENodeTitleType::FullTitle).ToString().Replace(TEXT("\n"),TEXT(" | "))));for(auto P:N->Pins){FString Links;for(auto L:P->LinkedTo)Links+=L->GetOwningNode()->NodeGuid.ToString()+TEXT(".")+L->PinName.ToString()+TEXT(" ");Line(FString::Printf(TEXT("   %s %s <%s> default=%s object=%s links=%s"),P->Direction==EGPD_Input?TEXT("IN"):TEXT("OUT"),*P->PinName.ToString(),*P->PinType.PinCategory.ToString(),*P->DefaultValue,*GetPathNameSafe(P->DefaultObject),*Links));}}}
}
void Inspect(){
 Report.Empty();for(auto P:{TEXT("/Game/My_content/BPI_interface.BPI_interface"),TEXT("/Game/My_content/BP_deca.BP_deca"),TEXT("/Game/My_content/Cassete.Cassete"),TEXT("/Game/My_content/test_char.test_char"),TEXT("/Game/My_content/Radio_1.Radio_1")})BlueprintInfo(P);
 TArray<FAssetData> Imported;FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByPath(TEXT("/Game/StationIntegration/Art"),Imported,true);for(auto A:Imported)if(auto M=Cast<UStaticMesh>(A.GetAsset()))MeshInfo(M);auto Dir=FPaths::ProjectSavedDir()/TEXT("StationInspection");IFileManager::Get().MakeDirectory(*Dir,true);check(FFileHelper::SaveStringToFile(Report,*(Dir/TEXT("slava-blueprints.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));UE_LOG(LogTemp,Display,TEXT("STATION INSPECTION COMPLETE"));
}
void ImportModels(){
 Report.Empty();
 for(auto Name:{TEXT("Desk"),TEXT("TapeDeck"),TEXT("Radio")}){
  auto UI=NewObject<UFbxImportUI>();UI->bAutomatedImportShouldDetectType=false;UI->MeshTypeToImport=FBXIT_StaticMesh;UI->bImportAsSkeletal=false;UI->bImportMaterials=true;UI->bImportTextures=false;UI->bImportAnimations=false;UI->StaticMeshImportData->bCombineMeshes=false;UI->StaticMeshImportData->bTransformVertexToAbsolute=true;UI->StaticMeshImportData->bBakePivotInVertex=false;UI->StaticMeshImportData->bAutoGenerateCollision=false;UI->StaticMeshImportData->bBuildNanite=false;UI->StaticMeshImportData->bGenerateLightmapUVs=true;UI->StaticMeshImportData->bConvertScene=true;UI->StaticMeshImportData->bConvertSceneUnit=true;
  auto T=NewObject<UAssetImportTask>();T->Filename=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../Tools/StationIntegration/SourceArt/")+Name+TEXT(".fbx"));T->DestinationPath=FString(TEXT("/Game/StationIntegration/Art/"))+Name;T->DestinationName=FString(TEXT("SM_"))+Name;T->bAutomated=true;T->bSave=true;T->bReplaceExisting=false;T->Options=UI;
  FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({T});for(auto O:T->Result)if(auto M=Cast<UStaticMesh>(O))MeshInfo(M);
 }
 auto Dir=FPaths::ProjectSavedDir()/TEXT("StationInspection");IFileManager::Get().MakeDirectory(*Dir,true);FFileHelper::SaveStringToFile(Report,*(Dir/TEXT("imported-meshes.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);UE_LOG(LogTemp,Display,TEXT("STATION MODEL IMPORT COMPLETE"));
}
}
