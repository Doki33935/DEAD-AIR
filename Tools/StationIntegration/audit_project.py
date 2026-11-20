"""Read-only UE5.0.3 registry/scene inventory. No package saves or deletes."""
import json, os
import unreal as u
out=os.path.join(u.Paths.project_saved_dir(), 'ProjectAudit')
os.makedirs(out,exist_ok=True)
reg=u.AssetRegistryHelpers.get_asset_registry()
reg.scan_paths_synchronous(['/Game'],True)
opts=u.AssetRegistryDependencyOptions(True,True,True,True,True)
assets=reg.get_assets_by_path('/Game',True)
rows=[]
for a in assets:
 package=str(a.package_name)
 rows.append({'path':str(a.object_path),'package':package,'class':str(a.asset_class),
 'dependencies':sorted(str(x) for x in reg.get_dependencies(package,opts) if str(x).startswith('/Game/')),
 'referencers':sorted(str(x) for x in reg.get_referencers(package,opts) if str(x).startswith('/Game/'))})
with open(os.path.join(out,'assets.json'),'w',encoding='utf-8') as f:json.dump(rows,f,ensure_ascii=False,indent=2)
u.EditorLoadingAndSavingUtils.load_map('/Game/StationIntegration/StationIntegrationMap')
actors=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
meshes={};instances=[]
for a in actors:
 for c in a.get_components_by_class(u.StaticMeshComponent):
  mesh=c.static_mesh
  if not mesh:continue
  path=mesh.get_path_name()
  materials=[c.get_material(i) for i in range(c.get_num_materials())]
  meshes[path]={'path':path,'name':mesh.get_name(),'slots':[str(x.material_slot_name) for x in mesh.get_editor_property('static_materials')]}
  t=c.get_world_transform(); r=t.rotation.rotator();v=t.translation;s=t.scale3d
  bp,be,_=u.SystemLibrary.get_component_bounds(c)
  instances.append({'bounds_centre_cm':[bp.x,bp.y,bp.z],'bounds_extent_cm':[be.x,be.y,be.z],'visible':c.is_visible(),'hidden_in_game':c.get_editor_property('hidden_in_game'),'actor':a.get_actor_label(),'actor_class':a.get_class().get_name(),'component':c.get_name(),'mesh':path,'materials':[m.get_path_name() if m else None for m in materials],'position_cm':[v.x,v.y,v.z],'rotation_degrees':[r.pitch,r.yaw,r.roll],'scale':[s.x,s.y,s.z]})
with open(os.path.join(out,'scene.json'),'w',encoding='utf-8') as f:json.dump({'meshes':list(meshes.values()),'instances':instances},f,ensure_ascii=False,indent=2)
u.log('PROJECT AUDIT COMPLETE assets=%d meshes=%d instances=%d' % (len(rows),len(meshes),len(instances)))
