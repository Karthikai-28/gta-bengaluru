# Risk Register

| Risk | Impact | Mitigation |
|---|---|---|
| Scope becomes GTA-sized too early | Critical | Vertical-slice gate before expansion |
| City generation looks generic | High | Hand-author hero locations and local asset kits |
| Traffic AI consumes too much CPU | High | Simulation LODs and reduced AI outside player bubble |
| Too many unique high-resolution assets | High | Modular kits, atlases, Nanite where appropriate, HLOD |
| Linux-specific engine/plugin issues | Medium | Keep engine/plugin versions pinned and tested |
| Google data licensing problems | Critical | Use only permitted APIs/data; prefer OSM/GIS for production topology |
| Real brands create legal exposure | High | Fictional brands and modified designs |
| Mission system becomes Blueprint spaghetti | High | C++ framework + data-driven mission definitions |
| Save files break after updates | High | Versioned save schema and migrations |
| NPC population causes frame drops | High | Crowd LOD states and pooled actors |
| One developer becomes bottlenecked by art | Critical | Procedural generation + marketplace/licensed placeholder assets for prototype |
