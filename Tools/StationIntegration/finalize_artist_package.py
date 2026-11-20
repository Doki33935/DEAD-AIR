"""Host-only image format conversion and integrity report; no visual retouching."""
import csv,hashlib,json,os
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from PIL import Image
root=Path(os.environ.get('DEAD_AIR_ARTIST_DIR',str(Path.home()/'Desktop/DEAD_AIR_3D_ARTIST'))).resolve()
manifest=json.loads((root/'manifest.json').read_text(encoding='utf-8'))
def convert(item):
 source=(root/item['file']).resolve();assert root in source.parents
 with Image.open(source) as image:
  dest=source.with_suffix('.png')
  if source.suffix.lower()!='.png':image.save(dest)
  with Image.open(dest) as check:assert image.mode==check.mode and image.size==check.size and image.tobytes()==check.tobytes(),source
  return item,dest.relative_to(root).as_posix(),image.size
for item,path,size in ThreadPoolExecutor(max_workers=3).map(convert,manifest['textures']):
 item['file']=path;item['width'],item['height']=size
 (root/path).stat()
for item in manifest['textures']:
 for ext in ['.tga','.bmp']:
  source=(root/item['file']).with_suffix(ext).resolve();assert root in source.parents
  if source.exists():source.unlink()
for item in manifest['models']+manifest['textures']:
 file=(root/item['file']).resolve();assert root in file.parents and file.stat().st_size>0
 item['sha256']=hashlib.sha256(file.read_bytes()).hexdigest()
(root/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (root/'Materials.csv').open('w',encoding='utf-8-sig',newline='') as stream:
 writer=csv.writer(stream,delimiter=';');writer.writerow(['UE material','Texture PNG files']);by={t['asset']:t['file'] for t in manifest['textures']}
 for m in manifest['materials']:writer.writerow([m['asset'],' | '.join(by[p] for p in m['textures'] if p in by)])
report={'models':len(manifest['models']),'game_models':sum(not x['asset'].startswith('/Engine/') for x in manifest['models']),'textures':len(manifest['textures']),'instances':len(manifest['instances']),'total_bytes':sum(p.stat().st_size for p in root.rglob('*') if p.is_file()),'all_files_nonempty':True,'png_pixels_verified':True,'mesh_format':'FBX','texture_format':'PNG'}
print(json.dumps(report))
(root/'export-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
