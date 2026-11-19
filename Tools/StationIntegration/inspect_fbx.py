"""Read FBX binary metadata without altering the supplied files."""
import array
import json
import struct
import sys
import zlib
from pathlib import Path


def read_fbx(path):
    data = Path(path).read_bytes()
    if not data.startswith(b'Kaydara FBX Binary'):
        raise ValueError('Expected binary FBX')
    version = struct.unpack_from('<I', data, 23)[0]
    wide = version >= 7500
    fmt, header = ('<QQQB', 25) if wide else ('<IIIB', 13)

    def prop(pos):
        tag = chr(data[pos]); pos += 1
        scalar = {'Y': 'h', 'C': '?', 'I': 'i', 'F': 'f', 'D': 'd', 'L': 'q'}
        if tag in scalar:
            f = '<' + scalar[tag]
            return struct.unpack_from(f, data, pos)[0], pos + struct.calcsize(f)
        if tag in 'SR':
            count = struct.unpack_from('<I', data, pos)[0]; pos += 4
            value = data[pos:pos + count]
            return (value.decode('utf-8', errors='replace') if tag == 'S' else {'bytes': count}), pos + count
        if tag in 'fdilbc':
            count, encoding, length = struct.unpack_from('<III', data, pos); pos += 12
            raw = data[pos:pos + length]
            if encoding:
                raw = zlib.decompress(raw)
            a = array.array({'f': 'f', 'd': 'd', 'i': 'i', 'l': 'q', 'b': 'b', 'c': 'b'}[tag])
            a.frombytes(raw)
            if len(a) != count:
                raise ValueError('Invalid array length')
            return a, pos + length
        raise ValueError(f'Unsupported property {tag!r}')

    def node(pos):
        end, count, _, length = struct.unpack_from(fmt, data, pos)
        if end == 0:
            return None, pos + header
        pos += header
        name = data[pos:pos + length].decode('utf-8'); pos += length
        values = []
        for _ in range(count):
            value, pos = prop(pos); values.append(value)
        children = []
        while pos < end - header:
            child, pos = node(pos)
            if child is None:
                break
            children.append(child)
        return {'name': name, 'values': values, 'children': children}, end

    roots, pos = [], 27
    while pos + header < len(data):
        n, pos = node(pos)
        if n is None:
            break
        roots.append(n)
    return version, roots


def children(n, name):
    return [c for c in n['children'] if c['name'] == name]


def properties(n):
    result = {}
    for group in children(n, 'Properties70'):
        for p in children(group, 'P'):
            result[p['values'][0]] = p['values'][4:]
    return result


def summarize(path):
    version, roots = read_fbx(path)
    objects = next(n for n in roots if n['name'] == 'Objects')['children']
    result = {'file': str(path), 'version': version, 'global': {}, 'models': [], 'geometry': [], 'materials': [], 'textures': [], 'connections': []}
    for root in roots:
        if root['name'] == 'GlobalSettings': result['global'] = properties(root)
        if root['name'] == 'Connections': result['connections'] = [c['values'] for c in root['children']]
    for obj in objects:
        kind = obj['name']; entry = {'id': obj['values'][0], 'name': obj['values'][1], 'type': obj['values'][2] if len(obj['values']) > 2 else '', 'properties': properties(obj)}
        if kind == 'Model': result['models'].append(entry)
        if kind == 'Material': result['materials'].append(entry)
        if kind in ('Texture', 'Video'):
            entry['fields'] = {c['name']: c['values'] for c in obj['children'] if c['name'] != 'Properties70'}
            result['textures'].append(entry)
        if kind == 'Geometry':
            for key in ('Vertices', 'PolygonVertexIndex'):
                arr = children(obj, key)
                if arr:
                    vals = arr[0]['values'][0]
                    if key == 'Vertices':
                        entry['vertices'] = len(vals) // 3
                        entry['bounds'] = [[min(vals[i::3]), max(vals[i::3])] for i in range(3)]
                    else: entry['polygons'] = sum(v < 0 for v in vals)
            entry['uv_layers'] = len(children(obj, 'LayerElementUV'))
            entry['color_layers'] = len(children(obj, 'LayerElementColor'))
            entry['normals'] = len(children(obj, 'LayerElementNormal'))
            result['geometry'].append(entry)
    return result


if __name__ == '__main__':
    output = [summarize(p) for p in sys.argv[1:]]
    sys.stdout.reconfigure(encoding='utf-8')
    print(json.dumps(output, ensure_ascii=False, indent=2))
