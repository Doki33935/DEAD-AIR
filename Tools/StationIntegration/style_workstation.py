"""Warm task light and a clear two-instrument composition; retain device dimensions/UI."""
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
positions={'SM_Headset_643eb9':u.Vector(460,327,92),'SM_single_spiral_notepad_c51708':u.Vector(440,360,91.13720454),'SM_pencil_holder':u.Vector(422,439,98),'SM_SM_yghlabs_tier_2_63a59e':u.Vector(447,410,90)}
for actor in actors:
 if isinstance(actor,u.StaticMeshActor) and actor.static_mesh_component.static_mesh:
  name=actor.static_mesh_component.static_mesh.get_name()
  if name in positions:actor.set_actor_location(positions[name],False,True)
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
node.set_editor_property('const_b',.07)
u.MaterialEditingLibrary.recompile_material(labels)
assert u.EditorAssetLibrary.save_loaded_asset(labels,False)
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/StationIntegration/StationIntegrationMap')
assert u.EditorLoadingAndSavingUtils.save_map(world,'/Game/RoomIntegration/RoomIntegrationMap')
report={'radio_body_centre_cm':[rb.x,rb.y,rb.z],'deck_body_centre_cm':[db.x,db.y,db.z],'lamp_temperature_kelvin':2900,'lamp_candela':.08,'lamp_radius_cm':100,'prop_actor_positions':{k:[v.x,v.y,v.z] for k,v in positions.items()},'overview_height_cm':137.31442656,'manual_radio_widget_layout':'preserved'}
with open(os.path.join(u.Paths.project_saved_dir(),'ProjectAudit','workstation-layout.json'),'w',encoding='utf-8') as f:json.dump(report,f,ensure_ascii=False,indent=2)
u.log('WORKSTATION STYLE COMPLETE %s' % report)
