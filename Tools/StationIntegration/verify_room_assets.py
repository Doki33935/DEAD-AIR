"""UE 5.0.3 editor checks for native texture decoding and room proportions."""
import json
import os
import unreal as u

root = os.path.join(u.Paths.project_saved_dir(), 'RoomImport')
data = json.load(open(os.path.join(root, 'room-assets.json'), encoding='utf-8'))
checks = 0


def expect(value, message):
    global checks
    assert value, message
    checks += 1
    u.log('ROOM ASSET VERIFY PASS: ' + message)


decoded = 0
for row in data['textures']:
    if row.get('native_png_bgra'):
        texture = u.load_asset(row['source'])
        expect(texture is not None and u.EditorAssetLibrary.get_metadata_tag(texture, 'DeadAir.SourceDecode') == row['decoded_sha256'],
               'Native BGRA PNG decoded: ' + row['source'])
        decoded += 1
expect(decoded == 42, '42 BGRA PNG sources corrected; grayscale PNG/JPEG are preserved')

box = u.load_asset('/Game/RoomArt/Materials/M_MI_uewlfg2va_83d854')
base = u.MaterialEditingLibrary.get_material_property_input_node(box, u.MaterialProperty.MP_BASE_COLOR)
expect(base.get_editor_property('texture').get_name() == 'T_uewlfg2va_2K_B', 'Cardboard uses its scanned albedo')
metal = u.MaterialEditingLibrary.get_material_property_input_node(box, u.MaterialProperty.MP_METALLIC)
expect(metal.get_editor_property('r') == 0, 'Cardboard material is dielectric')

assert u.EditorLevelLibrary.load_level('/Game/StationIntegration/StationIntegrationMap')
actors = u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
deck = next(a for a in actors if a.get_class().get_name() == 'BP_TapeDeckIntegrated_C')
body = next(c for c in deck.get_components_by_class(u.StaticMeshComponent) if c.get_name() == 'Base_dec')
lo, hi = body.get_local_bounds()
expect(abs((hi.x - lo.x) * deck.get_actor_scale3d().x - 240 * .15434139966964722) < .01,
       'Deck case width matches the submitted UE5.4 placement')
expect(abs((hi.y - lo.y) * deck.get_actor_scale3d().y - 160 * .15434139966964722) < .01,
       'Deck case depth matches the submitted UE5.4 placement')

rig = next(a for a in actors if a.get_class().get_name() == 'BP_DeskWorkstationRig_C')
view = next(c for c in rig.get_components_by_class(u.ChildActorComponent) if c.get_name() == 'ViewCenter')
expect(abs(view.get_editor_property('relative_location').z - 112) < .01, 'Overview raised to a seated eye height of 137.31 cm')
expect(abs(view.get_editor_property('relative_rotation').pitch + 10.5) < .01, 'Overview faces instruments instead of looking down from above')

windows = 0
for actor in actors:
    if isinstance(actor, u.StaticMeshActor) and actor.static_mesh_component.static_mesh:
        c = actor.static_mesh_component
        if c.static_mesh.get_name() == 'SM_Оконная_Рама_Куб_191755':
            material = c.get_material(0)
            opacity = u.MaterialEditingLibrary.get_material_property_input_node(material, u.MaterialProperty.MP_OPACITY)
            expect(material.get_editor_property('blend_mode') == u.BlendMode.BLEND_TRANSLUCENT
                   and abs(opacity.get_editor_property('r') - .025) < .001, 'Window uses clear translucent glass')
            windows += 1
expect(windows == 2, 'Both window panes are transparent')
bulb = next(a for a in actors if a.get_actor_label() == 'ROOM — central dim bulb')
expect(abs(bulb.light_component.get_editor_property('intensity') - 1.25) < .01, 'Main room light reduced from 6 to 1.25 cd')
keyboard = [a for a in actors if isinstance(a, u.StaticMeshActor) and a.static_mesh_component.static_mesh
            and a.static_mesh_component.static_mesh.get_name() == 'SM_KeyboardCC0']
expect(len(keyboard) == 1, 'One licensed CC0 keyboard is present')
kc, ke, _ = u.SystemLibrary.get_component_bounds(keyboard[0].static_mesh_component)
expect(abs(ke.y * 2 - 36) < .1, 'Keyboard retains a real 36 cm width')
monitor = next(a for a in actors if a.get_actor_label() == 'ROOM — Монитор2')
mc, me, _ = u.SystemLibrary.get_component_bounds(monitor.static_mesh_component)
expect(kc.x + ke.x < mc.x - me.x and abs(kc.y - mc.y) < 3, 'Keyboard stands in front of the monitor without intersecting it')
tiny = next(a for a in actors if a.get_actor_label() == 'ROOM — tiny desk lamp')
expect(0 < tiny.light_component.get_editor_property('intensity') <= .1, 'Desk lamp is a very weak local light')
dc, de, _ = u.SystemLibrary.get_component_bounds(body)
expect(abs(dc.x - 396) < .1 and abs(dc.y - 412) < .1, 'Deck moved onto the free part of the desk')
dishes = [a for a in actors if isinstance(a, u.StaticMeshActor) and a.static_mesh_component.static_mesh
          and a.static_mesh_component.static_mesh.get_name() == 'SM_DishLatestClean']
expect(len(dishes) == 1, 'New checked Yarik dish replaces the old scene mesh')
u.log('ROOM ASSET VERIFY COMPLETE: %d checks, 0 failures' % checks)
