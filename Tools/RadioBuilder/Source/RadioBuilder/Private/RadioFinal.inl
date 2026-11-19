// Editor-only authoring. The generated assets depend exclusively on stock UE classes.
namespace FinalRadio {
using namespace RadioBuild;
UEdGraphPin* V(UEdGraph* G,const TCHAR* N) { return Get(G,N)->FindPin(N); }
UEdGraphPin* Op(UEdGraph* G,const TCHAR* N,UEdGraphPin* A,UEdGraphPin* B=nullptr,const TCHAR* Number=nullptr) { auto C=Call(G,UKismetMathLibrary::StaticClass(),N); Link(A,Pin(C,TEXT("A"))); if(B)Link(B,Pin(C,TEXT("B"))); else if(Number)Def(C,TEXT("B"),Number); return Pin(C,TEXT("ReturnValue")); }
UEdGraphPin* Clamp(UEdGraph* G,UEdGraphPin* Value,const TCHAR* Min,const TCHAR* Max) { auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("FClamp")); Link(Value,Pin(C,TEXT("Value"))); Def(C,TEXT("Min"),Min); Def(C,TEXT("Max"),Max); return Pin(C,TEXT("ReturnValue")); }
UEdGraphPin* PC(UEdGraph* G) { return Pin(Call(G,UGameplayStatics::StaticClass(),TEXT("GetPlayerController")),TEXT("ReturnValue")); }
UEdGraphPin* Lock(UEdGraph* G) { auto Distance=Op(G,TEXT("Subtract_DoubleDouble"),V(G,TEXT("FineFrequencyKHz")),V(G,TEXT("StationFrequencyKHz"))); auto Abs=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Abs")); Link(Distance,Pin(Abs,TEXT("A"))); auto Near=Op(G,TEXT("LessEqual_DoubleDouble"),Pin(Abs,TEXT("ReturnValue")),V(G,TEXT("LockWidthKHz"))); return Op(G,TEXT("BooleanAND"),Near,V(G,TEXT("Powered"))); }
UK2Node_CallFunction* Invoke(UEdGraph* G,UEdGraphPin*& Exec,const TCHAR* Name) { auto C=Call(G,Radio->SkeletonGeneratedClass,Name); Link(Exec,Pin(C,TEXT("execute"))); Exec=Pin(C,TEXT("then")); return C; }
void Assign(UEdGraph* G,UEdGraphPin*& Exec,const TCHAR* Name,UEdGraphPin* Value=nullptr,const TCHAR* Default=nullptr) { auto S=Set(G,Name); if(Value)Link(Value,Pin(S,Name)); if(Default)Def(S,Name,Default); Link(Exec,Pin(S,TEXT("execute"))); Exec=Pin(S,TEXT("then")); }
UMaterial* WaveMaterial() {
 auto P=CreatePackage(TEXT("/Game/RadioExample/M_RadioWaveform")); auto M=NewObject<UMaterial>(P,TEXT("M_RadioWaveform"),RF_Public|RF_Standalone); M->MaterialDomain=MD_UI; M->BlendMode=BLEND_Translucent;
 auto Constant=[&](float F){auto N=NewObject<UMaterialExpressionConstant>(M);N->R=F;M->Expressions.Add(N);return N;};
 auto Add=[&](UMaterialExpression* A,UMaterialExpression* B){auto N=NewObject<UMaterialExpressionAdd>(M);N->A.Expression=A;N->B.Expression=B;M->Expressions.Add(N);return N;};
 auto Mul=[&](UMaterialExpression* A,UMaterialExpression* B){auto N=NewObject<UMaterialExpressionMultiply>(M);N->A.Expression=A;N->B.Expression=B;M->Expressions.Add(N);return N;};
 auto Sin=[&](UMaterialExpression* A){auto N=NewObject<UMaterialExpressionSine>(M);N->Input.Expression=A;N->Period=1;M->Expressions.Add(N);return N;};
 auto Parameter=[&](FName Name,float Default){auto N=NewObject<UMaterialExpressionScalarParameter>(M);N->ParameterName=Name;N->DefaultValue=Default;M->Expressions.Add(N);return N;};
 auto UV=NewObject<UMaterialExpressionTextureCoordinate>(M);M->Expressions.Add(UV);
 auto X=NewObject<UMaterialExpressionComponentMask>(M);X->Input.Expression=UV;X->R=true;M->Expressions.Add(X);
 auto Y=NewObject<UMaterialExpressionComponentMask>(M);Y->Input.Expression=UV;Y->G=true;M->Expressions.Add(Y);
 auto T=NewObject<UMaterialExpressionTime>(M);M->Expressions.Add(T);
 auto Rate=Add(Constant(3),Mul(Parameter(TEXT("Tuning"),0),Constant(14)));
 auto Phase=Mul(T,Constant(.8f));
 auto Fundamental=Mul(Sin(Add(Mul(X,Rate),Phase)),Constant(.30f));
 // A single smooth trace. Filter changes its height, never adds a second wave.
 auto Center=Add(Constant(.5),Mul(Fundamental,Add(Constant(.55f),Mul(Parameter(TEXT("Filter"),0),Constant(.45f)))));
 auto Sub=NewObject<UMaterialExpressionSubtract>(M);Sub->A.Expression=Y;Sub->B.Expression=Center;M->Expressions.Add(Sub);
 auto Abs=NewObject<UMaterialExpressionAbs>(M);Abs->Input.Expression=Sub;M->Expressions.Add(Abs);
 auto Saturate=NewObject<UMaterialExpressionClamp>(M);Saturate->Input.Expression=Mul(Abs,Constant(85));Saturate->MinDefault=0;Saturate->MaxDefault=1;M->Expressions.Add(Saturate);
 auto Line=NewObject<UMaterialExpressionOneMinus>(M);Line->Input.Expression=Saturate;M->Expressions.Add(Line);
 auto Green=NewObject<UMaterialExpressionConstant3Vector>(M);Green->Constant=FLinearColor(.12f,.9f,.42f);M->Expressions.Add(Green);
 auto Background=NewObject<UMaterialExpressionConstant3Vector>(M);Background->Constant=FLinearColor(.001f,.008f,.004f);M->Expressions.Add(Background);
 M->EmissiveColor.Expression=Add(Background,Mul(Mul(Line,Parameter(TEXT("Power"),0)),Green)); M->Opacity.Expression=Constant(1);
 FAssetRegistryModule::AssetCreated(M);M->PostEditChange();Save(M);return M;
}
void Build() {
 UE_LOG(LogTemp,Display,TEXT("FINAL RADIO START"));
 auto SourceWorld=GEditor->GetEditorWorldContext().World();AActor* Source=nullptr;
 for(TActorIterator<AActor> It(SourceWorld);It;++It)if(It->GetClass()->GetName().Contains(TEXT("Radio_1")))Source=*It;check(Source);
 TMap<FString,FTransform> Layout;for(TActorIterator<AActor> It(SourceWorld);It;++It){TArray<UStaticMeshComponent*> Cs;It->GetComponents(Cs);for(auto C:Cs)if(C->GetStaticMesh()&&C->GetStaticMesh()->GetPathName().Contains(TEXT("/Art/Radio/")))Layout.Add(C->GetStaticMesh()->GetName(),C->GetComponentTransform().GetRelativeTransform(Source->GetActorTransform()));}
 auto P=CreatePackage(TEXT("/Game/RadioExample/BP_RadioExample"));Radio=FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(),P,TEXT("BP_RadioExample"),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());FAssetRegistryModule::AssetCreated(Radio);
 auto Float=Type(UEdGraphSchema_K2::PC_Real);Float.PinSubCategory=UEdGraphSchema_K2::PC_Double;
 for(auto Pair:{TPair<FName,FString>(TEXT("FineFrequencyKHz"),TEXT("153400")),{TEXT("StationFrequencyKHz"),TEXT("153450")},{TEXT("LockWidthKHz"),TEXT("1.2")},{TEXT("Volume"),TEXT("0.5")},{TEXT("Filter"),TEXT("0")},{TEXT("DragCenterX"),TEXT("0")},{TEXT("DragCenterY"),TEXT("0")},{TEXT("MouseAngle"),TEXT("0")},{TEXT("LastMouseAngle"),TEXT("0")},{TEXT("DragDelta"),TEXT("0")},{TEXT("FrequencySensitivity"),TEXT("0.333333333333")},{TEXT("LevelSensitivity"),TEXT("0.003703703704")}})Var(Radio,Pair.Key,Float,Pair.Value);
 Var(Radio,TEXT("FrequencyKHz"),Type(UEdGraphSchema_K2::PC_Int),TEXT("153400"));Var(Radio,TEXT("DragMode"),Type(UEdGraphSchema_K2::PC_Int),TEXT("0"));
 Var(Radio,TEXT("LastAngleValid"),Type(UEdGraphSchema_K2::PC_Boolean),TEXT("false"));
 Var(Radio,TEXT("Powered"),Type(UEdGraphSchema_K2::PC_Boolean),TEXT("false"));Var(Radio,TEXT("Receiving"),Type(UEdGraphSchema_K2::PC_Boolean),TEXT("false"));
 for(auto Name:{TEXT("SignalClip"),TEXT("NoiseClip")})Var(Radio,Name,Type(UEdGraphSchema_K2::PC_Object,USoundBase::StaticClass()),TEXT("None"));
 auto Root=Component(TEXT("RadioRoot"),USceneComponent::StaticClass());
 for(auto Part:{TEXT("Body"),TEXT("Frequency"),TEXT("Gain"),TEXT("Power"),TEXT("Volume")}){
  auto Name=FString(TEXT("SM_Radio_"))+Part;check(Layout.Contains(Name));auto N=Radio->SimpleConstructionScript->CreateNode(UStaticMeshComponent::StaticClass(),FName(*FString::Printf(TEXT("Radio_%s"),Part)));Root->AddChildNode(N);auto C=CastChecked<UStaticMeshComponent>(N->ComponentTemplate);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Art/Radio/%s.%s"),*Name,*Name)));C->SetRelativeTransform(Layout[Name]);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if(FString(Part)!=TEXT("Body")){auto B=Radio->SimpleConstructionScript->CreateNode(UBoxComponent::StaticClass(),FName(*FString::Printf(TEXT("Click_%s"),Part)));N->AddChildNode(B);auto Box=CastChecked<UBoxComponent>(B->ComponentTemplate);Box->SetBoxExtent(FVector(1.4));Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->SetGenerateOverlapEvents(false);}
 }
 for(auto Name:{TEXT("ReceiverAudio"),TEXT("InterferenceAudio")}){auto Sound=Radio->SimpleConstructionScript->CreateNode(UAudioComponent::StaticClass(),Name);Root->AddChildNode(Sound);CastChecked<UAudioComponent>(Sound->ComponentTemplate)->bAutoActivate=false;}
 auto CameraNode=Radio->SimpleConstructionScript->CreateNode(UCameraComponent::StaticClass(),TEXT("ExampleCamera"));Root->AddChildNode(CameraNode);auto Camera=CastChecked<UCameraComponent>(CameraNode->ComponentTemplate);Camera->SetRelativeLocation(FVector(0,170,84));Camera->SetRelativeRotation((FVector(0,32,56)-Camera->GetRelativeLocation()).Rotation());Camera->FieldOfView=40;
 auto ScreenNode=Radio->SimpleConstructionScript->CreateNode(UWidgetComponent::StaticClass(),TEXT("RadioScreen"));Root->AddChildNode(ScreenNode);auto Screen=CastChecked<UWidgetComponent>(ScreenNode->ComponentTemplate);Screen->SetDrawSize(FVector2D(1040,960));Screen->SetRelativeLocation(FVector(-16,50.08,56));Screen->SetRelativeRotation(FRotator(0,90,0));Screen->SetRelativeScale3D(FVector(.025));Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);Screen->SetCastShadow(false);Screen->SetTwoSided(false);
 Compile(Radio);
 UK2Node_FunctionEntry* E;UEdGraphPin* Exec;
 // Voice and interference are separate: adjusting the filter never restarts a sentence.
 auto Audio=Fun(Radio,TEXT("UpdateAudioLevel"),E);Exec=Pin(E,TEXT("then"));
 for(auto Name:{TEXT("ReceiverAudio"),TEXT("InterferenceAudio")}){
  UEdGraphPin* Level=V(Audio,TEXT("Volume"));
  if(FString(Name)==TEXT("InterferenceAudio")) {
   auto Remaining=Call(Audio,UKismetMathLibrary::StaticClass(),TEXT("Subtract_DoubleDouble"));Def(Remaining,TEXT("A"),TEXT("0.6"));Link(Op(Audio,TEXT("Multiply_DoubleDouble"),V(Audio,TEXT("Filter")),nullptr,TEXT("0.57")),Pin(Remaining,TEXT("B")));
   Level=Op(Audio,TEXT("Multiply_DoubleDouble"),Level,Pin(Remaining,TEXT("ReturnValue")));
  }
  auto SetVolume=Call(Audio,UAudioComponent::StaticClass(),TEXT("SetVolumeMultiplier"));Link(V(Audio,Name),Pin(SetVolume,TEXT("self")));Link(Level,Pin(SetVolume,TEXT("NewVolumeMultiplier")));Link(Exec,Pin(SetVolume,TEXT("execute")));Exec=Pin(SetVolume,TEXT("then"));
 }
 auto Refresh=Fun(Radio,TEXT("RefreshAudio"),E);Exec=Pin(E,TEXT("then"));
 for(auto Name:{TEXT("ReceiverAudio"),TEXT("InterferenceAudio")}){auto Stop=Call(Refresh,UAudioComponent::StaticClass(),TEXT("Stop"));Link(V(Refresh,Name),Pin(Stop,TEXT("self")));Link(Exec,Pin(Stop,TEXT("execute")));Exec=Pin(Stop,TEXT("then"));}
 Assign(Refresh,Exec,TEXT("Receiving"),Lock(Refresh));
 auto Branch=Node<UK2Node_IfThenElse>(Refresh);Branch->AllocateDefaultPins();Link(V(Refresh,TEXT("Powered")),Pin(Branch,TEXT("Condition")));Link(Exec,Pin(Branch,TEXT("execute")));Exec=Pin(Branch,TEXT("then"));
 for(auto Name:{TEXT("ReceiverAudio"),TEXT("InterferenceAudio")}){auto SetSound=Call(Refresh,UAudioComponent::StaticClass(),TEXT("SetSound"));Link(V(Refresh,Name),Pin(SetSound,TEXT("self")));Link(V(Refresh,FString(Name)==TEXT("ReceiverAudio")?TEXT("SignalClip"):TEXT("NoiseClip")),Pin(SetSound,TEXT("NewSound")));Link(Exec,Pin(SetSound,TEXT("execute")));Exec=Pin(SetSound,TEXT("then"));}
 Invoke(Refresh,Exec,TEXT("UpdateAudioLevel"));
 auto NoisePlay=Call(Refresh,UAudioComponent::StaticClass(),TEXT("Play"));Link(V(Refresh,TEXT("InterferenceAudio")),Pin(NoisePlay,TEXT("self")));Link(Exec,Pin(NoisePlay,TEXT("execute")));Exec=Pin(NoisePlay,TEXT("then"));
 auto VoiceBranch=Node<UK2Node_IfThenElse>(Refresh);VoiceBranch->AllocateDefaultPins();Link(V(Refresh,TEXT("Receiving")),Pin(VoiceBranch,TEXT("Condition")));Link(Exec,Pin(VoiceBranch,TEXT("execute")));
 auto VoicePlay=Call(Refresh,UAudioComponent::StaticClass(),TEXT("Play"));Link(V(Refresh,TEXT("ReceiverAudio")),Pin(VoicePlay,TEXT("self")));Link(Pin(VoiceBranch,TEXT("then")),Pin(VoicePlay,TEXT("execute")));Compile(Radio);
 auto Reception=Fun(Radio,TEXT("RefreshReception"),E);auto Changed=Op(Reception,TEXT("NotEqual_BoolBool"),Lock(Reception),V(Reception,TEXT("Receiving")));auto B=Node<UK2Node_IfThenElse>(Reception);B->AllocateDefaultPins();Link(Pin(E,TEXT("then")),Pin(B,TEXT("execute")));Link(Changed,Pin(B,TEXT("Condition")));Exec=Pin(B,TEXT("then"));Invoke(Reception,Exec,TEXT("RefreshAudio"));Exec=Pin(B,TEXT("else"));Invoke(Reception,Exec,TEXT("UpdateAudioLevel"));Compile(Radio);
 // One embedded face, with transparent space between the actual instrument windows.
 auto W=CreatePackage(TEXT("/Game/RadioExample/WBP_RadioDisplay"));Panel=CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(),W,TEXT("WBP_RadioDisplay"),BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));FAssetRegistryModule::AssetCreated(Panel);Var(Panel,TEXT("RadioReference"),Type(UEdGraphSchema_K2::PC_Object,Radio->GeneratedClass),TEXT("None"));auto Canvas=Panel->WidgetTree->ConstructWidget<UCanvasPanel>();Panel->WidgetTree->RootWidget=Canvas;
 auto Image=[&](const TCHAR* Name,float X,float Y,float Width,float Height){auto I=Panel->WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),Name);I->bIsVariable=true;I->SetColorAndOpacity(FLinearColor(.002,.012,.007,1));auto S=Canvas->AddChildToCanvas(I);S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(Width,Height));return I;};
 auto Wave=Image(TEXT("Waveform"),20,40,1000,440);Wave->SetBrushFromMaterial(WaveMaterial());Wave->SetColorAndOpacity(FLinearColor::White);
 Image(TEXT("FrequencyArtwork_PNG"),30,700,470,220);Image(TEXT("LevelArtwork_PNG"),550,700,460,220);
 auto Freq=Label(Canvas,TEXT("FrequencyReadout"),TEXT("153,400"),50,730,440,95,66);Label(Canvas,TEXT("FrequencyUnit"),TEXT("МГц"),60,830,400,60,32);
 auto VolumeLabel=Label(Canvas,TEXT("VolumeReadout"),TEXT("50 %"),575,710,400,65,44);auto FilterLabel=Label(Canvas,TEXT("FilterReadout"),TEXT("ФИЛЬТР 0 %"),575,778,400,50,29);auto Status=Label(Canvas,TEXT("SignalStatus"),TEXT("ВЫКЛ"),575,840,400,55,30);
 TextReturn(Panel,TEXT("ReadFrequency"),[](UEdGraph* G){auto D=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Divide_IntInt"));Link(Read(G,TEXT("FrequencyKHz")),Pin(D,TEXT("A")));Def(D,TEXT("B"),TEXT("1000"));auto Mod=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Percent_IntInt"));Link(Read(G,TEXT("FrequencyKHz")),Pin(Mod,TEXT("A")));Def(Mod,TEXT("B"),TEXT("1000"));auto A=Call(G,UKismetStringLibrary::StaticClass(),TEXT("Conv_IntToString"));Link(Pin(D,TEXT("ReturnValue")),Pin(A,TEXT("InInt")));auto F=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_IntToText"));Link(Pin(Mod,TEXT("ReturnValue")),Pin(F,TEXT("Value")));Def(F,TEXT("MinimumIntegralDigits"),TEXT("3"));Def(F,TEXT("bUseGrouping"),TEXT("false"));auto S=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_TextToString"));Link(Pin(F,TEXT("ReturnValue")),Pin(S,TEXT("InText")));auto Join=Call(G,UKismetStringLibrary::StaticClass(),TEXT("Concat_StrStr"));Link(Pin(A,TEXT("ReturnValue")),Pin(Join,TEXT("A")));Def(Join,TEXT("B"),TEXT(","));auto Last=Call(G,UKismetStringLibrary::StaticClass(),TEXT("Concat_StrStr"));Link(Pin(Join,TEXT("ReturnValue")),Pin(Last,TEXT("A")));Link(Pin(S,TEXT("ReturnValue")),Pin(Last,TEXT("B")));auto T=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_StringToText"));Link(Pin(Last,TEXT("ReturnValue")),Pin(T,TEXT("InString")));return Pin(T,TEXT("ReturnValue"));});BindText(Panel,Freq,TEXT("ReadFrequency"));
 TextReturn(Panel,TEXT("ReadVolume"),[](UEdGraph* G){auto C=Call(G,UKismetTextLibrary::StaticClass(),TEXT("AsPercent_Float"));Link(Read(G,TEXT("Volume")),Pin(C,TEXT("Value")));Def(C,TEXT("MaximumFractionalDigits"),TEXT("0"));return Pin(C,TEXT("ReturnValue"));});BindText(Panel,VolumeLabel,TEXT("ReadVolume"));
 TextReturn(Panel,TEXT("ReadFilter"),[](UEdGraph* G){auto Percent=Call(G,UKismetTextLibrary::StaticClass(),TEXT("AsPercent_Float"));Link(Read(G,TEXT("Filter")),Pin(Percent,TEXT("Value")));Def(Percent,TEXT("MaximumFractionalDigits"),TEXT("0"));auto S=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_TextToString"));Link(Pin(Percent,TEXT("ReturnValue")),Pin(S,TEXT("InText")));auto Join=Call(G,UKismetStringLibrary::StaticClass(),TEXT("Concat_StrStr"));Def(Join,TEXT("A"),TEXT("ФИЛЬТР "));Link(Pin(S,TEXT("ReturnValue")),Pin(Join,TEXT("B")));auto T=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_StringToText"));Link(Pin(Join,TEXT("ReturnValue")),Pin(T,TEXT("InString")));return Pin(T,TEXT("ReturnValue"));});BindText(Panel,FilterLabel,TEXT("ReadFilter"));
 TextReturn(Panel,TEXT("ReadStatus"),[](UEdGraph* G){auto Signal=Call(G,UKismetMathLibrary::StaticClass(),TEXT("SelectString"));Def(Signal,TEXT("A"),TEXT("СИГНАЛ"));Def(Signal,TEXT("B"),TEXT("ПОИСК"));Link(Read(G,TEXT("Receiving")),Pin(Signal,TEXT("bPickA")));auto Power=Call(G,UKismetMathLibrary::StaticClass(),TEXT("SelectString"));Link(Pin(Signal,TEXT("ReturnValue")),Pin(Power,TEXT("A")));Def(Power,TEXT("B"),TEXT("ВЫКЛ"));Link(Read(G,TEXT("Powered")),Pin(Power,TEXT("bPickA")));auto T=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_StringToText"));Link(Pin(Power,TEXT("ReturnValue")),Pin(T,TEXT("InString")));return Pin(T,TEXT("ReturnValue"));});BindText(Panel,Status,TEXT("ReadStatus"));Compile(Panel);Screen->SetWidgetClass(Panel->GeneratedClass.Get());Compile(Radio);
 auto Presentation=Fun(Radio,TEXT("UpdatePresentation"),E);Exec=Pin(E,TEXT("then"));
 for(auto Part:{TEXT("Frequency"),TEXT("Gain"),TEXT("Volume")}){UEdGraphPin* Angle;if(FString(Part)==TEXT("Frequency"))Angle=Op(Presentation,TEXT("Multiply_DoubleDouble"),Op(Presentation,TEXT("Subtract_DoubleDouble"),V(Presentation,TEXT("FineFrequencyKHz")),nullptr,TEXT("153400")),nullptr,TEXT("3"));else Angle=Op(Presentation,TEXT("Subtract_DoubleDouble"),Op(Presentation,TEXT("Multiply_DoubleDouble"),V(Presentation,FString(Part)==TEXT("Gain")?TEXT("Filter"):Part),nullptr,TEXT("270")),nullptr,TEXT("135"));auto Rotation=Call(Presentation,UKismetMathLibrary::StaticClass(),TEXT("MakeRotator"));Link(Angle,Pin(Rotation,TEXT("Yaw")));auto Compose=Call(Presentation,UKismetMathLibrary::StaticClass(),TEXT("ComposeRotators"));Link(Pin(Rotation,TEXT("ReturnValue")),Pin(Compose,TEXT("A")));auto Base=Layout[FString(TEXT("SM_Radio_"))+Part].Rotator();Def(Compose,TEXT("B"),FString::Printf(TEXT("%f,%f,%f"),Base.Pitch,Base.Yaw,Base.Roll));auto Rotate=Call(Presentation,USceneComponent::StaticClass(),TEXT("K2_SetRelativeRotation"));auto Name=FString(TEXT("Radio_"))+Part;Link(V(Presentation,*Name),Pin(Rotate,TEXT("self")));Link(Pin(Compose,TEXT("ReturnValue")),Pin(Rotate,TEXT("NewRotation")));Link(Exec,Pin(Rotate,TEXT("execute")));Exec=Pin(Rotate,TEXT("then"));}
 auto Widget=Call(Presentation,UWidgetComponent::StaticClass(),TEXT("GetUserWidgetObject"));Link(V(Presentation,TEXT("RadioScreen")),Pin(Widget,TEXT("self")));auto CastWidget=Node<UK2Node_DynamicCast>(Presentation);CastWidget->TargetType=Panel->GeneratedClass;CastWidget->AllocateDefaultPins();CastWidget->SetPurity(true);Link(Pin(Widget,TEXT("ReturnValue")),CastWidget->GetCastSourcePin());auto WaveGet=Get(Presentation,TEXT("Waveform"),Panel->GeneratedClass);Link(CastWidget->GetCastResultPin(),Pin(WaveGet,TEXT("self")));auto MID=Call(Presentation,UImage::StaticClass(),TEXT("GetDynamicMaterial"));Link(Pin(WaveGet,TEXT("Waveform")),Pin(MID,TEXT("self")));Link(Exec,Pin(MID,TEXT("execute")));Exec=Pin(MID,TEXT("then"));
 for(auto Name:{TEXT("Tuning"),TEXT("Filter"),TEXT("Power")}){auto SetParam=Call(Presentation,UMaterialInstanceDynamic::StaticClass(),TEXT("SetScalarParameterValue"));Link(Pin(MID,TEXT("ReturnValue")),Pin(SetParam,TEXT("self")));Def(SetParam,TEXT("ParameterName"),Name);UEdGraphPin* Value;if(FString(Name)==TEXT("Tuning"))Value=Op(Presentation,TEXT("Divide_DoubleDouble"),Op(Presentation,TEXT("Subtract_DoubleDouble"),V(Presentation,TEXT("FineFrequencyKHz")),nullptr,TEXT("153400")),nullptr,TEXT("150"));else if(FString(Name)==TEXT("Filter"))Value=V(Presentation,TEXT("Filter"));else {auto Select=Call(Presentation,UKismetMathLibrary::StaticClass(),TEXT("SelectFloat"));Def(Select,TEXT("A"),FString(Name)==TEXT("Power")?TEXT("1"):TEXT("0"));Def(Select,TEXT("B"),FString(Name)==TEXT("Power")?TEXT("0"):TEXT("1"));Link(V(Presentation,FString(Name)==TEXT("Power")?TEXT("Powered"):TEXT("Receiving")),Pin(Select,TEXT("bPickA")));Value=Pin(Select,TEXT("ReturnValue"));}Link(Value,Pin(SetParam,TEXT("Value")));Link(Exec,Pin(SetParam,TEXT("execute")));Exec=Pin(SetParam,TEXT("then"));}
 Compile(Radio);
 for(auto Part:{TEXT("Frequency"),TEXT("Filter"),TEXT("Volume")}){auto Name=FString(TEXT("Apply"))+Part+TEXT("Drag");auto G=Fun(Radio,*Name,E);Exec=Pin(E,TEXT("then"));auto Variable=FString(Part)==TEXT("Frequency")?TEXT("FineFrequencyKHz"):Part;auto Delta=Op(G,TEXT("Multiply_DoubleDouble"),V(G,TEXT("DragDelta")),V(G,FString(Part)==TEXT("Frequency")?TEXT("FrequencySensitivity"):TEXT("LevelSensitivity")));Assign(G,Exec,Variable,Clamp(G,Op(G,TEXT("Add_DoubleDouble"),V(G,Variable),Delta),FString(Part)==TEXT("Frequency")?TEXT("153400"):TEXT("0"),FString(Part)==TEXT("Frequency")?TEXT("153550"):TEXT("1")));if(FString(Part)==TEXT("Frequency")){auto Round=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Round"));Link(V(G,TEXT("FineFrequencyKHz")),Pin(Round,TEXT("A")));Assign(G,Exec,TEXT("FrequencyKHz"),Pin(Round,TEXT("ReturnValue")));}Invoke(G,Exec,TEXT("RefreshReception"));Invoke(G,Exec,TEXT("UpdatePresentation"));}
 auto Toggle=Fun(Radio,TEXT("TogglePower"),E);Exec=Pin(E,TEXT("then"));auto Not=Call(Toggle,UKismetMathLibrary::StaticClass(),TEXT("Not_PreBool"));Link(V(Toggle,TEXT("Powered")),Pin(Not,TEXT("A")));Assign(Toggle,Exec,TEXT("Powered"),Pin(Not,TEXT("ReturnValue")));Invoke(Toggle,Exec,TEXT("RefreshAudio"));Invoke(Toggle,Exec,TEXT("UpdatePresentation"));Compile(Radio);
 // Screen Y grows downwards, so atan2(Y, X) grows clockwise.
 // Ignore the centre to prevent unstable angles, and normalise across -180/+180.
 auto Drag=Fun(Radio,TEXT("HandleDrag"),E);Exec=Pin(E,TEXT("then"));
 auto Mouse=Call(Drag,APlayerController::StaticClass(),TEXT("GetMousePosition"));Link(PC(Drag),Pin(Mouse,TEXT("self")));
 auto DX=Op(Drag,TEXT("Subtract_DoubleDouble"),Pin(Mouse,TEXT("LocationX")),V(Drag,TEXT("DragCenterX")));
 auto DY=Op(Drag,TEXT("Subtract_DoubleDouble"),Pin(Mouse,TEXT("LocationY")),V(Drag,TEXT("DragCenterY")));
 auto RadiusSquared=Op(Drag,TEXT("Add_DoubleDouble"),Op(Drag,TEXT("Multiply_DoubleDouble"),DX,DX),Op(Drag,TEXT("Multiply_DoubleDouble"),DY,DY));
 auto Valid=Node<UK2Node_IfThenElse>(Drag);Valid->AllocateDefaultPins();Link(Exec,Pin(Valid,TEXT("execute")));Link(Op(Drag,TEXT("BooleanAND"),Pin(Mouse,TEXT("ReturnValue")),Op(Drag,TEXT("Greater_DoubleDouble"),RadiusSquared,nullptr,TEXT("64"))),Pin(Valid,TEXT("Condition")));
 Exec=Pin(Valid,TEXT("else"));Assign(Drag,Exec,TEXT("LastAngleValid"),nullptr,TEXT("false"));
 Exec=Pin(Valid,TEXT("then"));auto Angle=Call(Drag,UKismetMathLibrary::StaticClass(),TEXT("DegAtan2"));Link(DY,Pin(Angle,TEXT("Y")));Link(DX,Pin(Angle,TEXT("X")));Assign(Drag,Exec,TEXT("MouseAngle"),Pin(Angle,TEXT("ReturnValue")));
 auto HasLast=Node<UK2Node_IfThenElse>(Drag);HasLast->AllocateDefaultPins();Link(Exec,Pin(HasLast,TEXT("execute")));Link(V(Drag,TEXT("LastAngleValid")),Pin(HasLast,TEXT("Condition")));
 Exec=Pin(HasLast,TEXT("else"));Assign(Drag,Exec,TEXT("LastMouseAngle"),V(Drag,TEXT("MouseAngle")));Assign(Drag,Exec,TEXT("LastAngleValid"),nullptr,TEXT("true"));
 Exec=Pin(HasLast,TEXT("then"));auto Normal=Call(Drag,UKismetMathLibrary::StaticClass(),TEXT("NormalizeAxis"));Link(Op(Drag,TEXT("Subtract_DoubleDouble"),V(Drag,TEXT("MouseAngle")),V(Drag,TEXT("LastMouseAngle"))),Pin(Normal,TEXT("Angle")));Assign(Drag,Exec,TEXT("DragDelta"),Pin(Normal,TEXT("ReturnValue")));Assign(Drag,Exec,TEXT("LastMouseAngle"),V(Drag,TEXT("MouseAngle")));
 auto BF=Node<UK2Node_IfThenElse>(Drag);BF->AllocateDefaultPins();Link(Exec,Pin(BF,TEXT("execute")));Link(Op(Drag,TEXT("EqualEqual_IntInt"),V(Drag,TEXT("DragMode")),nullptr,TEXT("1")),Pin(BF,TEXT("Condition")));Exec=Pin(BF,TEXT("then"));Invoke(Drag,Exec,TEXT("ApplyFrequencyDrag"));
 auto BG=Node<UK2Node_IfThenElse>(Drag);BG->AllocateDefaultPins();Link(Pin(BF,TEXT("else")),Pin(BG,TEXT("execute")));Link(Op(Drag,TEXT("EqualEqual_IntInt"),V(Drag,TEXT("DragMode")),nullptr,TEXT("2")),Pin(BG,TEXT("Condition")));Exec=Pin(BG,TEXT("then"));Invoke(Drag,Exec,TEXT("ApplyFilterDrag"));Exec=Pin(BG,TEXT("else"));Invoke(Drag,Exec,TEXT("ApplyVolumeDrag"));Compile(Radio);
 auto EG=Events(Radio);auto Begin=Event(EG,AActor::StaticClass(),TEXT("ReceiveBeginPlay"));Exec=Pin(Begin,TEXT("then"));
 for(auto Name:{TEXT("bShowMouseCursor"),TEXT("bEnableClickEvents")}){auto S=Node<UK2Node_VariableSet>(EG);S->VariableReference.SetExternalMember(Name,APlayerController::StaticClass());S->AllocateDefaultPins();Def(S,Name,TEXT("true"));Link(PC(EG),Pin(S,TEXT("self")));Link(Exec,Pin(S,TEXT("execute")));Exec=Pin(S,TEXT("then"));}
 auto Mode=Call(EG,UWidgetBlueprintLibrary::StaticClass(),TEXT("SetInputMode_GameAndUIEx"));Link(PC(EG),Pin(Mode,TEXT("PlayerController")));Def(Mode,TEXT("bHideCursorDuringCapture"),TEXT("false"));Link(Exec,Pin(Mode,TEXT("execute")));Exec=Pin(Mode,TEXT("then"));auto Self=Node<UK2Node_Self>(EG);Self->AllocateDefaultPins();auto View=Call(EG,APlayerController::StaticClass(),TEXT("SetViewTargetWithBlend"));Link(PC(EG),Pin(View,TEXT("self")));Link(Pin(Self,TEXT("self")),Pin(View,TEXT("NewViewTarget")));Link(Exec,Pin(View,TEXT("execute")));Exec=Pin(View,TEXT("then"));
 auto UserWidget=Call(EG,UWidgetComponent::StaticClass(),TEXT("GetUserWidgetObject"));Link(V(EG,TEXT("RadioScreen")),Pin(UserWidget,TEXT("self")));auto WC=Node<UK2Node_DynamicCast>(EG);WC->TargetType=Panel->GeneratedClass;WC->AllocateDefaultPins();WC->SetPurity(true);Link(Pin(UserWidget,TEXT("ReturnValue")),WC->GetCastSourcePin());auto Ref=Node<UK2Node_VariableSet>(EG);Ref->VariableReference.SetExternalMember(TEXT("RadioReference"),Panel->GeneratedClass);Ref->AllocateDefaultPins();Link(WC->GetCastResultPin(),Pin(Ref,TEXT("self")));Link(Pin(Self,TEXT("self")),Pin(Ref,TEXT("RadioReference")));Link(Exec,Pin(Ref,TEXT("execute")));Exec=Pin(Ref,TEXT("then"));Invoke(EG,Exec,TEXT("UpdatePresentation"));
 for(int Index=1;Index<=4;Index++){
  auto Part=Index==1?TEXT("Frequency"):Index==2?TEXT("Gain"):Index==3?TEXT("Volume"):TEXT("Power");
  auto EventNode=Node<UK2Node_ComponentBoundEvent>(EG);auto Name=FName(*FString::Printf(TEXT("Click_%s"),Part));EventNode->InitializeComponentBoundEventParams(FindFProperty<FObjectProperty>(Radio->SkeletonGeneratedClass,Name),FindFProperty<FMulticastDelegateProperty>(UPrimitiveComponent::StaticClass(),TEXT("OnClicked")));EventNode->AllocateDefaultPins();Exec=Pin(EventNode,TEXT("then"));
  if(Index==4){Invoke(EG,Exec,TEXT("TogglePower"));continue;}
  Assign(EG,Exec,TEXT("DragMode"),nullptr,*FString::FromInt(Index));Assign(EG,Exec,TEXT("LastAngleValid"),nullptr,TEXT("false"));
  auto KnobPosition=Call(EG,USceneComponent::StaticClass(),TEXT("K2_GetComponentLocation"));Link(V(EG,*FString::Printf(TEXT("Radio_%s"),Part)),Pin(KnobPosition,TEXT("self")));
  auto Project=Call(EG,APlayerController::StaticClass(),TEXT("ProjectWorldLocationToScreen"));Link(PC(EG),Pin(Project,TEXT("self")));Link(Pin(KnobPosition,TEXT("ReturnValue")),Pin(Project,TEXT("WorldLocation")));Def(Project,TEXT("bPlayerViewportRelative"),TEXT("true"));
  auto Split=Call(EG,UKismetMathLibrary::StaticClass(),TEXT("BreakVector2D"));Link(Pin(Project,TEXT("ScreenLocation")),Pin(Split,TEXT("InVec")));
  auto OnScreen=Node<UK2Node_IfThenElse>(EG);OnScreen->AllocateDefaultPins();Link(Exec,Pin(OnScreen,TEXT("execute")));Link(Pin(Project,TEXT("ReturnValue")),Pin(OnScreen,TEXT("Condition")));Exec=Pin(OnScreen,TEXT("else"));Assign(EG,Exec,TEXT("DragMode"),nullptr,TEXT("0"));Exec=Pin(OnScreen,TEXT("then"));
  Assign(EG,Exec,TEXT("DragCenterX"),Pin(Split,TEXT("X")));Assign(EG,Exec,TEXT("DragCenterY"),Pin(Split,TEXT("Y")));Invoke(EG,Exec,TEXT("HandleDrag"));
 }
 auto Tick=Event(EG,AActor::StaticClass(),TEXT("ReceiveTick"));auto Active=Node<UK2Node_IfThenElse>(EG);Active->AllocateDefaultPins();Link(Pin(Tick,TEXT("then")),Pin(Active,TEXT("execute")));Link(Op(EG,TEXT("Greater_IntInt"),V(EG,TEXT("DragMode")),nullptr,TEXT("0")),Pin(Active,TEXT("Condition")));auto Down=Call(EG,APlayerController::StaticClass(),TEXT("IsInputKeyDown"));Link(PC(EG),Pin(Down,TEXT("self")));Def(Down,TEXT("Key"),TEXT("LeftMouseButton"));auto Held=Node<UK2Node_IfThenElse>(EG);Held->AllocateDefaultPins();Link(Pin(Active,TEXT("then")),Pin(Held,TEXT("execute")));Link(Pin(Down,TEXT("ReturnValue")),Pin(Held,TEXT("Condition")));Exec=Pin(Held,TEXT("then"));Invoke(EG,Exec,TEXT("HandleDrag"));Exec=Pin(Held,TEXT("else"));Invoke(EG,Exec,TEXT("HandleDrag"));Assign(EG,Exec,TEXT("DragMode"),nullptr,TEXT("0"));
 Compile(Radio);Compile(Panel);
 auto CDO=Radio->GeneratedClass->GetDefaultObject();for(auto Pair:{TPair<FName,FString>(TEXT("NoiseClip"),TEXT("RadioNoise")),{TEXT("SignalClip"),TEXT("RadioSignal")}}){auto Clip=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/RadioExample/Audio/%s.%s"),*Pair.Value,*Pair.Value));check(Clip);FindFProperty<FObjectProperty>(Radio->GeneratedClass,Pair.Key)->SetObjectPropertyValue_InContainer(CDO,Clip);}Save(Panel);Save(Radio);
 auto World=GEditor->NewMap();World->GetWorldSettings()->DefaultGameMode=AGameModeBase::StaticClass();auto Instance=World->SpawnActor<AActor>(Radio->GeneratedClass);Instance->SetActorLabel(TEXT("RADIO — drag controls and embedded display"));auto Light=World->SpawnActor<ADirectionalLight>();Light->SetActorRotation(FRotator(-35,-40,0));Light->GetLightComponent()->SetIntensity(3);auto Sky=World->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetIntensity(1);auto Table=World->SpawnActor<AStaticMeshActor>();Table->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Table->SetActorLocation(FVector(0,30,34));Table->SetActorScale3D(FVector(2,1.5,.1));check(FEditorFileUtils::SaveLevel(World->PersistentLevel,*FPackageName::LongPackageNameToFilename(TEXT("/Game/RadioExample/RadioExampleMap"),FPackageName::GetMapPackageExtension())));
 UE_LOG(LogTemp,Display,TEXT("FINAL RADIO COMPLETE: native Blueprint graphs, circular drag, speech filter, smooth waveform and embedded readouts"));
}
void PolishGraphs() {
 using namespace RadioBuild;
 for(auto Path:{TEXT("/Game/RadioExample/BP_RadioExample.BP_RadioExample"),TEXT("/Game/RadioExample/WBP_RadioDisplay.WBP_RadioDisplay")}) {
  auto B=LoadObject<UBlueprint>(nullptr,Path);check(B);
  for(auto& Variable:B->NewVariables) {
   FString N=Variable.VarName.ToString();
   Variable.Category=FText::FromString(N.Contains(TEXT("Mouse"))||N.Contains(TEXT("Drag"))?TEXT("Input"):N.Contains(TEXT("Sensitivity"))||N.Contains(TEXT("Station"))||N.Contains(TEXT("LockWidth"))?TEXT("Settings"):N.Contains(TEXT("Clip"))?TEXT("Audio"):TEXT("Radio State"));
  }
  for(auto G:B->FunctionGraphs)for(auto Node:G->Nodes)if(auto C=Cast<UK2Node_CallFunction>(Node))if(C->GetFunctionName()==TEXT("AsPercent_Float"))Def(C,TEXT("MaximumFractionalDigits"),TEXT("0"));
  Compile(B);
  TArray<UEdGraph*> Graphs=B->FunctionGraphs;Graphs.Append(B->UbergraphPages);
  for(auto G:Graphs) {int32 Index=0;for(auto N:G->Nodes) {N->Modify();N->NodePosX=(Index%5)*500;N->NodePosY=(Index/5)*380;Index++;if(Cast<UK2Node_FunctionEntry>(N)) {N->NodePosX=-500;N->NodePosY=0;N->NodeComment=G->GetName()+TEXT(" — см. раздел 11 UE5_ARCHITECTURE.md");N->bCommentBubbleVisible=true;}}
   G->NotifyGraphChanged();
  }
  B->MarkPackageDirty();Save(B);
 }
 // Keep muted speech advancing; restoring Volume must not restart or lose it.
 for(auto Name:{TEXT("RadioSignal"),TEXT("RadioNoise")}) {
  auto Clip=LoadObject<USoundWave>(nullptr,*FString::Printf(TEXT("/Game/RadioExample/Audio/%s.%s"),Name,Name));check(Clip);
  Clip->VirtualizationMode=EVirtualizationMode::PlayWhenSilent;Clip->bLooping=true;Clip->MarkPackageDirty();Save(Clip);
 }
 // Integration checks execute the saved Blueprint, rather than a parallel C++ radio.
 FEditorScriptExecutionGuard ScriptGuard;
 auto BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/RadioExample/BP_RadioExample.BP_RadioExample"));
 auto Instance=GEditor->GetEditorWorldContext().World()->SpawnActor<AActor>(BP->GeneratedClass);
 auto SetNumber=[&](const TCHAR* N,double Value){auto P=FindFProperty<FDoubleProperty>(BP->GeneratedClass,N);check(P);P->SetPropertyValue_InContainer(Instance,Value);};
 auto ReadNumber=[&](const TCHAR* N){return FindFProperty<FDoubleProperty>(BP->GeneratedClass,N)->GetPropertyValue_InContainer(Instance);};
 auto Run=[&](const TCHAR* N){auto F=Instance->FindFunction(N);check(F);Instance->ProcessEvent(F,nullptr);};
 auto Object=[&](const TCHAR* N){return FindFProperty<FObjectProperty>(BP->GeneratedClass,N)->GetObjectPropertyValue_InContainer(Instance);};
 auto Screen=CastChecked<UWidgetComponent>(Object(TEXT("RadioScreen")));Screen->InitWidget();auto Widget=Screen->GetUserWidgetObject();check(Widget);
 FindFProperty<FObjectProperty>(Widget->GetClass(),TEXT("RadioReference"))->SetObjectPropertyValue_InContainer(Widget,Instance);
 auto Voice=CastChecked<UAudioComponent>(Object(TEXT("ReceiverAudio")));auto Noise=CastChecked<UAudioComponent>(Object(TEXT("InterferenceAudio")));
 int32 Checks=0;auto Expect=[&](bool Result,const TCHAR* Name){checkf(Result,TEXT("RADIO CHECK FAILED: %s"),Name);Checks++;UE_LOG(LogTemp,Display,TEXT("RADIO CHECK PASS: %s"),Name);};
 auto Knob=CastChecked<USceneComponent>(Object(TEXT("Radio_Frequency")));auto Camera=CastChecked<UCameraComponent>(Object(TEXT("ExampleCamera")));
 auto Project=[&](FVector P){auto D=P-Camera->GetComponentLocation();return FVector2D(FVector::DotProduct(D,Camera->GetRightVector()),-FVector::DotProduct(D,Camera->GetUpVector()))/FVector::DotProduct(D,Camera->GetForwardVector());};
 auto MarkerAngle=[&](){auto D=Project(Knob->GetComponentTransform().TransformPosition(FVector(1,0,0)))-Project(Knob->GetComponentLocation());return FMath::RadiansToDegrees(FMath::Atan2(D.Y,D.X));};
 SetNumber(TEXT("FineFrequencyKHz"),153400);Run(TEXT("UpdatePresentation"));auto Before=MarkerAngle();SetNumber(TEXT("DragDelta"),90);Run(TEXT("ApplyFrequencyDrag"));auto VisualDelta=FMath::FindDeltaAngleDegrees(Before,MarkerAngle());UE_LOG(LogTemp,Display,TEXT("RADIO CLOCKWISE VISUAL DELTA: %f"),VisualDelta);
 SetNumber(TEXT("Volume"),.8);
 for(double Filter:{0.,.5,1.}){SetNumber(TEXT("Filter"),Filter);Run(TEXT("UpdateAudioLevel"));Expect(FMath::IsNearlyEqual(Voice->VolumeMultiplier,.8f),TEXT("Filter preserves voice level"));Expect(FMath::IsNearlyEqual(Noise->VolumeMultiplier,float(.8*(.6-.57*Filter))),TEXT("Filter reduces separate interference"));}
 for(auto Pair:{TPair<const TCHAR*,const TCHAR*>(TEXT("Filter"),TEXT("ApplyFilterDrag")),{TEXT("Volume"),TEXT("ApplyVolumeDrag")}}){SetNumber(TEXT("DragDelta"),10000);Run(Pair.Value);Expect(ReadNumber(Pair.Key)==1,TEXT("Level upper clamp"));SetNumber(TEXT("DragDelta"),-10000);Run(Pair.Value);Expect(ReadNumber(Pair.Key)==0,TEXT("Level lower clamp"));}
 SetNumber(TEXT("DragDelta"),10000);Run(TEXT("ApplyFrequencyDrag"));Expect(ReadNumber(TEXT("FineFrequencyKHz"))==153550,TEXT("Frequency upper clamp"));SetNumber(TEXT("DragDelta"),-10000);Run(TEXT("ApplyFrequencyDrag"));Expect(ReadNumber(TEXT("FineFrequencyKHz"))==153400,TEXT("Frequency lower clamp"));
 SetNumber(TEXT("FineFrequencyKHz"),153450);Run(TEXT("TogglePower"));Expect(FindFProperty<FBoolProperty>(BP->GeneratedClass,TEXT("Receiving"))->GetPropertyValue_InContainer(Instance),TEXT("Station locks at target"));Run(TEXT("TogglePower"));Expect(!Voice->IsPlaying()&&!Noise->IsPlaying(),TEXT("OFF stops both audio components"));
 Instance->Destroy();UE_LOG(LogTemp,Display,TEXT("RADIO VERIFICATION COMPLETE: %d checks"),Checks);
 UE_LOG(LogTemp,Display,TEXT("RADIO GRAPH LAYOUT COMPLETE"));
}

}
