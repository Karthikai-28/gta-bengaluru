"""CPU triangle rasterizer for source-art review on hosts without Blender render data.
Reads evaluated mesh geometry; this is not a mock-up or an Unreal gameplay capture.
"""
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image

source=Path(sys.argv[1]);output=Path(sys.argv[2])
data=json.loads(source.read_text())
v=np.asarray(data['vertices']);triangles=np.asarray(data['triangles']);normals=np.asarray(data['normals'])
colors=np.asarray(data['colors'])
size=(720,900)
eye=np.array([2.6,5,2.4]);target=np.array([0,0,.98])
forward=target-eye;forward/=np.linalg.norm(forward)
right=np.cross(forward,[0,0,1]);right/=np.linalg.norm(right)
up=np.cross(right,forward)
relative=v-target
screen=np.stack([relative@right*size[1]/2.15+size[0]/2,-relative@up*size[1]/2.15+size[1]/2,relative@forward],axis=1)
buffer=np.full((size[1],size[0]),np.inf)
canvas=np.empty((size[1],size[0],3));canvas[:]=[.12,.155,.18]
light=np.array([-2.,4.,5.]);light/=np.linalg.norm(light)
for ids,material in zip(triangles,data['material_indices']):
    pts=screen[ids];low=np.maximum(np.floor(pts[:,:2].min(axis=0)).astype(int),0)
    high=np.minimum(np.ceil(pts[:,:2].max(axis=0)).astype(int),np.array(size)-1)
    if np.any(high<low):continue
    xx,yy=np.meshgrid(np.arange(low[0],high[0]+1)+.5,np.arange(low[1],high[1]+1)+.5)
    a,b,c=pts
    denominator=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
    if abs(denominator)<1e-8:continue
    w0=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/denominator
    w1=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/denominator
    w2=1-w0-w1;depth=w0*a[2]+w1*b[2]+w2*c[2]
    region=buffer[low[1]:high[1]+1,low[0]:high[0]+1]
    mask=(w0>=0)&(w1>=0)&(w2>=0)&(depth<region)
    if not mask.any():continue
    n=w0[...,None]*normals[ids[0]]+w1[...,None]*normals[ids[1]]+w2[...,None]*normals[ids[2]]
    n/=np.maximum(np.linalg.norm(n,axis=2,keepdims=True),1e-8)
    shade=.33+.67*np.maximum(0,n@light)
    rgb=np.clip(colors[material][:3]*shade[...,None],0,1)**(1/2.2)
    pixels=canvas[low[1]:high[1]+1,low[0]:high[0]+1]
    pixels[mask]=rgb[mask];region[mask]=depth[mask]
assert np.isfinite(buffer).sum()>15000, 'Mesh did not render'
Image.fromarray((canvas*255).astype('uint8')).save(output)
print(f'Human source preview rendered: {output}')
