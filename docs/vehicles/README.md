# Namma City Vehicle Design Bible

This directory contains the vehicle specification system for the project.

- **Vehicle documents:** 677
- **Categories:** 62
- [Master index](MASTER_INDEX.md)
- [Specification schema](VEHICLE_SPEC_SCHEMA.md)
- [Physics conventions](PHYSICS_CONVENTIONS.md)
- [Branding and data policy](BRANDING_AND_DATA_POLICY.md)

## How to use these files

The current documents are **prototype vehicle design sheets**. They contain complete game-facing fields and approximate class-correct physical targets so engineering can start before a model-by-model verification pass. When a vehicle enters production:

1. choose the exact real reference/model year, if any;
2. verify source specs;
3. create the fictional production design;
4. lock game-physics values separately;
5. run the acceptance tests in the vehicle document.

## Folder naming

Folders are numbered to keep the vehicle taxonomy stable in source control. Vehicle IDs (`VEH-xx-xxx`) are persistent and should be used by backlog tasks, asset names and data tables.
