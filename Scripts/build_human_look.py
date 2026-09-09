"""Blender: author an original stylized adult man; optionally skin to exported Manny.
blender -b --python Scripts/build_human_look.py -- --output Art/Characters
Pass --reference Saved/HumanLook/Manny.fbx for the production skeleton.
The reference is only used for bone locations; none of its robot surface is exported.
"""
import argparse
import json
import math
import sys
from pathlib import Path
import bpy
from mathutils import Vector

args = argparse.ArgumentParser()
args.add_argument('--output', required=True)
args.add_argument('--reference')
opt = args.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else [])
out = Path(opt.output).resolve(); out.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
if opt.reference:
    bpy.ops.import_scene.fbx(filepath=str(Path(opt.reference).resolve()), automatic_bone_orientation=False)
    rig = next(o for o in scene.objects if o.type == 'ARMATURE')
    for obj in list(scene.objects):
        if obj != rig: bpy.data.objects.remove(obj, do_unlink=True)
    # Manny's single 'root' bone tops the exported file, where Blender's importer
    # consumes it as the armature object and leaves its children as separate roots.
    # Unreal imports one root only, so put the bone back before anything is skinned.
    orphans = [b.name for b in rig.data.bones if b.parent is None]
    if len(orphans) > 1:
        bpy.context.view_layer.objects.active = rig
        bpy.ops.object.mode_set(mode='EDIT')
        edit_bones = rig.data.edit_bones
        root = edit_bones.new('root')
        root.head = (0, 0, 0)
        root.tail = Vector(root.head) + Vector((0, min(edit_bones[n].length for n in orphans), 0))
        for name in orphans:
            edit_bones[name].parent = root
            edit_bones[name].use_connect = False
        bpy.ops.object.mode_set(mode='OBJECT')
else:
    bpy.ops.object.armature_add()
    rig = bpy.context.object; rig.name = 'PreviewSkeleton'
    bpy.ops.object.mode_set(mode='EDIT'); rig.data.edit_bones.remove(rig.data.edit_bones[0])
    joints = {'root': ((0,0,0),None), 'pelvis': ((0,0,1),'root')}
    last = 'pelvis'
    for i,z in enumerate([1.1,1.2,1.3,1.4,1.48],1):
        name = f'spine_{i:02}'; joints[name] = ((0,0,z),last); last=name
    for name,z in [('neck_01',1.54),('neck_02',1.58),('head',1.62)]:
        joints[name] = ((0,0,z),last); last=name
    for side,sign in [('l',-1),('r',1)]:
        for name,xyz,parent in [('clavicle',(.06,0,1.48),'spine_05'),('upperarm',(.19,0,1.47),f'clavicle_{side}'),
            ('lowerarm',(.40,0,1.22),f'upperarm_{side}'),('hand',(.52,.01,1.00),f'lowerarm_{side}'),
            ('thigh',(.095,0,.98),'pelvis'),('calf',(.105,.025,.55),f'thigh_{side}'),('foot',(.105,0,.12),f'calf_{side}'),
            ('ball',(.105,.14,.05),f'foot_{side}')]:
            joints[f'{name}_{side}'] = ((xyz[0]*sign,xyz[1],xyz[2]),parent)
        for finger,offset in [('thumb',-.035),('index',-.017),('middle',0),('ring',.017),('pinky',.032)]:
            for i in range(1,4):
                joints[f'{finger}_{i:02}_{side}'] = (((.54+i*.018)*sign,.01+offset,.96-i*.016),
                    f'hand_{side}' if i==1 else f'{finger}_{i-1:02}_{side}')
    for name,(xyz,parent) in joints.items():
        b=rig.data.edit_bones.new(name); b.head=(xyz[0],-xyz[1],xyz[2]); b.tail=b.head+Vector((0,0,.035))
        if parent: b.parent=rig.data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')

rig.name = "Armature"  # Unreal strips this FBX container instead of adding a new root bone.
bones = {b.name: rig.matrix_world @ b.head_local for b in rig.data.bones}
required = ['pelvis','head','upperarm_l','upperarm_r','calf_l','calf_r','hand_l','hand_r']
assert all(n in bones for n in required), 'Reference must be the Manny skeleton'
origin = bones.get('root',Vector((0,0,0)))
up = (bones['head']-bones['pelvis']).normalized()
right = (bones['upperarm_r']-bones['upperarm_l']).normalized()
front = right.cross(up).normalized(); right=up.cross(front).normalized()
unit = (bones['head']-bones['pelvis']).length/.62

def world(p): return origin + unit*(right*p[0]+front*p[1]+up*p[2])
def local(p):
    d=(p-origin)/unit
    return Vector((d.dot(right),d.dot(front),d.dot(up)))
J={n:local(p) for n,p in bones.items()}
materials={}
for name,color,rough in [('Skin',(.30,.125,.062,1),.70),('Hair',(.012,.009,.007,1),.88),
    ('Shirt',(.045,.25,.31,1),.88),('Seam',(.022,.13,.17,1),.90),('Trousers',(.025,.037,.065,1),.92),
    ('Leather',(.055,.026,.014,1),.7),('Sole',(.018,.022,.026,1),.9),('EyeWhite',(.74,.72,.65,1),.4),
    ('Iris',(.035,.016,.008,1),.45),('Lip',(.19,.055,.035,1),.8),('Button',(.68,.59,.43,1),.6)]:
    m=bpy.data.materials.new(name); m.diffuse_color=color; m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF'); bs.inputs['Base Color'].default_value=color; bs.inputs['Roughness'].default_value=rough
    materials[name]=m
parts=[]

def finish(obj,name,mat,weights):
    obj.name=name; obj.data.materials.append(materials[mat])
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for poly in obj.data.polygons: poly.use_smooth=True
    for bone,weight in weights.items():
        if bone in bones: obj.vertex_groups.new(name=bone).add(list(range(len(obj.data.vertices))),weight,'REPLACE')
    parts.append(obj)
    return obj

def ellipsoid(name,p,size,mat,bone):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=world(p))
    o=bpy.context.object
    from mathutils import Matrix
    o.rotation_euler=Matrix((right,front,up)).transposed().to_euler()
    o.scale=Vector(size)*unit
    return finish(o,name,mat,{bone:1})

def tube(name,a,b,r1,r2,mat,bone,child=None):
    a,b=Vector(a),Vector(b); axis=(b-a).normalized()
    u=axis.cross(Vector((0,1,0))).normalized(); v=axis.cross(u).normalized()
    verts=[]; faces=[]; steps=8; sides=20
    for row in range(steps+1):
        t=row/steps; radius=(r1*(1-t)+r2*t)*(1+.05*math.sin(t*math.pi))
        for i in range(sides):
            angle=i*2*math.pi/sides
            verts.append(world(a.lerp(b,t)+radius*(u*math.cos(angle)+v*math.sin(angle))))
    for row in range(steps):
        for i in range(sides):
            a0=row*sides+i; a1=row*sides+(i+1)%sides
            faces.append((a0,a1,a1+sides,a0+sides))
    faces.extend([tuple(reversed(range(sides))),tuple(steps*sides+i for i in range(sides))])
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); scene.collection.objects.link(obj); finish(obj,name,mat,{})
    g=obj.vertex_groups.new(name=bone); h=obj.vertex_groups.new(name=child) if child else None
    for row in range(steps+1):
        # Blend into the child across the last 20% to close the bending joint.
        w=max(0,(row/steps-.8)/.2)*.5 if h else 0
        ids=list(range(row*sides,(row+1)*sides)); g.add(ids,1-w,'REPLACE')
        if h: h.add(ids,w,'REPLACE')
    return obj

pelvis=J['pelvis']; neck=J.get('neck_01',J['head']-Vector((0,0,.08)))
# Continuous fitted shirt, weighted between successive spine joints.
verts=[]; faces=[]; rings=[(-.10,.18,.117),(.015,.165,.115),(.13,.16,.11),(.27,.19,.12),(.40,.205,.105),(.46,.18,.09),(.48,.065,.055)]
for dz,rx,ry in rings:
    for i in range(32):
        angle=2*math.pi*i/32; verts.append(world(pelvis+Vector((rx*math.cos(angle),ry*math.sin(angle),dz))))
for j in range(len(rings)-1):
    for i in range(32): faces.append((j*32+i,j*32+(i+1)%32,(j+1)*32+(i+1)%32,(j+1)*32+i))
faces.append(tuple(reversed(range(32))))
faces.append(tuple((len(rings)-1)*32+i for i in range(32)))
mesh=bpy.data.meshes.new('Shirt');mesh.from_pydata(verts,[],faces);mesh.update()
shirt=bpy.data.objects.new('Short sleeve shirt',mesh);scene.collection.objects.link(shirt);finish(shirt,shirt.name,'Shirt',{})
spines=[n for n in ['pelvis','spine_01','spine_02','spine_03','spine_04','spine_05'] if n in J]
groups={n:shirt.vertex_groups.new(name=n) for n in spines}
for i,v in enumerate(shirt.data.vertices):
    z=local(v.co).z
    nearest=sorted(spines,key=lambda n:abs(J[n].z-z))[:2]
    ws=[1/max(.025,abs(J[n].z-z))**2 for n in nearest]
    for n,w in zip(nearest,ws): groups[n].add([i],w/sum(ws),'REPLACE')
ellipsoid('Trouser seat',pelvis+Vector((0,0,-.055)),(.15,.105,.125),'Trousers','pelvis')
tube('Neck',pelvis+Vector((0,0,.445)),J['head']+Vector((0,0,.02)),.051,.050,'Skin','neck_01','head')
for side in ['l','r']:
    hip,knee,ankle=[J[f'{n}_{side}'] for n in ['thigh','calf','foot']]
    tube('Trouser upper '+side,hip+Vector((0,0,.02)),knee,.085,.06,'Trousers',f'thigh_{side}',f'calf_{side}')
    tube('Trouser lower '+side,knee,ankle+Vector((0,0,.025)),.061,.041,'Trousers',f'calf_{side}',f'foot_{side}')
    ellipsoid('Knee '+side,knee,(.06,.06,.061),'Trousers',f'calf_{side}')
    foot=ankle+Vector((0,.065,-.048))
    tube('Sock '+side,ankle+Vector((0,0,.05)),ankle-Vector((0,0,.05)),.038,.037,'Trousers',f'foot_{side}')
    ellipsoid('Shoe '+side,foot,(.058,.13,.053),'Leather',f'foot_{side}')
    ellipsoid('Sole '+side,foot+Vector((0,0,-.032)),(.059,.132,.023),'Sole',f'foot_{side}')
    shoulder,elbow,wrist=[J[f'{n}_{side}'] for n in ['upperarm','lowerarm','hand']]
    sleeve=shoulder.lerp(elbow,.57)
    ellipsoid('Shirt shoulder '+side,shoulder,(.083,.083,.083),'Shirt',f'upperarm_{side}')
    tube('Sleeve '+side,shoulder,sleeve,.081,.066,'Shirt',f'upperarm_{side}')
    tube('Upper arm '+side,sleeve,elbow,.055,.042,'Skin',f'upperarm_{side}',f'lowerarm_{side}')
    ellipsoid('Elbow '+side,elbow,(.042,.043,.042),'Skin',f'lowerarm_{side}')
    tube('Forearm '+side,elbow,wrist,.044,.027,'Skin',f'lowerarm_{side}',f'hand_{side}')
    ends=[J[n] for n in [f'index_01_{side}',f'pinky_01_{side}'] if n in J]
    palm_end=sum(ends,Vector())/len(ends) if ends else wrist+Vector((0,0,-.07))
    tube('Palm '+side,wrist,palm_end,.029,.035,'Skin',f'hand_{side}')
    for finger in ['thumb','index','middle','ring','pinky']:
        for i in range(1,4):
            name=f'{finger}_{i:02}_{side}'
            if name not in J: continue
            a=J[name]; nxt=f'{finger}_{i+1:02}_{side}'
            b=J[nxt] if nxt in J else a+(a-J[f'{finger}_{i-1:02}_{side}'])*.65
            tube(name,a,b,.009 if finger!='thumb' else .012,.007,'Skin',name,nxt if nxt in J else None)
            ellipsoid('Knuckle '+name,a,(.009,.009,.009),'Skin',name)
# Face is original geometry, with separate lids, irises, lips, nose, ears and hair.
head=J['head']+Vector((0,0,.025))
ellipsoid('Face',head,(.080,.073,.119),'Skin','head')
ellipsoid('Jaw',head+Vector((0,.006,-.065)),(.063,.061,.050),'Skin','head')
for sign in [-1,1]:
    ellipsoid('Ear',head+Vector((sign*.079,0,-.005)),(.017,.022,.034),'Skin','head')
    ellipsoid('Eye socket',head+Vector((sign*.031,.062,.02)),(.027,.016,.021),'Skin','head')
    ellipsoid('Eye',head+Vector((sign*.031,.074,.022)),(.017,.004,.007),'EyeWhite','head')
    ellipsoid('Iris',head+Vector((sign*.031,.078,.022)),(.007,.0025,.007),'Iris','head')
    ellipsoid('Eye glint',head+Vector((sign*.031-.002,.080,.024)),(.0018,.001,.0018),'EyeWhite','head')
    ellipsoid('Brow',head+Vector((sign*.031,.072,.038)),(.022,.008,.004),'Hair','head')
    ellipsoid('Moustache',head+Vector((sign*.013,.073,-.036)),(.018,.009,.006),'Hair','head')
ellipsoid('Nose bridge',head+Vector((0,.071,-.002)),(.013,.02,.029),'Skin','head')
ellipsoid('Nose tip',head+Vector((0,.085,-.017)),(.019,.020,.012),'Skin','head')
ellipsoid('Lower lip',head+Vector((0,.065,-.049)),(.024,.009,.005),'Lip','head')
# Close-cropped hair cap, open below the hairline rather than a second full head.
verts=[];faces=[]
for row in range(9):
    phi=.02+(1.4-.02)*row/8
    for i in range(32):
        theta=2*math.pi*i/32
        verts.append(world(head+Vector((.082*math.sin(phi)*math.cos(theta),.075*math.sin(phi)*math.sin(theta),.122*math.cos(phi)))))
for j in range(8):
    for i in range(32): faces.append((j*32+i,j*32+(i+1)%32,(j+1)*32+(i+1)%32,(j+1)*32+i))
mesh=bpy.data.meshes.new('Hair cap');mesh.from_pydata(verts,[],faces);mesh.update()
obj=bpy.data.objects.new('Short black hair',mesh);scene.collection.objects.link(obj);finish(obj,obj.name,'Hair',{'head':1})
# Shirt details sit on the chest and follow the same spine region.
for dz in [.12,.21,.30,.39]:
    bone=min(spines,key=lambda n:abs(J[n].z-(pelvis.z+dz)))
    ellipsoid('Shirt button',pelvis+Vector((0,.122,dz)),(.006,.004,.006),'Button',bone)
for sign in [-1,1]:
    ellipsoid('Collar',pelvis+Vector((sign*.055,.066,.463)),(.036,.015,.016),'Seam','spine_05')

# Weld the shirt and sleeves into a continuous garment. Transfer the original
# blend weights back by nearest surface vertices after the voxel union.
from mathutils.kdtree import KDTree
cloth=[o for o in parts if o.data.materials[0].name == 'Shirt']
bpy.ops.object.select_all(action='DESELECT')
for o in cloth: o.select_set(True)
bpy.context.view_layer.objects.active=shirt;bpy.ops.object.join()
parts=[o for o in parts if o not in cloth or o==shirt]
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.object.mode_set(mode='OBJECT')
positions=[v.co.copy() for v in shirt.data.vertices]
weights=[{shirt.vertex_groups[g.group].name:g.weight for g in v.groups} for v in shirt.data.vertices]
tree=KDTree(len(positions))
for i,p in enumerate(positions): tree.insert(p,i)
tree.balance()
remesh=shirt.modifiers.new('Continuous shirt fabric','REMESH');remesh.mode='VOXEL';remesh.voxel_size=.009*unit
bpy.ops.object.modifier_apply(modifier=remesh.name)
smooth=shirt.modifiers.new('Relax fabric','SMOOTH');smooth.factor=.65;smooth.iterations=5
bpy.ops.object.modifier_apply(modifier=smooth.name)
shirt.vertex_groups.clear()
for name in bones: shirt.vertex_groups.new(name=name)
for v in shirt.data.vertices:
    nearest=tree.find_n(v.co,3); blended={};total=0
    for co,idx,dist in nearest:
        weight=1/max(.001*unit,dist)**2;total+=weight
        for name,value in weights[idx].items(): blended[name]=blended.get(name,0)+weight*value
    for name,value in blended.items(): shirt.vertex_groups[name].add([v.index],value/total,'REPLACE')
for poly in shirt.data.polygons: poly.use_smooth=True

# One skinned draw object. Reference skeleton hierarchy and bind transforms survive.
bpy.ops.object.select_all(action='DESELECT')
for obj in parts: obj.select_set(True)
bpy.context.view_layer.objects.active=shirt; bpy.ops.object.join()
body=bpy.context.object; body.name='SK_NammaMan'
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False);bpy.ops.object.mode_set(mode='OBJECT')
mod=body.modifiers.new('Human joints','ARMATURE');mod.object=rig
body.parent=rig;body.matrix_parent_inverse=rig.matrix_world.inverted()
assert all(len(v.groups)>0 for v in body.data.vertices), 'Unweighted vertex'
bpy.ops.object.select_all(action='DESELECT');body.select_set(True);rig.select_set(True)
bpy.context.view_layer.objects.active=rig
bpy.ops.export_scene.fbx(filepath=str(out/'SK_NammaMan.fbx'),use_selection=True,object_types={'ARMATURE','MESH'},
    add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',use_armature_deform_only=False)
(out/'manifest.json').write_text(json.dumps({'style':'Original stylized South Indian adult man',
    'production_skeleton':bool(opt.reference),'vertices':len(body.data.vertices),'bones':len(rig.data.bones),
    'materials':list(materials),'reference':opt.reference},indent=2)+'\n')
# A source-art review render, clearly separate from the Unreal gameplay test.
scene.render.engine='CYCLES';scene.cycles.samples=24
scene.render.resolution_x=720;scene.render.resolution_y=900;scene.render.resolution_percentage=100
scene.world.color=(.22,.22,.22)
bpy.ops.object.camera_add(location=world((2.6,5.0,2.4)))
cam=bpy.context.object;target=world((0,0,1.0));cam.rotation_euler=(target-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO';cam.data.ortho_scale=2.15*unit;scene.camera=cam
for pos,power,size in [((2,3,4),450,3),((-3,1,2),250,3),((0,-2,3),350,2)]:
    bpy.ops.object.light_add(type='AREA',location=world(pos));lamp=bpy.context.object
    lamp.data.energy=power*unit*unit;lamp.data.shape='DISK';lamp.data.size=size*unit
    lamp.rotation_euler=(target-lamp.location).to_track_quat('-Z','Y').to_euler()
scene.render.film_transparent=False
scene.render.filepath=str(out/'human-look-preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(out/'NammaMan.blend'))
# Export evaluated triangles for a deterministic software review render. This
# host's Blender install lacks its color-management/render resources.
depsgraph=bpy.context.evaluated_depsgraph_get()
evaluated=body.evaluated_get(depsgraph)
review=evaluated.to_mesh();review.calc_loop_triangles()
preview={
    'vertices':[list(local(evaluated.matrix_world @ v.co)) for v in review.vertices],
    'normals':[[v.normal.dot(right),v.normal.dot(front),v.normal.dot(up)] for v in review.vertices],
    'triangles':[list(t.vertices) for t in review.loop_triangles],
    'material_indices':[t.material_index for t in review.loop_triangles],
    'colors':[list(m.diffuse_color) for m in body.data.materials],
}
(out/'review-mesh.json').write_text(json.dumps(preview))
evaluated.to_mesh_clear()
print('NAMMA_HUMAN_ART_OK',out)
