#include "Compression/CompressedBuffer.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/CustomVersion.h"

// Editor-only recovery of independently validated source payloads. Never load or
// patch a newer package through the old UObject linker.
namespace RoomBridge {
bool Preview(){
 static bool Requested=false,PrefixSet=false;static int Stage=0;static double At=0;static TWeakObjectPtr<ACameraActor> Survey;
 auto Now=FPlatformTime::Seconds();
 if(Stage==99){if(!GEditor->PlayWorld&&Now>At+2){FPlatformMisc::RequestExit(false);return false;}return true;}
 if(!Requested){GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground=false;FRequestPlaySessionParams P;P.GlobalMapOverride=TEXT("/Game/RoomIntegration/RoomIntegrationMap");GEditor->RequestPlaySession(P);Requested=true;return true;}
 auto W=GEditor->PlayWorld;if(!W)return true;
 if(!PrefixSet&&W->GetGameInstance()){MenuVerify::String(W->GetGameInstance(),TEXT("StoragePrefix"),FString::Printf(TEXT("DA_ROOM_QA_%lld"),FDateTime::Now().GetTicks()));PrefixSet=true;}
 AActor *Rig=nullptr,*Radio=nullptr,*Deck=nullptr;
 for(TActorIterator<AActor> I(W);I;++I){if(I->GetClass()->GetName()==TEXT("BP_DeskWorkstationRig_C"))Rig=*I;if(I->GetClass()->GetName()==TEXT("BP_RadioIntegrated_C"))Radio=*I;if(I->GetClass()->GetName()==TEXT("BP_TapeDeckIntegrated_C"))Deck=*I;}
 if(!Rig||!Radio||!Deck)return true;
 auto Shot=[&](const TCHAR* Name){auto Dir=FPaths::ProjectSavedDir()/TEXT("RoomImport/Checks");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/Name,false,false);};
 if(Stage==0&&W->GetTimeSeconds()>3&&(!GShaderCompilingManager||!GShaderCompilingManager->IsCompiling())){
  // Capture the actual default overview, not the hidden desktop cursor's
  // incidental pan position. This override exists only in this PIE review.
  auto PC=UGameplayStatics::GetPlayerController(W,0);PC->bEnableClickEvents=false;
  auto View=CastChecked<USceneComponent>(MenuVerify::Obj(Rig,TEXT("ViewCenter")));
  auto BP=CastChecked<UBlueprint>(Rig->GetClass()->ClassGeneratedBy);
  View->SetRelativeTransform(CastChecked<USceneComponent>(StationBuild::SCS(BP,TEXT("ViewCenter"))->ComponentTemplate)->GetRelativeTransform());
  int Models=0;for(TActorIterator<AStaticMeshActor> I(W);I;++I)if(I->GetActorLabel().StartsWith(TEXT("ROOM")))Models++;
  UE_LOG(LogTemp,Display,TEXT("ROOM PREVIEW actors=%d rig=%s radio=%s deck=%s"),Models,*Rig->GetActorLocation().ToString(),*Radio->GetActorLocation().ToString(),*Deck->GetActorLocation().ToString());
  Shot(TEXT("room-overview.png"));At=Now;Stage=1;return true;
 }
 if(Stage==1&&Now>At+.4){UGameplayStatics::GetPlayerController(W,0)->bEnableClickEvents=true;MenuVerify::Run(Rig,TEXT("FocusRadio"));At=Now;Stage=2;return true;}
 if(Stage==2&&Now>At+.8){auto C=CastChecked<UPrimitiveComponent>(MenuVerify::Obj(Radio,TEXT("Click_Power")));if(!MenuVerify::Flag(Radio,TEXT("Powered")))C->OnClicked.Broadcast(C,EKeys::LeftMouseButton);At=Now;Stage=21;return true;}
 if(Stage==21&&Now>At+.4){Shot(TEXT("room-radio.png"));At=Now;Stage=3;return true;}
 if(Stage==3&&Now>At+.4){MenuVerify::Run(Rig,TEXT("FocusDeck"));At=Now;Stage=4;return true;}
 if(Stage==4&&Now>At+.8){Shot(TEXT("room-deck.png"));At=Now;Stage=5;return true;}
 if(Stage==5&&Now>At+.4){auto Camera=W->SpawnActor<ACameraActor>();Survey=Camera;Camera->SetActorLocation(FVector(70,85,170));Camera->SetActorRotation((FVector(350,330,110)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(85);UGameplayStatics::GetPlayerController(W,0)->SetViewTargetWithBlend(Camera,0);At=Now;Stage=6;return true;}
 if(Stage==6&&Now>At+.6){Shot(TEXT("room-survey.png"));At=Now;Stage=7;return true;}
 if(Stage==7&&Now>At+.4){auto Camera=Survey.Get();Camera->SetActorLocation(FVector(495,370,195));Camera->SetActorRotation((FVector(190,165,105)-Camera->GetActorLocation()).Rotation());At=Now;Stage=8;return true;}
 if(Stage==8&&Now>At+.6){Shot(TEXT("room-reverse.png"));At=Now;Stage=9;return true;}
 if(Stage==9&&Now>At+.4){auto Camera=Survey.Get();Camera->SetActorLocation(FVector(140,290,100));Camera->SetActorRotation((FVector(54,382,45)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(55);
  // Inspection light is transient PIE-only, never saved in the horror room.
  auto Inspection=W->SpawnActor<APointLight>();Inspection->SetActorLocation(FVector(95,320,120));Inspection->GetLightComponent()->SetIntensity(4);Inspection->GetLightComponent()->SetLightColor(FLinearColor::White);
  At=Now;Stage=10;return true;}
 if(Stage==10&&Now>At+.6){Shot(TEXT("room-box-material-check.png"));At=Now;Stage=11;return true;}
 if(Stage==11&&Now>At+.4){UE_LOG(LogTemp,Display,TEXT("ROOM PREVIEW COMPLETE"));GEditor->RequestEndPlayMap();At=Now;Stage=99;return true;}
 return true;
}
void RecoverTextures(){
 auto Root=FPaths::ProjectSavedDir()/TEXT("RoomImport");auto Out=Root/TEXT("RecoveredTextures");IFileManager::Get().MakeDirectory(*Out,true);TArray<FString> Lines;
 check(FFileHelper::LoadFileToStringArray(Lines,*(Root/TEXT("texture-recovery.tsv"))));int Count=0;
 for(auto Line:Lines){TArray<FString> P;Line.ParseIntoArray(P,TEXT("\t"),false);check(P.Num()==3&&!P[2].Contains(TEXT("/"))&&!P[2].Contains(TEXT("\\")));TArray<uint8> Bytes;check(FFileHelper::LoadFileToArray(Bytes,*P[0]));FMemoryReader Reader(Bytes,true);Reader.Seek(FCString::Atoi64(*P[1]));auto Compressed=FCompressedBuffer::Load(Reader);check(Compressed);auto Raw=Compressed.Decompress();if(!Raw||Raw.GetSize()>=256*1024*1024){UE_LOG(LogTemp,Error,TEXT("ROOM TEXTURE DECODE FAILED %s rawsize=%llu compressed=%llu"),*P[2],Compressed.GetRawSize(),Compressed.GetCompressedSize());continue;}check(FFileHelper::SaveArrayToFile(TArrayView<const uint8>((const uint8*)Raw.GetData(),Raw.GetSize()),*(Out/P[2])));Count++;}
 UE_LOG(LogTemp,Display,TEXT("ROOM TEXTURE RECOVERY COMPLETE: %d source payloads"),Count);
}
void RecoverMeshes(){
 auto Root=FPaths::ProjectSavedDir()/TEXT("RoomImport");TArray<FString> Lines;
 check(FFileHelper::LoadFileToStringArray(Lines,*(Root/TEXT("mesh-recovery.tsv"))));
 FString Report=TEXT("key\tvertices\ttriangles\tuv\tbounds\tgroups\n");int Count=0;
 for(auto Line:Lines){
  TArray<FString> P;Line.ParseIntoArray(P,TEXT("\t"),false);check(P.Num()==5);
  TArray<uint8> Bytes;check(FFileHelper::LoadFileToArray(Bytes,*P[0]));
  FMemoryReader Package(Bytes,true);Package.Seek(FCString::Atoi64(*P[1]));
  auto Compressed=FCompressedBuffer::Load(Package);check(Compressed);auto Raw=Compressed.Decompress();check(Raw&&Raw.GetSize()<256*1024*1024);
  TArray<uint8> Data;Data.Append((const uint8*)Raw.GetData(),Raw.GetSize());FMemoryReader Reader(Data,true);Reader.SetCustomVersions(FCurrentCustomVersions::GetAll());FMeshDescription MD;Reader<<MD;
  check(!Reader.IsError()&&Reader.Tell()==Data.Num());check(MD.Triangles().Num()>0);if(MD.Triangles().Num()!=FCString::Atoi(*P[2]))UE_LOG(LogTemp,Warning,TEXT("ROOM SOURCE DETAIL %s raw=%d rendered_metadata=%s; preserving original source mesh"),*P[4],MD.Triangles().Num(),*P[2]);
  FStaticMeshAttributes A(MD);auto Pos=A.GetVertexPositions();auto UV=A.GetVertexInstanceUVs();FBox Box(EForceInit::ForceInit);
  const int UVCount=UV.GetNumChannels();const int WantedUV=FCString::Atoi(*P[3]);check(UVCount>0&&UVCount<=WantedUV);
  for(auto V:MD.Vertices().GetElementIDs()){check(!Pos[V].ContainsNaN());Box+=FVector(Pos[V]);}
  for(auto T:MD.Triangles().GetElementIDs())for(auto VI:MD.GetTriangleVertexInstances(T)){check(MD.IsVertexInstanceValid(VI));check(MD.IsVertexValid(MD.GetVertexInstanceVertex(VI)));for(int C=0;C<UV.GetNumChannels();C++)check(!UV.Get(VI,C).ContainsNaN());}
  FString PackageName=TEXT("/Game/RoomArt/Meshes/")+P[4];auto Mesh=LoadObject<UStaticMesh>(nullptr,*(PackageName+TEXT(".")+P[4]));
  if(!Mesh){Mesh=NewObject<UStaticMesh>(CreatePackage(*PackageName),*P[4],RF_Public|RF_Standalone);Mesh->AddSourceModel();FAssetRegistryModule::AssetCreated(Mesh);}
  Mesh->SetLightingGuid();Mesh->GetStaticMaterials().Empty();FString Groups;
  auto Names=A.GetPolygonGroupMaterialSlotNames();auto Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
  for(auto G:MD.PolygonGroups().GetElementIDs()){auto Name=Names[G];if(Name.IsNone())Name=FName(*FString::Printf(TEXT("Material_%d"),G.GetValue()));Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,Name,Name));Groups+=Name.ToString()+TEXT(";");}
  auto& S=Mesh->GetSourceModel(0).BuildSettings;S.bRecomputeNormals=false;S.bRecomputeTangents=false;S.bRemoveDegenerates=false;S.bGenerateLightmapUVs=UVCount<WantedUV;S.SrcLightmapIndex=0;S.DstLightmapIndex=UVCount;S.bUseFullPrecisionUVs=true;
  Mesh->CreateMeshDescription(0,MoveTemp(MD));Mesh->CommitMeshDescription(0);Mesh->SetLightMapCoordinateIndex(UVCount<WantedUV?UVCount:(UVCount>1?1:0));Mesh->Build(false);Mesh->CreateBodySetup();Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;Mesh->GetBodySetup()->CreatePhysicsMeshes();Mesh->PostEditChange();RadioBuild::Save(Mesh);
  Report+=FString::Printf(TEXT("%s\t%d\t%d\t%d\t%s\t%s\n"),*P[4],Mesh->GetMeshDescription(0)->Vertices().Num(),Mesh->GetMeshDescription(0)->Triangles().Num(),UVCount,*Box.ToString(),*Groups);Count++;
  UE_LOG(LogTemp,Display,TEXT("ROOM RECOVERED %s triangles=%s bounds=%s groups=%s"),*P[4],*P[2],*Box.ToString(),*Groups);
 }
 FFileHelper::SaveStringToFile(Report,*(Root/TEXT("recovered-meshes.tsv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 UE_LOG(LogTemp,Display,TEXT("ROOM MESH RECOVERY COMPLETE: %d meshes"),Count);
}
void Probe() {
 FString File;int64 Offset=0;
 FParse::Value(FCommandLine::Get(),TEXT("RoomPayloadFile="),File);
 FParse::Value(FCommandLine::Get(),TEXT("RoomPayloadOffset="),Offset);
 TArray<uint8> Bytes;
 if(!FFileHelper::LoadFileToArray(Bytes,*File)||Offset<0||Offset+64>Bytes.Num()){
  UE_LOG(LogTemp,Error,TEXT("ROOM BRIDGE invalid payload input"));return;
 }
 FMemoryReader Package(Bytes,true);Package.Seek(Offset);
 auto Compressed=FCompressedBuffer::Load(Package);
 if(!Compressed){UE_LOG(LogTemp,Error,TEXT("ROOM BRIDGE compressed buffer rejected"));return;}
 auto Raw=Compressed.Decompress();
 if(!Raw||Raw.GetSize()>256*1024*1024){UE_LOG(LogTemp,Error,TEXT("ROOM BRIDGE decompression rejected"));return;}
 auto Dir=FPaths::ProjectSavedDir()/TEXT("RoomImport/Recovered");IFileManager::Get().MakeDirectory(*Dir,true);
 auto Out=Dir/TEXT("probe.raw");FFileHelper::SaveArrayToFile(TArrayView<const uint8>((const uint8*)Raw.GetData(),Raw.GetSize()),*Out);
 UE_LOG(LogTemp,Display,TEXT("ROOM BRIDGE decompressed %llu bytes"),Raw.GetSize());
 if(FParse::Param(FCommandLine::Get(),TEXT("RoomMeshProbe"))){
  TArray<uint8> Data;Data.Append((const uint8*)Raw.GetData(),Raw.GetSize());
  FMemoryReader Reader(Data,true);Reader.SetCustomVersions(FCurrentCustomVersions::GetAll());
  FMeshDescription MD;Reader<<MD;
  UE_LOG(LogTemp,Display,TEXT("ROOM BRIDGE mesh vertices=%d triangles=%d consumed=%lld/%d error=%d"),MD.Vertices().Num(),MD.Triangles().Num(),Reader.Tell(),Data.Num(),Reader.IsError());
 }
}
}
