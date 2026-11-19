// Editor-only integration checks. Use a unique SaveGame namespace; never touch player slots.
namespace MenuVerify {
UObject* Obj(UObject* O,const TCHAR* N){return O?FindFProperty<FObjectProperty>(O->GetClass(),N)->GetObjectPropertyValue_InContainer(O):nullptr;}
double Num(UObject* O,const TCHAR* N){return FindFProperty<FDoubleProperty>(O->GetClass(),N)->GetPropertyValue_InContainer(O);}
void Number(UObject* O,const TCHAR* N,double V){FindFProperty<FDoubleProperty>(O->GetClass(),N)->SetPropertyValue_InContainer(O,V);}
bool Flag(UObject* O,const TCHAR* N){return FindFProperty<FBoolProperty>(O->GetClass(),N)->GetPropertyValue_InContainer(O);}
void Bool(UObject* O,const TCHAR* N,bool V){FindFProperty<FBoolProperty>(O->GetClass(),N)->SetPropertyValue_InContainer(O,V);}
void String(UObject* O,const TCHAR* N,const FString& V){FindFProperty<FStrProperty>(O->GetClass(),N)->SetPropertyValue_InContainer(O,V);}
void Run(UObject* O,const TCHAR* N){check(O&&O->FindFunction(N));O->ProcessEvent(O->FindFunction(N),nullptr);}
bool Tick(){
 static bool Requested=false,PrefixSet=false;static int Stage=0,Checks=0,Failed=0;static double Started=FPlatformTime::Seconds(),At=0;static TWeakObjectPtr<AActor> Previous;
 static FString Prefix=FString::Printf(TEXT("DA_UI_QA_%lld"),FDateTime::Now().GetTicks());
 auto Expect=[&](bool Good,const TCHAR* Label){++Checks;if(!Good)++Failed;UE_LOG(LogTemp,Display,TEXT("MENU VERIFY %s: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Label);};
 auto Finish=[&](){UE_LOG(LogTemp,Display,TEXT("MENU VERIFY COMPLETE: %d checks, %d failures, namespace=%s"),Checks,Failed,*Prefix);Stage=99;At=FPlatformTime::Seconds();GEditor->RequestEndPlayMap();};
 if(Stage==99){if(!GEditor->PlayWorld&&FPlatformTime::Seconds()>At+2){FPlatformMisc::RequestExit(false);return false;}return true;}
 if(FPlatformTime::Seconds()-Started>100){Expect(false,TEXT("Integration timeout"));Finish();return true;}
 if(!Requested){FRequestPlaySessionParams Params;Params.GlobalMapOverride=TEXT("/Game/UI/MainMenuMap");GEditor->RequestPlaySession(Params);Requested=true;return true;}
 auto W=GEditor->PlayWorld;if(!W)return true;
 if(!PrefixSet&&W->GetGameInstance()){String(W->GetGameInstance(),TEXT("StoragePrefix"),Prefix);PrefixSet=true;}
 AActor* M=nullptr;for(TActorIterator<AActor> It(W);It;++It)if(It->GetClass()->GetName()==TEXT("BP_MenuManager_C"))M=*It;
 if(!M||!Obj(M,TEXT("View")))return true;
 auto View=CastChecked<UUserWidget>(Obj(M,TEXT("View")));auto Widget=[&](const TCHAR* N){return View->GetWidgetFromName(N);};
 auto Click=[&](const TCHAR* N){CastChecked<UButton>(Widget(N))->OnClicked.Broadcast();};
 auto Shot=[&](const TCHAR* N){IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("MenuChecks")),true);FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("MenuChecks")/N,true,false);};
 auto Page=[&](){return CastChecked<UWidgetSwitcher>(Widget(TEXT("Pages")))->GetActiveWidgetIndex();};
 auto Modal=[&](){return Widget(TEXT("Modal"))->GetVisibility()==ESlateVisibility::Visible;};
 auto Select=[&](const TCHAR* Suffix){String(M,TEXT("SelectedKey"),Prefix+Suffix);};
 auto Save=[&](const TCHAR* Suffix){Select(Suffix);Run(M,TEXT("CaptureAndSave"));};
 double Now=FPlatformTime::Seconds();
 if(Stage==0){
  String(M,TEXT("StoragePrefix"),Prefix);Run(M,TEXT("InitializePreferences"));Run(M,TEXT("RefreshSlots"));
  Expect(Page()==0&&!Flag(M,TEXT("InGame")),TEXT("Fresh session opens main menu"));
  Expect(!Widget(TEXT("MainContinue"))->GetIsEnabled(),TEXT("Continue disabled without valid autosave"));
  Click(TEXT("MainLoad"));Expect(Page()==2&&!Widget(TEXT("Slot1"))->GetIsEnabled()&&!Widget(TEXT("Slot2"))->GetIsEnabled()&&!Widget(TEXT("Slot3"))->GetIsEnabled(),TEXT("Three empty load slots disabled"));Click(TEXT("SlotsBack"));Expect(Page()==0,TEXT("Back returns to main"));
  Click(TEXT("MainSettings"));Expect(Page()==3,TEXT("Settings opens"));auto P=Obj(M,TEXT("Prefs"));CastChecked<USlider>(Widget(TEXT("MasterSlider")))->OnValueChanged.Broadcast(.37f);CastChecked<USlider>(Widget(TEXT("MasterSlider")))->OnMouseCaptureEnd.Broadcast();CastChecked<UCheckBox>(Widget(TEXT("SubtitlesCheck")))->OnCheckStateChanged.Broadcast(false);Run(M,TEXT("SyncSettings"));Number(P,TEXT("MasterVolume"),1);Bool(P,TEXT("Subtitles"),true);Run(M,TEXT("InitializePreferences"));P=Obj(M,TEXT("Prefs"));Expect(FMath::IsNearlyEqual(Num(P,TEXT("MasterVolume")),.37,.001)&&!Flag(P,TEXT("Subtitles")),TEXT("Preferences round trip through disk"));Click(TEXT("SettingsBack"));
  Click(TEXT("MainExit"));Expect(Modal(),TEXT("Exit requires confirmation"));Click(TEXT("Cancel"));Expect(!Modal()&&Page()==0,TEXT("Cancel exit keeps main menu"));
  At=Now;Stage=1;return true;
 }
 auto VideoTexture=LoadObject<UMediaTexture>(nullptr,TEXT("/Game/UI/Media/MT_MenuBackground.MT_MenuBackground"));
 if(Stage==1&&Now>At+3&&(VideoTexture->GetWidth()==1920||Now>At+15)){
  auto Movie=LoadObject<UMediaPlayer>(nullptr,TEXT("/Game/UI/Media/MP_MenuBackground.MP_MenuBackground"));Expect(Movie&&Movie->IsPlaying()&&Movie->GetVideoTrackDimensions(INDEX_NONE,INDEX_NONE).X>0,TEXT("Video decodes and plays on current RHI"));
  Expect(Movie->GetPlayerName()==TEXT("ElectraPlayer")&&VideoTexture->GetMediaPlayer()==Movie&&VideoTexture->GetWidth()==1920&&VideoTexture->GetHeight()==1080,TEXT("Saved media texture receives decoded frames after reload"));Shot(TEXT("menu-main.png"));At=Now;Stage=11;return true;
 }
 if(Stage==11&&Now>At+1){Click(TEXT("MainSettings"));Shot(TEXT("menu-settings.png"));At=Now;Stage=12;return true;}
 if(Stage==12&&Now>At+1){Click(TEXT("SettingsBack"));Previous=M;Click(TEXT("MainNew"));Stage=2;return true;}
 if(Stage==2&&Previous.Get()!=M&&Flag(M,TEXT("InGame"))){
  auto R=Obj(M,TEXT("Radio"));Expect(R&&!UGameplayStatics::IsGamePaused(W),TEXT("New game reaches playable radio"));Expect(Flag(M,TEXT("AutoAvailable")),TEXT("New game creates valid autosave"));Expect(FMath::IsNearlyEqual(Num(Obj(M,TEXT("Prefs")),TEXT("MasterVolume")),.37,.001),TEXT("Preferences survive level transition"));
  Number(R,TEXT("FineFrequencyKHz"),153450);Number(R,TEXT("Volume"),.73);Number(R,TEXT("Filter"),.42);Bool(R,TEXT("Powered"),true);Run(R,TEXT("RefreshReception"));Run(R,TEXT("UpdatePresentation"));
  Click(TEXT("HUDMenu"));Expect(Page()==1&&UGameplayStatics::IsGamePaused(W),TEXT("Mouse menu pauses game"));Click(TEXT("PauseSave"));Expect(Page()==2&&Flag(M,TEXT("Saving"))&&Widget(TEXT("Slot3"))->GetIsEnabled(),TEXT("Save page enables all three slots"));
  for(int I=1;I<=3;I++){auto Action=FString::Printf(TEXT("ClickSlot%d"),I);Click(*FString::Printf(TEXT("Slot%d"),I));Expect(Flag(M,TEXT("LastSaveSucceeded")),*FString::Printf(TEXT("Manual slot %d saved"),I));}
  Click(TEXT("Slot1"));Expect(Modal(),TEXT("Existing slot prompts overwrite"));Number(R,TEXT("Volume"),.12);Click(TEXT("Cancel"));auto Disk=UGameplayStatics::LoadGameFromSlot(Prefix+TEXT("_Manual_1"),0);Expect(Disk&&FMath::IsNearlyEqual(Num(Disk,TEXT("Volume")),.73,.001),TEXT("Cancel overwrite preserves old file"));
  Click(TEXT("Slot1"));Click(TEXT("Confirm"));Disk=UGameplayStatics::LoadGameFromSlot(Prefix+TEXT("_Manual_1"),0);Expect(Disk&&FMath::IsNearlyEqual(Num(Disk,TEXT("Volume")),.12,.001),TEXT("Confirm overwrite replaces file"));
  Number(R,TEXT("FineFrequencyKHz"),153520);Number(R,TEXT("Volume"),.99);Number(R,TEXT("Filter"),.1);Bool(R,TEXT("Powered"),false);Click(TEXT("SlotsBack"));Click(TEXT("PauseLoad"));Click(TEXT("Slot2"));Expect(FMath::IsNearlyEqual(Num(R,TEXT("FineFrequencyKHz")),153450,.001)&&FMath::IsNearlyEqual(Num(R,TEXT("Volume")),.73,.001)&&FMath::IsNearlyEqual(Num(R,TEXT("Filter")),.42,.001)&&Flag(R,TEXT("Powered")),TEXT("Load restores complete radio snapshot"));Expect(!UGameplayStatics::IsGamePaused(W)&&Widget(TEXT("MenuLayer"))->GetVisibility()==ESlateVisibility::Collapsed,TEXT("Load resumes gameplay"));
  Run(M,TEXT("OpenPause"));Number(R,TEXT("FineFrequencyKHz"),153460);Save(TEXT("_Manual_3"));Disk=UGameplayStatics::LoadGameFromSlot(Prefix+TEXT("_Manual_3"),0);FindFProperty<FIntProperty>(Disk->GetClass(),TEXT("Version"))->SetPropertyValue_InContainer(Disk,99);UGameplayStatics::SaveGameToSlot(CastChecked<USaveGame>(Disk),Prefix+TEXT("_Manual_3"),0);Run(M,TEXT("OpenLoad"));Expect(!Flag(M,TEXT("Slot3Available"))&&!Widget(TEXT("Slot3"))->GetIsEnabled(),TEXT("Unknown save version blocked"));Select(TEXT("_Manual_3"));Run(M,TEXT("RequestLoad"));Expect(!Flag(M,TEXT("Busy"))&&FMath::IsNearlyEqual(Num(R,TEXT("FineFrequencyKHz")),153460,.001),TEXT("Invalid load leaves session intact and unlocks retry"));
  Number(Disk,TEXT("Frequency"),-1);FindFProperty<FIntProperty>(Disk->GetClass(),TEXT("Version"))->SetPropertyValue_InContainer(Disk,1);UGameplayStatics::SaveGameToSlot(CastChecked<USaveGame>(Disk),Prefix+TEXT("_Manual_3"),0);Run(M,TEXT("RefreshSlots"));Expect(!Flag(M,TEXT("Slot3Available")),TEXT("Out of range data blocked"));
  FFileHelper::SaveStringToFile(TEXT("invalid save bytes"),*(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Prefix+TEXT("_Manual_3.sav"))));Run(M,TEXT("RefreshSlots"));Expect(!Flag(M,TEXT("Slot3Available")),TEXT("Truncated file does not crash menu"));
  Bool(M,TEXT("Busy"),true);Select(TEXT("_Manual_2"));Run(M,TEXT("RequestLoad"));Expect(FMath::IsNearlyEqual(Num(R,TEXT("FineFrequencyKHz")),153460,.001),TEXT("Repeated load blocked while busy"));Bool(M,TEXT("Busy"),false);
  Click(TEXT("SlotsBack"));Click(TEXT("PauseMain"));Expect(Modal(),TEXT("Return to main requires confirmation"));Click(TEXT("Cancel"));Expect(Page()==1&&!Modal(),TEXT("Cancel return keeps paused session"));Save(TEXT("_Manual_3"));Run(M,TEXT("OpenSave"));Shot(TEXT("menu-save.png"));At=Now;Stage=21;return true;
 }
 if(Stage==21&&Now>At+1){Click(TEXT("SlotsBack"));Click(TEXT("PauseMain"));Shot(TEXT("menu-confirm.png"));At=Now;Stage=22;return true;}
 if(Stage==22&&Now>At+1){Previous=M;Click(TEXT("Confirm"));Stage=3;return true;}
 if(Stage==3&&Previous.Get()!=M&&!Flag(M,TEXT("InGame"))){
  Expect(Page()==0&&Widget(TEXT("MainContinue"))->GetIsEnabled(),TEXT("Return saves and enables Continue"));Run(M,TEXT("RequestNewGame"));Expect(Modal(),TEXT("New game warns about replacing autosave"));Run(M,TEXT("CancelAction"));Expect(!Modal()&&Widget(TEXT("MainContinue"))->GetIsEnabled(),TEXT("Cancel new game preserves Continue"));Previous=M;Click(TEXT("MainContinue"));Stage=4;return true;
 }
 if(Stage==4&&Previous.Get()!=M&&Flag(M,TEXT("InGame"))){
  auto R=Obj(M,TEXT("Radio"));Expect(FMath::IsNearlyEqual(Num(R,TEXT("FineFrequencyKHz")),153460,.001)&&!UGameplayStatics::IsGamePaused(W),TEXT("Continue restores last session across maps"));Expect(!Flag(Obj(M,TEXT("Prefs")),TEXT("Subtitles")),TEXT("Subtitle setting persists across maps"));
  Finish();return true;
 }
 return true;
}
}
