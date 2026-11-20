"""Read-only inventory of uncooked UE4/UE5 packages; never converts asset versions.

Reports package headers, export classes and hard package references, not mesh QA.
Geometry, collision, materials and runtime behaviour still require the right UE editor.
Serialization thresholds are from the installed UE 5.0 source, with UE5 soft-path lists.
"""
import argparse
import collections
import csv
import json
from pathlib import Path
import struct
import sys


class Reader:
    def __init__(self, data, offset=0):
        self.data, self.offset = data, offset

    def value(self, fmt):
        values = struct.unpack_from('<' + fmt, self.data, self.offset)
        self.offset += struct.calcsize('<' + fmt)
        return values[0] if len(values) == 1 else values

    def string(self):
        count = self.value('i')
        if abs(count) > 1048576:
            raise ValueError('Invalid FString length')
        size = abs(count) * (2 if count < 0 else 1)
        raw = self.data[self.offset:self.offset + size]
        if len(raw) != size:
            raise ValueError('Truncated FString')
        self.offset += size
        return raw.decode('utf-16-le' if count < 0 else 'utf-8').rstrip('\0')


def registry_tags(path, thumbnail_offset):
    """Read the on-disk asset registry immediately following the thumbnail table.

    Return declared editor metadata, never interpret these numbers as live mesh QA.
    Some old packages have no thumbnail table; their metadata is left unavailable.
    """
    if thumbnail_offset <= 0:
        return []
    with path.open('rb') as stream:
        stream.seek(thumbnail_offset)
        data = stream.read(1048576)
    r = Reader(data)
    count = r.value('i')
    if not 0 <= count < 10000:
        return []
    for _ in range(count):
        r.string()
        r.string()
        r.value('i')
    for dependency_prefix in (8, 0):
        try:
            ar = Reader(data, r.offset + dependency_prefix)
            count = ar.value('i')
            if not 0 < count < 10000:
                continue
            assets = []
            for _ in range(count):
                object_name, asset_class = ar.string(), ar.string()
                tags = {}
                tag_count = ar.value('i')
                if not 0 <= tag_count < 10000:
                    raise ValueError('Invalid tag count')
                for _ in range(tag_count):
                    key, value = ar.string(), ar.string()
                    # Import paths contain local artist paths, not public review data.
                    if key != 'AssetImportData':
                        tags[key] = value
                assets.append({'name': object_name, 'class': asset_class, 'tags': tags})
            return assets
        except (ValueError, struct.error, UnicodeError):
            continue
    return []


def inspect(path, root):
    result = {'path': path.relative_to(root).as_posix(), 'bytes': path.stat().st_size}
    with path.open('rb') as stream:
        data = stream.read(65536)
        r = Reader(data)
        if r.value('I') != 0x9E2A83C1:
            raise ValueError('Not a little-endian Unreal package')
        legacy = r.value('i')
        if not -8 <= legacy <= -3:
            raise ValueError('Unsupported legacy format')
        if legacy != -4:
            r.value('i')
        ue4 = r.value('i')
        ue5 = r.value('i') if legacy <= -8 else 0
        licensee = r.value('i')
        custom = []
        count = r.value('i')
        if not 0 <= count < 1024:
            raise ValueError('Invalid custom version count')
        for _ in range(count):
            guid = r.value('IIII')
            version = r.value('i')
            friendly = r.string() if legacy >= -5 else ''
            custom.append({'guid': ''.join(f'{n:08X}' for n in guid),
                           'version': version, 'friendly': friendly})
        header_size = r.value('i')
        package_name = r.string()
        flags = r.value('I')
        name_count, name_offset = r.value('ii')
        summary_start = r.offset
        if header_size > len(data):
            if not header_size <= result['bytes']:
                raise ValueError('Header extends beyond file')
            stream.seek(0)
            data = stream.read(header_size)
        names = []
        nr = Reader(data, name_offset)
        for _ in range(name_count):
            names.append(nr.string())
            if ue4 >= 504:
                nr.offset += 4

        # Only the 5.1+ soft-path list differs here. Validate table boundaries,
        # rather than silently assuming a newer header has the old layout.
        candidates = [8, 0] if ue5 > 1004 else [0]
        tables = None
        for soft_path_bytes in candidates:
            try:
                sr = Reader(data, summary_start + soft_path_bytes)
                if ue4 >= 516 and not flags & 0x80000000:
                    sr.string()
                if ue4 >= 459:
                    sr.value('ii')
                ec, eo, ic, io, depends = sr.value('iiiii')
                if ue4 >= 384:
                    sr.value('ii')
                if ue4 >= 510:
                    sr.value('i')
                thumbnail = sr.value('i')
                if not (0 <= ec < 100000 and 0 <= ic < 100000
                        and 0 <= eo <= header_size and 0 <= io <= header_size
                        and 0 <= depends <= header_size):
                    continue
                import_stride = 28 + (8 if ue4 >= 520 and not flags & 0x80000000 else 0)
                import_stride += 4 if ue5 >= 1003 else 0
                if ic and io + ic * import_stride > header_size:
                    continue
                imports = []

                def name(ar):
                    index, number = ar.value('ii')
                    if not 0 <= index < len(names) or number < 0:
                        raise ValueError('Invalid FName index')
                    return names[index] + ('_' + str(number - 1) if number else '')

                for i in range(ic):
                    ar = Reader(data, io + i * import_stride)
                    class_package, class_name = name(ar), name(ar)
                    outer = ar.value('i')
                    object_name = name(ar)
                    imports.append({'class': class_name, 'name': object_name, 'outer': outer})
                stride = (depends - eo) // ec if ec else 0
                if ec and ((depends - eo) % ec or not 64 <= stride <= 192):
                    continue
                exports = []
                for i in range(ec):
                    ar = Reader(data, eo + i * stride)
                    ci, si = ar.value('ii')
                    if ue4 >= 508:
                        ar.value('i')
                    outer = ar.value('i')
                    object_name = name(ar)
                    ar.value('I')
                    size, offset = ar.value('qq' if ue4 >= 511 else 'ii')
                    if not (0 <= offset <= result['bytes'] and 0 <= size <= result['bytes'] - offset):
                        raise ValueError('Export extends beyond file')
                    cls = imports[-ci - 1]['name'] if ci < 0 else '(local class)'
                    exports.append({'name': object_name, 'class': cls, 'outer': outer,
                                    'offset': offset, 'size': size})
                tables = (imports, exports, thumbnail)
                break
            except (ValueError, IndexError, struct.error, UnicodeError):
                continue
        if tables is None:
            raise ValueError('Could not validate import/export tables')
        imports, exports, thumbnail = tables
        saved_engine = ''
        branch = data.find(b'++UE', 0, name_offset)
        if branch >= 14:
            major, minor, patch, changelist = struct.unpack_from('<HHHI', data, branch - 14)
            if major in (4, 5):
                saved_engine = f'{major}.{minor}.{patch}'
        result.update(ue4=ue4, ue5=ue5, licensee=licensee, saved_engine=saved_engine,
                      package_name=package_name, exports=exports, custom_versions=custom,
                      names=names, thumbnail_offset=thumbnail,
                      dependencies=sorted({i['name'] for i in imports
                                           if i['class'] == 'Package' and i['name'].startswith('/')}),
                      classes=sorted({e['class'] for e in exports}),
                      too_new_for_5_0=ue4 > 522 or ue5 > 1004)
        result['primary_class'] = next((e['class'] for e in exports
                                        if e['outer'] <= 0 and e['class'] != 'MetaData'), '')
        try:
            result['registry'] = registry_tags(path, thumbnail)
        except (ValueError, struct.error, UnicodeError):
            result['registry'] = []
        if result['registry']:
            result['primary_class'] = result['registry'][0]['class'].split('.')[-1]
            tags = result['registry'][0]['tags']
            for key in ('Triangles', 'Vertices', 'UVChannels', 'ApproxSize',
                        'CollisionPrims', 'CollisionComplexity', 'LODs', 'NaniteEnabled'):
                result[key] = tags.get(key, '')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('content', type=Path)
    parser.add_argument('--json', type=Path, required=True)
    parser.add_argument('--csv', type=Path, required=True)
    args = parser.parse_args()
    root = args.content.resolve()
    reports = []
    for path in sorted(root.rglob('*')):
        if path.suffix.lower() not in ('.uasset', '.umap'):
            continue
        try:
            reports.append(inspect(path, root))
        except Exception as exc:
            reports.append({'path': path.relative_to(root).as_posix(), 'error': str(exc)})
    packages = {'/Game/' + str(Path(r['path']).with_suffix('')).replace('\\', '/') for r in reports}
    for report in reports:
        report['missing_game_dependencies'] = [p for p in report.get('dependencies', [])
                                                if p.startswith('/Game/') and p not in packages]
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(reports, ensure_ascii=False, indent=2), encoding='utf-8')
    args.csv.parent.mkdir(parents=True, exist_ok=True)
    with args.csv.open('w', encoding='utf-8-sig', newline='') as stream:
        fields = ['path', 'primary_class', 'saved_engine', 'ue4', 'ue5',
                  'too_new_for_5_0', 'Triangles', 'Vertices', 'UVChannels', 'ApproxSize',
                  'CollisionPrims', 'CollisionComplexity', 'LODs', 'NaniteEnabled',
                  'missing_game_dependencies', 'error']
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction='ignore')
        writer.writeheader()
        for report in reports:
            row = dict(report)
            row['missing_game_dependencies'] = '; '.join(row['missing_game_dependencies'])
            writer.writerow(row)
    print('Packages:', len(reports), 'errors:', sum('error' in r for r in reports))
    print('Classes:', dict(collections.Counter(r.get('primary_class', 'ERROR') for r in reports)))
    print('Too-new headers:', sum(r.get('too_new_for_5_0', False) for r in reports))
    print('Packages with missing Game dependencies:', sum(bool(r['missing_game_dependencies']) for r in reports))


if __name__ == '__main__':
    sys.stdout.reconfigure(encoding='utf-8')
    main()
