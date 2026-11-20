// Editor authoring only. Runtime camera behaviour is saved as Blueprint nodes.
namespace DeskRefinement {
using namespace ReceiverUpgrade;
void BuildLook() {
 auto Rig=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_DeskWorkstationRig"));check(Rig);
 for(auto N:{TEXT("RoomTurn"),TEXT("RoomTurnTarget"),TEXT("RoomEdgeTime")})Variable(Rig,N,StationBuild::Real(),TEXT("0"));
 Variable(Rig,TEXT("RoomLookWidth"),StationBuild::Real(),TEXT("48"));Variable(Rig,TEXT("RoomLookYaw"),StationBuild::Real(),TEXT("-108"));
 Declare(Rig,{TEXT("UpdateRoomTurn"),TEXT("ApplyRoomTurnPose"),TEXT("ResetRoomTurn")});
 UEdGraphPin* X;auto G=Reset(Rig,TEXT("ApplyRoomTurnPose"),X);
 Assign(G,X,TEXT("RoomTurn"),Interp(G,V(G,TEXT("RoomTurn")),V(G,TEXT("RoomTurnTarget")),TEXT("2.2")));
 struct Blend {const TCHAR* Name;double A,B;};for(auto Pair:{Blend{TEXT("RoomLookWidth"),48,16},Blend{TEXT("RoomLookYaw"),-108,40}}){
  auto L=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Lerp"));Def(L,TEXT("A"),FString::SanitizeFloat(Pair.A));Def(L,TEXT("B"),FString::SanitizeFloat(Pair.B));Link(V(G,TEXT("RoomTurn")),Pin(L,TEXT("Alpha")));Assign(G,X,Pair.Name,Pin(L,TEXT("ReturnValue")));
 }
 G=Reset(Rig,TEXT("ResetRoomTurn"),X);for(auto N:{TEXT("RoomTurn"),TEXT("RoomTurnTarget"),TEXT("RoomEdgeTime")})Assign(G,X,N,nullptr,TEXT("0"));Assign(G,X,TEXT("RoomLookWidth"),nullptr,TEXT("48"));Assign(G,X,TEXT("RoomLookYaw"),nullptr,TEXT("-108"));
 G=Reset(Rig,TEXT("UpdateRoomTurn"),X);
 auto Mouse=Call(G,APlayerController::StaticClass(),TEXT("GetMousePosition"));Link(PC(G),Pin(Mouse,TEXT("self")));auto Size=Call(G,APlayerController::StaticClass(),TEXT("GetViewportSize"));Link(PC(G),Pin(Size,TEXT("self")));
 auto Can=Op(G,TEXT("BooleanAND"),Op(G,TEXT("EqualEqual_IntInt"),V(G,TEXT("CameraMode")),nullptr,TEXT("0")),Op(G,TEXT("Not_PreBool"),V(G,TEXT("Transitioning"))));
 Can=Op(G,TEXT("BooleanAND"),Can,Pin(Mouse,TEXT("ReturnValue")));Can=Op(G,TEXT("BooleanAND"),Can,Op(G,TEXT("Greater_IntInt"),Pin(Size,TEXT("SizeX")),nullptr,TEXT("0")));
 Can=Op(G,TEXT("BooleanAND"),Can,Ext(G,APlayerController::StaticClass(),TEXT("bEnableClickEvents"),PC(G)));auto Down=Call(G,APlayerController::StaticClass(),TEXT("IsInputKeyDown"));Link(PC(G),Pin(Down,TEXT("self")));Def(Down,TEXT("Key"),TEXT("LeftMouseButton"));Can=Op(G,TEXT("BooleanAND"),Can,Op(G,TEXT("Not_PreBool"),Pin(Down,TEXT("ReturnValue"))));auto Guard=Branch(G,X,Can);X=Pin(Guard,TEXT("then"));
 auto NX=Clamp(G,Op(G,TEXT("Subtract_DoubleDouble"),Op(G,TEXT("Multiply_DoubleDouble"),Op(G,TEXT("Divide_DoubleDouble"),Pin(Mouse,TEXT("LocationX")),Pin(Size,TEXT("SizeX"))),nullptr,TEXT("2")),nullptr,TEXT("1")),TEXT("-1"),TEXT("1"));
 auto Edge=Op(G,TEXT("Greater_DoubleDouble"),Abs(G,NX),nullptr,TEXT("0.9"));Assign(G,X,TEXT("RoomEdgeTime"),Select(G,Edge,Op(G,TEXT("Add_DoubleDouble"),V(G,TEXT("RoomEdgeTime")),Delta(G))));
 auto Ready=Branch(G,X,Op(G,TEXT("Greater_DoubleDouble"),V(G,TEXT("RoomEdgeTime")),nullptr,TEXT("0.35")));
 auto Then=Pin(Ready,TEXT("then"));Assign(G,Then,TEXT("RoomTurnTarget"),Select(G,Op(G,TEXT("Greater_DoubleDouble"),NX,nullptr,TEXT("0")),Scalar(G,TEXT("1"))));Invoke(Rig,G,Then,TEXT("ApplyRoomTurnPose"));auto Else=Pin(Ready,TEXT("else"));Invoke(Rig,G,Else,TEXT("ApplyRoomTurnPose"));
 auto Look=Graph(Rig,TEXT("UpdateDeskLook"));UK2Node_CallFunction* YawMultiply=nullptr;
 auto LookNodes=Look->Nodes;for(auto N:LookNodes)if(auto C=Cast<UK2Node_CallFunction>(N)){
  if(C->GetFunctionName()==TEXT("Multiply_DoubleDouble")){
   auto B=Pin(C,TEXT("B"));bool IsYaw=B->DefaultValue==TEXT("175");for(auto L:B->LinkedTo)if(auto Get=Cast<UK2Node_VariableGet>(L->GetOwningNode()))if(Get->GetVarName()==TEXT("RoomLookWidth"))IsYaw=true;if(IsYaw){YawMultiply=C;B->BreakAllPinLinks();Link(V(Look,TEXT("RoomLookWidth")),B);}
   if(B->DefaultValue==TEXT("-42"))Def(C,TEXT("B"),TEXT("-14"));
  }
  for(auto P:C->Pins)if(P->Direction==EGPD_Input&&!P->LinkedTo.Num()&&P->PinType.PinCategory==UEdGraphSchema_K2::PC_Real&&FMath::IsNearlyEqual(FCString::Atod(*P->DefaultValue),104.,.0001))RadioBuild::Schema->TrySetDefaultValue(*P,TEXT("112"));
 }
 check(YawMultiply);auto Out=Pin(YawMultiply,TEXT("ReturnValue"));UK2Node_CallFunction* AddYaw=nullptr;for(auto P:Out->LinkedTo)if(auto C=Cast<UK2Node_CallFunction>(P->GetOwningNode()))if(C->GetFunctionName()==TEXT("Add_DoubleDouble"))AddYaw=C;check(AddYaw);Link(V(Look,TEXT("RoomLookYaw")),Pin(AddYaw,TEXT("B")));
 for(auto Pair:{TPair<UEdGraph*,const TCHAR*>(Look,TEXT("UpdateRoomTurn")),{Graph(Rig,TEXT("FocusCenter")),TEXT("ResetRoomTurn")}}){bool Done=false;for(auto N:Pair.Key->Nodes)if(auto C=Cast<UK2Node_CallFunction>(N))if(C->GetFunctionName()==Pair.Value)Done=true;if(!Done)Append(Rig,Pair.Key,Pair.Value);}
 auto View=CastChecked<UChildActorComponent>(StationBuild::SCS(Rig,TEXT("ViewCenter"))->ComponentTemplate);View->SetRelativeLocation(FVector(94,117,112));
 FTransform Placement(FRotator(0,180,0),FVector(555.15,451.32,-6.5),FVector(1.28405738));FVector Eye(386,344,145),Target(386,412,95);auto Focus=CastChecked<UChildActorComponent>(StationBuild::SCS(Rig,TEXT("ViewDeck"))->ComponentTemplate);Focus->SetRelativeLocation(Placement.InverseTransformPosition(Eye));auto R=(Target-Eye).Rotation();R.Yaw-=180;Focus->SetRelativeRotation(R);CastChecked<ACameraActor>(Focus->GetChildActorTemplate())->GetCameraComponent()->SetFieldOfView(54);
 Compile(Rig);Save(Rig);UE_LOG(LogTemp,Display,TEXT("DESK REFINEMENT LOOK COMPLETE desk +/-48, bed +/-16, edge delay .35, height137.31"));
}
void Keyboard() {
 TArray<FString> Lines;check(FFileHelper::LoadFileToStringArray(Lines,*(FPaths::ProjectDir()/TEXT("../Tools/StationIntegration/SourceArt/KeyboardTevy/keyboard-mesh.tsv"))));check(Lines.Num()%3==0);
 FMeshDescription MD;FStaticMeshAttributes A(MD);A.Register();auto P=A.GetVertexPositions();auto N=A.GetVertexInstanceNormals();auto UV=A.GetVertexInstanceUVs();UV.SetNumChannels(1);auto Group=MD.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Keyboard");
 for(int I=0;I<Lines.Num();I+=3){TArray<FVertexInstanceID> Corners;for(int J=0;J<3;J++){TArray<FString> V;Lines[I+J].ParseIntoArray(V,TEXT("\t"));check(V.Num()==8);auto Id=MD.CreateVertex();auto VI=MD.CreateVertexInstance(Id);P[Id]=FVector3f(FCString::Atof(*V[0]),FCString::Atof(*V[1]),FCString::Atof(*V[2]));N[VI]=FVector3f(FCString::Atof(*V[3]),FCString::Atof(*V[4]),FCString::Atof(*V[5]));UV.Set(VI,0,FVector2f(FCString::Atof(*V[6]),FCString::Atof(*V[7])));Corners.Add(VI);}MD.CreateTriangle(Group,Corners);}
 auto M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/RoomArt/Keyboard/SM_KeyboardCC0"));if(!M){M=NewObject<UStaticMesh>(CreatePackage(TEXT("/Game/RoomArt/Keyboard/SM_KeyboardCC0")),TEXT("SM_KeyboardCC0"),RF_Public|RF_Standalone);M->AddSourceModel();FAssetRegistryModule::AssetCreated(M);}M->GetStaticMaterials().Empty();M->GetStaticMaterials().Add(FStaticMaterial(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/RoomArt/Keyboard/M_KeyboardCC0")),TEXT("Keyboard")));M->CreateMeshDescription(0,MoveTemp(MD));M->CommitMeshDescription(0);auto& S=M->GetSourceModel(0).BuildSettings;S.bRecomputeNormals=false;S.bRecomputeTangents=true;S.bUseFullPrecisionUVs=true;M->Build(false);M->PostEditChange();Save(M);UE_LOG(LogTemp,Display,TEXT("DESK REFINEMENT KEYBOARD triangles=%d"),Lines.Num()/3);
}
void Dish() {
 auto Source=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/RoomArt/DishLatest/DishLatest"));check(Source);auto Old=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/RoomArt/Meshes/SM_Тарелка_NEW_3_935ca5"));check(Old);auto MD=*Source->GetMeshDescription(0);FStaticMeshAttributes A(MD);auto P=A.GetVertexPositions();TSet<FString> Seen;TArray<FTriangleID> Remove;
 for(auto T:MD.Triangles().GetElementIDs()){TArray<FString> Keys;for(auto VI:MD.GetTriangleVertexInstances(T)){auto V=P[MD.GetVertexInstanceVertex(VI)];Keys.Add(FString::Printf(TEXT("%.4f,%.4f,%.4f"),V.X,V.Y,V.Z));}Keys.Sort();auto Key=FString::Join(Keys,TEXT("|"));if(Seen.Contains(Key))Remove.Add(T);else Seen.Add(Key);}
 for(auto T:Remove)MD.DeleteTriangle(T);
 auto NewBox=Source->GetBoundingBox(),OldBox=Old->GetBoundingBox();double Scale=FMath::Max(OldBox.GetSize().X,OldBox.GetSize().Y)/FMath::Max(NewBox.GetSize().X,NewBox.GetSize().Y);
 for(auto V:MD.Vertices().GetElementIDs()){auto Q=FVector(P[V])-NewBox.GetCenter();Q*=Scale;Q.X+=OldBox.GetCenter().X;Q.Y+=OldBox.GetCenter().Y;Q.Z+=(NewBox.GetSize().Z*.5)*Scale+OldBox.Min.Z;P[V]=FVector3f(Q);}
 auto M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/RoomArt/DishLatest/SM_DishLatestClean"));if(!M){M=NewObject<UStaticMesh>(CreatePackage(TEXT("/Game/RoomArt/DishLatest/SM_DishLatestClean")),TEXT("SM_DishLatestClean"),RF_Public|RF_Standalone);M->AddSourceModel();FAssetRegistryModule::AssetCreated(M);}M->GetStaticMaterials()=Source->GetStaticMaterials();M->CreateMeshDescription(0,MoveTemp(MD));M->CommitMeshDescription(0);auto& S=M->GetSourceModel(0).BuildSettings;S.bRecomputeNormals=false;S.bRecomputeTangents=true;M->Build(false);M->PostEditChange();Save(M);UE_LOG(LogTemp,Display,TEXT("DESK REFINEMENT DISH removed=%d remaining=%d scale=%f"),Remove.Num(),M->GetMeshDescription(0)->Triangles().Num(),Scale);
}
void RepairPlaybackGuard() {
 auto BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/StationIntegration/BP_TapeDeckIntegrated"));check(BP);
 // Audio thread can deliver its zero callback after the first valid percentage.
 // Forward playback must not move backwards; reverse playback must not move forwards.
 if(FindFProperty<FBoolProperty>(BP->GeneratedClass,TEXT("DeskPlaybackGuardApplied")))return;
 Variable(BP,TEXT("DeskPlaybackGuardApplied"),Type(UEdGraphSchema_K2::PC_Boolean),TEXT("true"));
 auto G=Events(BP);auto Nodes=G->Nodes;
 for(auto N:Nodes)if(auto S=Cast<UK2Node_VariableSet>(N))if(S->GetVarName()==TEXT("PositionSeconds")){
  auto Value=Pin(S,TEXT("PositionSeconds"));if(!Value->LinkedTo.Num())continue;
  auto Candidate=Value->LinkedTo[0];auto In=Pin(S,TEXT("execute"));auto Before=In->LinkedTo;In->BreakAllPinLinks();
  auto Reverse=Op(G,TEXT("EqualEqual_IntInt"),V(G,TEXT("TransportMode")),nullptr,TEXT("3"));
  auto ForwardOK=Op(G,TEXT("GreaterEqual_DoubleDouble"),Candidate,Op(G,TEXT("Subtract_DoubleDouble"),V(G,TEXT("PositionSeconds")),nullptr,TEXT("0.05")));
  auto ReverseOK=Op(G,TEXT("LessEqual_DoubleDouble"),Candidate,Op(G,TEXT("Add_DoubleDouble"),V(G,TEXT("PositionSeconds")),nullptr,TEXT("0.05")));
  auto Good=Op(G,TEXT("BooleanOR"),Op(G,TEXT("BooleanAND"),Reverse,ReverseOK),Op(G,TEXT("BooleanAND"),Op(G,TEXT("Not_PreBool"),Reverse),ForwardOK));
  for(auto P:Before){auto Guard=Branch(G,P,Good);Link(Pin(Guard,TEXT("then")),In);}
 }
 Compile(BP);Save(BP);
}
void Build(){BuildLook();Keyboard();Dish();RepairPlaybackGuard();}
}
