"""Apply the latest desk arrangement to existing UE5.0 maps, preserving devices."""
import json
import os
import unreal as u

assert u.EditorLevelLibrary.load_level('/Game/StationIntegration/StationIntegrationMap')
world = u.EditorLevelLibrary.get_editor_world()
editor = u.get_editor_subsystem(u.EditorActorSubsystem)
actors = editor.get_all_level_actors()
rig = next(a for a in actors if a.get_class().get_name() == 'BP_DeskWorkstationRig_C')
deck = next(a for a in actors if a.get_class().get_name() == 'BP_TapeDeckIntegrated_C')
deck.set_actor_location(u.Vector(396, 412, deck.get_actor_location().z), False, True)
tray = next(c for c in deck.get_components_by_class(u.SceneComponent) if c.get_name() == 'CassetteTray')
old = tray.get_editor_property('relative_location')
tray.set_relative_location(u.Vector(12, -35, old.z), False, True)
view = next(c for c in rig.get_components_by_class(u.ChildActorComponent) if c.get_name() == 'ViewCenter')
view.set_relative_location(u.Vector(94, 117, 112), False, True)
view.set_relative_rotation(u.Rotator(pitch=-10.5, yaw=-108, roll=0), False, True)
focus = next(c for c in rig.get_components_by_class(u.ChildActorComponent) if c.get_name() == 'ViewDeck')
eye, target = u.Vector(386, 344, 145), u.Vector(386, 412, 95)
focus.set_world_location(eye, False, True)
focus.set_world_rotation(u.MathLibrary.find_look_at_rotation(eye, target), False, True)

for actor in actors:
    if actor.get_actor_label() in ['ROOM — tiny desk lamp', 'ROOM — CC0 keyboard']:
        editor.destroy_actor(actor)
lamp = editor.spawn_actor_from_class(u.PointLight, u.Vector(460, 418, 128))
lamp.set_actor_label('ROOM — tiny desk lamp')
lamp.light_component.set_mobility(u.ComponentMobility.MOVABLE)
lamp.light_component.set_intensity(.06)
lamp.light_component.set_light_color(u.LinearColor(.95, .86, .72, 1))
lamp.light_component.set_editor_property('attenuation_radius', 70)
lamp.light_component.set_editor_property('source_radius', .8)

mesh = u.load_asset('/Game/RoomArt/Keyboard/SM_KeyboardCC0')
assert mesh
keyboard = editor.spawn_actor_from_class(u.StaticMeshActor, u.Vector(493, 315, 90.14))
keyboard.set_actor_label('ROOM — CC0 keyboard')
keyboard.static_mesh_component.set_static_mesh(mesh)
keyboard.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)

dish = u.load_asset('/Game/RoomArt/DishLatest/SM_DishLatestClean')
steel = u.load_asset('/Game/RoomArt/DishLatest/M_DishMetal')
assert dish and steel
replaced = 0
for actor in actors:
    if isinstance(actor, u.StaticMeshActor) and actor.static_mesh_component.static_mesh:
        comp = actor.static_mesh_component
        if comp.static_mesh.get_name() in ['SM_Тарелка_NEW_3_935ca5', 'SM_DishLatestClean']:
            comp.set_static_mesh(dish)
            for i in range(comp.get_num_materials()):
                comp.set_material(i, steel)
            actor.set_actor_label('ROOM — satellite dish Yarik NEW4')
            replaced += 1
assert replaced == 1

# Rectangles are checked against the notebook, pencil holder and organisers.
body = next(c for c in deck.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'Base_dec')
centre, extent, _ = u.SystemLibrary.get_component_bounds(body)
for actor in editor.get_all_level_actors():
    if isinstance(actor, u.StaticMeshActor) and actor.static_mesh_component.static_mesh:
        c = actor.static_mesh_component
        if c.static_mesh.get_name() in ['SM_single_spiral_notepad_c51708', 'SM_pencil_holder', 'SM_document_tray']:
            p, e, _ = u.SystemLibrary.get_component_bounds(c)
            assert abs(centre.x - p.x) > extent.x + e.x or abs(centre.y - p.y) > extent.y + e.y, actor.get_actor_label()

assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/StationIntegration/StationIntegrationMap')
assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/RoomIntegration/RoomIntegrationMap')
report = {'deck_body_centre': [centre.x, centre.y, centre.z], 'cassette_tray_side': 'left',
          'overview_height_cm': -6.5 + 112 * 1.28405738, 'desk_light_candelas': .06,
          'keyboard_position': [493, 315, 90.14], 'dish_actors_replaced': replaced}
with open(os.path.join(u.Paths.project_saved_dir(), 'RoomImport', 'desk-refinement.json'), 'w', encoding='utf-8') as stream:
    json.dump(report, stream, indent=2)
u.log('DESK REFINEMENT SCENE COMPLETE %s' % report)
