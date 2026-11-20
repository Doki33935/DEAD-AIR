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
color.set_editor_property('constant', u.LinearColor(1.6, 1.75, 1.95, 1))
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
light.light_component.set_intensity(1.25)
light.light_component.set_light_color(u.LinearColor(.82, .86, 1, 1))
light.light_component.set_editor_property('attenuation_radius', 650)
light.light_component.set_editor_property('source_radius', 2)

# Match the submitted complete deck (240 x 160 cm at source scale .1543414).
# The working FBX is already normalised; scaling it by the desk rig again had
# made it almost twice as large. Keep the free cassette at its physical size.
import math
tape = next(a for a in all_actors if a.get_class().get_name() == 'BP_CassetteIntegrated_C')
tape_scale = tape.get_actor_scale3d()
case = next(c for c in deck.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'Base_dec')
lo, hi = case.get_local_bounds()
deck_scale = 240 * .15434139966964722 / (hi.x - lo.x)
deck.set_actor_scale3d(u.Vector(deck_scale, deck_scale, deck_scale))
deck.set_actor_rotation(u.Rotator(pitch=0, yaw=-86, roll=0), True)
source_offset = -171.3317755 * .15434139966964722
deck_centre = u.Vector(436 + math.cos(math.radians(-86)) * source_offset,
                       401 + math.sin(math.radians(-86)) * source_offset, 94)
centre, _, _ = u.SystemLibrary.get_component_bounds(case)
deck.set_actor_location(deck.get_actor_location() + deck_centre - centre, False, True)
tape.set_actor_scale3d(tape_scale)

glass = next(c for c in deck.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'Detail_Стекло')
gc, ge, _ = u.SystemLibrary.get_component_bounds(glass)
shell = next(c for c in tape.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'CassetteShell')
sl, _ = shell.get_local_bounds()
shell_bottom = sl.z * shell.get_editor_property('relative_scale3d').z + shell.get_editor_property('relative_location').z
socket_pos = socket.get_editor_property('relative_location')
socket_z = (gc.z + ge.z + .1 - deck.get_actor_location().z - shell_bottom * tape_scale.z) / deck_scale
socket.set_relative_location(u.Vector(socket_pos.x, socket_pos.y, socket_z), False, True)
tray = next(c for c in deck.get_components_by_class(u.SceneComponent) if c.get_name() == 'CassetteTray')
tray_z = (90.1 - deck.get_actor_location().z - shell_bottom * tape_scale.z) / deck_scale
tray.set_relative_location(u.Vector(12, 35, tray_z), False, True)

overview = next(c for c in rig.get_components_by_class(u.ChildActorComponent) if c.get_name() == 'ViewCenter')
overview.set_relative_location(u.Vector(94, 117, 104), False, True)
overview.set_relative_rotation(u.Rotator(pitch=-10.5, yaw=-108, roll=0), False, True)


def simple_material(name, color_value, opacity=None):
    mat = u.load_asset('/Game/RoomArt/Materials/' + name)
    if not mat:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name, '/Game/RoomArt/Materials', u.Material, u.MaterialFactoryNew())
    lib = u.MaterialEditingLibrary
    lib.delete_all_material_expressions(mat)
    col = lib.create_material_expression(mat, u.MaterialExpressionConstant3Vector)
    col.set_editor_property('constant', u.LinearColor(*color_value, 1))
    if opacity is None:
        mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
        lib.connect_material_property(col, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        mat.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
        lib.connect_material_property(col, '', u.MaterialProperty.MP_BASE_COLOR)
        for prop, value in [(u.MaterialProperty.MP_OPACITY, opacity),
                            (u.MaterialProperty.MP_ROUGHNESS, .12),
                            (u.MaterialProperty.MP_SPECULAR, .12)]:
            expr = lib.create_material_expression(mat, u.MaterialExpressionConstant)
            expr.set_editor_property('r', value)
            assert lib.connect_material_property(expr, '', prop)
    mat.set_editor_property('two_sided', True)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
    return mat


clear_glass = simple_material('M_RoomClearGlass', [.006, .009, .012], .025)
window_count = 0
for actor in actors.get_all_level_actors():
    if isinstance(actor, u.StaticMeshActor) and actor.static_mesh_component.static_mesh:
        comp = actor.static_mesh_component
        if comp.static_mesh.get_name() == 'SM_Оконная_Рама_Куб_191755':
            comp.set_material(0, clear_glass)
            comp.set_cast_shadow(False)
            comp.set_editor_property('affect_distance_field_lighting', False)
            window_count += 1
    if isinstance(actor, u.SkyLight):
        actor.light_component.set_editor_property('intensity', .012)
    if actor.get_actor_label() == 'ROOM — window cool light':
        actor.light_component.set_intensity(.008)
    if isinstance(actor, u.PostProcessVolume):
        settings = actor.get_editor_property('settings')
        settings.set_editor_property('override_color_saturation', True)
        settings.set_editor_property('color_saturation', u.Vector4(.52, .52, .52, 1))
        actor.set_editor_property('settings', settings)

# A very faint technical night backdrop makes transmission through the panes
# visible. The room's supplied snow hill remains the only exterior geometry.
sky_mat = simple_material('M_RoomNightBackdrop', [.006, .009, .015])
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == 'ROOM — night backdrop':
        actors.destroy_actor(actor)
night = actors.spawn_actor_from_class(u.StaticMeshActor, u.Vector(280, 230, 0))
night.set_actor_label('ROOM — night backdrop')
night.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Sphere'))
night.static_mesh_component.set_material(0, sky_mat)
night.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
night.static_mesh_component.set_cast_shadow(False)
night.static_mesh_component.set_editor_property('affect_distance_field_lighting', False)
night.set_actor_scale3d(u.Vector(2000, 2000, 2000))

assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/StationIntegration/StationIntegrationMap')
assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/RoomIntegration/RoomIntegrationMap')
report = {'removed_decorations': removed, 'radio_case_centre': [506, 414, 107],
          'radio_yaw': 150, 'radio_uniform_scale': scale,
          'bulb_position': [bulb_position.x, bulb_position.y, bulb_position.z], 'bulb_candelas': 1.25,
          'deck_scale': deck_scale, 'deck_centre': [deck_centre.x, deck_centre.y, deck_centre.z],
          'cassette_socket_local_z': socket_z, 'cassette_tray_local_z': tray_z,
          'transparent_windows': window_count, 'overview_height_cm': 127.04196752}
root = os.path.join(u.Paths.project_saved_dir(), 'RoomImport')
with open(os.path.join(root, 'room-polish-placement.json'), 'w', encoding='utf-8') as stream:
    json.dump(report, stream, ensure_ascii=False, indent=2)
u.log('ROOM POLISH SCENE COMPLETE %s' % report)
