# Combat System

## Design goal

Cinematic third-person action with readable feedback and data-driven tuning.

## Combat layers

1. Unarmed/melee
2. Firearms
3. Throwables
4. Explosive/projectile weapons
5. Environmental damage
6. Vehicle impact damage

## Core weapon interface

```text
CanFire()
Fire()
StopFire()
Reload()
Equip()
Unequip()
GetAmmoState()
GetAimModifiers()
```

## Damage model

Use gameplay-level abstractions rather than over-simulating anatomy.

- Base damage
- Distance falloff
- Hit region multiplier
- Armor modifier
- Critical state
- Knockdown threshold
- Explosion impulse

## AI requirements

Combat NPCs should:

- Seek cover
- Reposition
- Suppress
- Retreat when appropriate
- React to allies going down
- Investigate sounds
- Lose track of player when line of sight breaks
