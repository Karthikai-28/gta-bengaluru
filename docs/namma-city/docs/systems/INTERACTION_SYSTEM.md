# Interaction System

Create a generic interaction interface used by player, NPCs, vehicles, doors, pickups, shops, and mission objects.

## Interaction types

- Use
- Talk
- Pick up
- Open/close
- Enter/exit
- Buy/sell
- Hack/use terminal as fictional gameplay abstraction
- Inspect
- Photograph

## Design rule

Mission code should call reusable interactions rather than creating one-off logic for every mission object.
