// Incremental editor authoring. All shipped interactions remain Blueprint code.
namespace RoomPolish {
using namespace ReceiverUpgrade;
void Build() {
 auto Tape=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_CassetteIntegrated.BP_CassetteIntegrated"));
 auto Rig=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_DeskWorkstationRig.BP_DeskWorkstationRig"));check(Tape&&Rig);
 auto Source=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/RoomArt/Meshes/SM_low_poly_vhs_tape_free_3d_model_248e7d.SM_low_poly_vhs_tape_free_3d_model_248e7d"));check(Source);
 const TCHAR* MaterialPath=TEXT("/Game/StationIntegration/Art/Materials/M_CassetteAssetAnimated");
 auto Mat=LoadObject<UMaterial>(nullptr,MaterialPath);
 if(!Mat){Mat=NewObject<UMaterial>(CreatePackage(MaterialPath),TEXT("M_CassetteAssetAnimated"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(Mat);}
 Mat->Expressions.Empty();
 auto Tex=NewObject<UMaterialExpressionTextureObject>(Mat);Tex->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Fab/Low_Poly_VHS_Tape_Free_3D_model/low_poly_vhs_tape_free_3d_model_texture_0"));check(Tex->Texture);Mat->Expressions.Add(Tex);
 auto UV=NewObject<UMaterialExpressionTextureCoordinate>(Mat);Mat->Expressions.Add(UV);
 auto C=NewObject<UMaterialExpressionCustom>(Mat);Mat->Expressions.Add(C);C->OutputType=CMOT_Float3;
 auto Input=[&](const TCHAR* Name,UMaterialExpression* E){FCustomInput I;I.InputName=Name;I.Input.Expression=E;C->Inputs.Add(I);};
 C->Inputs.Empty();Input(TEXT("Tex"),Tex);Input(TEXT("UV"),UV);
 for(auto Name:{TEXT("ReelAngleA"),TEXT("ReelAngleB")}){auto P=NewObject<UMaterialExpressionScalarParameter>(Mat);P->ParameterName=Name;Mat->Expressions.Add(P);Input(Name,P);}
 // This scanned asset has flat, photographed reels. Animate only the bright
 // hubs behind the two windows: the case, label and glass reflections stay still.
 C->Code=TEXT("float3 col=Texture2DSample(Tex,TexSampler,UV).rgb;\n")
 TEXT("float2 centers[2]={float2(.302,.496),float2(.698,.496)};\n")
 TEXT("float angles[2]={ReelAngleA,ReelAngleB};\n")
 TEXT("for(int i=0;i<2;i++){float2 p=UV-centers[i];float r=length(p);\n")
 TEXT("float ring=smoothstep(.025,.038,r)*(1-smoothstep(.083,.096,r));\n")
 TEXT("float window=i==0?1-smoothstep(.293,.300,UV.x):smoothstep(.700,.707,UV.x);\n")
 TEXT("float silver=smoothstep(.20,.55,dot(col,float3(.2126,.7152,.0722)));\n")
 TEXT("float spoke=pow(saturate(.5+.5*cos(7*(atan2(p.y,p.x)-angles[i]*.01745329252))),12);\n")
 TEXT("col*=1-.78*ring*window*silver*spoke;}return col;");Mat->BaseColor.Expression=C;
 auto Normal=NewObject<UMaterialExpressionTextureSample>(Mat);Normal->Texture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Fab/Low_Poly_VHS_Tape_Free_3D_model/low_poly_vhs_tape_free_3d_model_texture_1"));Normal->SamplerType=SAMPLERTYPE_Normal;Mat->Expressions.Add(Normal);Mat->Normal.Expression=Normal;
 auto Rough=NewObject<UMaterialExpressionConstant>(Mat);Rough->R=.82;Mat->Expressions.Add(Rough);Mat->Roughness.Expression=Rough;
 Mat->Metallic.Expression=nullptr;Mat->Metallic.Constant=0;Mat->PostEditChange();Save(Mat);
 auto Shell=CastChecked<UStaticMeshComponent>(StationBuild::SCS(Tape,TEXT("CassetteShell"))->ComponentTemplate);
 Shell->SetStaticMesh(Source);Shell->OverrideMaterials.Empty();Shell->SetMaterial(0,Mat);
 Shell->SetRelativeTransform(FTransform(FRotator(0,90,0),FVector(0,0,-.6554),FVector(.3953325,.5252245,.428523)));
 Shell->SetVisibility(true);Shell->SetHiddenInGame(false);Shell->SetMobility(EComponentMobility::Movable);Shell->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Shell->SetCollisionResponseToAllChannels(ECR_Ignore);Shell->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 // Keep the named phase drivers and existing Blueprint references, remove every
 // mesh of the old placeholder cassette from the live component hierarchy.
 for(auto Name:{TEXT("cassete_base"),TEXT("ReelA"),TEXT("ReelB"),TEXT("ReelIndexA"),TEXT("ReelIndexB")})if(auto N=StationBuild::SCS(Tape,Name))if(auto P=Cast<UStaticMeshComponent>(N->ComponentTemplate)){P->SetStaticMesh(nullptr);P->SetVisibility(false);P->SetHiddenInGame(true);P->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 Declare(Tape,{TEXT("UpdateAssetReels")});UEdGraphPin* X;auto G=Reset(Tape,TEXT("UpdateAssetReels"),X);
 for(auto Pair:{TPair<const TCHAR*,const TCHAR*>(TEXT("ReelA"),TEXT("ReelAngleA")),{TEXT("ReelB"),TEXT("ReelAngleB")}}){
  auto R=Call(G,UKismetMathLibrary::StaticClass(),TEXT("BreakRotator"));Link(Ext(G,USceneComponent::StaticClass(),TEXT("RelativeRotation"),V(G,Pair.Key)),Pin(R,TEXT("InRot")));
  auto Set=Flow(G,X,UMeshComponent::StaticClass(),TEXT("SetScalarParameterValueOnMaterials"),V(G,TEXT("CassetteShell")));Def(Set,TEXT("ParameterName"),Pair.Value);Link(Pin(R,TEXT("Yaw")),Pin(Set,TEXT("ParameterValue")));
 }
 auto Animate=Graph(Tape,TEXT("AnimateReels"));bool Added=false;UK2Node_CallFunction* LastRotation=nullptr;
 for(auto N:Animate->Nodes)if(auto Fn=Cast<UK2Node_CallFunction>(N)){
  if(Fn->GetFunctionName()==TEXT("UpdateAssetReels"))Added=true;
  if(Fn->GetFunctionName()==TEXT("K2_AddLocalRotation"))for(auto P:Pin(Fn,TEXT("self"))->LinkedTo)if(auto Get=Cast<UK2Node_VariableGet>(P->GetOwningNode()))if(Get->GetVarName()==TEXT("ReelB"))LastRotation=Fn;
 }
 if(!Added)Append(Tape,Animate,TEXT("UpdateAssetReels"));
 // Append() inserts at entry. Sync once more after both rotations so the
 // visible phase never trails the driver by a frame, including on STOP.
 check(LastRotation);auto Tail=Pin(LastRotation,TEXT("then"));bool TailSync=false;for(auto P:Tail->LinkedTo)if(auto Fn=Cast<UK2Node_CallFunction>(P->GetOwningNode()))if(Fn->GetFunctionName()==TEXT("UpdateAssetReels"))TailSync=true;
 if(!TailSync){auto Next=Tail->LinkedTo;Tail->BreakAllPinLinks();auto End=Tail;Invoke(Tape,Animate,End,TEXT("UpdateAssetReels"));for(auto P:Next)Link(End,P);}
 Compile(Tape);Save(Tape);
 // Preserve the existing smooth cursor-directed look; release its narrow desk
 // sector so the bed and the rest of the room are reachable without hotkeys.
 auto Look=Graph(Rig,TEXT("UpdateDeskLook"));int Yaw=0,Pitch=0;
 for(auto N:Look->Nodes)if(auto Fn=Cast<UK2Node_CallFunction>(N))if(Fn->GetFunctionName()==TEXT("Multiply_DoubleDouble"))for(auto P:Fn->Pins)if(P->Direction==EGPD_Input&&!P->LinkedTo.Num()){
  if(P->DefaultValue==TEXT("48")||P->DefaultValue==TEXT("175")){RadioBuild::Schema->TrySetDefaultValue(*P,TEXT("175"));Yaw++;}
  if(P->DefaultValue==TEXT("-12")||P->DefaultValue==TEXT("-42")){RadioBuild::Schema->TrySetDefaultValue(*P,TEXT("-42"));Pitch++;}
 }
 check(Yaw==1&&Pitch==1);
 auto View=CastChecked<UChildActorComponent>(StationBuild::SCS(Rig,TEXT("ViewRadio"))->ComponentTemplate);
 FTransform Placement(FRotator(0,180,0),FVector(555.15,451.32,-6.5),FVector(1.28405738));
 FVector Eye(446,310.0769515,119),Target(493.5,392.3493649,107);
 View->SetRelativeLocation(Placement.InverseTransformPosition(Eye));auto Rotation=(Target-Eye).Rotation();Rotation.Yaw-=180;View->SetRelativeRotation(Rotation);
 CastChecked<ACameraActor>(View->GetChildActorTemplate())->GetCameraComponent()->SetFieldOfView(54);
 Compile(Rig);Save(Rig);
 UE_LOG(LogTemp,Display,TEXT("ROOM POLISH BLUEPRINT COMPLETE asset cassette, material reels, yaw +/-175 pitch +/-42"));
}
bool Verify() {
 static bool Requested=false,PrefixSet=false;static int Stage=0,Checks=0,Failed=0,MouseX=0,MouseY=0;static double At=0,Started=FPlatformTime::Seconds();static float StoppedPhase=0;
 auto Now=FPlatformTime::Seconds();
 auto Expect=[&](bool Good,const TCHAR* Message){Checks++;if(!Good)Failed++;UE_LOG(LogTemp,Display,TEXT("ROOM POLISH VERIFY %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Message);};
 if(Stage==99){if(!GEditor->PlayWorld&&Now>At+2){FPlatformMisc::RequestExit(false);return false;}return true;}
 if(Now-Started>60){UE_LOG(LogTemp,Error,TEXT("ROOM POLISH VERIFY TIMEOUT"));GEditor->RequestEndPlayMap();At=Now;Stage=99;return true;}
 if(!Requested){GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground=false;FRequestPlaySessionParams P;P.GlobalMapOverride=TEXT("/Game/StationIntegration/StationIntegrationMap");GEditor->RequestPlaySession(P);Requested=true;return true;}
 auto W=GEditor->PlayWorld;if(!W)return true;
 if(!PrefixSet&&W->GetGameInstance()){MenuVerify::String(W->GetGameInstance(),TEXT("StoragePrefix"),FString::Printf(TEXT("DA_RP_QA_%lld"),FDateTime::Now().GetTicks()));PrefixSet=true;}
 if(W->GetTimeSeconds()<3)return true;
 AActor *Rig=nullptr,*RadioActor=nullptr,*Deck=nullptr,*Bed=nullptr;APointLight* Bulb=nullptr;int DecorativeTapes=0,SimpleMice=0;
 for(TActorIterator<AActor> I(W);I;++I){auto N=I->GetClass()->GetName();if(N==TEXT("BP_DeskWorkstationRig_C"))Rig=*I;if(N==TEXT("BP_RadioIntegrated_C"))RadioActor=*I;if(N==TEXT("BP_TapeDeckIntegrated_C"))Deck=*I;
  if(auto A=Cast<AStaticMeshActor>(*I))if(auto M=A->GetStaticMeshComponent()->GetStaticMesh()){if(M->GetName().Contains(TEXT("Military_Bed")))Bed=A;if(M->GetName()==TEXT("SM_low_poly_vhs_tape_free_3d_model_248e7d"))DecorativeTapes++;if(M->GetName()==TEXT("SM_computer_mouse"))SimpleMice++;}
  if(I->GetActorLabel()==TEXT("ROOM — central dim bulb"))Bulb=Cast<APointLight>(*I);
 }
 if(!Rig||!RadioActor||!Deck||!Bed)return true;
 auto PC=UGameplayStatics::GetPlayerController(W,0);auto View=CastChecked<UChildActorComponent>(MenuVerify::Obj(Rig,TEXT("ViewCenter")));auto Camera=CastChecked<ACameraActor>(View->GetChildActor());
 auto Tape=CastChecked<AActor>(MenuVerify::Obj(Deck,TEXT("CassetteReference")));auto Shell=CastChecked<UStaticMeshComponent>(MenuVerify::Obj(Tape,TEXT("CassetteShell")));
 if(Stage==0){
  Expect(DecorativeTapes==0,TEXT("No decorative duplicate cassettes remain"));Expect(SimpleMice==0,TEXT("The less detailed right-hand mouse is removed"));
  Expect(Shell->GetStaticMesh()&&Shell->GetStaticMesh()->GetName()==TEXT("SM_low_poly_vhs_tape_free_3d_model_248e7d"),TEXT("The playable cassette uses the free source model"));
  int Visible=0;TArray<UStaticMeshComponent*> Parts;Tape->GetComponents(Parts);for(auto C:Parts)if(C->GetStaticMesh()&&C->IsVisible()&&!C->bHiddenInGame)Visible++;
  Expect(Visible==1,TEXT("Only the source cassette body is rendered, without placeholder reels or cubes"));
  auto Body=CastChecked<UStaticMeshComponent>(MenuVerify::Obj(RadioActor,TEXT("Radio_Body")));Expect(Body->Bounds.Origin.Equals(FVector(506,414,107),.1),TEXT("RadioActor case centre matches the submitted room"));Expect(FMath::IsNearlyEqual(Body->Bounds.BoxExtent.Z*2,33.16787f,.1f),TEXT("RadioActor height matches the submitted model scale"));
  Expect(Bulb&&Bulb->GetActorLocation().X==275&&Bulb->GetActorLocation().Y==225,TEXT("The dim bulb sits in the centre of the room"));
  Expect(Bulb&&FVector::DotProduct(Bulb->GetActorLocation()-Camera->GetActorLocation(),Camera->GetActorForwardVector())<0,TEXT("The room bulb is behind the seated character"));
  auto Target=Bed->GetComponentsBoundingBox().GetCenter();auto Desired=(Target-Camera->GetActorLocation()).Rotation();Desired.Yaw-=Rig->GetActorRotation().Yaw;auto BP=CastChecked<UBlueprint>(Rig->GetClass()->ClassGeneratedBy);auto Base=CastChecked<USceneComponent>(StationBuild::SCS(BP,TEXT("ViewCenter"))->ComponentTemplate)->GetRelativeRotation();
  double NX=FMath::FindDeltaAngleDegrees(Base.Yaw,Desired.Yaw)/175.,NY=(Desired.Pitch-Base.Pitch)/-42.;Expect(FMath::Abs(NX)<1&&FMath::Abs(NY)<.76,TEXT("The bed lies within the released mouse look range"));
  int SX=0,SY=0;PC->GetViewportSize(SX,SY);MouseX=FMath::RoundToInt((NX+1)*SX*.5);MouseY=FMath::RoundToInt((NY+1)*SY*.5);UWidgetBlueprintLibrary::SetFocusToGameViewport();PC->SetMouseLocation(MouseX,MouseY);At=Now;Stage=1;return true;
 }
 if(Stage==1&&Now<=At+2.0){PC->SetMouseLocation(MouseX,MouseY);MenuVerify::Run(Rig,TEXT("UpdateDeskLook"));return true;}
 if(Stage==1&&Now>At+2.0){
  FVector2D Screen;int SX=0,SY=0;PC->GetViewportSize(SX,SY);bool Projected=PC->ProjectWorldLocationToScreen(Bed->GetComponentsBoundingBox().GetCenter(),Screen);
  Expect(Projected&&FMath::Abs(Screen.X-SX*.5)<SX*.15&&FMath::Abs(Screen.Y-SY*.5)<SY*.15,TEXT("Actual Blueprint mouse look turns the camera onto the bed"));
  auto Dir=FPaths::ProjectSavedDir()/TEXT("RoomPolishChecks");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/TEXT("bed-look.png"),true,false);
  MenuVerify::Run(Rig,TEXT("FocusDeck"));At=Now;Stage=2;return true;
 }
 if(Stage==2&&Now>At+.8){Expect(FindFProperty<FIntProperty>(Rig->GetClass(),TEXT("CameraMode"))->GetPropertyValue_InContainer(Rig)>0&&PC->bShowMouseCursor,TEXT("Instrument focus still shows the cursor after looking behind"));
  MenuVerify::Run(Deck,TEXT("InsertTape"));MenuVerify::Run(Deck,TEXT("DeckPlay"));At=Now;Stage=3;return true;
 }
 if(Stage==3&&Now>At+.5){
  auto MID=Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0));float Phase=0;bool Animated=MID&&MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ReelAngleA")),Phase);
  Expect(Animated&&FMath::Abs(Phase)>15,TEXT("Real playback drives the free cassette material reel animation"));
  float Other=0;Expect(MID&&MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ReelAngleB")),Other)&&FMath::Abs(Other-Phase)>1,TEXT("The second photographed hub has its own moving phase"));
  FHitResult Hit;auto Centre=Shell->Bounds.Origin;bool HasHit=W->LineTraceSingleByChannel(Hit,Centre+FVector(0,0,80),Centre,ECC_Visibility);
  Expect(HasHit&&Hit.GetComponent()==Shell,TEXT("The seated source cassette is visible above the well floor instead of buried in the deck"));StoppedPhase=Phase;
  MenuVerify::Run(Deck,TEXT("DeckStop"));MenuVerify::Run(Rig,TEXT("FocusCenter"));At=Now;Stage=4;return true;
 }
 if(Stage==4&&Now>At+.8){Expect(FindFProperty<FIntProperty>(Rig->GetClass(),TEXT("CameraMode"))->GetPropertyValue_InContainer(Rig)==0&&!PC->bShowMouseCursor,TEXT("Returning from the instrument hides the cursor and restores overview"));
  float Phase=0;auto MID=Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0));Expect(MID&&MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ReelAngleA")),Phase)&&FMath::IsNearlyEqual(Phase,StoppedPhase,.1f),TEXT("STOP freezes the actual visible material phase"));
  UE_LOG(LogTemp,Display,TEXT("ROOM POLISH VERIFY COMPLETE: %d checks, %d failures"),Checks,Failed);GEditor->RequestEndPlayMap();At=Now;Stage=99;return true;
 }
 return true;
}
void Audit() {
 auto M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/RoomArt/Meshes/SM_low_poly_vhs_tape_free_3d_model_248e7d.SM_low_poly_vhs_tape_free_3d_model_248e7d"));check(M);
 auto MD=M->GetMeshDescription(0);check(MD);FStaticMeshAttributes A(*MD);auto P=A.GetVertexPositions();
 FString CSV=TEXT("triangle,group,x1,y1,z1,x2,y2,z2,x3,y3,z3\n");
 for(auto T:MD->Triangles().GetElementIDs()) {CSV+=FString::Printf(TEXT("%d,%d"),T.GetValue(),MD->GetTrianglePolygonGroup(T).GetValue());for(auto VI:MD->GetTriangleVertexInstances(T)){auto V=P[MD->GetVertexInstanceVertex(VI)];CSV+=FString::Printf(TEXT(",%.6f,%.6f,%.6f"),V.X,V.Y,V.Z);}CSV+=TEXT("\n");}
 FFileHelper::SaveStringToFile(CSV,*(FPaths::ProjectSavedDir()/TEXT("RoomImport/vhs-triangles.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 for(auto Name:{TEXT("BP_CassetteIntegrated"),TEXT("BP_TapeDeckIntegrated"),TEXT("BP_DeskWorkstationRig")}) {
  auto BP=LoadObject<UBlueprint>(nullptr,*FString::Printf(TEXT("/Game/StationIntegration/%s.%s"),Name,Name));check(BP);
  for(auto N:BP->SimpleConstructionScript->GetAllNodes())if(auto C=Cast<USceneComponent>(N->ComponentTemplate))UE_LOG(LogTemp,Display,TEXT("ROOM AUDIT %s %s %s"),Name,*N->GetVariableName().ToString(),*C->GetRelativeTransform().ToHumanReadableString());
 }
 UE_LOG(LogTemp,Display,TEXT("ROOM AUDIT COMPLETE vhs=%s"),*M->GetBoundingBox().ToString());
}
}
