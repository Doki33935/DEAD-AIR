"""Desk fit and warm lighting. Requires current -FurnitureGeometryAudit output.
Editor-only tool: real collision, seating and clicks are verified in PIE.
"""
import json, os
import unreal as u
world=u.EditorLoadingAndSavingUtils.load_map('/Game/StationIntegration/StationIntegrationMap')
editor=u.get_editor_subsystem(u.EditorActorSubsystem);actors=editor.get_all_level_actors()
radio=next(a for a in actors if a.get_class().get_name()=='BP_RadioIntegrated_C')
deck=next(a for a in actors if a.get_class().get_name()=='BP_TapeDeckIntegrated_C')
def component(a,name):return next(c for c in a.get_components_by_class(u.StaticMeshComponent) if c.get_name()==name)
def centre(a,name):return u.SystemLibrary.get_component_bounds(component(a,name))[0]
# Move complete actors: controls, audio and their owned close-up camera follow together.
rc=centre(radio,'Radio_Body');radio.set_actor_location(radio.get_actor_location()+u.Vector(498-rc.x,404-rc.y,0),False,True)
dc=centre(deck,'Base_dec');deck.set_actor_location(deck.get_actor_location()+u.Vector(414-dc.x,402-dc.y,0),False,True)
positions={'SM_Headset_643eb9':u.Vector(503,273,92),'SM_single_spiral_notepad_c51708':u.Vector(478,345,91.13720454),'SM_pencil_holder':u.Vector(422,439,98),'SM_SM_yghlabs_tier_2_63a59e':u.Vector(447,410,90)}
for actor in actors:
 if isinstance(actor,u.StaticMeshActor) and actor.static_mesh_component.static_mesh:
  name=actor.static_mesh_component.static_mesh.get_name()
  if name in positions:actor.set_actor_location(positions[name],False,True)
# The tower is removed from the scene. Keep its source asset out of this mutation.
for actor in actors:
 if isinstance(actor,u.StaticMeshActor) and actor.static_mesh_component.static_mesh and actor.static_mesh_component.static_mesh.get_name()=='SM_scene_50c5ec':
  editor.destroy_actor(actor)
actors=editor.get_all_level_actors()
table=next(a for a in actors if isinstance(a,u.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name().endswith('_54910a'))
# Commandlet worlds have no physics scene. Use transformed triangles from
# -FurnitureGeometryAudit; actual collision is verified separately in PIE.
def geometry(name):
 with open(os.path.join(u.Paths.project_saved_dir(),'FurnitureFit',name+'.tsv'),encoding='utf-8-sig') as f:
  points=[tuple(map(float,line.split()[:3])) for line in f if line.strip()]
 return [points[i:i+3] for i in range(0,len(points),3)]
table_triangles=geometry('table');well_triangles=geometry('Base_dec')
assert abs(min(p[0] for tri in table_triangles for p in tri if p[1]<373.82 and p[2]>89)-455)<.02,'Run FurnitureWidenTop and FurnitureGeometryAudit first'
def surface_height(triangles,x,y,ceiling=160):
 hits=[]
 for a,b,c in triangles:
  det=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
  if abs(det)<1e-8:continue
  w1=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/det
  w2=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/det
  if min(w1,w2,1-w1-w2)<-1e-7:continue
  z=w1*a[2]+w2*b[2]+(1-w1-w2)*c[2]
  if z<ceiling:hits.append(z)
 assert hits,'No supporting surface under %.3f %.3f' % (x,y)
 return max(hits)
def table_height(x,y):return surface_height(table_triangles,x,y)
contacts=[]
for actor in actors:
 if not isinstance(actor,u.StaticMeshActor) or not actor.static_mesh_component.static_mesh:continue
 name=actor.static_mesh_component.static_mesh.get_name()
 if name not in ['SM_Headset_643eb9','SM_single_spiral_notepad_c51708','SM_KeyboardCC0','SM_Med_PC_Mouse','SM_television_2_32fe98']:continue
 c,e,_=u.SystemLibrary.get_component_bounds(actor.static_mesh_component)
 height=table_height(c.x,c.y)
 for dx in [-.9,0,.9]:
  for dy in [-.9,0,.9]:assert abs(table_height(c.x+dx*e.x,c.y+dy*e.y)-height)<.15,name+' extends off the tabletop'
 actor.set_actor_location(actor.get_actor_location()+u.Vector(0,0,height+.02-(c.z-e.z)),False,True)
 c,e,_=u.SystemLibrary.get_component_bounds(actor.static_mesh_component)
 contacts.append({'mesh':name,'bottom_cm':c.z-e.z,'surface_cm':height,'gap_cm':c.z-e.z-height})
# The well floor is 96.88 cm in this source-sized deck; glass is the cover, not a solid collider.
glass=next(c for c in deck.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='Detail_Стекло')
glass.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);glass.set_cast_shadow(False)
tape=next(a for a in actors if a.get_class().get_name()=='BP_CassetteIntegrated_C')
shell=next(c for c in tape.get_components_by_class(u.StaticMeshComponent) if c.get_name()=='CassetteShell')
c,e,_=u.SystemLibrary.get_component_bounds(shell)
visual_bottom_offset=c.z-e.z-tape.get_actor_location().z
socket=next(c for c in deck.get_components_by_class(u.SceneComponent) if c.get_name()=='CassetteSocket')
tray=next(c for c in deck.get_components_by_class(u.SceneComponent) if c.get_name()=='CassetteTray')
loc=socket.get_world_transform().translation
well_floor=surface_height(well_triangles,loc.x,loc.y,98.2)
socket.set_world_location(u.Vector(loc.x,loc.y,well_floor+.025-visual_bottom_offset),False,True)
loc=tray.get_world_transform().translation;tray.set_world_location(u.Vector(loc.x,loc.y,table_height(loc.x,loc.y)+.02-visual_bottom_offset),False,True)
tape_scale=tape.get_actor_scale3d();tape.set_actor_location(tray.get_world_transform().translation,False,True);tape.set_actor_rotation(tray.get_world_transform().rotation.rotator(),False);tape.set_actor_scale3d(tape_scale)
glass_material=u.load_asset('/Game/StationIntegration/Art/Materials/M_DeckGlass')
opacity=u.MaterialEditingLibrary.get_material_property_input_node(glass_material,u.MaterialProperty.MP_OPACITY)
if not isinstance(opacity,u.MaterialExpressionConstant):
 opacity=u.MaterialEditingLibrary.create_material_expression(glass_material,u.MaterialExpressionConstant)
 assert u.MaterialEditingLibrary.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
opacity.set_editor_property('r',.10)
u.MaterialEditingLibrary.recompile_material(glass_material);assert u.EditorAssetLibrary.save_loaded_asset(glass_material,False)
# The imported fixture used WorldGridMaterial, making its cage look white.
housing=u.load_asset('/Game/RoomArt/Materials/M_RoomLampHousing')
if not housing:
 housing=u.AssetToolsHelpers.get_asset_tools().create_asset('M_RoomLampHousing','/Game/RoomArt/Materials',u.Material,u.MaterialFactoryNew())
 for prop,cls,value in [(u.MaterialProperty.MP_BASE_COLOR,u.MaterialExpressionConstant3Vector,u.LinearColor(.035,.025,.018,1)),(u.MaterialProperty.MP_METALLIC,u.MaterialExpressionConstant,.60),(u.MaterialProperty.MP_ROUGHNESS,u.MaterialExpressionConstant,.72)]:
  node=u.MaterialEditingLibrary.create_material_expression(housing,cls)
  node.set_editor_property('constant' if cls==u.MaterialExpressionConstant3Vector else 'r',value)
  assert u.MaterialEditingLibrary.connect_material_property(node,'',prop)
 u.MaterialEditingLibrary.recompile_material(housing);assert u.EditorAssetLibrary.save_loaded_asset(housing,False)
fixture=next(a for a in actors if isinstance(a,u.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name()=='SM_ceilinglamp_c60969')
fixture.static_mesh_component.set_material(0,housing)
central=next(a for a in actors if a.get_actor_label()=='ROOM — central dim bulb')
central.light_component.set_editor_property('use_temperature',True)
central.light_component.set_editor_property('temperature',3300)
central.light_component.set_light_color(u.LinearColor(1,1,1,1))
central.light_component.set_intensity(1.25)
bulb_material=u.load_asset('/Game/RoomArt/Materials/M_RoomBulb')
emission=u.MaterialEditingLibrary.get_material_property_input_node(bulb_material,u.MaterialProperty.MP_EMISSIVE_COLOR)
if not isinstance(emission,u.MaterialExpressionConstant3Vector):
 emission=u.MaterialEditingLibrary.create_material_expression(bulb_material,u.MaterialExpressionConstant3Vector)
 assert u.MaterialEditingLibrary.connect_material_property(emission,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
emission.set_editor_property('constant',u.LinearColor(.33,.22,.12,1))
u.MaterialEditingLibrary.recompile_material(bulb_material);assert u.EditorAssetLibrary.save_loaded_asset(bulb_material,False)
lamp=next(a for a in actors if a.get_actor_label()=='ROOM — tiny desk lamp')
lamp.set_actor_location(u.Vector(449,388,131),False,True)
lamp.light_component.set_mobility(u.ComponentMobility.MOVABLE)
lamp.light_component.set_editor_property('intensity_units',u.LightUnits.CANDELAS)
lamp.light_component.set_editor_property('use_temperature',True)
lamp.light_component.set_editor_property('temperature',2900)
lamp.light_component.set_light_color(u.LinearColor(1,1,1,1))
lamp.light_component.set_intensity(.08)
lamp.light_component.set_editor_property('attenuation_radius',100)
lamp.light_component.set_editor_property('source_radius',.7)
# Validate real world-space body bounds, not asset-origin positions.
rb,re,_=u.SystemLibrary.get_component_bounds(component(radio,'Radio_Body'))
db,de,_=u.SystemLibrary.get_component_bounds(component(deck,'Base_dec'))
def separated(p,e,q,f):return abs(p.x-q.x)>e.x+f.x or abs(p.y-q.y)>e.y+f.y or abs(p.z-q.z)>e.z+f.z
assert separated(rb,re,db,de)
for actor in actors:
 if isinstance(actor,u.StaticMeshActor) and actor.static_mesh_component.static_mesh and actor.static_mesh_component.static_mesh.get_name() in positions:
  if 'yghlabs' in actor.static_mesh_component.static_mesh.get_name():continue # lamp head is above the radio; its AABB includes empty arm space
  p,e,_=u.SystemLibrary.get_component_bounds(actor.static_mesh_component)
  assert separated(rb,re,p,e),actor.get_actor_label()+' overlaps radio'
  assert separated(db,de,p,e),actor.get_actor_label()+' overlaps deck'
# Only the printed glyphs receive a restrained emissive lift, not the deck body.
labels=u.load_asset('/Game/StationIntegration/Art/Materials/M_DeckLabelLowLight')
node=u.MaterialEditingLibrary.get_material_property_input_node(labels,u.MaterialProperty.MP_EMISSIVE_COLOR)
assert isinstance(node,u.MaterialExpressionMultiply)
if abs(node.get_editor_property('const_b')-.07)>1e-6:
 node.set_editor_property('const_b',.07)
 u.MaterialEditingLibrary.recompile_material(labels)
 assert u.EditorAssetLibrary.save_loaded_asset(labels,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/StationIntegration/StationIntegrationMap')
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/RoomIntegration/RoomIntegrationMap')
report={'surface_contacts':contacts,'well_floor_cm':well_floor,'cassette_visual_bottom_offset_cm':visual_bottom_offset,'cassette_socket_relative_z':socket.get_editor_property('relative_location').z,'cassette_tray_relative_z':tray.get_editor_property('relative_location').z,'central_temperature_kelvin':3300,'system_unit_removed':True,'computer_top_width_cm':100.15,'computer_top_front_y_cm':255,'radio_body_centre_cm':[rb.x,rb.y,rb.z],'deck_body_centre_cm':[db.x,db.y,db.z],'lamp_temperature_kelvin':2900,'lamp_candela':.08,'lamp_radius_cm':100,'prop_actor_positions':{a.static_mesh_component.static_mesh.get_name():[a.get_actor_location().x,a.get_actor_location().y,a.get_actor_location().z] for a in actors if isinstance(a,u.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name() in positions},'overview_height_cm':137.31442656,'manual_radio_widget_layout':'preserved'}
with open(os.path.join(u.Paths.project_saved_dir(),'ProjectAudit','workstation-layout.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('WORKSTATION STYLE COMPLETE %s' % report)
