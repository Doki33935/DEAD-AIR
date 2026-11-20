// Editor-only room corrections; runtime remains ordinary Blueprint/materials.
namespace RoomMood {
using namespace ReceiverUpgrade;
void Build() {
 auto BoxMat=LoadObject<UMaterial>(nullptr,TEXT("/Game/RoomArt/Materials/M_MI_uewlfg2va_83d854"));check(BoxMat);
 UE_LOG(LogTemp,Display,TEXT("ROOM MOOD original box material expressions=%d"),BoxMat->Expressions.Num());
 for(auto E:BoxMat->Expressions)if(auto T=Cast<UMaterialExpressionTextureSample>(E))UE_LOG(LogTemp,Display,TEXT("ROOM MOOD original texture %s"),*GetPathNameSafe(T->Texture));
 // Cardboard is a dielectric. Preserve its original scanned colour and normal,
 // but do not interpret the pack's blue mask as metallic reflectance.
 BoxMat->Expressions.Empty();
 auto Texture=[&](const TCHAR* Suffix,EMaterialSamplerType Type){auto T=NewObject<UMaterialExpressionTextureSample>(BoxMat);auto P=FString::Printf(TEXT("/Game/Fab/Megascans/3D/Cardboard_Boxes_Pack_uewlfg2va/Medium/T_uewlfg2va_2K_%s"),Suffix);T->Texture=LoadObject<UTexture2D>(nullptr,*P);check(T->Texture);T->SamplerType=Type;BoxMat->Expressions.Add(T);return T;};
 BoxMat->BaseColor.Expression=Texture(TEXT("B"),SAMPLERTYPE_Color);
 BoxMat->Normal.Expression=Texture(TEXT("N"),SAMPLERTYPE_Normal);
 auto ORM=Texture(TEXT("ORM"),SAMPLERTYPE_Masks);BoxMat->Roughness.Expression=ORM;BoxMat->Roughness.Mask=BoxMat->Roughness.MaskG=1;
 BoxMat->AmbientOcclusion.Expression=ORM;BoxMat->AmbientOcclusion.Mask=BoxMat->AmbientOcclusion.MaskR=1;
 auto Metal=NewObject<UMaterialExpressionConstant>(BoxMat);Metal->R=0;BoxMat->Expressions.Add(Metal);BoxMat->Metallic.Expression=Metal;BoxMat->Specular.Expression=nullptr;BoxMat->Specular.Constant=.15;
 BoxMat->PostEditChange();Save(BoxMat);
 for(auto Name:{TEXT("SM_uewlfg2va_LOD0_TIER2_000_87f3be"),TEXT("SM_uewlfg2va_LOD0_TIER2_001_4d53e7"),TEXT("SM_uewlfg2va_LOD0_TIER2_002_3f5f12"),TEXT("SM_uewlfg2va_LOD0_TIER2_003_b28065")}){
  auto M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/RoomArt/Meshes/%s"),Name));check(M);auto MD=M->GetMeshDescription(0);check(MD);FStaticMeshAttributes A(*MD);auto UV=A.GetVertexInstanceUVs();FVector2f Lo(MAX_flt),Hi(-MAX_flt);
  for(auto VI:MD->VertexInstances().GetElementIDs()){auto V=UV.Get(VI,0);Lo.X=FMath::Min(Lo.X,V.X);Lo.Y=FMath::Min(Lo.Y,V.Y);Hi.X=FMath::Max(Hi.X,V.X);Hi.Y=FMath::Max(Hi.Y,V.Y);}
  UE_LOG(LogTemp,Display,TEXT("ROOM MOOD box UV %s min=%s max=%s"),Name,*Lo.ToString(),*Hi.ToString());check((Hi-Lo).Size()>.1);
 }
 auto Rig=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_DeskWorkstationRig.BP_DeskWorkstationRig"));check(Rig);
 auto Centre=CastChecked<UChildActorComponent>(StationBuild::SCS(Rig,TEXT("ViewCenter"))->ComponentTemplate);auto Old=Centre->GetRelativeRotation();
 Centre->SetRelativeLocation(FVector(94,117,104));Centre->SetRelativeRotation(FRotator(-10.5,-108,0));CastChecked<ACameraActor>(Centre->GetChildActorTemplate())->GetCameraComponent()->SetFieldOfView(85);
 auto G=Graph(Rig,TEXT("UpdateDeskLook"));int Height=0,Pitch=0,Yaw=0;
 for(auto N:G->Nodes)for(auto P:N->Pins)if(P->Direction==EGPD_Input&&!P->LinkedTo.Num()&&P->PinType.PinCategory==UEdGraphSchema_K2::PC_Real){
  double V=FCString::Atod(*P->DefaultValue);
  if(FMath::IsNearlyEqual(V,128.,.0001)||FMath::IsNearlyEqual(V,104.,.0001)){RadioBuild::Schema->TrySetDefaultValue(*P,TEXT("104"));Height++;}
  else if(FMath::IsNearlyEqual(V,Old.Pitch,.0001)){RadioBuild::Schema->TrySetDefaultValue(*P,TEXT("-10.5"));Pitch++;}
  else if(FMath::IsNearlyEqual(V,Old.Yaw,.0001)){RadioBuild::Schema->TrySetDefaultValue(*P,TEXT("-108"));Yaw++;}
 }
 check(Height==1&&Pitch==1&&Yaw==1);
 // Focus the now correctly sized deck and its adjacent tape together.
 FTransform Placement(FRotator(0,180,0),FVector(555.15,451.32,-6.5),FVector(1.28405738));FVector Eye(440,364,137),Target(440,423,97);
 auto Focus=CastChecked<UChildActorComponent>(StationBuild::SCS(Rig,TEXT("ViewDeck"))->ComponentTemplate);Focus->SetRelativeLocation(Placement.InverseTransformPosition(Eye));auto R=(Target-Eye).Rotation();R.Yaw-=180;Focus->SetRelativeRotation(R);CastChecked<ACameraActor>(Focus->GetChildActorTemplate())->GetCameraComponent()->SetFieldOfView(54);
 Compile(Rig);Save(Rig);
 auto Deck=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_TapeDeckIntegrated"));check(Deck);
 auto Text=CastChecked<UTextRenderComponent>(StationBuild::SCS(Deck,TEXT("Label_1"))->ComponentTemplate);
 auto Labels=LoadObject<UMaterial>(nullptr,TEXT("/Game/StationIntegration/Art/Materials/M_DeckLabelLowLight"));
 if(!Labels){
  auto Source=Text->GetMaterial(0)->GetMaterial();
  Labels=CastChecked<UMaterial>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().DuplicateAsset(TEXT("M_DeckLabelLowLight"),TEXT("/Game/StationIntegration/Art/Materials"),Source));
  auto Input=Labels->BaseColor.Expression?Labels->BaseColor:Labels->EmissiveColor;
  auto Dim=NewObject<UMaterialExpressionMultiply>(Labels);Dim->A=Input;Dim->ConstB=.025;Labels->Expressions.Add(Dim);Labels->EmissiveColor.Expression=Dim;Labels->EmissiveColor.OutputIndex=0;Labels->SetShadingModel(MSM_Unlit);Labels->PostEditChange();Save(Labels);
 }
 for(int I=1;I<=5;I++)CastChecked<UTextRenderComponent>(StationBuild::SCS(Deck,*FString::Printf(TEXT("Label_%d"),I))->ComponentTemplate)->SetTextMaterial(Labels);
 Compile(Deck);Save(Deck);UE_LOG(LogTemp,Display,TEXT("ROOM MOOD BLUEPRINT COMPLETE overview height=127.04 pitch=-10.5"));
}
}
