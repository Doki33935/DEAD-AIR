"""Run inside UE 5.0.3 Python: import recovered source images and rebuild PBR bindings."""
import hashlib
import json
import os
import unreal as u

root = os.path.abspath(os.path.join(u.Paths.project_saved_dir(), 'RoomImport'))
data = json.load(open(os.path.join(root, 'room-assets.json'), encoding='utf-8'))
tools = u.AssetToolsHelpers.get_asset_tools()
lib = u.MaterialEditingLibrary
material_map = {}


def save(asset):
    assert u.EditorAssetLibrary.save_loaded_asset(asset, False), asset.get_path_name()


for row in data['textures']:
    path, name = row['source'].rsplit('/', 1)
    tex = u.load_asset(row['source'])
    if not tex:
        task = u.AssetImportTask()
        task.filename = row['file']
        task.destination_path = path
        task.destination_name = name
        task.automated = True
        task.save = True
        task.replace_existing = False
        tools.import_asset_tasks([task])
        tex = u.load_asset(row['source'])
    assert isinstance(tex, u.Texture2D), row['source']
    roles = ' '.join(row['roles']).lower()
    normal = 'normal' in roles or name.lower().endswith('_n')
    linear = normal or any(x in roles for x in ('roughness', 'metallic', 'ambientocclusion'))
    # Compatible SurvivalProps materials sample their diffuse images as linear.
    tex.set_editor_property('srgb', not linear and not (not roles and name.lower().endswith('_c')))
    if normal:
        tex.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_NORMALMAP)
    elif linear:
        tex.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_MASKS)
    save(tex)


def constant(mat, value):
    expr = lib.create_material_expression(mat, u.MaterialExpressionConstant)
    expr.set_editor_property('r', float(value))
    return expr


def link(mat, expr, output, prop):
    assert lib.connect_material_property(expr, output, prop)


for row in data['materials']:
    name = 'M_' + row['source'].rsplit('/', 1)[-1] + '_' + hashlib.sha256(row['source'].encode()).hexdigest()[:6]
    path = '/Game/RoomArt/Materials/' + name
    mat = u.load_asset(path)
    if not mat:
        mat = tools.create_asset(name, '/Game/RoomArt/Materials', u.Material, u.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    mat.set_editor_property('two_sided', False)
    samples = {}
    for role, source in row['textures'].items():
        if not source:
            continue
        tex = u.load_asset(source.split('.')[0])
        assert isinstance(tex, u.Texture2D), source
        sample = lib.create_material_expression(mat, u.MaterialExpressionTextureSample)
        sample.set_editor_property('texture', tex)
        sample.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_NORMAL if 'normal' in role.lower()
                                   else u.MaterialSamplerType.SAMPLERTYPE_COLOR if tex.get_editor_property('srgb')
                                   else u.MaterialSamplerType.SAMPLERTYPE_MASKS if tex.get_editor_property('compression_settings') == u.TextureCompressionSettings.TC_MASKS
                                   else u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
        samples[role.lower()] = sample
    base = samples.get('basecolortexture')
    if base:
        link(mat, base, 'RGB', u.MaterialProperty.MP_BASE_COLOR)
    else:
        color = lib.create_material_expression(mat, u.MaterialExpressionConstant3Vector)
        color.set_editor_property('constant', u.LinearColor(*row['color'][:3], 1))
        link(mat, color, '', u.MaterialProperty.MP_BASE_COLOR)
    if 'normaltexture' in samples:
        link(mat, samples['normaltexture'], 'RGB', u.MaterialProperty.MP_NORMAL)
    packed = samples.get('metallicroughnesstexture')
    if packed:
        link(mat, packed, 'G', u.MaterialProperty.MP_ROUGHNESS)
        metal_factor = row['scalars'].get('MetallicFactor', 1.)
        link(mat, packed if metal_factor else constant(mat, 0.), 'B' if metal_factor else '', u.MaterialProperty.MP_METALLIC)
    else:
        link(mat, samples.get('roughnesstexture') or constant(mat, .65),
             'R' if 'roughnesstexture' in samples else '', u.MaterialProperty.MP_ROUGHNESS)
        link(mat, samples.get('metallictexture') or constant(mat, 0.),
             'R' if 'metallictexture' in samples else '', u.MaterialProperty.MP_METALLIC)
    if 'ambientocclusiontexture' in samples:
        link(mat, samples['ambientocclusiontexture'], 'R', u.MaterialProperty.MP_AMBIENT_OCCLUSION)
    if 'emissivecolortexture' in samples:
        link(mat, samples['emissivecolortexture'], 'RGB', u.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
    save(mat)
    material_map[row['source']] = path


def material(source):
    if not source:
        return None
    source = source.split('.')[0]
    return u.load_asset(material_map.get(source, source))


for row in data['meshes']:
    mesh = u.load_asset(row['asset'])
    assert isinstance(mesh, u.StaticMesh), row['asset']
    if not row['newer']:
        continue
    slots = {s['name']: s for s in row.get('slots', [])}
    for i, slot in enumerate(mesh.get_editor_property('static_materials')):
        original = slots.get(str(slot.material_slot_name))
        if original:
            mat = material(original.get('material'))
            if mat:
                mesh.set_material(i, mat)
    save(mesh)

data['material_map'] = material_map
with open(os.path.join(root, 'room-ready.json'), 'w', encoding='utf-8') as stream:
    json.dump(data, stream, ensure_ascii=False, indent=2)
u.log('ROOM ASSET IMPORT COMPLETE: %d meshes, %d textures, %d rebuilt materials' %
      (len(data['meshes']), len(data['textures']), len(material_map)))
