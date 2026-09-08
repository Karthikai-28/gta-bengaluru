# Player Controller

## Keyboard and mouse default

| Input | Action |
|---|---|
| WASD | Move |
| Mouse | Camera/look |
| Shift | Sprint |
| Space | Jump / context vault |
| Ctrl | Crouch |
| E | Interact |
| F | Enter/exit vehicle |
| RMB | Aim |
| LMB | Fire / melee attack |
| R | Reload |
| G | Throwable |
| Tab | Weapon wheel |
| M | Map |
| P | Phone |
| Alt | Free look |

## Movement states

- Walk
- Jog
- Sprint
- Crouch
- Jump
- Fall
- Land
- Vault
- Mantle
- Climb
- Swim later
- Ragdoll
- Recover

## Design requirements

- Input buffering for vault/interact actions
- Camera collision
- Aim shoulder swap
- Adjustable FOV
- Mouse sensitivity settings
- Gamepad dead-zone settings
- Rebindable controls
- Animation-independent gameplay state
