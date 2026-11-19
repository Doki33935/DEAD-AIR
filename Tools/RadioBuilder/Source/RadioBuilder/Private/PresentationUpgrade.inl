// Incremental Blueprint/UMG authoring; never recreate the manually arranged radio UI.
namespace PresentationUpgrade {
using namespace ReceiverUpgrade;

void Once(UBlueprint* BP, UEdGraph* G, const TCHAR* Fn) {
 for (auto N:G->Nodes) if (auto C=Cast<UK2Node_CallFunction>(N)) if(C->GetFunctionName()==Fn) return;
 Append(BP,G,Fn);
}
void Tail(UBlueprint* BP, UEdGraph* G, const TCHAR* Fn) {
 for(auto N:G->Nodes)if(auto C=Cast<UK2Node_CallFunction>(N))if(C->GetFunctionName()==Fn)return;
 auto Nodes=G->Nodes;
 for(auto N:Nodes) {
  auto P=N->FindPin(TEXT("then"));
  if(P && !P->LinkedTo.Num()) {auto X=P; Invoke(BP,G,X,Fn);}
 }
}
void Hide(UBlueprint* BP, const TCHAR* Name) {
 auto WB=CastChecked<UWidgetBlueprint>(BP); if(auto W=WB->WidgetTree->FindWidget(Name)) W->SetVisibility(ESlateVisibility::Collapsed);
 TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
 for(auto G:Graphs) for(auto N:G->Nodes) if(auto C=Cast<UK2Node_CallFunction>(N)) if(C->GetFunctionName()==TEXT("SetVisibility")) {
  auto P=Pin(C,TEXT("self")); if(P->LinkedTo.Num()) if(auto Vn=Cast<UK2Node_VariableGet>(P->LinkedTo[0]->GetOwningNode())) if(Vn->GetVarName()==Name) Def(C,TEXT("InVisibility"),TEXT("Collapsed"));
 }
}
UEdGraphPin* CameraManager(UEdGraph* G) {return Pin(Call(G,UGameplayStatics::StaticClass(),TEXT("GetPlayerCameraManager")),TEXT("ReturnValue"));}
void ManualFade(UEdGraph* G,UEdGraphPin*& X,const TCHAR* Alpha) {
 auto C=Flow(G,X,APlayerCameraManager::StaticClass(),TEXT("SetManualCameraFade"),CameraManager(G));
 Def(C,TEXT("InFadeAmount"),Alpha); Def(C,TEXT("Color"),TEXT("(R=0,G=0,B=0,A=1)")); Def(C,TEXT("bInFadeAudio"),TEXT("false"));
}
UEdGraphPin* RealTime(UEdGraph* G) {return Pin(Call(G,UGameplayStatics::StaticClass(),TEXT("GetRealTimeSeconds")),TEXT("ReturnValue"));}
UEdGraphPin* MenuWidget(UEdGraph* G,const TCHAR* Name) {return Ext(G,MenuBP->GeneratedClass,Name,V(G,TEXT("View")));}
void MenuVisibility(UEdGraph* G,UEdGraphPin*& X,const TCHAR* Name,const TCHAR* Value) {
 auto C=Flow(G,X,UWidget::StaticClass(),TEXT("SetVisibility"),MenuWidget(G,Name));Def(C,TEXT("InVisibility"),Value);
}
void Recenter(UEdGraph* G,UEdGraphPin*& X) {
 auto Size=Call(G,APlayerController::StaticClass(),TEXT("GetViewportSize")); Link(PC(G),Pin(Size,TEXT("self")));
 auto Set=Flow(G,X,APlayerController::StaticClass(),TEXT("SetMouseLocation"),PC(G));
 Link(Op(G,TEXT("Divide_IntInt"),Pin(Size,TEXT("SizeX")),nullptr,TEXT("2")),Pin(Set,TEXT("X")));
 Link(Op(G,TEXT("Divide_IntInt"),Pin(Size,TEXT("SizeY")),nullptr,TEXT("2")),Pin(Set,TEXT("Y")));
}
void CamerasAndInput() {
 // The receiver may own the view in its standalone sample, but never on a workstation map.
 auto EG=Events(ReceiverBP); auto Nodes=EG->Nodes;
 for(auto N:Nodes) if(auto C=Cast<UK2Node_CallFunction>(N)) if(C->GetFunctionName()==TEXT("SetViewTargetWithBlend")) {
  auto In=Pin(C,TEXT("execute")); if(!In->LinkedTo.Num()) continue;
  auto Before=In->LinkedTo[0]; In->BreakAllPinLinks(); auto X=Before;
  auto Find=Flow(EG,X,UGameplayStatics::StaticClass(),TEXT("GetActorOfClass")); Pin(Find,TEXT("ActorClass"))->DefaultObject=RigBP->GeneratedClass;
  auto B=Branch(EG,X,Valid(EG,Pin(Find,TEXT("ReturnValue"))));
  for(auto After:Pin(C,TEXT("then"))->LinkedTo) Link(Pin(B,TEXT("then")),After);
  Link(Pin(B,TEXT("else")),In);
 }
 Declare(RigBP,{TEXT("InitializeDeskCamera")}); UEdGraphPin* X;
 auto G=Reset(RigBP,TEXT("InitializeDeskCamera"),X);
 Assign(G,X,TEXT("CameraMode"),nullptr,TEXT("0")); Assign(G,X,TEXT("Transitioning"),nullptr,TEXT("false"));
 auto View=Flow(G,X,APlayerController::StaticClass(),TEXT("SetViewTargetWithBlend"),PC(G));
 Link(Ext(G,UChildActorComponent::StaticClass(),TEXT("ChildActor"),V(G,TEXT("ViewCenter"))),Pin(View,TEXT("NewViewTarget"))); Def(View,TEXT("BlendTime"),TEXT("0"));
 ManualFade(G,X,TEXT("1")); Recenter(G,X);
 auto EventsG=Events(RigBP); for(auto N:EventsG->Nodes) if(auto E=Cast<UK2Node_Event>(N)) if(E->EventReference.GetMemberName()==TEXT("ReceiveBeginPlay")) {
  auto Out=Pin(E,TEXT("then")); if(!Out->LinkedTo.Num())continue; auto Next=Out->LinkedTo; Out->BreakAllPinLinks(); X=Out; Invoke(RigBP,EventsG,X,TEXT("InitializeDeskCamera")); for(auto P:Next) Link(X,P); break;
 }
 // Delayed widget setup must not initiate a second blend or reset the view selected at BeginPlay.
 G=Graph(RigBP,TEXT("InitializeBench"));
 for(auto N:G->Nodes) if(auto C=Cast<UK2Node_CallFunction>(N)) if(C->GetFunctionName()==TEXT("FocusCenter")) {
  auto Before=Pin(C,TEXT("execute"))->LinkedTo;auto After=Pin(C,TEXT("then"))->LinkedTo;
  Pin(C,TEXT("execute"))->BreakAllPinLinks();Pin(C,TEXT("then"))->BreakAllPinLinks();
  for(auto P:Before) {X=P;Invoke(RigBP,G,X,TEXT("ApplyGates"));for(auto Q:After)Link(X,Q);}break;
 }
 // No invisible top strip reserves any part of the desk look area.
 for(auto Fn:{TEXT("UpdateDeskLook"),TEXT("UpdateInstrumentGaze")}) for(auto N:Graph(RigBP,Fn)->Nodes) if(auto C=Cast<UK2Node_CallFunction>(N)) {
  if(C->GetFunctionName()==TEXT("Multiply_DoubleDouble")) if(Pin(C,TEXT("B"))->DefaultValue==TEXT("0.12")||Pin(C,TEXT("B"))->DefaultValue==TEXT("0.15")) Def(C,TEXT("B"),TEXT("0"));
  if(C->GetFunctionName()==TEXT("Concat_StrStr")) {
   auto P=Pin(C,TEXT("A"));
   if(P->DefaultValue.Contains(TEXT("ЛКМ"))) Def(C,TEXT("A"),TEXT("ЛКМ приблизить"));
   if(P->DefaultValue==TEXT("Наведите центр экрана на прибор")) Def(C,TEXT("A"),TEXT(""));
  }
 }
 G=Reset(RigBP,TEXT("UpdateDeskCursor"),X);
 auto Show=Op(G,TEXT("NotEqual_IntInt"),V(G,TEXT("CameraMode")),nullptr,TEXT("0"));
 auto Changed=Branch(G,X,Op(G,TEXT("NotEqual_BoolBool"),Ext(G,APlayerController::StaticClass(),TEXT("bShowMouseCursor"),PC(G)),Show));
 auto B=Branch(G,Pin(Changed,TEXT("then")),Show);
 for(auto Side:{TPair<UEdGraphPin*,bool>(Pin(B,TEXT("then")),true),{Pin(B,TEXT("else")),false}}) {
  auto E=Side.Key;
  Put(G,E,APlayerController::StaticClass(),TEXT("bShowMouseCursor"),PC(G),nullptr,Side.Value?TEXT("true"):TEXT("false"));
  auto Mode=Flow(G,E,UWidgetBlueprintLibrary::StaticClass(),TEXT("SetInputMode_GameAndUIEx"));Link(PC(G),Pin(Mode,TEXT("PlayerController")));
  Def(Mode,TEXT("bHideCursorDuringCapture"),TEXT("false"));Def(Mode,TEXT("InMouseLockMode"),Side.Value?TEXT("DoNotLock"):TEXT("LockAlways"));
  Recenter(G,E);
 }
 Declare(RigBP,{TEXT("RecenterFocusedCursor")});G=Reset(RigBP,TEXT("RecenterFocusedCursor"),X);
 auto Focused=Branch(G,X,Op(G,TEXT("Greater_IntInt"),V(G,TEXT("CameraMode")),nullptr,TEXT("0")));X=Pin(Focused,TEXT("then"));Invoke(RigBP,G,X,TEXT("CenterLookCursor"));
 Tail(RigBP,Graph(RigBP,TEXT("FinishViewTransition")),TEXT("RecenterFocusedCursor"));
 for(auto N:{TEXT("Center"),TEXT("Long"),TEXT("Short"),TEXT("Radio"),TEXT("Deck"),TEXT("MainMenu"),TEXT("DeckPanel")})Hide(ControlsBP,N);
 // Focus functions live on the rig, not on the controls widget.
 for(auto FG:RigBP->FunctionGraphs) for(auto N:FG->Nodes) if(auto C=Cast<UK2Node_CallFunction>(N)) if(C->GetFunctionName()==TEXT("SetVisibility")) {
  auto P=Pin(C,TEXT("self"));if(P->LinkedTo.Num())if(auto Vn=Cast<UK2Node_VariableGet>(P->LinkedTo[0]->GetOwningNode()))if(Vn->GetVarName()==TEXT("DeckPanel"))Def(C,TEXT("InVisibility"),TEXT("Collapsed"));
 }
 Hide(MenuBP,TEXT("HUDMenu"));
 for(auto FG:MenuManager->FunctionGraphs) for(auto N:FG->Nodes) if(auto C=Cast<UK2Node_CallFunction>(N)) if(C->GetFunctionName()==TEXT("SetVisibility")) {
  auto P=Pin(C,TEXT("self"));if(P->LinkedTo.Num())if(auto Vn=Cast<UK2Node_VariableGet>(P->LinkedTo[0]->GetOwningNode()))if(Vn->GetVarName()==TEXT("HUDMenu"))Def(C,TEXT("InVisibility"),TEXT("Collapsed"));
 }
 G=Reset(MenuManager,TEXT("ShowMenuCursor"),X);
 Put(G,X,APlayerController::StaticClass(),TEXT("bShowMouseCursor"),PC(G),nullptr,TEXT("true"));
 auto Mode=Flow(G,X,UWidgetBlueprintLibrary::StaticClass(),TEXT("SetInputMode_GameAndUIEx"));Link(PC(G),Pin(Mode,TEXT("PlayerController")));Def(Mode,TEXT("bHideCursorDuringCapture"),TEXT("false"));
 Recenter(G,X);
}
void RadioScreens() {
 auto Canvas=CastChecked<UCanvasPanel>(DisplayBP->WidgetTree->FindWidget(TEXT("Waveform"))->GetParent());
 for(auto Pair:{TPair<const TCHAR*,const TCHAR*>(TEXT("Waveform"),TEXT("OffWaveform")),{TEXT("FrequencyArtwork_PNG"),TEXT("OffFrequency")},{TEXT("LevelArtwork_PNG"),TEXT("OffLevel")}}) {
  auto Old=CastChecked<UCanvasPanelSlot>(DisplayBP->WidgetTree->FindWidget(Pair.Key)->Slot);
  auto W=Cast<UImage>(DisplayBP->WidgetTree->FindWidget(Pair.Value));if(!W)W=DisplayBP->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),Pair.Value);
  W->bIsVariable=true;W->SetColorAndOpacity(FLinearColor::Black);W->SetVisibility(ESlateVisibility::HitTestInvisible);
  auto Slot=Cast<UCanvasPanelSlot>(W->Slot);if(!Slot)Slot=Canvas->AddChildToCanvas(W);
  Slot->SetLayout(Old->GetLayout());Slot->SetZOrder(100);
 }
 Compile(DisplayBP);Declare(ReceiverBP,{TEXT("UpdateScreenPower")});UEdGraphPin* X;auto G=Reset(ReceiverBP,TEXT("UpdateScreenPower"),X);
 auto Widget=Flow(G,X,UWidgetComponent::StaticClass(),TEXT("GetUserWidgetObject"),V(G,TEXT("RadioScreen")));
 auto W=MenuBuild::Cast(G,DisplayBP->GeneratedClass,Pin(Widget,TEXT("ReturnValue")))->GetCastResultPin();
 auto Good=Branch(G,X,Valid(G,W));auto On=Branch(G,Pin(Good,TEXT("then")),V(G,TEXT("Powered")));
 for(auto Side:{TPair<UEdGraphPin*,bool>(Pin(On,TEXT("then")),true),{Pin(On,TEXT("else")),false}}) {
  auto E=Side.Key;for(auto Name:{TEXT("OffWaveform"),TEXT("OffFrequency"),TEXT("OffLevel")}) {
   auto C=Flow(G,E,UWidget::StaticClass(),TEXT("SetVisibility"),Ext(G,DisplayBP->GeneratedClass,Name,W));Def(C,TEXT("InVisibility"),Side.Value?TEXT("Collapsed"):TEXT("HitTestInvisible"));
  }
  if(!Side.Value)for(auto Name:{TEXT("ScreenPhosphorLight"),TEXT("FrequencyScreenLight"),TEXT("LevelScreenLight")}) {
   auto C=Flow(G,E,ULightComponent::StaticClass(),TEXT("SetIntensity"),V(G,Name));Def(C,TEXT("NewIntensity"),TEXT("0"));
  }
 }
 // Last operation after light animation, also immediate on a power click.
 Tail(ReceiverBP,Graph(ReceiverBP,TEXT("UpdateRadioAnimation")),TEXT("UpdateScreenPower"));
 Tail(ReceiverBP,Graph(ReceiverBP,TEXT("UpdatePresentation")),TEXT("UpdateScreenPower"));
 auto M=LoadObject<UMaterial>(nullptr,TEXT("/Game/RadioExample/M_ReceiverDisplay.M_ReceiverDisplay"));
 for(auto E:M->Expressions)if(auto C=Cast<UMaterialExpressionCustom>(E)) C->Code=C->Code.Replace(TEXT("return bg+result*saturate(Power);"),TEXT("return (bg+result)*saturate(Power);"));
 M->PostEditChange();Save(M);
 // The profile loader used to silently cap the wider five-kHz lock to four.
 for(auto N:Graph(ReceiverBP,TEXT("ScanProfile"))->Nodes)if(auto C=Cast<UK2Node_CallFunction>(N))if(C->GetFunctionName()==TEXT("FClamp"))if(Pin(C,TEXT("Max"))->DefaultValue==TEXT("4"))Def(C,TEXT("Max"),TEXT("18"));
}
void Sidebar() {
 auto M=LoadObject<UMaterial>(nullptr,TEXT("/Game/UI/Media/M_MenuSidebar.M_MenuSidebar"));
 if(!M){M=NewObject<UMaterial>(CreatePackage(TEXT("/Game/UI/Media/M_MenuSidebar")),TEXT("M_MenuSidebar"),RF_Public|RF_Standalone);FAssetRegistryModule::AssetCreated(M);}
 M->MaterialDomain=MD_UI;M->BlendMode=BLEND_Translucent;M->Expressions.Empty();
 auto UV=NewObject<UMaterialExpressionTextureCoordinate>(M);M->Expressions.Add(UV);
 auto Shader=NewObject<UMaterialExpressionCustom>(M);Shader->OutputType=CMOT_Float1;FCustomInput I;I.InputName=TEXT("UV");I.Input.Expression=UV;Shader->Inputs.Add(I);
 Shader->Code=TEXT("return lerp(.99,.64,smoothstep(0.0,1.0,UV.x));");M->Expressions.Add(Shader);
 auto Black=NewObject<UMaterialExpressionConstant3Vector>(M);Black->Constant=FLinearColor::Black;M->Expressions.Add(Black);M->EmissiveColor.Expression=Black;M->Opacity.Expression=Shader;M->PostEditChange();Save(M);
 MenuBuild::Menu=MenuBP;MenuBuild::Font=LoadObject<UFont>(nullptr,TEXT("/Game/UI/Fonts/IBM_Plex_Mono.IBM_Plex_Mono"));
 int Index=0;for(auto Name:{TEXT("Panel_0"),TEXT("Panel_1")}){
  auto W=CastChecked<UImage>(MenuBP->WidgetTree->FindWidget(Name));W->SetBrushFromMaterial(M);W->SetColorAndOpacity(FLinearColor::White);
  auto TitleName=Index++==0?TEXT("MainTitle"):TEXT("PauseTitle");
  if(!MenuBP->WidgetTree->FindWidget(TitleName))MenuBuild::Label(CastChecked<UCanvasPanel>(W->GetParent()),TitleName,TEXT("DEAD AIR"),90,100,540,100,64,true);
 }
}
void FadesAndCaptions() {
 Variable(MenuManager,TEXT("RevealStarted"),Type(UEdGraphSchema_K2::PC_Boolean),TEXT("false"));
 Variable(MenuManager,TEXT("FadeState"),Type(UEdGraphSchema_K2::PC_Int),TEXT("0"));
 Variable(MenuManager,TEXT("FadeStartedAt"),StationBuild::Real(),TEXT("0"));
 Variable(MenuManager,TEXT("FadeAlpha"),StationBuild::Real(),TEXT("0"));
 Declare(MenuManager,{TEXT("BeginDeskReveal"),TEXT("UpdateScreenFade"),TEXT("ResetMainPresentation")});
 auto Root=CastChecked<UCanvasPanel>(MenuBP->WidgetTree->FindWidget(TEXT("SubtitleText"))->GetParent());
 auto Fade=Cast<UImage>(MenuBP->WidgetTree->FindWidget(TEXT("ScreenFade")));if(!Fade){Fade=MenuBP->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("ScreenFade"));auto S=Root->AddChildToCanvas(Fade);S->SetPosition(FVector2D::ZeroVector);S->SetSize(FVector2D(1920,1080));S->SetZOrder(1000);}
 Fade->bIsVariable=true;Fade->SetColorAndOpacity(FLinearColor::Black);Fade->SetRenderOpacity(0);Fade->SetVisibility(ESlateVisibility::HitTestInvisible);Compile(MenuBP);
 UEdGraphPin* X;auto G=Reset(MenuManager,TEXT("BeginDeskReveal"),X);auto Fresh=Branch(G,X,Op(G,TEXT("Not_PreBool"),V(G,TEXT("RevealStarted"))));X=Pin(Fresh,TEXT("then"));
 Assign(G,X,TEXT("RevealStarted"),nullptr,TEXT("true"));Assign(G,X,TEXT("FadeState"),nullptr,TEXT("1"));Assign(G,X,TEXT("FadeStartedAt"),RealTime(G));Assign(G,X,TEXT("FadeAlpha"),nullptr,TEXT("1"));
 auto Opacity=Flow(G,X,UWidget::StaticClass(),TEXT("SetRenderOpacity"),MenuWidget(G,TEXT("ScreenFade")));Def(Opacity,TEXT("InOpacity"),TEXT("1"));ManualFade(G,X,TEXT("0"));
 Tail(MenuManager,Graph(MenuManager,TEXT("ResumeGame")),TEXT("BeginDeskReveal"));
 G=Reset(MenuManager,TEXT("ResetMainPresentation"),X);Assign(G,X,TEXT("InGame"),nullptr,TEXT("false"));Assign(G,X,TEXT("FadeState"),nullptr,TEXT("0"));Assign(G,X,TEXT("FadeAlpha"),nullptr,TEXT("0"));MenuVisibility(G,X,TEXT("SubtitleText"),TEXT("Collapsed"));
 Opacity=Flow(G,X,UWidget::StaticClass(),TEXT("SetRenderOpacity"),MenuWidget(G,TEXT("ScreenFade")));Def(Opacity,TEXT("InOpacity"),TEXT("0"));ManualFade(G,X,TEXT("0"));Once(MenuManager,Graph(MenuManager,TEXT("ShowMain")),TEXT("ResetMainPresentation"));
 // Separate display visibility from the bound text: stale text cannot survive a map transition.
 Declare(MenuManager,{TEXT("HideCaptions"),TEXT("ShowCaptions")});G=Reset(MenuManager,TEXT("HideCaptions"),X);MenuVisibility(G,X,TEXT("SubtitleText"),TEXT("Collapsed"));Once(MenuManager,Graph(MenuManager,TEXT("OpenPause")),TEXT("HideCaptions"));
 G=Reset(MenuManager,TEXT("ShowCaptions"),X);MenuVisibility(G,X,TEXT("SubtitleText"),TEXT("HitTestInvisible"));Once(MenuManager,Graph(MenuManager,TEXT("ResumeGame")),TEXT("ShowCaptions"));
 G=Reset(MenuManager,TEXT("ReturnMainMenu"),X);auto Free=Branch(G,X,Op(G,TEXT("Not_PreBool"),V(G,TEXT("Busy"))));X=Pin(Free,TEXT("then"));Invoke(MenuManager,G,X,TEXT("SaveAuto"));auto Saved=Branch(G,X,V(G,TEXT("LastSaveSucceeded")));X=Pin(Saved,TEXT("then"));
 Assign(G,X,TEXT("Busy"),nullptr,TEXT("true"));Assign(G,X,TEXT("FadeState"),nullptr,TEXT("2"));Assign(G,X,TEXT("FadeStartedAt"),RealTime(G));Assign(G,X,TEXT("FadeAlpha"),nullptr,TEXT("0"));MenuVisibility(G,X,TEXT("SubtitleText"),TEXT("Collapsed"));
 auto Disable=Flow(G,X,UWidget::StaticClass(),TEXT("SetIsEnabled"),MenuWidget(G,TEXT("MenuLayer")));Def(Disable,TEXT("bInIsEnabled"),TEXT("false"));
 Put(G,X,APlayerController::StaticClass(),TEXT("bEnableClickEvents"),PC(G),nullptr,TEXT("false"));
 G=Reset(MenuManager,TEXT("UpdateScreenFade"),X);auto Good=Branch(G,X,Valid(G,V(G,TEXT("View"))));auto Active=Branch(G,Pin(Good,TEXT("then")),Op(G,TEXT("Greater_IntInt"),V(G,TEXT("FadeState")),nullptr,TEXT("0")));X=Pin(Active,TEXT("then"));
 auto Progress=Clamp(G,Op(G,TEXT("Divide_DoubleDouble"),Op(G,TEXT("Subtract_DoubleDouble"),RealTime(G),V(G,TEXT("FadeStartedAt"))),nullptr,TEXT("2")),TEXT("0"),TEXT("1"));
 auto Enter=Op(G,TEXT("EqualEqual_IntInt"),V(G,TEXT("FadeState")),nullptr,TEXT("1"));
 Assign(G,X,TEXT("FadeAlpha"),Select(G,Enter,Op(G,TEXT("Subtract_DoubleDouble"),Scalar(G,TEXT("1")),Progress)));
 // SelectFloat's B needs a connected pin for the outgoing fade.
 for(auto N:G->Nodes)if(auto C=Cast<UK2Node_CallFunction>(N))if(C->GetFunctionName()==TEXT("SelectFloat"))Link(Progress,Pin(C,TEXT("B")));
 Opacity=Flow(G,X,UWidget::StaticClass(),TEXT("SetRenderOpacity"),MenuWidget(G,TEXT("ScreenFade")));Link(V(G,TEXT("FadeAlpha")),Pin(Opacity,TEXT("InOpacity")));
 auto Done=Branch(G,X,Op(G,TEXT("GreaterEqual_DoubleDouble"),Progress,nullptr,TEXT("1")));auto Direction=Branch(G,Pin(Done,TEXT("then")),Enter);X=Pin(Direction,TEXT("then"));Assign(G,X,TEXT("FadeState"),nullptr,TEXT("0"));X=Pin(Direction,TEXT("else"));
 auto Open=Flow(G,X,UGameplayStatics::StaticClass(),TEXT("OpenLevel"));Def(Open,TEXT("LevelName"),TEXT("MainMenuMap"));
 auto Defaults=CastChecked<AActor>(MenuManager->GeneratedClass->GetDefaultObject());Defaults->PrimaryActorTick.bCanEverTick=true;Defaults->PrimaryActorTick.bStartWithTickEnabled=true;Defaults->PrimaryActorTick.bTickEvenWhenPaused=true;
 auto EG=Events(MenuManager);UK2Node_Event* Tick=nullptr;for(auto N:EG->Nodes)if(auto E=Cast<UK2Node_Event>(N))if(E->EventReference.GetMemberName()==TEXT("ReceiveTick"))Tick=E;
 if(!Tick)Tick=Event(EG,AActor::StaticClass(),TEXT("ReceiveTick"));Tick->SetEnabledState(ENodeEnabledState::Enabled,true);X=Pin(Tick,TEXT("then"));if(!X->LinkedTo.Num())Invoke(MenuManager,EG,X,TEXT("UpdateScreenFade"));
 // Explicit empty returns and a menu/fade guard, independent from the widget's visibility.
 G=Graph(MenuBP,TEXT("ReadSubtitle"));auto Out=Pin(Entry(G),TEXT("then"));auto Next=Out->LinkedTo;Out->BreakAllPinLinks();X=Out;
 auto Manager=V(G,TEXT("ManagerReference"));auto Guard=Branch(G,X,Valid(G,Manager));
 auto Room=Branch(G,Pin(Guard,TEXT("then")),Ext(G,MenuManager->GeneratedClass,TEXT("InGame"),Manager));
 auto NoFade=Branch(G,Pin(Room,TEXT("then")),Op(G,TEXT("EqualEqual_IntInt"),Ext(G,MenuManager->GeneratedClass,TEXT("FadeState"),Manager),nullptr,TEXT("0")));
 auto Visibility=Call(G,UWidget::StaticClass(),TEXT("GetVisibility"));Link(Ext(G,MenuBP->GeneratedClass,TEXT("MenuLayer"),Self(G)),Pin(Visibility,TEXT("self")));
 auto Closed=Call(G,UKismetMathLibrary::StaticClass(),TEXT("EqualEqual_ByteByte"));Link(Pin(Visibility,TEXT("ReturnValue")),Pin(Closed,TEXT("A")));Def(Closed,TEXT("B"),TEXT("1"));
 auto Gameplay=Branch(G,Pin(NoFade,TEXT("then")),Pin(Closed,TEXT("ReturnValue")));
 for(auto E:{Pin(Guard,TEXT("else")),Pin(Room,TEXT("else")),Pin(NoFade,TEXT("else")),Pin(Gameplay,TEXT("else"))})Result(G,E,TEXT("ReadSubtitle"),Type(UEdGraphSchema_K2::PC_Text),Text(G,String(G,TEXT(""))));
 for(auto P:Next)Link(Pin(Gameplay,TEXT("then")),P);
 auto ResultNodes=G->Nodes;for(auto N:ResultNodes)if(auto R=Cast<UK2Node_FunctionResult>(N))if(auto P=R->FindPin(TEXT("ReturnValue")))if(!P->LinkedTo.Num())Link(Text(G,String(G,TEXT(""))),P);
}
void Build() {
 TransportUpgrade::Load();CamerasAndInput();RadioScreens();Sidebar();FadesAndCaptions();
 // Empty fullscreen UMG containers must not supply a Slate cursor above the hidden game cursor.
 for(auto WB:{MenuBP,ControlsBP})for(UWidget* W=WB->WidgetTree->RootWidget;W;){
  W->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
  auto Container=Cast<UPanelWidget>(W);W=Container&&Container->GetChildrenCount()==1?Container->GetChildAt(0):nullptr;
 }
 // Missing optional reverse data must never play the original clip while the tape counter runs backwards.
 auto Rewind=Graph(DeckBP,TEXT("DeckRewind"));auto Out=Pin(Entry(Rewind),TEXT("then"));auto Next=Out->LinkedTo;Out->BreakAllPinLinks();
 auto Guard=Branch(Rewind,Out,Valid(Rewind,V(Rewind,TEXT("ReverseSound"))));for(auto P:Next)Link(Pin(Guard,TEXT("then")),P);
 auto Missing=Pin(Guard,TEXT("else"));StationBuild::Status(Rewind,Missing,TEXT("Для этой кассеты не подготовлена обратная дорожка"));
 for(auto B:{ReceiverBP,static_cast<UBlueprint*>(DisplayBP),RigBP,ChildRadio,DeckBP,static_cast<UBlueprint*>(ControlsBP),MenuManager,static_cast<UBlueprint*>(MenuBP)}){Compile(B);Layout(B);Save(B);}
 ReceiverUpgrade::Scene();UE_LOG(LogTemp,Display,TEXT("PRESENTATION UPGRADE COMPLETE"));
}
}
