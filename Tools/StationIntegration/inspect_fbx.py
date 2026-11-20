"""Read FBX metadata without altering supplied binary or ASCII files."""
import array
import collections
import json
import math
import re
import struct
import sys
import zlib
from pathlib import Path


def read_ascii_fbx(data):
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|;[^\n]*|\n|[{}:,*]|[^\s{}:,*]+',
                        data.decode('utf-8-sig'))
    tokens = [t for t in tokens if not t.startswith(';')]
    pos = 0

    def scalar(token):
        if token.startswith('"'):
            return token[1:-1].replace('\\"', '"').replace('\\\\', '\\')
        try:
            return int(token)
        except ValueError:
            try:
                return float(token)
            except ValueError:
                return token

    def nodes():
        nonlocal pos
        result = []
        while pos < len(tokens):
            if tokens[pos] == '\n':
                pos += 1
                continue
            if tokens[pos] == '}':
                pos += 1
                break
            name = tokens[pos]
            pos += 1
            if tokens[pos] != ':':
                raise ValueError('Expected FBX node colon')
            pos += 1
            values, sub = [], []
            while pos < len(tokens):
                token = tokens[pos]
                if token in ('\n', '}'):
                    break
                pos += 1
                if token == '{':
                    sub = nodes()
                    break
                if token in (',', '*'):
                    if token == ',' and pos < len(tokens) and tokens[pos] == '\n':
                        pos += 1
                    continue
                values.append(scalar(token))
            # ASCII numeric arrays wrap values in an `a` child; binary uses one array.
            if len(sub) == 1 and sub[0]['name'] == 'a':
                expected = values[0]
                values = [sub[0]['values']]
                if len(values[0]) != expected:
                    raise ValueError('Invalid ASCII array length')
                sub = []
            result.append({'name': name, 'values': values, 'children': sub})
        return result

    roots = nodes()
    header = next(n for n in roots if n['name'] == 'FBXHeaderExtension')
    version = next(n['values'][0] for n in header['children'] if n['name'] == 'FBXVersion')
    return version, roots


def read_fbx(path):
    data = Path(path).read_bytes()
    if not data.startswith(b'Kaydara FBX Binary'):
        return read_ascii_fbx(data)
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


def topology(vertices, indices):
    """Check original control-point topology; opposite winding is relative to the majority."""
    points = list(zip(vertices[::3], vertices[1::3], vertices[2::3]))
    faces, face = [], []
    for index in indices:
        face.append(-index - 1 if index < 0 else index)
        if index < 0:
            faces.append(face)
            face = []
    valid = not face and all(len(f) >= 3 and all(0 <= i < len(points) for i in f) for f in faces)
    result = {'finite_vertices': all(math.isfinite(x) for x in vertices), 'valid_polygon_indices': valid}
    if not valid or not result['finite_vertices']:
        return result
    result['triangles'] = sum(len(f) - 2 for f in faces)
    result['ngons'] = sum(len(f) > 4 for f in faces)
    coincident = collections.Counter(tuple(sorted(tuple(round(x, 8) for x in points[i]) for i in f)) for f in faces)
    result['coincident_polygon_extras'] = sum(n - 1 for n in coincident.values())
    parents = list(range(len(points)))

    def root(i):
        while parents[i] != i:
            parents[i] = parents[parents[i]]
            i = parents[i]
        return i

    for f in faces:
        for i in f[1:]:
            parents[root(i)] = root(f[0])
    shells = collections.defaultdict(list)
    for f in faces:
        shells[root(f[0])].append(f)
    volumes, boundary, nonmanifold, inconsistent = [], 0, 0, 0
    for shell in shells.values():
        edges, volume = collections.defaultdict(list), 0.0
        for f in shell:
            for i, j in zip(f, f[1:] + f[:1]):
                edges[tuple(sorted((i, j)))].append(1 if i < j else -1)
            x = points[f[0]]
            for i, j in zip(f[1:-1], f[2:]):
                y, z = points[i], points[j]
                volume += (x[0] * (y[1]*z[2] - y[2]*z[1]) + x[1] * (y[2]*z[0] - y[0]*z[2]) + x[2] * (y[0]*z[1] - y[1]*z[0])) / 6
        boundary += sum(len(e) == 1 for e in edges.values())
        nonmanifold += sum(len(e) > 2 for e in edges.values())
        inconsistent += sum(len(e) == 2 and e[0] == e[1] for e in edges.values())
        if all(len(e) == 2 for e in edges.values()):
            volumes.append(volume)
    positive = sum(v > 1e-12 for v in volumes)
    negative = sum(v < -1e-12 for v in volumes)
    result.update(connected_shells=len(shells), closed_shells=len(volumes),
                  opposite_winding_closed_shells=min(positive, negative),
                  boundary_edges=boundary, nonmanifold_edges=nonmanifold,
                  inconsistent_edge_winding=inconsistent)
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
            arrays = {}
            for key in ('Vertices', 'PolygonVertexIndex'):
                arr = children(obj, key)
                if arr:
                    vals = arr[0]['values'][0]
                    arrays[key] = vals
                    if key == 'Vertices':
                        entry['vertices'] = len(vals) // 3
                        entry['bounds'] = [[min(vals[i::3]), max(vals[i::3])] for i in range(3)]
                    else: entry['polygons'] = sum(v < 0 for v in vals)
            entry['uv_layers'] = len(children(obj, 'LayerElementUV'))
            entry['color_layers'] = len(children(obj, 'LayerElementColor'))
            entry['normals'] = len(children(obj, 'LayerElementNormal'))
            if 'Vertices' in arrays and 'PolygonVertexIndex' in arrays:
                entry.update(topology(arrays['Vertices'], arrays['PolygonVertexIndex']))
            result['geometry'].append(entry)
    return result


if __name__ == '__main__':
    output = [summarize(p) for p in sys.argv[1:]]
    sys.stdout.reconfigure(encoding='utf-8')
    print(json.dumps(output, ensure_ascii=False, indent=2))
