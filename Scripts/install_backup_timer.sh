#!/usr/bin/env bash

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"

SYSTEMD_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user"
mkdir -p "$SYSTEMD_DIR"

sed "s|@PROJECT_ROOT@|$PROJECT_ROOT|g" \
    "$PROJECT_ROOT/Scripts/systemd/namma-city-backup.service.in" \
    > "$SYSTEMD_DIR/namma-city-backup.service"
install -m 0644 "$PROJECT_ROOT/Scripts/systemd/namma-city-backup.timer" \
    "$SYSTEMD_DIR/namma-city-backup.timer"

systemctl --user daemon-reload
systemctl --user enable --now namma-city-backup.timer
systemctl --user list-timers namma-city-backup.timer
