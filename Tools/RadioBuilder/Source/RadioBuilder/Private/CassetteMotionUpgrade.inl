// Editor-only incremental authoring. Runtime motion is ordinary Blueprint code.
namespace CassetteMotionUpgrade {
using namespace ReceiverUpgrade;
UBlueprint* TapeBP;
UStaticMesh* Dish;

void Slots() {
 for(int I=1;I<=3;I++) {
  auto B=CastChecked<UButton>(MenuBP->WidgetTree->FindWidget(*FString::Printf(TEXT("Slot%d"),I)));
  auto Style=B->WidgetStyle;
  Style.Normal=FSlateColorBrush(FLinearColor(.018,.024,.025,.99));
  Style.Hovered=FSlateColorBrush(FLinearColor(.07,.10,.095,1));
  Style.Pressed=FSlateColorBrush(FLinearColor(.11,.09,.045,1));
  Style.Disabled=FSlateColorBrush(FLinearColor(.018,.022,.023,.99));
  Style.NormalForeground=FLinearColor(.94,.92,.84);
  Style.HoveredForeground=FLinearColor(1,.77,.37);
  Style.PressedForeground=FLinearColor(1,.88,.55);
  Style.DisabledForeground=FLinearColor(.66,.69,.66);
  B->SetStyle(Style);
 }
 // Keep date punctuation, remove the decorative separator between words.
 for(auto G:MenuManager->FunctionGraphs)for(auto N:G->Nodes)for(auto P:N->Pins)
  if(P->Direction==EGPD_Input&&P->PinType.PinCategory==UEdGraphSchema_K2::PC_String&&P->DefaultValue.Contains(TEXT("·")))
   RadioBuild::Schema->TrySetDefaultValue(*P,P->DefaultValue.Replace(TEXT("  ·  "),P->DefaultValue.Contains(TEXT("Пусто"))||P->DefaultValue.Contains(TEXT("Не удалось"))?TEXT("\n"):TEXT("   ")).Replace(TEXT("·"),TEXT(" ")));
 Compile(MenuBP);Compile(MenuManager);
}

UStaticMesh* ReelMesh(UStaticMesh* Source,int Side,int Axis,double Split,FVector& Center) {
 auto MD=Source->GetMeshDescription(0);check(MD);FStaticMeshAttributes SA(*MD);
 auto Pos=SA.GetVertexPositions();FBox Box(ForceInit);int Kept=0;
 for(auto T:MD->Triangles().GetElementIDs()) {
  auto VI=MD->GetTriangleVertexInstances(T);double Mean=0;double Min=MAX_dbl,Max=-MAX_dbl;
  for(auto V:VI){double P=Pos[MD->GetVertexInstanceVertex(V)][Axis];Mean+=P/3;Min=FMath::Min(Min,P);Max=FMath::Max(Max,P);}
  checkf(!(Min<Split-.001&&Max>Split+.001),TEXT("Reel split would cut a triangle; artist separation required"));
  if((Mean<Split?0:1)==Side){Kept++;for(auto V:VI)Box+=FVector(Pos[MD->GetVertexInstanceVertex(V)]);}
 }
 check(Kept>0);Center=Box.GetCenter();
 FMeshDescription NewMD;FStaticMeshAttributes NA(NewMD);NA.Register();
 auto NP=NA.GetVertexPositions();auto NU=NA.GetVertexInstanceUVs();auto SU=SA.GetVertexInstanceUVs();NU.SetNumChannels(SU.GetNumChannels());
 auto NN=NA.GetVertexInstanceNormals();auto SN=SA.GetVertexInstanceNormals();
 TMap<FVertexID,FVertexID> Vertices;auto Group=NewMD.CreatePolygonGroup();
 NA.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Cassette");
 for(auto T:MD->Triangles().GetElementIDs()) {
  auto VI=MD->GetTriangleVertexInstances(T);double Mean=0;for(auto V:VI)Mean+=Pos[MD->GetVertexInstanceVertex(V)][Axis]/3;
  if((Mean<Split?0:1)!=Side)continue;
  TArray<FVertexInstanceID> Corners;
  for(auto OldVI:VI){auto OldV=MD->GetVertexInstanceVertex(OldVI);auto Found=Vertices.Find(OldV);FVertexID NV;
   if(Found)NV=*Found;else{NV=NewMD.CreateVertex();Vertices.Add(OldV,NV);NP[NV]=FVector3f(FVector(Pos[OldV])-Center);}
   auto NVI=NewMD.CreateVertexInstance(NV);NN[NVI]=SN[OldVI];for(int C=0;C<SU.GetNumChannels();C++)NU.Set(NVI,C,SU.Get(OldVI,C));Corners.Add(NVI);
  }
  NewMD.CreateTriangle(Group,Corners);
 }
 auto Name=Side==0?TEXT("SM_Cassette_ReelA"):TEXT("SM_Cassette_ReelB");
 auto M=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/StationIntegration/Art/TapeDeck/%s.%s"),Name,Name));
 if(!M)M=CastChecked<UStaticMesh>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().DuplicateAsset(Name,TEXT("/Game/StationIntegration/Art/TapeDeck"),Source));
 *M->GetMeshDescription(0)=MoveTemp(NewMD);M->CommitMeshDescription(0);
 auto& Settings=M->GetSourceModel(0).BuildSettings;Settings.bRecomputeNormals=true;Settings.bRecomputeTangents=true;Settings.bUseMikkTSpace=false;M->Build(false);M->PostEditChange();Save(M);
 UE_LOG(LogTemp,Display,TEXT("CASSETTE REEL %s triangles=%d pivot=%s bounds=%s"),Name,Kept,*Center.ToString(),*M->GetBoundingBox().GetSize().ToString());return M;
}

void Reels() {
 auto Old=CastChecked<UStaticMeshComponent>(StationBuild::SCS(TapeBP,TEXT("cassete_base"))->ComponentTemplate);
 auto Source=Old->GetStaticMesh();auto B=Source->GetBoundingBox();auto Size=B.GetSize();int Axis=Size.Y>Size.X?1:0;double Split=B.GetCenter()[Axis];
 Old->SetVisibility(false);Old->SetHiddenInGame(true);
 auto Root=StationBuild::SCS(TapeBP,TEXT("DefaultSceneRoot"));
 for(int I=0;I<2;I++) {
  FVector Center;auto Mesh=ReelMesh(Source,I,Axis,Split,Center);auto Name=I==0?TEXT("ReelA"):TEXT("ReelB");
  auto NodeN=StationBuild::SCS(TapeBP,Name);if(!NodeN)NodeN=StationBuild::Part(TapeBP,Root,Name,UStaticMeshComponent::StaticClass());
  auto C=CastChecked<UStaticMeshComponent>(NodeN->ComponentTemplate);C->SetStaticMesh(Mesh);C->SetRelativeLocation(Center);C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  // A small painted index makes motion legible on the temporary untextured reels.
  auto MarkName=I==0?TEXT("ReelIndexA"):TEXT("ReelIndexB");auto Mark=StationBuild::SCS(TapeBP,MarkName);if(!Mark)Mark=StationBuild::Part(TapeBP,NodeN,MarkName,UStaticMeshComponent::StaticClass());
  auto Marker=CastChecked<UStaticMeshComponent>(Mark->ComponentTemplate);Marker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Marker->SetRelativeLocation(FVector(2.7,0,Mesh->GetBoundingBox().Max.Z+.035));Marker->SetRelativeScale3D(FVector(.018,.005,.0005));Marker->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StationIntegration/Art/Materials/M_DeckButtons.M_DeckButtons")));Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);Marker->SetMobility(EComponentMobility::Movable);Marker->SetCastShadow(false);
 }
 Declare(TapeBP,{TEXT("AnimateReels")});UEdGraphPin* X;auto G=Reset(TapeBP,TEXT("AnimateReels"),X);auto D=V(G,TEXT("DeckReference"));
 auto Good=Branch(G,X,Valid(G,D));auto Playing=Op(G,TEXT("BooleanAND"),Ext(G,DeckBP->GeneratedClass,TEXT("Playing"),D),Ext(G,DeckBP->GeneratedClass,TEXT("HasCassette"),D));
 auto On=Branch(G,Pin(Good,TEXT("then")),Op(G,TEXT("BooleanAND"),Playing,Op(G,TEXT("Not_PreBool"),Ext(G,DeckBP->GeneratedClass,TEXT("CassetteMoving"),D))));X=Pin(On,TEXT("then"));
 auto Mode=Ext(G,DeckBP->GeneratedClass,TEXT("TransportMode"),D);
 auto Speed=Select(G,Op(G,TEXT("Greater_IntInt"),Mode,nullptr,TEXT("1")),Scalar(G,TEXT("3")),TEXT("1"));
 auto Direction=Select(G,Op(G,TEXT("EqualEqual_IntInt"),Mode,nullptr,TEXT("3")),Scalar(G,TEXT("-1")),TEXT("1"));
 for(auto Pair:{TPair<const TCHAR*,double>(TEXT("ReelA"),90),{TEXT("ReelB"),82}}) {
  auto R=Call(G,UKismetMathLibrary::StaticClass(),TEXT("MakeRotator"));
  auto Angle=Op(G,TEXT("Multiply_DoubleDouble"),Op(G,TEXT("Multiply_DoubleDouble"),Delta(G),Speed),Op(G,TEXT("Multiply_DoubleDouble"),Direction,nullptr,*FString::SanitizeFloat(Pair.Value)));
  Link(Angle,Pin(R,TEXT("Yaw")));auto Turn=Flow(G,X,USceneComponent::StaticClass(),TEXT("K2_AddLocalRotation"),V(G,Pair.Key));Link(Pin(R,TEXT("ReturnValue")),Pin(Turn,TEXT("DeltaRotation")));
 }
 auto CDO=CastChecked<AActor>(TapeBP->GeneratedClass->GetDefaultObject());CDO->PrimaryActorTick.bCanEverTick=true;CDO->PrimaryActorTick.bStartWithTickEnabled=true;
 auto EG=Events(TapeBP);UK2Node_Event* Tick=nullptr;for(auto N:EG->Nodes)if(auto E=Cast<UK2Node_Event>(N))if(E->EventReference.GetMemberName()==TEXT("ReceiveTick"))Tick=E;
 if(!Tick)Tick=Event(EG,AActor::StaticClass(),TEXT("ReceiveTick"));Tick->SetEnabledState(ENodeEnabledState::Enabled,true);X=Pin(Tick,TEXT("then"));if(!X->LinkedTo.Num())Invoke(TapeBP,EG,X,TEXT("AnimateReels"));
 Compile(TapeBP);
}

UEdGraphPin* ActorValue(UEdGraph* G,const TCHAR* Fn){auto C=Call(G,AActor::StaticClass(),Fn);Link(V(G,TEXT("CassetteReference")),Pin(C,TEXT("self")));return Pin(C,TEXT("ReturnValue"));}
UEdGraphPin* ComponentValue(UEdGraph* G,const TCHAR* Name,const TCHAR* Fn){auto C=Call(G,USceneComponent::StaticClass(),Fn);Link(V(G,Name),Pin(C,TEXT("self")));return Pin(C,TEXT("ReturnValue"));}
void StartMotion(UEdGraph* G,UEdGraphPin* X,bool Insert) {
 Assign(G,X,TEXT("CassetteMoving"),nullptr,TEXT("true"));Assign(G,X,TEXT("CassetteMotion"),nullptr,Insert?TEXT("1"):TEXT("2"));Assign(G,X,TEXT("CassetteMotionTime"),nullptr,TEXT("0"));
 Assign(G,X,TEXT("CassetteStart"),ActorValue(G,TEXT("K2_GetActorLocation")));Assign(G,X,TEXT("CassetteStartRotation"),ActorValue(G,TEXT("K2_GetActorRotation")));
 auto Name=Insert?TEXT("CassetteSocket"):TEXT("CassetteTray");Assign(G,X,TEXT("CassetteTarget"),ComponentValue(G,Name,TEXT("K2_GetComponentLocation")));Assign(G,X,TEXT("CassetteTargetRotation"),ComponentValue(G,Name,TEXT("K2_GetComponentRotation")));
 auto A=Flow(G,X,AActor::StaticClass(),TEXT("K2_AttachToComponent"),V(G,TEXT("CassetteReference")));Link(V(G,TEXT("DefaultSceneRoot")),Pin(A,TEXT("Parent")));for(auto P:{TEXT("LocationRule"),TEXT("RotationRule"),TEXT("ScaleRule")})Def(A,P,TEXT("KeepWorld"));Def(A,TEXT("bWeldSimulatedBodies"),TEXT("false"));
 StationBuild::Status(G,X,Insert?TEXT("Установка кассеты"):TEXT("Извлечение кассеты"));
}
void Motion() {
 for(auto N:{TEXT("CassetteStart"),TEXT("CassetteTarget")})Variable(DeckBP,N,Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FVector>::Get()));
 for(auto N:{TEXT("CassetteStartRotation"),TEXT("CassetteTargetRotation")})Variable(DeckBP,N,Type(UEdGraphSchema_K2::PC_Struct,TBaseStructure<FRotator>::Get()));
 Variable(DeckBP,TEXT("CassetteMoving"),Type(UEdGraphSchema_K2::PC_Boolean),TEXT("false"));Variable(DeckBP,TEXT("CassetteMotion"),Type(UEdGraphSchema_K2::PC_Int),TEXT("0"));Variable(DeckBP,TEXT("CassetteMotionTime"),StationBuild::Real(),TEXT("0"));
 Declare(DeckBP,{TEXT("BeginCassetteInsert"),TEXT("BeginCassetteEject"),TEXT("UpdateCassetteMotion"),TEXT("CancelCassetteMotion")});
 UEdGraphPin* X;auto G=Reset(DeckBP,TEXT("BeginCassetteInsert"),X);
 auto Can=Op(G,TEXT("BooleanAND"),V(G,TEXT("ControlsEnabled")),Op(G,TEXT("Not_PreBool"),V(G,TEXT("CassetteMoving"))));
 Can=Op(G,TEXT("BooleanAND"),Can,Op(G,TEXT("Not_PreBool"),V(G,TEXT("HasCassette"))));Can=Op(G,TEXT("BooleanAND"),Can,Valid(G,V(G,TEXT("CassetteReference"))));
 auto Ready=Branch(G,X,Can);X=Pin(Ready,TEXT("then"));
 auto Row=Node<UK2Node_GetDataTableRow>(G);Row->AllocateDefaultPins();Row->GetDataTablePin()->DefaultObject=TransportUpgrade::TapeTable;Row->PinDefaultValueChanged(Row->GetDataTablePin());Link(V(G,TEXT("TapeID")),Row->GetRowNamePin());Link(X,Pin(Row,TEXT("execute")));
 auto S=TransportUpgrade::TapeStruct;auto Split=Node<UK2Node_BreakStruct>(G);Split->StructType=S;Split->AllocateDefaultPins();Link(Row->GetResultPin(),Split->Pins[0]);
 auto ValidRow=Op(G,TEXT("BooleanAND"),Valid(G,Pin(Split,*Field(S,TEXT("Clip"))->GetName())),Op(G,TEXT("Greater_DoubleDouble"),Pin(Split,*Field(S,TEXT("Duration"))->GetName()),nullptr,TEXT("0")));
 auto Data=Branch(G,Row->GetThenPin(),ValidRow);StartMotion(G,Pin(Data,TEXT("then")),true);auto Fail=Pin(Data,TEXT("else"));StationBuild::Status(G,Fail,TEXT("На кассете нет доступной записи"));Fail=Row->GetRowNotFoundPin();StationBuild::Status(G,Fail,TEXT("Не удалось прочитать кассету"));
 G=Reset(DeckBP,TEXT("BeginCassetteEject"),X);Ready=Branch(G,X,Op(G,TEXT("BooleanAND"),Op(G,TEXT("BooleanAND"),V(G,TEXT("ControlsEnabled")),V(G,TEXT("HasCassette"))),Op(G,TEXT("Not_PreBool"),V(G,TEXT("CassetteMoving")))));X=Pin(Ready,TEXT("then"));Invoke(DeckBP,G,X,TEXT("DeckStop"));StartMotion(G,X,false);
 G=Reset(DeckBP,TEXT("ToggleCassette"),X);auto B=Branch(G,X,V(G,TEXT("HasCassette")));auto E=Pin(B,TEXT("then"));Invoke(DeckBP,G,E,TEXT("BeginCassetteEject"));E=Pin(B,TEXT("else"));Invoke(DeckBP,G,E,TEXT("BeginCassetteInsert"));
 G=Reset(DeckBP,TEXT("CancelCassetteMotion"),X);Assign(G,X,TEXT("CassetteMoving"),nullptr,TEXT("false"));Assign(G,X,TEXT("CassetteMotion"),nullptr,TEXT("0"));Assign(G,X,TEXT("CassetteMotionTime"),nullptr,TEXT("0"));Invoke(DeckBP,G,X,TEXT("UpdateCassettePlacement"));
 G=Reset(DeckBP,TEXT("UpdateCassetteMotion"),X);B=Branch(G,X,V(G,TEXT("CassetteMoving")));X=Pin(B,TEXT("then"));
 Assign(G,X,TEXT("CassetteMotionTime"),Op(G,TEXT("Add_DoubleDouble"),V(G,TEXT("CassetteMotionTime")),Delta(G)));
 auto T=Clamp(G,Op(G,TEXT("Divide_DoubleDouble"),V(G,TEXT("CassetteMotionTime")),nullptr,TEXT("0.85")),TEXT("0"),TEXT("1"));
 auto Smooth=Op(G,TEXT("Multiply_DoubleDouble"),Op(G,TEXT("Multiply_DoubleDouble"),T,T),Op(G,TEXT("Subtract_DoubleDouble"),Scalar(G,TEXT("3")),Op(G,TEXT("Multiply_DoubleDouble"),T,nullptr,TEXT("2"))));
 auto Location=Call(G,UKismetMathLibrary::StaticClass(),TEXT("VLerp"));Link(V(G,TEXT("CassetteStart")),Pin(Location,TEXT("A")));Link(V(G,TEXT("CassetteTarget")),Pin(Location,TEXT("B")));Link(Smooth,Pin(Location,TEXT("Alpha")));
 auto Sine=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Sin"));Link(Op(G,TEXT("Multiply_DoubleDouble"),Smooth,nullptr,TEXT("3.14159265359")),Pin(Sine,TEXT("A")));
 auto Lift=Op(G,TEXT("Multiply_VectorFloat"),Vec(G,0,0,10),Pin(Sine,TEXT("ReturnValue")));
 auto Rotation=Call(G,UKismetMathLibrary::StaticClass(),TEXT("RLerp"));Link(V(G,TEXT("CassetteStartRotation")),Pin(Rotation,TEXT("A")));Link(V(G,TEXT("CassetteTargetRotation")),Pin(Rotation,TEXT("B")));Link(Smooth,Pin(Rotation,TEXT("Alpha")));Def(Rotation,TEXT("bShortestPath"),TEXT("true"));
 auto Move=Flow(G,X,AActor::StaticClass(),TEXT("K2_SetActorLocationAndRotation"),V(G,TEXT("CassetteReference")));Link(Op(G,TEXT("Add_VectorVector"),Pin(Location,TEXT("ReturnValue")),Lift),Pin(Move,TEXT("NewLocation")));Link(Pin(Rotation,TEXT("ReturnValue")),Pin(Move,TEXT("NewRotation")));Def(Move,TEXT("bTeleport"),TEXT("true"));
 auto Done=Branch(G,X,Op(G,TEXT("GreaterEqual_DoubleDouble"),T,nullptr,TEXT("1")));X=Pin(Done,TEXT("then"));Assign(G,X,TEXT("CassetteMoving"),nullptr,TEXT("false"));auto Direction=Branch(G,X,Op(G,TEXT("EqualEqual_IntInt"),V(G,TEXT("CassetteMotion")),nullptr,TEXT("1")));
 for(auto Side:{TPair<UEdGraphPin*,const TCHAR*>(Pin(Direction,TEXT("then")),TEXT("InsertTape")),{Pin(Direction,TEXT("else")),TEXT("EjectTape")}}){E=Side.Key;Assign(G,E,TEXT("CassetteMotion"),nullptr,TEXT("0"));Invoke(DeckBP,G,E,Side.Value);Invoke(DeckBP,G,E,TEXT("UpdateCassettePlacement"));}
 PresentationUpgrade::Once(DeckBP,Graph(DeckBP,TEXT("AnimateDeckButtons")),TEXT("UpdateCassetteMotion"));
 for(auto Fn:{TEXT("DeckPlay"),TEXT("DeckFastForward"),TEXT("DeckRewind"),TEXT("DeckStop"),TEXT("DeckRecord")}){
  G=Graph(DeckBP,Fn);auto Out=Pin(Entry(G),TEXT("then"));auto Next=Out->LinkedTo;Out->BreakAllPinLinks();auto Guard=Branch(G,Out,Op(G,TEXT("Not_PreBool"),V(G,TEXT("CassetteMoving"))));for(auto P:Next)Link(Pin(Guard,TEXT("then")),P);
 }
 // Loading cancels the visual transition; the saved state is always a settled logical state.
 Declare(MenuManager,{TEXT("CancelWorkstationMotion")});G=Reset(MenuManager,TEXT("CancelWorkstationMotion"),X);auto Find=Flow(G,X,UGameplayStatics::StaticClass(),TEXT("GetActorOfClass"));Pin(Find,TEXT("ActorClass"))->DefaultObject=DeckBP->GeneratedClass;auto D=MenuBuild::Cast(G,DeckBP->GeneratedClass,Pin(Find,TEXT("ReturnValue")))->GetCastResultPin();auto Has=Branch(G,X,Valid(G,D));X=Pin(Has,TEXT("then"));Flow(G,X,DeckBP->GeneratedClass,TEXT("CancelCassetteMotion"),D);
 PresentationUpgrade::Once(MenuManager,Graph(MenuManager,TEXT("RestoreWorkstation")),TEXT("CancelWorkstationMotion"));Compile(DeckBP);Compile(MenuManager);
}

void RestoreGuards() {
 // UE may deliver an initial zero playback percentage after Play(StartTime).
 // Keep the restored position until the audio cursor reaches the requested seek.
 auto G=Events(DeckBP);auto Nodes=G->Nodes;
 for(auto N:Nodes)if(auto S=Cast<UK2Node_VariableSet>(N))if(S->GetVarName()==TEXT("PositionSeconds")){
  auto Value=Pin(S,TEXT("PositionSeconds"));if(!Value->LinkedTo.Num())continue;
  auto Candidate=Value->LinkedTo[0];auto Before=Pin(S,TEXT("execute"))->LinkedTo;
  auto Difference=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Abs"));Link(Op(G,TEXT("Subtract_DoubleDouble"),Candidate,V(G,TEXT("PositionSeconds"))),Pin(Difference,TEXT("A")));
  auto Near=Op(G,TEXT("Less_DoubleDouble"),Pin(Difference,TEXT("ReturnValue")),nullptr,TEXT("0.75"));
  auto Accept=Op(G,TEXT("BooleanAND"),Op(G,TEXT("Not_PreBool"),V(G,TEXT("SeekingAudio"))),Op(G,TEXT("BooleanOR"),V(G,TEXT("HasPlaybackPercent")),Near));
  Pin(S,TEXT("execute"))->BreakAllPinLinks();for(auto P:Before){auto B=Branch(G,P,Accept);Link(Pin(B,TEXT("then")),Pin(S,TEXT("execute")));}
 }
 // Desired player is transient: select Electra each time the source is opened.
 G=Graph(MenuManager,TEXT("StartMenuVideo"));Nodes=G->Nodes;
 for(auto N:Nodes)if(auto C=Cast<UK2Node_CallFunction>(N))if(C->GetFunctionName()==TEXT("OpenSource")){
  auto In=Pin(C,TEXT("execute"));auto Before=In->LinkedTo;In->BreakAllPinLinks();
  for(auto P:Before){auto X=P;auto Set=Flow(G,X,UMediaPlayer::StaticClass(),TEXT("SetDesiredPlayerName"));Pin(Set,TEXT("self"))->DefaultObject=Pin(C,TEXT("self"))->DefaultObject;Def(Set,TEXT("PlayerName"),TEXT("ElectraPlayer"));Link(X,In);}
 }
}
void ImportDish() {
 auto UI=NewObject<UFbxImportUI>();UI->bAutomatedImportShouldDetectType=false;UI->MeshTypeToImport=FBXIT_StaticMesh;UI->bImportAsSkeletal=false;UI->bImportMaterials=false;UI->bImportTextures=false;UI->bImportAnimations=false;
 UI->StaticMeshImportData->bCombineMeshes=true;UI->StaticMeshImportData->bTransformVertexToAbsolute=true;UI->StaticMeshImportData->bAutoGenerateCollision=false;UI->StaticMeshImportData->bBuildNanite=false;UI->StaticMeshImportData->bGenerateLightmapUVs=false;UI->StaticMeshImportData->bConvertScene=true;UI->StaticMeshImportData->bConvertSceneUnit=true;
 auto Task=NewObject<UAssetImportTask>();Task->Filename=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../Tools/StationIntegration/SourceArt/DishReview.fbx"));Task->DestinationPath=TEXT("/Game/StationIntegration/Art/DishReview");Task->DestinationName=TEXT("SM_DishReview");Task->Options=UI;Task->bAutomated=true;Task->bSave=true;Task->bReplaceExisting=false;
 Dish=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/StationIntegration/Art/DishReview/SM_DishReview.SM_DishReview"));
 if(!Dish){FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().ImportAssetTasks({Task});Dish=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/StationIntegration/Art/DishReview/SM_DishReview.SM_DishReview"));}check(Dish);
 auto M=StationBuild::Material(TEXT("M_DishReview"),FLinearColor(.14,.135,.145),.65,.25);for(int I=0;I<Dish->GetStaticMaterials().Num();I++)Dish->SetMaterial(I,M);Save(Dish);
 auto MD=Dish->GetMeshDescription(0);check(MD);FStaticMeshAttributes A(*MD);
 UE_LOG(LogTemp,Display,TEXT("DISH REVIEW vertices=%d triangles=%d uv=%d materialSlots=%d boundsMin=%s boundsMax=%s sizeCm=%s"),MD->Vertices().Num(),MD->Triangles().Num(),A.GetVertexInstanceUVs().GetNumChannels(),Dish->GetStaticMaterials().Num(),*Dish->GetBoundingBox().Min.ToString(),*Dish->GetBoundingBox().Max.ToString(),*Dish->GetBoundingBox().GetSize().ToString());
}
void Build() {
 TransportUpgrade::Load();TapeBP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_CassetteIntegrated.BP_CassetteIntegrated"));check(TapeBP);
 Motion();Reels();Slots();RestoreGuards();ImportDish();
 // Include the whole side tray in the close-up now that insertion uses the physical cassette.
 auto DeckView=CastChecked<UChildActorComponent>(StationBuild::SCS(RigBP,TEXT("ViewDeck"))->ComponentTemplate);CastChecked<ACameraActor>(DeckView->GetChildActorTemplate())->GetCameraComponent()->SetFieldOfView(66);
 for(auto B:{TapeBP,DeckBP,RigBP,MenuManager,static_cast<UBlueprint*>(MenuBP)}){Compile(B);Layout(B);Save(B);}
 ReceiverUpgrade::Scene();UE_LOG(LogTemp,Display,TEXT("CASSETTE MOTION UPGRADE COMPLETE"));
}
}
