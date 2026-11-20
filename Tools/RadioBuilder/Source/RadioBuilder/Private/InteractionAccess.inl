// Verification calls the production controller API; never simulates removed UI shortcuts.
namespace InteractionAccess {
APlayerController* Control(UObject* Context){return UGameplayStatics::GetPlayerController(Context,0);}
UObject* Property(UObject* Object,const TCHAR* Name){auto P=Object?FindFProperty<FObjectProperty>(Object->GetClass(),Name):nullptr;return P?P->GetObjectPropertyValue_InContainer(Object):nullptr;}
AActor* Find(UObject* Context,const TCHAR* ClassName){auto W=Context->GetWorld();for(TActorIterator<AActor> I(W);I;++I)if(I->GetClass()->GetName()==ClassName)return *I;return nullptr;}
bool FocusActor(UObject* Context,AActor* Instrument){auto PC=Control(Context);auto F=PC->FindFunction(TEXT("FocusInstrument"));check(F);struct {AActor* Target;bool Result;} Args{Instrument,false};PC->ProcessEvent(F,&Args);return Args.Result;}
bool Focus(UObject* Context,const TCHAR* ClassName){return FocusActor(Context,Find(Context,ClassName));}
bool FocusHovered(UObject* Context){return FocusActor(Context,Cast<AActor>(Property(Control(Context),TEXT("HoveredInstrument"))));}
void Back(UObject* Context){auto PC=Control(Context);auto F=PC->FindFunction(TEXT("ReturnToDesk"));check(F);PC->ProcessEvent(F,nullptr);}
void Gaze(UObject* Context){auto PC=Control(Context);PC->PlayerCameraManager->UpdateCamera(.016f);auto F=PC->FindFunction(TEXT("UpdateInstrumentGaze"));check(F);PC->ProcessEvent(F,nullptr);}
}
