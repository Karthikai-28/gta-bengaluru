# Save System

## Save data

- Player transform / safe respawn anchor
- Mission state
- Story decisions
- Money
- Inventory
- Owned vehicles
- Vehicle modifications
- Properties/businesses
- World unlocks
- Collectibles
- Settings separate from gameplay save

## Requirements

- Autosave at safe moments
- Manual save from safe locations/menu
- Atomic writes to prevent corruption
- Version number in every save
- Migration code for older saves
- Backup previous save before overwrite
- Never save temporary AI actors individually unless necessary
