// Editor-only geometry measurements for contact, desk proportions and cassette seating.
namespace FurnitureFit {
void WidenTop(){
 auto W=GEditor->GetEditorWorldContext().World();UStaticMeshComponent* Comp=nullptr;for(TActorIterator<AStaticMeshActor> It(W);It;++It)if(auto M=It->GetStaticMeshComponent()->GetStaticMesh())if(M->GetName().Contains(TEXT("Стол2")))Comp=It->GetStaticMeshComponent();check(Comp);auto Mesh=Comp->GetStaticMesh();auto MD=Mesh->GetMeshDescription(0);check(MD);FStaticMeshAttributes Attr(*MD);auto Pos=Attr.GetVertexPositions();auto T=Comp->GetComponentTransform();int Changed=0;double OldMinX=478.111797,OldMaxX=555.14982,OldMinY=277.447754,OldMaxY=373.753;
 double MinX=DBL_MAX;for(auto ID:MD->Vertices().GetElementIDs()){auto P=T.TransformPosition(FVector(Pos[ID]));if(P.Y<373.82&&FMath::Abs(P.Z-89.658)<.025)MinX=FMath::Min(MinX,P.X);}
 if(MinX<456){check(FMath::Abs(MinX-455)<.02);UE_LOG(LogTemp,Display,TEXT("FURNITURE TABLETOP ALREADY WIDENED"));return;}
 check(FMath::Abs(MinX-OldMinX)<.02);
 for(auto ID:MD->Vertices().GetElementIDs()){auto P=T.TransformPosition(FVector(Pos[ID]));if(P.Y<373.82&&(FMath::Abs(P.Z-83.238)<.025||FMath::Abs(P.Z-89.658)<.025)){
   P.X=455.+(P.X-OldMinX)*(OldMaxX-455.)/(OldMaxX-OldMinX);P.Y=255.+(P.Y-OldMinY)*(OldMaxY-255.)/(OldMaxY-OldMinY);Pos[ID]=FVector3f(T.InverseTransformPosition(P));Changed++;
  }}
 check(Changed>0);Mesh->CommitMeshDescription(0);auto& Settings=Mesh->GetSourceModel(0).BuildSettings;Settings.bRecomputeNormals=false;Settings.bRecomputeTangents=true;Mesh->Build(false);Mesh->CreateBodySetup();Mesh->GetBodySetup()->InvalidatePhysicsData();Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;Mesh->GetBodySetup()->CreatePhysicsMeshes();Mesh->PostEditChange();RadioBuild::Save(Mesh);UE_LOG(LogTemp,Display,TEXT("FURNITURE TABLETOP WIDENED vertices=%d; legs and remaining geometry unchanged"),Changed);
}
bool SeatAudit(){
 static int Stage=0,Checks=0,Failures=0;static bool Requested=false;static double At=0,Started=FPlatformTime::Seconds();const auto Now=FPlatformTime::Seconds();
 auto Expect=[&](bool Good,const TCHAR* Message){Checks++;if(!Good)Failures++;UE_LOG(LogTemp,Display,TEXT("FURNITURE VERIFY %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Message);};
 auto Finish=[&](){UE_LOG(LogTemp,Display,TEXT("FURNITURE VERIFY COMPLETE: %d checks, %d failures"),Checks,Failures);GEditor->RequestEndPlayMap();At=Now;Stage=99;};
 if(Stage==99){if(!GEditor->PlayWorld&&Now>At+2){FPlatformMisc::RequestExit(false);return false;}return true;}
 if(Now-Started>80){Expect(false,TEXT("Runtime timeout"));Finish();return true;}
 if(!Requested){GetMutableDefault<UEditorPerformanceSettings>()->bThrottleCPUWhenNotForeground=false;FRequestPlaySessionParams P;P.GlobalMapOverride=TEXT("/Game/StationIntegration/StationIntegrationMap");GEditor->RequestPlaySession(P);Requested=true;return true;}
 auto W=GEditor->PlayWorld;if(!W||W->GetTimeSeconds()<3)return true;
 auto Deck=InteractionAccess::Find(W,TEXT("BP_TapeDeckIntegrated_C"));if(!Deck)return true;
 auto Tape=CastChecked<AActor>(MenuVerify::Obj(Deck,TEXT("CassetteReference")));auto Shell=CastChecked<UStaticMeshComponent>(MenuVerify::Obj(Tape,TEXT("CassetteShell")));
 AStaticMeshActor* Table=nullptr;for(TActorIterator<AStaticMeshActor> It(W);It;++It)if(auto M=It->GetStaticMeshComponent()->GetStaticMesh())if(M->GetName().Contains(TEXT("Стол2")))Table=*It;check(Table);
 auto Trace=[&](AActor* Only,const FVector& P,double Top,double Bottom,FHitResult& Hit){FCollisionQueryParams Q(SCENE_QUERY_STAT(FurnitureContact),true);for(TActorIterator<AActor> It(W);It;++It)if(*It!=Only)Q.AddIgnoredActor(*It);return W->LineTraceSingleByChannel(Hit,FVector(P.X,P.Y,Top),FVector(P.X,P.Y,Bottom),ECC_Visibility,Q);};
 auto Photo=[&](const TCHAR* Name){auto Dir=FPaths::ProjectSavedDir()/TEXT("FurnitureFit");IFileManager::Get().MakeDirectory(*Dir,true);FScreenshotRequest::RequestScreenshot(Dir/Name,false,false);};
 auto TapeGap=[&](){FHitResult H;const auto C=Shell->Bounds.Origin;return Trace(Table,C,110,80,H)?Shell->Bounds.Origin.Z-Shell->Bounds.BoxExtent.Z-H.ImpactPoint.Z:DBL_MAX;};
 if(Stage==0){
  int Towers=0,Props=0;
  for(TActorIterator<AStaticMeshActor> It(W);It;++It)if(auto M=It->GetStaticMeshComponent()->GetStaticMesh()){
   auto Name=M->GetName();if(Name==TEXT("SM_scene_50c5ec"))Towers++;
   if(Name.Contains(TEXT("Headset"))||Name.Contains(TEXT("notepad"))||Name.Contains(TEXT("television"))||Name==TEXT("SM_KeyboardCC0")||Name==TEXT("SM_Med_PC_Mouse")){
    Props++;auto B=It->GetStaticMeshComponent()->Bounds;bool Supported=true;for(double DX:{-.9,0.,.9})for(double DY:{-.9,0.,.9}){FHitResult H;auto P=B.Origin+FVector(DX*B.BoxExtent.X,DY*B.BoxExtent.Y,0);Supported&=Trace(Table,P,160,0,H)&&FMath::Abs(B.Origin.Z-B.BoxExtent.Z-H.ImpactPoint.Z-.02)<.03;}
    Expect(Supported,*FString::Printf(TEXT("%s rests on tabletop across its footprint"),*Name));
   }
  }
  Expect(Props==5&&Towers==0,TEXT("All five computer props exist and system unit is absent"));
  FHitResult H;Expect(Trace(Table,FVector(456,256,0),160,0,H)&&FMath::Abs(H.ImpactPoint.Z-89.656)<.02,TEXT("Expanded tabletop has collision at its new front inner corner"));
  Expect(FMath::Abs(TapeGap()-.02)<.03,TEXT("Loose cassette rests on the real tabletop"));
  for(TActorIterator<APointLight> It(W);It;++It)if(It->GetActorLabel()==TEXT("ROOM — central dim bulb")){auto L=It->GetLightComponent();Expect(L->bUseTemperature&&FMath::Abs(L->Temperature-3300)<1&&FMath::Abs(L->Intensity-1.25)<.01,TEXT("Ceiling bulb is warm 3300 K at the original dim intensity"));}
  InteractionAccess::Focus(W,TEXT("BP_TapeDeckIntegrated_C"));At=Now;Stage=1;return true;
 }
 if(Stage==1&&Now>At+.8){Shell->OnClicked.Broadcast(Shell,EKeys::LeftMouseButton);At=Now;Stage=2;return true;}
 if(Stage==2&&Now>At+1.15){
  Expect(MenuVerify::Flag(Deck,TEXT("HasCassette"))&&!MenuVerify::Flag(Deck,TEXT("CassetteMoving")),TEXT("Physical cassette click completes insertion"));
  auto Bottom=Shell->Bounds.Origin.Z-Shell->Bounds.BoxExtent.Z,Top=Shell->Bounds.Origin.Z+Shell->Bounds.BoxExtent.Z;
  FHitResult H;bool Hit=Trace(Deck,Shell->Bounds.Origin,98.2,90,H);Expect(Hit&&FMath::Abs(Bottom-H.ImpactPoint.Z-.025)<.03,TEXT("Inserted cassette rests on cassette-well floor"));
  auto Glass=CastChecked<UStaticMeshComponent>(MenuVerify::Obj(Deck,TEXT("Detail_Стекло")));
  Expect(Top<Glass->Bounds.Origin.Z-Glass->Bounds.BoxExtent.Z&&Glass->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("Cassette fits below transparent cover without a blocking glass collider"));
  FCollisionQueryParams Q(SCENE_QUERY_STAT(CassetteClickable),true);FHitResult Click;bool Clickable=W->LineTraceSingleByChannel(Click,Shell->Bounds.Origin+FVector(0,0,15),Shell->Bounds.Origin-FVector(0,0,5),ECC_Visibility,Q)&&Click.GetComponent()==Shell;Expect(Clickable,TEXT("Seated cassette remains clickable through the transparent cover"));
  UE_LOG(LogTemp,Display,TEXT("FURNITURE SEATING floor=%.6f bottom=%.6f top=%.6f glass=%.6f"),H.ImpactPoint.Z,Bottom,Top,Glass->Bounds.Origin.Z-Glass->Bounds.BoxExtent.Z);
  Photo(TEXT("cassette-seated.png"));At=Now;Stage=3;return true;
 }
 if(Stage==3&&Now>At+.4){Shell->OnClicked.Broadcast(Shell,EKeys::LeftMouseButton);At=Now;Stage=4;return true;}
 if(Stage==4&&Now>At+1.15){Expect(!MenuVerify::Flag(Deck,TEXT("HasCassette"))&&FMath::Abs(TapeGap()-.02)<.03,TEXT("Eject returns cassette onto table without hovering"));
  auto Camera=W->SpawnActor<ACameraActor>();Camera->SetActorLocation(FVector(408,286,140));Camera->SetActorRotation((FVector(503,313,94)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(65);UGameplayStatics::GetPlayerController(W,0)->SetViewTargetWithBlend(Camera,0);At=Now;Stage=5;return true;}
 if(Stage==5&&Now>At+.7){Photo(TEXT("computer-layout.png"));At=Now;Stage=6;return true;}
 if(Stage==6&&Now>At+.4){Finish();return true;}return true;
}
void Audit(){
 auto W=GEditor->GetEditorWorldContext().World();check(W);auto Dir=FPaths::ProjectSavedDir()/TEXT("FurnitureFit");IFileManager::Get().MakeDirectory(*Dir,true);TArray<FString> Rows;
 for(TActorIterator<AActor> It(W);It;++It){auto A=*It;TArray<UStaticMeshComponent*> Components;A->GetComponents(Components);
  for(auto C:Components)if(auto Mesh=C->GetStaticMesh()){
   auto Name=Mesh->GetName();bool Table=Name.Contains(TEXT("Стол2"));bool Deck=A->GetClass()->GetName()==TEXT("BP_TapeDeckIntegrated_C");bool Tape=A->GetClass()->GetName()==TEXT("BP_CassetteIntegrated_C");bool Prop=Name.Contains(TEXT("Headset"))||Name.Contains(TEXT("notepad"))||Name.Contains(TEXT("television"))||Name.Contains(TEXT("Keyboard"))||Name==TEXT("SM_scene_50c5ec");if(!Table&&!Deck&&!Tape&&!Prop)continue;
   Rows.Add(FString::Printf(TEXT("COMP\t%s\t%s\t%s\t%s\t%s\t%s"),*A->GetActorLabel(),*C->GetName(),*Mesh->GetPathName(),*C->Bounds.Origin.ToString(),*C->Bounds.BoxExtent.ToString(),*C->GetComponentTransform().ToHumanReadableString()));
   for(int I=0;I<C->GetNumMaterials();I++)if(auto M=C->GetMaterial(I))Rows.Add(FString::Printf(TEXT("MAT\t%s\t%s\t%d\t%s\t%d"),*A->GetActorLabel(),*C->GetName(),I,*M->GetPathName(),int(M->GetBlendMode())));
   if(Table||Deck||Tape){auto MD=Mesh->GetMeshDescription(0);if(!MD)continue;FStaticMeshAttributes Attr(*MD);auto Pos=Attr.GetVertexPositions();auto UV=Attr.GetVertexInstanceUVs();TArray<FString> Triangles;
    for(auto T:MD->Triangles().GetElementIDs())for(auto VI:MD->GetTriangleVertexInstances(T)){auto P=C->GetComponentTransform().TransformPosition(FVector(Pos[MD->GetVertexInstanceVertex(VI)]));auto Tex=UV.Get(VI,0);Triangles.Add(FString::Printf(TEXT("%.9f\t%.9f\t%.9f\t%.9f\t%.9f"),P.X,P.Y,P.Z,Tex.X,Tex.Y));}
    FString Filename=Table?TEXT("table"):Tape?TEXT("cassette"):C->GetName();FFileHelper::SaveStringArrayToFile(Triangles,*(Dir/(Filename+TEXT(".tsv"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
   }
  }
  if(A->GetClass()->GetName()==TEXT("BP_TapeDeckIntegrated_C"))for(auto N:{TEXT("CassetteSocket"),TEXT("CassetteTray")})if(auto C=Cast<USceneComponent>(InteractionAccess::Property(A,N)))Rows.Add(FString::Printf(TEXT("ANCHOR\t%s\t%s\t%s"),N,*C->GetRelativeTransform().ToHumanReadableString(),*C->GetComponentTransform().ToHumanReadableString()));
 }
 FFileHelper::SaveStringArrayToFile(Rows,*(Dir/TEXT("geometry-audit.tsv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);UE_LOG(LogTemp,Display,TEXT("FURNITURE GEOMETRY AUDIT COMPLETE"));
}
}
