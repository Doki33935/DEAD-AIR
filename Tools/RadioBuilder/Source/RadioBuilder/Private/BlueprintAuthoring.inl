// Small shared editor graph helpers. No runtime code and no legacy scene generators.
namespace FinalRadio {
using namespace RadioBuild;
UEdGraphPin* V(UEdGraph* G,const TCHAR* N) { return Get(G,N)->FindPin(N); }
UEdGraphPin* Op(UEdGraph* G,const TCHAR* N,UEdGraphPin* A,UEdGraphPin* B=nullptr,const TCHAR* Number=nullptr) { auto C=Call(G,UKismetMathLibrary::StaticClass(),N); Link(A,Pin(C,TEXT("A"))); if(B)Link(B,Pin(C,TEXT("B"))); else if(Number)Def(C,TEXT("B"),Number); return Pin(C,TEXT("ReturnValue")); }
UEdGraphPin* Clamp(UEdGraph* G,UEdGraphPin* Value,const TCHAR* Min,const TCHAR* Max) { auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("FClamp")); Link(Value,Pin(C,TEXT("Value"))); Def(C,TEXT("Min"),Min); Def(C,TEXT("Max"),Max); return Pin(C,TEXT("ReturnValue")); }
UEdGraphPin* PC(UEdGraph* G) { return Pin(Call(G,UGameplayStatics::StaticClass(),TEXT("GetPlayerController")),TEXT("ReturnValue")); }
void Assign(UEdGraph* G,UEdGraphPin*& Exec,const TCHAR* Name,UEdGraphPin* Value=nullptr,const TCHAR* Default=nullptr) { auto S=Set(G,Name); if(Value)Link(Value,Pin(S,Name)); if(Default)Def(S,Name,Default); Link(Exec,Pin(S,TEXT("execute"))); Exec=Pin(S,TEXT("then")); }
}
namespace MenuBuild {
using namespace RadioBuild;using FinalRadio::V;using FinalRadio::Assign;
UK2Node_CallFunction* Flow(UEdGraph* G,UEdGraphPin*& X,UClass* C,const TCHAR* Name,UEdGraphPin* Target=nullptr){auto N=Call(G,C,Name);if(Target)Link(Target,Pin(N,TEXT("self")));if(auto P=N->FindPin(TEXT("execute"))){Link(X,P);X=Pin(N,TEXT("then"));}return N;}
UEdGraphPin* Ext(UEdGraph* G,UClass* C,const TCHAR* N,UEdGraphPin* Target){auto V=Get(G,N,C);Link(Target,Pin(V,TEXT("self")));return Pin(V,N);}
void Put(UEdGraph* G,UEdGraphPin*& X,UClass* C,const TCHAR* N,UEdGraphPin* Target,UEdGraphPin* Value=nullptr,const TCHAR* Default=nullptr){auto S=Node<UK2Node_VariableSet>(G);S->VariableReference.SetExternalMember(N,C);S->AllocateDefaultPins();Link(Target,Pin(S,TEXT("self")));if(Value)Link(Value,Pin(S,N));if(Default)Def(S,N,Default);Link(X,Pin(S,TEXT("execute")));X=Pin(S,TEXT("then"));}
UK2Node_IfThenElse* Branch(UEdGraph* G,UEdGraphPin* X,UEdGraphPin* Condition){auto B=Node<UK2Node_IfThenElse>(G);B->AllocateDefaultPins();Link(X,Pin(B,TEXT("execute")));Link(Condition,Pin(B,TEXT("Condition")));return B;}
UK2Node_DynamicCast* Cast(UEdGraph* G,UClass* C,UEdGraphPin* Object,bool Pure=true){auto N=Node<UK2Node_DynamicCast>(G);N->TargetType=C;N->AllocateDefaultPins();N->SetPurity(Pure);Link(Object,N->GetCastSourcePin());return N;}
UEdGraphPin* Join(UEdGraph* G,UEdGraphPin* A,UEdGraphPin* B=nullptr,const TCHAR* Default=nullptr){auto N=Call(G,UKismetStringLibrary::StaticClass(),TEXT("Concat_StrStr"));if(A)Link(A,Pin(N,TEXT("A")));if(B)Link(B,Pin(N,TEXT("B")));if(Default)Def(N,TEXT("B"),Default);return Pin(N,TEXT("ReturnValue"));}
UEdGraphPin* String(UEdGraph* G,const TCHAR* S){auto N=Call(G,UKismetStringLibrary::StaticClass(),TEXT("Concat_StrStr"));Def(N,TEXT("A"),S);return Pin(N,TEXT("ReturnValue"));}
UEdGraphPin* Text(UEdGraph* G,UEdGraphPin* S){auto N=Call(G,UKismetTextLibrary::StaticClass(),TEXT("Conv_StringToText"));Link(S,Pin(N,TEXT("InString")));return Pin(N,TEXT("ReturnValue"));}
UEdGraphPin* Valid(UEdGraph* G,UEdGraphPin* Object){auto C=Call(G,UKismetSystemLibrary::StaticClass(),TEXT("IsValid"));Link(Object,Pin(C,TEXT("Object")));return Pin(C,TEXT("ReturnValue"));}
}
namespace StationBuild {
using namespace RadioBuild;
FEdGraphPinType Real(){auto T=Type(UEdGraphSchema_K2::PC_Real);T.PinSubCategory=UEdGraphSchema_K2::PC_Double;return T;}
USCS_Node* SCS(UBlueprint* B,const TCHAR* N){for(auto X:B->SimpleConstructionScript->GetAllNodes())if(X->GetVariableName()==FName(N))return X;return nullptr;}
USCS_Node* Part(UBlueprint* B,USCS_Node* Root,const TCHAR* N,UClass* C){auto X=B->SimpleConstructionScript->CreateNode(C,N);if(Root)Root->AddChildNode(X);else B->SimpleConstructionScript->AddNode(X);return X;}
void ClearEvents(UBlueprint* B){for(auto G:B->UbergraphPages){auto Nodes=G->Nodes;for(auto N:Nodes)G->RemoveNode(N);}}
}
namespace ReceiverUpgrade {
using namespace RadioBuild;using FinalRadio::V;using FinalRadio::Op;using FinalRadio::Assign;using FinalRadio::PC;using FinalRadio::Clamp;
using MenuBuild::Flow;using MenuBuild::Ext;using MenuBuild::Put;using MenuBuild::Branch;using MenuBuild::Valid;using MenuBuild::Text;using MenuBuild::String;
UEdGraph* Graph(UBlueprint* B,const TCHAR* N){for(auto G:B->FunctionGraphs)if(G->GetFName()==N)return G;return nullptr;}
UK2Node_FunctionEntry* Entry(UEdGraph* G){for(auto N:G->Nodes)if(auto E=Cast<UK2Node_FunctionEntry>(N))return E;check(false);return nullptr;}
void Variable(UBlueprint* B,const TCHAR* N,FEdGraphPinType T,const FString& D=TEXT("")){if(FBlueprintEditorUtils::FindNewVariableIndex(B,N)==INDEX_NONE)Var(B,N,T,D);}
UEdGraph* Reset(UBlueprint* B,const TCHAR* N,UEdGraphPin*& X){auto G=Graph(B,N);if(!G){UK2Node_FunctionEntry* E;G=Fun(B,N,E);}auto E=Entry(G);auto Nodes=G->Nodes;for(auto OldNode:Nodes)if(OldNode!=E)FBlueprintEditorUtils::RemoveNode(B,OldNode,true);X=Pin(E,TEXT("then"));return G;}
void Declare(UBlueprint* B,std::initializer_list<const TCHAR*> Names){for(auto N:Names)if(!Graph(B,N)){UK2Node_FunctionEntry* E;Fun(B,N,E);}Compile(B);}
void Invoke(UBlueprint* B,UEdGraph* G,UEdGraphPin*& X,const TCHAR* Fn){Flow(G,X,B->SkeletonGeneratedClass,Fn);}
UEdGraphPin* Abs(UEdGraph* G,UEdGraphPin* A){auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Abs"));Link(A,Pin(C,TEXT("A")));return Pin(C,TEXT("ReturnValue"));}
UEdGraphPin* Select(UEdGraph* G,UEdGraphPin* Condition,UEdGraphPin* A,const TCHAR* B=TEXT("0")){auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("SelectFloat"));Link(Condition,Pin(C,TEXT("bPickA")));Link(A,Pin(C,TEXT("A")));Def(C,TEXT("B"),B);return Pin(C,TEXT("ReturnValue"));}
UEdGraphPin* Scalar(UEdGraph* G,const TCHAR* Value){auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Add_DoubleDouble"));Def(C,TEXT("A"),Value);Def(C,TEXT("B"),TEXT("0"));return Pin(C,TEXT("ReturnValue"));}
UEdGraphPin* Delta(UEdGraph* G){return Pin(Call(G,UGameplayStatics::StaticClass(),TEXT("GetWorldDeltaSeconds")),TEXT("ReturnValue"));}
UEdGraphPin* Interp(UEdGraph* G,UEdGraphPin* Current,UEdGraphPin* Target,const TCHAR* Speed){auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("FInterpTo"));Link(Current,Pin(C,TEXT("Current")));Link(Target,Pin(C,TEXT("Target")));Link(Delta(G),Pin(C,TEXT("DeltaTime")));Def(C,TEXT("InterpSpeed"),Speed);return Pin(C,TEXT("ReturnValue"));}
UEdGraphPin* Self(UEdGraph* G){auto N=Node<UK2Node_Self>(G);N->AllocateDefaultPins();return Pin(N,TEXT("self"));}
UEdGraphPin* Vec(UEdGraph* G,double X,double Y,double Z){auto C=Call(G,UKismetMathLibrary::StaticClass(),TEXT("MakeVector"));Def(C,TEXT("X"),FString::SanitizeFloat(X));Def(C,TEXT("Y"),FString::SanitizeFloat(Y));Def(C,TEXT("Z"),FString::SanitizeFloat(Z));return Pin(C,TEXT("ReturnValue"));}
void Append(UBlueprint* B,UEdGraph* G,const TCHAR* Fn){auto E=Entry(G);auto Old=Pin(E,TEXT("then"));if(Old->LinkedTo.Num()){auto Next=Old->LinkedTo[0];Old->BreakAllPinLinks();auto X=Old;Invoke(B,G,X,Fn);Link(X,Next);}}
void AfterNode(UBlueprint* B,UEdGraph* G,UEdGraphNode* N,const TCHAR* Fn){auto P=Pin(N,TEXT("then"));TArray<UEdGraphPin*> Next=P->LinkedTo;P->BreakAllPinLinks();auto X=P;Invoke(B,G,X,Fn);for(auto Q:Next)Link(X,Q);}
void BeforeNode(UBlueprint* B,UEdGraph* G,UEdGraphNode* N,const TCHAR* Fn){auto P=Pin(N,TEXT("execute"));check(P->LinkedTo.Num()==1);auto X=P->LinkedTo[0];P->BreakAllPinLinks();Invoke(B,G,X,Fn);Link(X,P);}
UEdGraphPin* ArrayLength(UEdGraph* G,UEdGraphPin* A){auto C=Node<UK2Node_CallArrayFunction>(G);C->SetFromFunction(UKismetArrayLibrary::StaticClass()->FindFunctionByName(TEXT("Array_Length")));C->AllocateDefaultPins();Link(A,Pin(C,TEXT("TargetArray")));return Pin(C,TEXT("ReturnValue"));}
UEdGraphPin* ArrayItem(UEdGraph* G,UEdGraphPin* A,UEdGraphPin* I){auto C=Node<UK2Node_GetArrayItem>(G);C->AllocateDefaultPins();Link(A,C->GetTargetArrayPin());Link(I,C->GetIndexPin());return C->GetResultPin();}
void Layout(UBlueprint* B){TArray<UEdGraph*> GS;B->GetAllGraphs(GS);for(auto G:GS){int I=0;for(auto N:G->Nodes){N->NodePosX=(I%6)*480;N->NodePosY=(I/6)*320;I++;}}}
}
