"""Read selected UE5.4 property tags and source mesh payload locations, without modifying inputs."""
import hashlib
import json
import math
from pathlib import Path
import struct

from inspect_room_packages import Reader


def properties(record, data, offset=0):
    """Decode the tagged subset needed for material parameters and actor references."""
    names = record['names']
    r = Reader(data, offset)

    def fname(ar):
        index, number = ar.value('ii')
        if not 0 <= index < len(names):
            raise ValueError('Invalid property name')
        return names[index] + ('_' + str(number - 1) if number else '')

    def type_name(depth=0):
        if depth > 8:
            raise ValueError('Invalid property type depth')
        name, count = fname(r), r.value('i')
        if not 0 <= count <= 8:
            raise ValueError('Invalid type parameter count')
        return name, [type_name(depth + 1) for _ in range(count)]

    result = {}
    while r.offset + 8 <= len(data):
        name = fname(r)
        if name == 'None':
            break
        typ = type_name()
        size, flags = r.value('i'), r.value('B')
        if flags & 1:
            r.value('i')
        if flags & 2:
            r.offset += 16
        if size < 0 or r.offset + size > len(data):
            raise ValueError('Property payload extends beyond export')
        payload = data[r.offset:r.offset + size]
        r.offset += size
        kind = typ[0]
        if kind == 'StrProperty':
            value = Reader(payload).string()
        elif kind in ('NameProperty', 'ByteProperty') and size == 8:
            value = fname(Reader(payload))
        elif kind in ('ObjectProperty', 'IntProperty') and size == 4:
            value = struct.unpack('<i', payload)[0]
        elif kind == 'FloatProperty' and size == 4:
            value = struct.unpack('<f', payload)[0]
        elif kind == 'BoolProperty':
            value = bool(flags & 0x10)
        elif kind == 'StructProperty':
            struct_name = typ[1][0][0]
            if struct_name == 'LinearColor' and size == 16:
                value = list(struct.unpack('<ffff', payload))
            elif not flags & 8:
                value, consumed = properties(record, payload)
                if consumed != size:
                    value = {'raw': payload.hex(), 'type': struct_name}
            else:
                value = {'raw': payload.hex(), 'type': struct_name}
        elif kind == 'ArrayProperty' and typ[1][0][0] == 'StructProperty' and (typ[1][0][1][0][0].endswith('ParameterValue') or typ[1][0][1][0][0] in ('StaticMaterial', 'StaticMeshSourceModel')):
            ar = Reader(payload)
            count = ar.value('i')
            if not 0 <= count < 10000:
                raise ValueError('Invalid struct array length')
            value = []
            for _ in range(count):
                item, consumed = properties(record, payload, ar.offset)
                ar.offset += consumed
                value.append(item)
        elif kind == 'ArrayProperty' and typ[1][0][0] == 'ObjectProperty':
            ar = Reader(payload)
            count = ar.value('i')
            value = [ar.value('i') for _ in range(count)]
        else:
            value = {'raw': payload.hex(), 'type': kind}
        result[name] = value
    return result, r.offset - offset


def export_properties(record, export, data):
    return properties(record, data[export['offset']:export['offset'] + export['size']], 1)[0]


def tagged(record, export, data, key):
    if key not in record['names']:
        return None
    names = record['names']
    raw = data[export['offset']:export['offset'] + export['size']]
    needle = struct.pack('<ii', names.index(key), 0)
    start = 0
    while True:
        pos = raw.find(needle, start)
        if pos < 0:
            return None
        start = pos + 1
        try:
            r = Reader(raw, pos + 8)

            def typename(depth=0):
                if depth > 8:
                    raise ValueError('Invalid type nesting')
                index, number, count = r.value('iii')
                if not 0 <= index < len(names) or not 0 <= count <= 8:
                    raise ValueError('Invalid type name')
                return (names[index], [typename(depth + 1) for _ in range(count)])

            typ = typename()
            size, flags = r.value('i'), r.value('B')
            if flags not in (0, 8) or size < 0 or r.offset + size > len(raw):
                continue
            value = raw[r.offset:r.offset + size]
            if typ[0] == 'StructProperty' and typ[1][0][0] in ('Vector', 'Rotator'):
                if size != 24:
                    continue
                values = struct.unpack('<ddd', value)
                if not all(math.isfinite(x) for x in values):
                    raise ValueError('Nonfinite transform')
                return list(values)
            if typ[0] == 'StrProperty':
                return Reader(value).string()
            if typ[0] == 'ObjectProperty':
                return struct.unpack('<i', value)[0]
            return {'type': typ, 'bytes': value.hex()}
        except (ValueError, IndexError, struct.error):
            continue


def imported(record, index):
    if index is None or index == 0:
        return None
    if index > 0:
        return record['exports'][index - 1]['name']
    item = record['imports'][-index - 1]
    outer = imported(record, item['outer'])
    return outer + '.' + item['name'] if outer else item['name']


def prepare(base):
    content = base / 'Room_For_Game/Content'
    packages = json.loads((base / 'package-audit.json').read_text(encoding='utf-8'))
    lookup = {'/Game/' + str(Path(r['path']).with_suffix('')).replace('\\', '/'): r for r in packages}
    actors = []
    for record in packages:
        if not record['path'].startswith('__ExternalActors__'):
            continue
        data = (content / record['path']).read_bytes()
        for exp in record['exports']:
            if exp['class'] not in ('StaticMeshComponent', 'SpotLightComponent', 'PointLightComponent',
                                    'DirectionalLightComponent', 'SkyLightComponent'):
                continue
            owner = next((e for e in record['exports'] if e['class'] == record['primary_class']), None)
            label = tagged(record, owner, data, 'ActorLabel') if owner else None
            mesh = imported(record, tagged(record, exp, data, 'StaticMesh'))
            actors.append({'label': label or exp['name'], 'component': exp['class'], 'mesh': mesh,
                           'location': tagged(record, exp, data, 'RelativeLocation') or [0, 0, 0],
                           'rotation': tagged(record, exp, data, 'RelativeRotation') or [0, 0, 0],
                           'scale': tagged(record, exp, data, 'RelativeScale3D') or [1, 1, 1],
                           'source': record['path'], 'attached': 'AttachParent' in record['names']})
    used = sorted({a['mesh'].split('.')[0] for a in actors if (a.get('mesh') or '').startswith('/Game/')})
    recovery = []
    for package in used:
        record = lookup[package]
        if record['primary_class'] != 'StaticMesh':
            continue
        key = 'SM_' + Path(record['path']).stem + '_' + hashlib.sha256(package.encode()).hexdigest()[:6]
        entry = {'package': package, 'key': key, 'path': str((content / record['path']).resolve()),
                 'triangles': int(record['Triangles']), 'uv': int(record['UVChannels']),
                 'newer': record['too_new_for_5_0']}
        if entry['newer']:
            data = Path(entry['path']).read_bytes()
            offsets = []
            pos = 0
            while True:
                pos = data.find(bytes.fromhex('b7756362'), pos)
                if pos < 0:
                    break
                raw_size, compressed_size = struct.unpack_from('>QQ', data, pos + 16)
                if 64 <= compressed_size <= len(data) - pos and raw_size > 0:
                    offsets.append(pos)
                pos += 4
            desc = next((e for e in record['exports']
                         if e['class'] == 'StaticMeshDescriptionBulkData'
                         and e['name'].startswith('StaticMeshDescriptionBulkData_')), None)
            if desc:
                description = data[desc['offset']:desc['offset'] + desc['size']]
                offsets = [p for p in offsets if data[p + 32:p + 52] in description]
            if len(offsets) != 1:
                raise ValueError(f'Expected one hash-matched LOD0 payload: {package}, {offsets}')
            entry['offset'] = offsets[0]
        recovery.append(entry)
    out = {'actors': actors, 'meshes': recovery}
    (base / 'layout-recovery.json').write_text(json.dumps(out, ensure_ascii=False, indent=2), encoding='utf-8')
    lines = ['\t'.join([r['path'], str(r['offset']), str(r['triangles']), str(r['uv']), r['key']])
             for r in recovery if r['newer']]
    (base / 'mesh-recovery.tsv').write_text('\n'.join(lines), encoding='utf-8')
    return out


if __name__ == '__main__':
    import sys
    sys.stdout.reconfigure(encoding='utf-8')
    result = prepare(Path('nto_dead_air/Saved/RoomImport'))
    print('Placed components', len(result['actors']), 'meshes', len(result['meshes']))
    print('New mesh payloads', sum(m['newer'] for m in result['meshes']))
    print('Attached components', [a['label'] for a in result['actors'] if a['attached']])
