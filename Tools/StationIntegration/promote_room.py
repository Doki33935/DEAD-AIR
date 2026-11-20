"""Save the reviewed room at the existing gameplay map address (UE 5.0.3)."""
import unreal as u

assert u.EditorLevelLibrary.load_level('/Game/RoomIntegration/RoomIntegrationMap')
world = u.EditorLevelLibrary.get_editor_world()
assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/StationIntegration/StationIntegrationMap')
u.log('ROOM GAMEPLAY MAP PROMOTED')
