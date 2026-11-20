"""Assign the real controller to all shipped/review maps; preserve scene transforms."""
import unreal as u
mode=u.load_class(None,'/Game/Interaction/BP_StationGameMode.BP_StationGameMode_C')
assert mode
for path in ['/Game/UI/MainMenuMap','/Game/StationIntegration/StationIntegrationMap','/Game/RoomIntegration/RoomIntegrationMap']:
 world=u.EditorLoadingAndSavingUtils.load_map(path)
 world.get_world_settings().set_editor_property('default_game_mode',mode)
 assert u.EditorLoadingAndSavingUtils.save_map(world,path)
u.log('INTERACTION MAP INSTALL COMPLETE')
