"""UE 5.0.3 editor authoring: preserve the workstation and add recovered room placement."""
import json
import math
import os
import unreal as u

root = os.path.abspath(os.path.join(u.Paths.project_saved_dir(), 'RoomImport'))
data = json.load(open(os.path.join(root, 'room-ready.json'), encoding='utf-8'))
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
assert u.EditorLevelLibrary.load_level('/Game/StationIntegration/StationIntegrationMap')
world = u.EditorLevelLibrary.get_editor_world()
mesh_map = {m['package']: m for m in data['meshes']}
origin = [3480., 2250., 815.]


def v(values):
    return u.Vector(*values)


def actor_class(actor):
    return actor.get_class().get_name()


def material(source):
    if not source:
        return None
    path = source.split('.')[0]
    return u.load_asset(data['material_map'].get(path, path))


current = actors.get_all_level_actors()
rig = next(a for a in current if actor_class(a) == 'BP_DeskWorkstationRig_C')
radio = next(a for a in current if actor_class(a) == 'BP_RadioIntegrated_C')
deck = next(a for a in current if actor_class(a) == 'BP_TapeDeckIntegrated_C')
for actor in current:
    if isinstance(actor, (u.StaticMeshActor, u.DirectionalLight, u.SkyLight, u.PointLight, u.PostProcessVolume, u.SkyAtmosphere)):
        actors.destroy_actor(actor)

desk_component = next(c for c in rig.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'Desk')
desk_component.set_visibility(False, True)
desk_component.set_hidden_in_game(True, True)
desk_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)

# The old workstation uses the same desk in a normalized coordinate frame.
# Move the cohesive rig, including instrument cameras and child instruments.
rig.set_actor_scale3d(u.Vector(1.28405738, 1.28405738, 1.28405738))
rig.set_actor_location_and_rotation(u.Vector(555.15, 451.32, -6.5), u.Rotator(pitch=0, yaw=180, roll=0), False, True)
rig.set_actor_label('WORKSTATION — integrated radio and deck')

placed = []
for row in data['actors']:
    if row['component'] != 'StaticMeshComponent' or not row.get('mesh'):
        continue
    source = row['mesh'].split('.')[0]
    if source in ('/Game/radio2', '/Game/MAGNITOFFFFON2'):
        continue
    info = mesh_map.get(source)
    mesh = u.load_asset(info['asset'] if info else source)
    assert isinstance(mesh, u.StaticMesh), source
    actor = actors.spawn_actor_from_class(u.StaticMeshActor, v([row['location'][i] - origin[i] for i in range(3)]), u.Rotator(pitch=row['rotation'][0], yaw=row['rotation'][1], roll=row['rotation'][2]))
    actor.set_actor_scale3d(v(row['scale']))
    actor.set_actor_label('ROOM — ' + row['label'])
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    comp.set_mobility(u.ComponentMobility.STATIC)
    comp.set_collision_enabled(u.CollisionEnabled.QUERY_ONLY)
    comp.set_collision_response_to_all_channels(u.CollisionResponseType.ECR_IGNORE)
    # Walls block gaze; small decorative props do not steal instrument clicks.
    wall = source.startswith('/Engine/BasicShapes/')
    comp.set_collision_response_to_channel(u.CollisionChannel.ECC_VISIBILITY,
                                           u.CollisionResponseType.ECR_BLOCK if wall else u.CollisionResponseType.ECR_IGNORE)
    source_slots = info.get('slots', []) if info else []
    overrides = row.get('materials', [])
    for i, slot in enumerate(mesh.get_editor_property('static_materials')):
        source_index = next((j for j, s in enumerate(source_slots) if s['name'] == str(slot.material_slot_name)), i)
        mat = material(overrides[source_index]) if source_index < len(overrides) else None
        if mat:
            comp.set_material(i, mat)
    # The archive has unassigned slots on the custom window glass.
    if source == '/Game/Оконная_Рама_Куб':
        glass = u.load_asset('/Game/StationIntegration/Art/Materials/M_DeckGlass')
        if glass:
            comp.set_material(0, glass)
        comp.set_cast_shadow(False)
    placed.append({'label': row['label'], 'mesh': mesh.get_path_name(),
                   'location': [row['location'][i] - origin[i] for i in range(3)]})

sun = actors.spawn_actor_from_class(u.DirectionalLight, u.Vector(0, 0, 500), u.Rotator(pitch=-25, yaw=-35, roll=0))
sun.set_actor_label('ROOM — cold exterior fill')
sun.light_component.set_editor_property('intensity', .04)
sun.light_component.set_light_color(u.LinearColor(.48, .63, 1, 1))
sky = actors.spawn_actor_from_class(u.SkyLight, u.Vector(280, 230, 300))
sky.set_actor_label('ROOM — ambient skylight')
sky.light_component.set_editor_property('intensity', .08)
sky.light_component.set_editor_property('real_time_capture', True)
for label, position, intensity, color, radius in [
        ('ceiling warm light', [280, 260, 240], 2.2, [1, .77, .52, 1], 850),
        ('desk lamp light', [430, 416, 150], .72, [1, .7, .38, 1], 250),
        ('window cool light', [550, 352, 170], .48, [.28, .48, 1, 1], 450)]:
    lamp = actors.spawn_actor_from_class(u.PointLight, v(position))
    lamp.set_actor_label('ROOM — ' + label)
    lamp.light_component.set_intensity(intensity)
    lamp.light_component.set_light_color(u.LinearColor(*color))
    lamp.light_component.set_editor_property('attenuation_radius', radius)

volume = actors.spawn_actor_from_class(u.PostProcessVolume, u.Vector(280, 230, 130))
volume.set_actor_label('ROOM — fixed exposure')
volume.set_editor_property('unbound', True)
settings = volume.get_editor_property('settings')
settings.set_editor_property('override_auto_exposure_min_brightness', True)
settings.set_editor_property('override_auto_exposure_max_brightness', True)
settings.set_editor_property('auto_exposure_min_brightness', 1.0)
settings.set_editor_property('auto_exposure_max_brightness', 1.0)
volume.set_editor_property('settings', settings)
atmosphere = actors.spawn_actor_from_class(u.SkyAtmosphere, u.Vector(0, 0, 0))
atmosphere.set_actor_label('ROOM — exterior atmosphere')
assert u.EditorLoadingAndSavingUtils.save_map(world, '/Game/RoomIntegration/RoomIntegrationMap')
with open(os.path.join(root, 'placed-room.json'), 'w', encoding='utf-8') as stream:
    json.dump({'models_placed': placed, 'replaced_by_working_blueprints': ['radio2', 'MAGNITOFFFFON2'],
               'map': '/Game/RoomIntegration/RoomIntegrationMap'}, stream, ensure_ascii=False, indent=2)
u.log('ROOM SCENE READY: %d placed models + working radio and deck' % len(placed))
