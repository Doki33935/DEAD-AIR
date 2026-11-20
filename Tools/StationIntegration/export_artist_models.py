"""Export the game scene's real StaticMeshes and source textures for texturing in DCC."""
import csv, hashlib, json, os, re
import unreal as u
OUT=os.environ.get('DEAD_AIR_ARTIST_DIR',os.path.expanduser('~/Desktop/DEAD_AIR_3D_ARTIST'))
FURNITURE_UPDATE='-ArtistFurnitureUpdate' in u.SystemLibrary.get_command_line()
if FURNITURE_UPDATE:OUT=os.path.join(OUT,'Updates','DeskFit')
LAYOUT_ONLY='-ArtistLayoutOnly' in u.SystemLibrary.get_command_line()
assert not os.path.exists(os.path.join(OUT,'manifest.json')) or '-RefreshArtistExport' in u.SystemLibrary.get_command_line() or LAYOUT_ONLY, 'Do not overwrite an artist work folder'
os.makedirs(OUT,exist_ok=True)
world=u.EditorLoadingAndSavingUtils.load_map('/Game/StationIntegration/StationIntegrationMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
def clean(name):
 return re.sub(r'[^a-zA-Z0-9_-]+','_',name).strip('_') or 'model'
def name_for(path):
 return clean(path.split('.')[-1])+'_'+hashlib.sha1(path.encode()).hexdigest()[:8]
def export(obj,filename,options=None):
 os.makedirs(os.path.dirname(filename),exist_ok=True)
 task=u.AssetExportTask();task.object=obj;task.filename=filename;task.automated=True;task.prompt=False;task.replace_identical=True
 if options:task.options=options
 if not u.Exporter.run_asset_export_task(task):
  assert filename.endswith('.tga'),filename
  filename=filename[:-4]+'.bmp';task.filename=filename
  assert u.Exporter.run_asset_export_task(task),filename
 assert os.path.getsize(filename)>0,filename
 return os.path.relpath(filename,OUT).replace('\\','/')
options=u.FbxExportOption();options.set_editor_property('ascii',False);options.set_editor_property('level_of_detail',False);options.set_editor_property('collision',False)
if FURNITURE_UPDATE:
 table=next(a for a in actors if isinstance(a,u.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name().endswith('_54910a'))
 mesh=table.static_mesh_component.static_mesh
 model=export(mesh,os.path.join(OUT,'Table_Wider.fbx'),options)
 reference=export(world,os.path.join(OUT,'Reference_Room.fbx'),options)
 t=table.static_mesh_component.get_world_transform();r=t.rotation.rotator();v=t.translation;s=t.scale3d
 update={'engine':'5.0.3','model':mesh.get_path_name(),'file':model,'reference_room':reference,'position_cm':[v.x,v.y,v.z],'rotation_degrees_pitch_yaw_roll':[r.pitch,r.yaw,r.roll],'scale':[s.x,s.y,s.z],'computer_top_world_size_cm':[100.1569,118.7518],'uv_and_material_slots':'unchanged','system_unit':'removed from scene only'}
 with open(os.path.join(OUT,'manifest.json'),'w',encoding='utf-8') as f:json.dump(update,f,ensure_ascii=False,indent=2)
 with open(os.path.join(OUT,'README_ОБНОВЛЕНИЕ.txt'),'w',encoding='utf-8-sig') as f:f.write('Обновление стола: компьютерная часть столешницы расширена примерно до 100 x 119 см. Ножки, UV0, pivots и material slots сохранены. Для дальнейшего текстурирования использовать Table_Wider.fbx вместо прежнего FBX стола. Reference_Room.fbx — только справочная расстановка; системный блок удалён, реквизит стоит на поверхности. Исходные Models/Textures и прежняя работа художника не перезаписывались. Единицы — сантиметры; при обратном импорте сохранять UV, масштаб, pivot и порядок материалов.\n')
 u.log('ARTIST FURNITURE UPDATE COMPLETE '+OUT)
else:
 models={};textures={};materials={};instances=[]
 if LAYOUT_ONLY:
  with open(os.path.join(OUT,'manifest.json'),encoding='utf-8') as f:previous=json.load(f)
  models={x['asset']:x for x in previous['models']};textures={x['asset']:x for x in previous['textures']};materials={x['asset']:x for x in previous['materials']}

 reg=u.AssetRegistryHelpers.get_asset_registry();reg.scan_paths_synchronous(['/Game'],True)
 opts=u.AssetRegistryDependencyOptions(True,True,True,True,True)
 texture_paths={str(a.package_name):str(a.object_path) for a in reg.get_assets_by_class('Texture2D',True)}
 def material_textures(mat):
  queue=[mat.get_path_name().split('.')[0]];seen=set();found=[]
  while queue:
   package=queue.pop()
   if package in seen:continue
   seen.add(package)
   if package in texture_paths:found.append(u.load_asset(texture_paths[package]));continue
   queue.extend(str(x) for x in reg.get_dependencies(package,opts) if str(x).startswith('/Game/'))
  return found
 for actor in actors:
  for comp in actor.get_components_by_class(u.StaticMeshComponent):
   mesh=comp.static_mesh
   if not mesh or not comp.is_visible() or comp.get_editor_property('hidden_in_game'):continue
   path=mesh.get_path_name()
   # Engine primitives are reference helpers (night sphere/bulb), never our authored geometry.
   category='Reference_Primitives' if path.startswith('/Engine/') else 'Models'
   if path not in models:
    assert not LAYOUT_ONLY, 'New geometry requires a separate fresh artist export: '+path
    models[path]={'asset':path,'file':export(mesh,os.path.join(OUT,category,name_for(path)+'.fbx'),options),'slots':[str(x.material_slot_name) for x in mesh.get_editor_property('static_materials')], 'material_assets':[]}
   mats=[]
   for i in range(comp.get_num_materials()):
    mat=comp.get_material(i)
    if not mat:mats.append(None);continue
    mp=mat.get_path_name();mats.append(mp)
    if mp not in models[path]['material_assets']:models[path]['material_assets'].append(mp)
    if mp not in materials:
     assert not LAYOUT_ONLY, 'New material requires a separate fresh artist export: '+mp
     used=material_textures(mat)
     materials[mp]={'asset':mp,'textures':[]}
     for tex in used:
      tp=tex.get_path_name()
      if not tp.startswith('/Game/') or not isinstance(tex,u.Texture2D):continue
      if tp not in textures:
       textures[tp]={'asset':tp,'file':export(tex,os.path.join(OUT,'Textures',name_for(tp)+'.tga')),'srgb':tex.get_editor_property('srgb'),'compression':str(tex.get_editor_property('compression_settings')),'flip_green_channel':tex.get_editor_property('flip_green_channel')}
      materials[mp]['textures'].append(tp)
   t=comp.get_world_transform();r=t.rotation.rotator();v=t.translation;s=t.scale3d
   instances.append({'actor':actor.get_actor_label(),'component':comp.get_name(),'model':path,'materials':mats,'position_cm':[v.x,v.y,v.z],'rotation_degrees_pitch_yaw_roll':[r.pitch,r.yaw,r.roll],'scale':[s.x,s.y,s.z]})
 # One assembled static reference, including the instrument component geometry.
 reference=export(world,os.path.join(OUT,'Reference_Room.fbx'),options)
 manifest={'engine':'5.0.3','coordinate_units':'centimetres; FBX Z up; keep pivots, scale, UV and material slot order','models':list(models.values()),'materials':list(materials.values()),'textures':list(textures.values()),'instances':instances,'reference_room':reference}
 with open(os.path.join(OUT,'manifest.json'),'w',encoding='utf-8') as f:json.dump(manifest,f,ensure_ascii=False,indent=2)
 with open(os.path.join(OUT,'Models.csv'),'w',encoding='utf-8-sig',newline='') as f:
  writer=csv.writer(f,delimiter=';');writer.writerow(['FBX','UE asset','Material slots','Materials']);writer.writerows((m['file'],m['asset'],' | '.join(m['slots']),' | '.join(m['material_assets'])) for m in models.values())
 u.log('ARTIST EXPORT COMPLETE models=%d textures=%d instances=%d destination=%s' % (len(models),len(textures),len(instances),OUT))
