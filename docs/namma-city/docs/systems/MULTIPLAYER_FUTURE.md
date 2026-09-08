# Multiplayer — Future Only

Do not build multiplayer during the first vertical slice.

If multiplayer becomes a future objective, start preserving some architecture choices now:

- Gameplay state should have clear authority ownership
- Avoid global singleton state for everything
- Keep player inventory data serializable
- Use event-driven state changes
- Separate cosmetic effects from authoritative logic

Do not incur multiplayer complexity until the single-player game is stable and fun.
