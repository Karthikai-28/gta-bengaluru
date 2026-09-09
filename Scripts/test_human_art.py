"""Run through Blender against the generated .blend; verifies actual skin deformation."""
import bpy
import math

body=bpy.data.objects['SK_NammaMan']
rig=next(m.object for m in body.modifiers if m.type=='ARMATURE')
assert 1000<len(body.data.vertices)<150000
for v in body.data.vertices:
    assert all(math.isfinite(x) for x in v.co)
    assert abs(sum(g.weight for g in v.groups)-1)<1e-4, f'Invalid skin weight sum at {v.index}'

def vertices():
    bpy.context.view_layer.update()
    evaluated=body.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh=evaluated.to_mesh()
    result=[v.co.copy() for v in mesh.vertices]
    evaluated.to_mesh_clear()
    return result

baseline=vertices()
head_group=body.vertex_groups['head'].index
head_ids=[v.index for v in body.data.vertices if any(g.group==head_group and g.weight>.99 for g in v.groups)]
foot_group=body.vertex_groups['foot_l'].index
foot_ids=[v.index for v in body.data.vertices if any(g.group==foot_group and g.weight>.99 for g in v.groups)]
assert len(head_ids)>100 and len(foot_ids)>100
head=rig.pose.bones['head'];head.rotation_mode='XYZ';head.rotation_euler.z=.4
posed=vertices()
assert max((posed[i]-baseline[i]).length for i in head_ids)>.001
assert max((posed[i]-baseline[i]).length for i in foot_ids)<1e-5
head.rotation_euler.z=0
bpy.context.view_layer.update()
print(f'NAMMA_HUMAN_ART_TEST_PASS: {len(baseline)} weighted vertices; head follows pose, feet stay fixed')
