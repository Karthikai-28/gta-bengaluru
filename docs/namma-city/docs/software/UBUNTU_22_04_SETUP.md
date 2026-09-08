# Ubuntu 22.04 Development Setup

## Recommended workstation baseline

- Ubuntu 22.04 LTS
- 32 GB RAM minimum; 64 GB preferred for large Unreal projects
- NVMe SSD; keep project + DerivedDataCache on fast storage
- Modern NVIDIA GPU strongly recommended for easiest UE5 Linux workflow
- Recent proprietary NVIDIA driver if using NVIDIA
- 12+ logical CPU threads recommended

## Install base packages

```bash
sudo apt update
sudo apt install -y \
  build-essential clang lld cmake ninja-build git git-lfs \
  python3 python3-pip python3-venv pkg-config \
  libvulkan1 vulkan-tools mesa-utils \
  unzip zip curl wget rsync jq

git lfs install
```

## Useful packages

```bash
sudo apt install -y qgis blender krita gimp inkscape audacity
```

Some packages may be older in Ubuntu repositories. For production work, prefer the vendor-supported distribution method when you need newer versions.

## Unreal Engine source build approach

On Linux, Unreal development commonly uses the Epic source distribution workflow. Keep the engine outside the game repository.

Suggested layout:

```text
~/dev/
├── engines/
│   └── UnrealEngine/
├── games/
│   └── namma-city/
├── tools/
│   └── world-builder/
└── datasets/
    └── bengaluru/
```

## Environment conventions

- Use lowercase, ASCII-only paths where possible.
- Avoid spaces in engine/project paths.
- Keep generated GIS datasets out of Git unless small.
- Put large assets in Git LFS.
- Never commit credentials/API keys.

## Suggested Git LFS patterns

```text
*.uasset
*.umap
*.fbx
*.blend
*.wav
*.flac
*.exr
*.hdr
*.png
*.tga
*.psd
```

## Linux build checks

Before adding gameplay systems, verify:

1. Blank UE5 project starts successfully.
2. Vulkan renderer works.
3. Packaged Development build launches.
4. Gamepad is detected.
5. Audio works.
6. Unreal Insights can capture traces.
7. Git LFS pull/checkout works.
