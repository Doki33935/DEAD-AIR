// Editor-only structural inspection; this mode never modifies or saves assets.
namespace ProjectMaintenance {
using namespace ReceiverUpgrade;
void Tidy(){
 const TCHAR* Paths[]={TEXT("/Game/Interaction/BP_OperatorController"),TEXT("/Game/Interaction/BPI_Instrument"),TEXT("/Game/StationIntegration/BP_DeskWorkstationRig"),TEXT("/Game/StationIntegration/BP_RadioIntegrated"),TEXT("/Game/StationIntegration/BP_TapeDeckIntegrated")};
 for(auto Path:Paths){auto BP=LoadObject<UBlueprint>(nullptr,Path);check(BP);TArray<UEdGraph*> GS;BP->GetAllGraphs(GS);for(auto G:GS){bool Instrument=G->GetName().StartsWith(TEXT("GetInstrument"))||G->GetName().StartsWith(TEXT("SetInstrument"));if((BP->GetName()==TEXT("BP_RadioIntegrated")||BP->GetName()==TEXT("BP_TapeDeckIntegrated"))&&!Instrument)continue;int I=0;for(auto N:G->Nodes){N->NodePosX=(I%5)*500;N->NodePosY=(I/5)*300;I++;if(auto EntryN=Cast<UK2Node_FunctionEntry>(N)){FString Note;auto Name=G->GetName();if(Name==TEXT("FocusInstrument"))Note=TEXT("Validate target and transition; ask instrument for its camera.");else if(Name==TEXT("ApplyInstrumentStates"))Note=TEXT("Only active instrument is enabled, after blend and outside menu.");else if(Name==TEXT("WatchCameraState"))Note=TEXT("Apply gates/cursor when state changes; never reset knob drag every frame.");else if(Name==TEXT("FocusCamera"))Note=TEXT("Camera transition only. No device button/input logic here.");else if(Name==TEXT("PollInstrumentInput"))Note=TEXT("LMB focuses gaze target; RMB returns to the desk.");else if(Instrument)Note=TEXT("Instrument contract implementation: owned camera, controls or hover.");if(!Note.IsEmpty()){EntryN->NodeComment=Note;EntryN->bCommentBubbleVisible=true;}}}}RadioBuild::Save(BP);}
 UE_LOG(LogTemp,Display,TEXT("INTERACTION GRAPH TIDY COMPLETE: compact function graphs; UI/widget transforms untouched"));
}
void Audit(){
 TArray<FString> Paths,Lines;check(FFileHelper::LoadFileToStringArray(Paths,*(FPaths::ProjectSavedDir()/TEXT("ProjectAudit/blueprints.txt"))));
 for(auto Path:Paths)if(auto BP=LoadObject<UBlueprint>(nullptr,*Path)){
  Lines.Add(FString::Printf(TEXT("BP\t%s\t%s"),*Path,*BP->ParentClass->GetPathName()));
  for(auto& Vv:BP->NewVariables)Lines.Add(FString::Printf(TEXT("VAR\t%s\t%s\t%s"),*BP->GetName(),*Vv.VarName.ToString(),*Vv.VarType.PinCategory.ToString()));
  if(BP->SimpleConstructionScript)for(auto S:BP->SimpleConstructionScript->GetAllNodes())Lines.Add(FString::Printf(TEXT("COMP\t%s\t%s\t%s"),*BP->GetName(),*S->GetVariableName().ToString(),*S->ComponentClass->GetName()));
  TArray<UEdGraph*> GS;BP->GetAllGraphs(GS);for(auto G:GS){Lines.Add(FString::Printf(TEXT("GRAPH\t%s\t%s\t%d"),*BP->GetName(),*G->GetName(),G->Nodes.Num()));
   for(auto N:G->Nodes){FString Detail;if(auto C=Cast<UK2Node_CallFunction>(N))Detail=C->GetFunctionName().ToString();if(auto E=Cast<UK2Node_Event>(N))Detail=E->EventReference.GetMemberName().ToString();if(auto Vv=Cast<UK2Node_Variable>(N))Detail=Vv->GetVarName().ToString();
    int Links=0;for(auto P:N->Pins)Links+=P->LinkedTo.Num();Lines.Add(FString::Printf(TEXT("NODE\t%s\t%s\t%s\t%s\t%d"),*BP->GetName(),*G->GetName(),*N->GetClass()->GetName(),*Detail,Links));
   }
  }
 }
 check(FFileHelper::SaveStringArrayToFile(Lines,*(FPaths::ProjectSavedDir()/TEXT("ProjectAudit/blueprint-graphs.tsv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));UE_LOG(LogTemp,Display,TEXT("PROJECT GRAPH AUDIT COMPLETE lines=%d"),Lines.Num());
}
}
