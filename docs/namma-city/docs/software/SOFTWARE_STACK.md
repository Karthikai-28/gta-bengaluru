# Software Stack

## Active Phase 1A constraints

Use a pinned Epic installed Linux UE5 build, C++, Enhanced Input and Unreal Python on the existing 15 GiB / Intel integrated-GPU machine. Start with primitive-based stylized art, low rendering settings and 720p/30. MetaHumans, motion matching, heavy GIS tooling, and population systems listed below are future options, not startup dependencies. See [workstation setup](UBUNTU_22_04_SETUP.md).

## Core development

| Need | Recommended tool | Purpose |
|---|---|---|
| Game engine | Unreal Engine 5 | World, rendering, gameplay, physics, AI |
| Gameplay code | C++ | Core systems and performance-sensitive logic |
| Rapid scripting | Unreal Blueprints | Missions, prototyping, tuning |
| IDE | JetBrains Rider or VS Code | C++/project editing |
| Version control | Git + Git LFS initially | Source and medium-size assets |
| Large-team VCS later | Perforce | Large binary asset workflows |

## World and GIS

| Tool | Use |
|---|---|
| QGIS | Inspect and clean geospatial data |
| OpenStreetMap | Base road/building topology subject to license obligations |
| GDAL/OGR | GIS conversion and automation |
| Python | Procedural preprocessing and validation |
| Blender | Modeling, Geometry Nodes, cleanup, LOD generation |
| Houdini Indie/FX optional | Advanced procedural city generation |

## Art

- Blender
- Krita or GIMP
- Substance 3D Painter / Designer if licensed
- Material Maker as an open-source alternative
- PureRef for reference boards
- Inkscape for vectors/icons

## Characters

- MetaHuman for early prototypes
- Blender for custom characters
- Mixamo only for prototyping where license is appropriate
- Unreal Control Rig
- Motion Matching / animation blueprint systems

## Audio

- REAPER or Ardour
- Audacity for quick edits
- Unreal MetaSounds
- Optional Wwise/FMOD later if the audio project becomes complex

## Engineering / automation

- Python 3
- CMake for standalone utilities
- clang-format
- clang-tidy
- pre-commit
- GitHub/GitLab CI or self-hosted CI
- RenderDoc
- Unreal Insights
- NVIDIA Nsight when on supported NVIDIA hardware

## Documentation and planning

- Markdown repository
- Mermaid diagrams in Markdown
- Jira / GitHub Issues / Linear for backlog tracking
- Draw.io / Excalidraw for diagrams

## Avoid as a foundation

- Scraping Google Street View imagery
- Downloading Google imagery to rebuild and redistribute a city
- Using real trademarks, logos, or branded vehicles without permission
- Building production workflows around web assets with unclear licenses
