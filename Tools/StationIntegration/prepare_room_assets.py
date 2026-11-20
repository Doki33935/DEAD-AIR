"""Prepare exact source PNG/JPEG images, tagged material parameters and compatible dependencies."""
import hashlib
import io
import json
from pathlib import Path
import shutil
import struct

from PIL import Image
from room_archive_layout import export_properties, imported


def main():
    base = Path('nto_dead_air/Saved/RoomImport').resolve()
    content = base / 'Room_For_Game/Content'
    target = Path('nto_dead_air/Content').resolve()
    packages = json.loads((base / 'package-audit.json').read_text(encoding='utf-8'))
    lookup = {'/Game/' + str(Path(r['path']).with_suffix('')).replace('\\', '/'): r for r in packages}
    layout = json.loads((base / 'layout-recovery.json').read_text(encoding='utf-8'))
    todo = [m['package'] for m in layout['meshes']]
    for actor in layout['actors']:
        rec = next(r for r in packages if r['path'] == actor['source'])
        data = (content / rec['path']).read_bytes()
        exp = next(e for e in rec['exports'] if e['class'] == actor['component'])
        props = export_properties(rec, exp, data) if actor['component'] == 'StaticMeshComponent' else {}
        actor['materials'] = [imported(rec, x) for x in props.get('OverrideMaterials', [])]
        todo += [m.split('.')[0] for m in actor['materials'] if m and m.startswith('/Game/')]
    seen = set()
    while todo:
        path = todo.pop()
        if path in seen:
            continue
        seen.add(path)
        todo += [p for p in lookup[path]['dependencies'] if p.startswith('/Game/')]

    recipes = []
    texture_lines = []
    for path in sorted(seen):
        rec = lookup[path]
        if not rec['too_new_for_5_0'] or rec['primary_class'] != 'Texture2D':
            continue
        filename = content / rec['path']
        data = filename.read_bytes()
        offsets = []
        pos = 0
        while True:
            pos = data.find(bytes.fromhex('b7756362'), pos)
            if pos < 0:
                break
            raw_size, size = struct.unpack_from('>QQ', data, pos + 16)
            if 64 <= size <= len(data) - pos:
                offsets.append((raw_size, pos))
            pos += 4
        if len(offsets) != 1:
            raise ValueError('Ambiguous texture source payload: ' + path)
        key = hashlib.sha256(path.encode()).hexdigest()[:12]
        texture_lines.append('\t'.join([str(filename), str(offsets[0][1]), key + '.bin']))
    (base / 'texture-recovery.tsv').write_text('\n'.join(texture_lines), encoding='utf-8')
    if '--texture-manifest-only' in __import__('sys').argv:
        print('Texture payloads ready:', len(texture_lines))
        return
    for path in sorted(seen):
        rec = lookup[path]
        if not rec['too_new_for_5_0']:
            source = content / rec['path']
            destination = target / rec['path']
            destination.parent.mkdir(parents=True, exist_ok=True)
            if destination.exists():
                if source.read_bytes() != destination.read_bytes():
                    # Existing project materials take precedence over archive libraries.
                    print('PRESERVED EXISTING', rec['path'])
                continue
            shutil.copy2(source, destination)
            continue
        if rec['primary_class'] not in ('Material', 'MaterialInstanceConstant'):
            continue
        data = (content / rec['path']).read_bytes()
        recipe = {'source': path, 'class': rec['primary_class'], 'textures': {}, 'scalars': {},
                  'color': [1, 1, 1, 1] if rec['primary_class'] == 'MaterialInstanceConstant' else [.55, .55, .55, 1], 'graph': []}
        graph_nodes = {}
        editor_inputs = {}
        for exp_index, exp in enumerate(rec['exports'], 1):
            if exp['class'] == 'MaterialInstanceConstant':
                props = export_properties(rec, exp, data)
                for p in props.get('TextureParameterValues', []):
                    recipe['textures'][p['ParameterInfo']['Name']] = imported(rec, p['ParameterValue'])
                for p in props.get('ScalarParameterValues', []):
                    recipe['scalars'][p['ParameterInfo']['Name']] = p['ParameterValue']
                for p in props.get('VectorParameterValues', []):
                    if p['ParameterInfo']['Name'] in ('BaseColorFactor', 'Color', 'Tint'):
                        recipe['color'] = p['ParameterValue']
            elif exp['class'].startswith('MaterialExpression'):
                props = export_properties(rec, exp, data)
                graph_nodes[exp_index] = props
                if exp['class'] == 'MaterialExpressionVectorParameter':
                    recipe['color'] = props.get('DefaultValue', recipe['color'])
                if 'Texture' in props:
                    props['Texture'] = imported(rec, props['Texture'])
                    role = props.get('ParameterName', '')
                    if not role:
                        role = 'NormalTexture' if 'Normal' in str(props.get('SamplerType')) else 'BaseColorTexture'
                    recipe['textures'][role] = props['Texture']
                recipe['graph'].append({'class': exp['class'], 'properties': props})
            elif exp['class'] == 'MaterialEditorOnlyData':
                editor_inputs = export_properties(rec, exp, data)
        for channel in ('BaseColor', 'Normal', 'Roughness', 'Metallic', 'AmbientOcclusion', 'EmissiveColor'):
            item = editor_inputs.get(channel)
            if isinstance(item, dict) and 'raw' in item and len(item['raw']) >= 8:
                index = struct.unpack('<i', bytes.fromhex(item['raw'])[:4])[0]
                node = graph_nodes.get(index, {})
                if isinstance(node.get('Texture'), str):
                    recipe['textures'][channel + 'Texture'] = node['Texture']
        recipes.append(recipe)

    textures = []
    texture_dir = base / 'Recovered/Textures'
    texture_dir.mkdir(parents=True, exist_ok=True)
    for path in sorted(seen):
        rec = lookup[path]
        if rec['primary_class'] != 'Texture2D' or not rec['too_new_for_5_0']:
            continue
        key = hashlib.sha256(path.encode()).hexdigest()[:12]
        data = (base / 'RecoveredTextures' / (key + '.bin')).read_bytes()
        with Image.open(io.BytesIO(data)) as image:
            dimensions = [image.width, image.height]
            image_format = image.format
            image.verify()
        if image_format not in ('PNG', 'JPEG'):
            raise ValueError('Unsupported recovered source image: ' + path)
        raw = data
        filename = texture_dir / (hashlib.sha256(path.encode()).hexdigest()[:12] + ('.png' if image_format == 'PNG' else '.jpg'))
        filename.write_bytes(raw)
        roles = [k for r in recipes for k, t in r['textures'].items() if t and t.split('.')[0] == path]
        textures.append({'source': path, 'file': str(filename), 'dimensions': dimensions,
                         'roles': roles, 'sha256': hashlib.sha256(raw).hexdigest()})
    for mesh in layout['meshes']:
        rec = lookup[mesh['package']]
        if not rec['too_new_for_5_0']:
            mesh['asset'] = mesh['package']
            continue
        data = (content / rec['path']).read_bytes()
        exp = next(e for e in rec['exports'] if e['class'] == 'StaticMesh')
        props = export_properties(rec, exp, data)
        mesh['asset'] = '/Game/RoomArt/Meshes/' + mesh['key']
        mesh['slots'] = [{'name': p.get('ImportedMaterialSlotName', p.get('MaterialSlotName')),
                          'material': imported(rec, p.get('MaterialInterface'))}
                         for p in props.get('StaticMaterials', [])]
    output = {'actors': layout['actors'], 'meshes': layout['meshes'], 'materials': recipes,
              'textures': textures}
    (base / 'room-assets.json').write_text(json.dumps(output, ensure_ascii=False, indent=2), encoding='utf-8')
    print('Prepared', len(layout['meshes']), 'meshes,', len(recipes), 'material recipes,',
          len(textures), 'verified source images;', len(seen), 'packages in dependency closure')


if __name__ == '__main__':
    main()
