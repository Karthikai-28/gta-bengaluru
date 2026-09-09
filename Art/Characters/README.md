# Namma human source art

Original stylized adult man: brown skin, short black hair, moustache, teal short-sleeve shirt, dark trousers and brown shoes. This is a designed individual for the Bengaluru setting; it does not attempt to represent every South Indian person's appearance.

`NammaMan.blend` and `SK_NammaMan.fbx` contain the source-art preview with an approximate skeleton. **Do not import that preview FBX as a replacement for Manny.** `Scripts/setup_human_look.sh` first exports the project's actual Manny bind skeleton, generates a production mesh around those bones in `Production/`, then imports it on the existing Unreal skeleton and physics asset. It does not alter animation blueprints or skeleton reference poses. The character loads the new mesh when that import is present.

Generate source art without Unreal:

```sh
blender -b --factory-startup -noaudio --python Scripts/build_human_look.py -- --output Art/Characters
python3 Scripts/render_human_preview.py Art/Characters/review-mesh.json Art/Characters/human-look-preview.png
```

The preview PNG is a software rasterization of the actual evaluated mesh with vertex normals and material colors, not a gameplay screenshot. The host Blender installation lacks working render/color-management resources. Source art has been generated and visually reviewed; all 45,528 vertices have normalized skin weights, and a Blender pose check confirms the face follows the head while the feet remain fixed; production bind-skeleton export, Unreal import, deformation checks and gameplay verification remain pending because engine execution was blocked by automatic approval review for workspace credits.

The mesh includes weighted fingers and a welded shirt/sleeve surface. The face uses geometric eyes, nose, lips, ears, brows and moustache. It is a stylized model, not a photoreal scanned character or a facial-animation rig.
