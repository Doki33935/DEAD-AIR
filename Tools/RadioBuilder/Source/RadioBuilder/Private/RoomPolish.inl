// Current room regression checks; obsolete room/camera migrations removed.
namespace RoomPolish {
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
  MenuVerify::Run(Rig,TEXT("ResetRoomTurn"));auto InitialBP=CastChecked<UBlueprint>(Rig->GetClass()->ClassGeneratedBy);View->SetRelativeTransform(CastChecked<USceneComponent>(StationBuild::SCS(InitialBP,TEXT("ViewCenter"))->ComponentTemplate)->GetRelativeTransform());
  Expect(FMath::IsNearlyEqual(MenuVerify::Num(Rig,TEXT("RoomLookWidth")),48.,.01),TEXT("Ordinary desk look uses the original small sector"));
  Expect(DecorativeTapes==0,TEXT("No decorative duplicate cassettes remain"));Expect(SimpleMice==0,TEXT("The less detailed right-hand mouse is removed"));
  Expect(Shell->GetStaticMesh()&&Shell->GetStaticMesh()->GetName()==TEXT("SM_low_poly_vhs_tape_free_3d_model_248e7d"),TEXT("The playable cassette uses the free source model"));
  int Visible=0;TArray<UStaticMeshComponent*> Parts;Tape->GetComponents(Parts);for(auto C:Parts)if(C->GetStaticMesh()&&C->IsVisible()&&!C->bHiddenInGame)Visible++;
  Expect(Visible==1,TEXT("Only the source cassette body is rendered, without placeholder reels or cubes"));
  auto Body=CastChecked<UStaticMeshComponent>(MenuVerify::Obj(RadioActor,TEXT("Radio_Body")));Expect(Body->Bounds.Origin.Equals(FVector(498,404,107),.1),TEXT("Radio case centre matches the revised clear desk composition"));Expect(FMath::IsNearlyEqual(Body->Bounds.BoxExtent.Z*2,33.16787f,.1f),TEXT("RadioActor height matches the submitted model scale"));
  Expect(Bulb&&Bulb->GetActorLocation().X==275&&Bulb->GetActorLocation().Y==225,TEXT("The dim bulb sits in the centre of the room"));
  Expect(Bulb&&FVector::DotProduct(Bulb->GetActorLocation()-Camera->GetActorLocation(),Camera->GetActorForwardVector())<0,TEXT("The room bulb is behind the seated character"));
  auto Target=Bed->GetComponentsBoundingBox().GetCenter();auto Desired=(Target-Camera->GetActorLocation()).Rotation();Desired.Yaw-=Rig->GetActorRotation().Yaw;auto BP=CastChecked<UBlueprint>(Rig->GetClass()->ClassGeneratedBy);auto Base=CastChecked<USceneComponent>(StationBuild::SCS(BP,TEXT("ViewCenter"))->ComponentTemplate)->GetRelativeRotation();
  double NX=.99,NY=(Desired.Pitch-Base.Pitch)/-14.;Expect(FMath::Abs(NY)<.9,TEXT("The bed is reachable by the deliberate edge turn without a wide direct look sector"));
  int SX=0,SY=0;PC->GetViewportSize(SX,SY);MouseX=FMath::RoundToInt((NX+1)*SX*.5);MouseY=FMath::RoundToInt((NY+1)*SY*.5);UWidgetBlueprintLibrary::SetFocusToGameViewport();PC->SetMouseLocation(MouseX,MouseY);At=Now;Stage=1;return true;
 }
 if(Stage==1&&Now<=At+2.8){PC->SetMouseLocation(MouseX,MouseY);MenuVerify::Run(Rig,TEXT("UpdateDeskLook"));return true;}
 if(Stage==1&&Now>At+2.8){int SX=0,SY=0;PC->GetViewportSize(SX,SY);MouseX=SX/2;PC->SetMouseLocation(MouseX,MouseY);At=Now;Stage=11;return true;}
 if(Stage==11&&Now<=At+1.0){PC->SetMouseLocation(MouseX,MouseY);MenuVerify::Run(Rig,TEXT("UpdateDeskLook"));return true;}
 if(Stage==11&&Now>At+1.0){
  Expect(MenuVerify::Num(Rig,TEXT("RoomTurn"))>.85&&MenuVerify::Num(Rig,TEXT("RoomLookWidth"))<22,TEXT("Edge hold selects a stable bed sector with limited local look"));
  FVector2D Screen;int SX=0,SY=0;PC->GetViewportSize(SX,SY);bool Projected=PC->ProjectWorldLocationToScreen(Bed->GetComponentsBoundingBox().GetCenter(),Screen);
  Expect(Projected&&FMath::Abs(Screen.X-SX*.5)<SX*.15&&FMath::Abs(Screen.Y-SY*.5)<SY*.15,TEXT("Actual Blueprint mouse look turns the camera onto the bed"));
  auto Dir=FPaths::ProjectSavedDir()/TEXT("RoomPolishChecks");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/TEXT("bed-look.png"),true,false);
  InteractionAccess::Focus(Rig,TEXT("BP_TapeDeckIntegrated_C"));At=Now;Stage=2;return true;
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
  MenuVerify::Run(Deck,TEXT("DeckStop"));InteractionAccess::Back(Rig);At=Now;Stage=4;return true;
 }
 if(Stage==4&&Now>At+.8){Expect(FindFProperty<FIntProperty>(Rig->GetClass(),TEXT("CameraMode"))->GetPropertyValue_InContainer(Rig)==0&&!PC->bShowMouseCursor,TEXT("Returning from the instrument hides the cursor and restores overview"));
  float Phase=0;auto MID=Cast<UMaterialInstanceDynamic>(Shell->GetMaterial(0));Expect(MID&&MID->GetScalarParameterValue(FMaterialParameterInfo(TEXT("ReelAngleA")),Phase)&&FMath::IsNearlyEqual(Phase,StoppedPhase,.1f),TEXT("STOP freezes the actual visible material phase"));
  UE_LOG(LogTemp,Display,TEXT("ROOM POLISH VERIFY COMPLETE: %d checks, %d failures"),Checks,Failed);GEditor->RequestEndPlayMap();At=Now;Stage=99;return true;
 }
 return true;
}
}
