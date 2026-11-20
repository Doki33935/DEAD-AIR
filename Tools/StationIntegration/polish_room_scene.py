"""UE 5.0.3: apply the requested room placement without rebuilding artist layouts."""
import json
import os
import unreal as u

assert u.EditorLevelLibrary.load_level('/Game/StationIntegration/StationIntegrationMap')
world = u.EditorLevelLibrary.get_editor_world()
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
all_actors = actors.get_all_level_actors()
radio = next(a for a in all_actors if a.get_class().get_name() == 'BP_RadioIntegrated_C')
rig = next(a for a in all_actors if a.get_class().get_name() == 'BP_DeskWorkstationRig_C')

# These are the three decorative copies of the same scanned cassette. The live
# BP_CassetteIntegrated now uses that asset and remains the sole physical tape.
removed = []
for actor in all_actors:
    if not isinstance(actor, u.StaticMeshActor):
        continue
    mesh = actor.static_mesh_component.static_mesh
    if mesh and (mesh.get_name() == 'SM_low_poly_vhs_tape_free_3d_model_248e7d'
                 or mesh.get_path_name() == '/Game/25_Office_Desktop_Items/Models/SM_computer_mouse.SM_computer_mouse'):
        removed.append(actor.get_actor_label())
        actors.destroy_actor(actor)

# Seat the thinner scanned shell above the existing well floor/glass plane.
deck = next(a for a in all_actors if a.get_class().get_name() == 'BP_TapeDeckIntegrated_C')
socket = next(c for c in deck.get_components_by_class(u.SceneComponent) if c.get_name() == 'CassetteSocket')
pos = socket.get_editor_property('relative_location')
socket.set_relative_location(u.Vector(pos.x, pos.y, 14.22), False, True)

# Match the submitted radio's case height, centre and front direction. Move the
# whole actor so screens, hitboxes, knobs and power animation remain together.
body = next(c for c in radio.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'Radio_Body')
lo, hi = body.get_local_bounds()
case_height = (hi.z - lo.z) * body.get_editor_property('relative_scale3d').z
scale = 20.0 * 1.6583936214447021 / case_height
radio.set_actor_scale3d(u.Vector(scale, scale, scale))
radio.set_actor_rotation(u.Rotator(pitch=0, yaw=150, roll=0), True)
centre, extent, radius = u.SystemLibrary.get_component_bounds(body)
desired = u.Vector(506, 414, 107)
radio.set_actor_location(radio.get_actor_location() + desired - centre, False, True)

front = u.Vector(-.5, -.866025403784, 0)
eye = desired + front * 120 + u.Vector(0, 0, 12)
target = desired + front * 25
view = next(c for c in rig.get_components_by_class(u.ChildActorComponent) if c.get_name() == 'ViewRadio')
view.set_world_location(eye, False, True)
view.set_world_rotation(u.MathLibrary.find_look_at_rotation(eye, target), False, True)

lamp = next(a for a in all_actors if a.get_actor_label() == 'ROOM — ceilinglamp')
lamp.set_actor_location(u.Vector(275, 225, 174.56), False, True)
lamp_centre, lamp_extent, _ = u.SystemLibrary.get_component_bounds(lamp.static_mesh_component)
bulb_position = u.Vector(275, 225, lamp_centre.z - lamp_extent.z - 1.5)

for actor in all_actors:
    if actor.get_actor_label() in ['ROOM — desk lamp light', 'ROOM — ceiling warm light']:
        actors.destroy_actor(actor)
    elif actor.get_actor_label() == 'ROOM — window cool light':
        actor.light_component.set_intensity(.04)

# A small visible warm bulb, directly beneath the existing ceiling fitting.
material_path = '/Game/RoomArt/Materials/M_RoomBulb'
material = u.load_asset(material_path)
if not material:
    material = u.AssetToolsHelpers.get_asset_tools().create_asset('M_RoomBulb', '/Game/RoomArt/Materials', u.Material, u.MaterialFactoryNew())
u.MaterialEditingLibrary.delete_all_material_expressions(material)
color = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionConstant3Vector)
color.set_editor_property('constant', u.LinearColor(6, 2.9, .9, 1))
assert u.MaterialEditingLibrary.connect_material_property(color, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
u.MaterialEditingLibrary.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material, False)

for actor in actors.get_all_level_actors():
    if actor.get_actor_label() in ['ROOM — central dim bulb', 'ROOM — visible central bulb']:
        actors.destroy_actor(actor)
bulb = actors.spawn_actor_from_class(u.StaticMeshActor, bulb_position)
bulb.set_actor_label('ROOM — visible central bulb')
bulb.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Sphere'))
bulb.static_mesh_component.set_material(0, material)
bulb.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
bulb.static_mesh_component.set_cast_shadow(False)
bulb.set_actor_scale3d(u.Vector(.025, .025, .035))
light = actors.spawn_actor_from_class(u.PointLight, bulb_position)
light.set_actor_label('ROOM — central dim bulb')
light.light_component.set_mobility(u.ComponentMobility.MOVABLE)
light.light_component.set_intensity(6.0)
light.light_component.set_light_color(u.LinearColor(1, .63, .32, 1))
light.light_component.set_editor_property('attenuation_radius', 650)
light.light_component.set_editor_property('source_radius', 2)

assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/StationIntegration/StationIntegrationMap')
assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/RoomIntegration/RoomIntegrationMap')
report = {'removed_decorations': removed, 'radio_case_centre': [506, 414, 107],
          'radio_yaw': 150, 'radio_uniform_scale': scale,
          'bulb_position': [bulb_position.x, bulb_position.y, bulb_position.z], 'bulb_candelas': 6.0}
root = os.path.join(u.Paths.project_saved_dir(), 'RoomImport')
with open(os.path.join(root, 'room-polish-placement.json'), 'w', encoding='utf-8') as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
u.log('ROOM POLISH SCENE COMPLETE %s' % report)
