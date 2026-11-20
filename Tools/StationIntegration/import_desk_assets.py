"""UE 5.0.3 editor import: standard CC0 keyboard textures and Yarik's latest dish."""
import os
import unreal as u

art = os.path.abspath(os.path.join(u.Paths.project_dir(), '..', 'Tools', 'StationIntegration', 'SourceArt'))
tools = u.AssetToolsHelpers.get_asset_tools()
lib = u.MaterialEditingLibrary
textures = {}
for role, filename in [('color', 'keyboard_color_1024.png'), ('normal', 'keyboard_normal_1024.png'), ('orm', 'keyboard_orm_1024.png')]:
    path = '/Game/RoomArt/Keyboard/T_Keyboard_' + role
    tex = u.load_asset(path)
    if not tex:
        task = u.AssetImportTask()
        task.filename = os.path.join(art, 'KeyboardTevy', filename)
        task.destination_path = '/Game/RoomArt/Keyboard'
        task.destination_name = 'T_Keyboard_' + role
        task.automated = True
        task.save = True
        tools.import_asset_tasks([task])
        tex = u.load_asset(path)
    assert tex
    tex.set_editor_property('srgb', role == 'color')
    if role == 'normal':
        tex.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('flip_green_channel', True)
    elif role == 'orm':
        tex.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_MASKS)
    assert u.EditorAssetLibrary.save_loaded_asset(tex, False)
    textures[role] = tex

mat = u.load_asset('/Game/RoomArt/Keyboard/M_KeyboardCC0')
if not mat:
    mat = tools.create_asset('M_KeyboardCC0', '/Game/RoomArt/Keyboard', u.Material, u.MaterialFactoryNew())
lib.delete_all_material_expressions(mat)
samples = {}
for role, texture in textures.items():
    sample = lib.create_material_expression(mat, u.MaterialExpressionTextureSample)
    sample.set_editor_property('texture', texture)
    sample.set_editor_property('sampler_type', {'color': u.MaterialSamplerType.SAMPLERTYPE_COLOR, 'normal': u.MaterialSamplerType.SAMPLERTYPE_NORMAL, 'orm': u.MaterialSamplerType.SAMPLERTYPE_MASKS}[role])
    samples[role] = sample
for role, output, prop in [('color', 'RGB', u.MaterialProperty.MP_BASE_COLOR), ('normal', 'RGB', u.MaterialProperty.MP_NORMAL), ('orm', 'G', u.MaterialProperty.MP_ROUGHNESS), ('orm', 'B', u.MaterialProperty.MP_METALLIC), ('orm', 'R', u.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
    assert lib.connect_material_property(samples[role], output, prop)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat, False)

if not u.load_asset('/Game/RoomArt/DishLatest/DishLatest'):
    task = u.AssetImportTask()
    task.filename = os.path.join(art, 'DishLatest.fbx')
    task.destination_path = '/Game/RoomArt/DishLatest'
    task.destination_name = 'DishLatest'
    task.automated = True
    task.save = True
    options = u.FbxImportUI()
    options.set_editor_property('automated_import_should_detect_type', False)
    options.set_editor_property('mesh_type_to_import', u.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property('import_mesh', True)
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)
    options.static_mesh_import_data.set_editor_property('combine_meshes', True)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
    options.static_mesh_import_data.set_editor_property('convert_scene', True)
    options.static_mesh_import_data.set_editor_property('convert_scene_unit', True)
    options.static_mesh_import_data.set_editor_property('transform_vertex_to_absolute', True)
    task.options = options
    tools.import_asset_tasks([task])
    u.log('DISH IMPORT OBJECTS %s' % task.imported_object_paths)
assert u.load_asset('/Game/RoomArt/DishLatest/DishLatest')

steel = u.load_asset('/Game/RoomArt/DishLatest/M_DishMetal')
if not steel:
    steel = tools.create_asset('M_DishMetal', '/Game/RoomArt/DishLatest', u.Material, u.MaterialFactoryNew())
lib.delete_all_material_expressions(steel)
col = lib.create_material_expression(steel, u.MaterialExpressionConstant3Vector)
col.set_editor_property('constant', u.LinearColor(.26, .28, .3, 1))
assert lib.connect_material_property(col, '', u.MaterialProperty.MP_BASE_COLOR)
for prop, val in [(u.MaterialProperty.MP_ROUGHNESS, .46), (u.MaterialProperty.MP_METALLIC, .55)]:
    expr = lib.create_material_expression(steel, u.MaterialExpressionConstant)
    expr.set_editor_property('r', val)
    assert lib.connect_material_property(expr, '', prop)
steel.set_editor_property('two_sided', True)
lib.recompile_material(steel)
assert u.EditorAssetLibrary.save_loaded_asset(steel, False)
u.log('DESK ASSET IMPORT COMPLETE keyboard PBR textures and latest dish')
