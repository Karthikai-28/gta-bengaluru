#!/usr/bin/env bash

# shellcheck source=/dev/null
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"

if [[ -z "${NAMMA_BACKUP_ROOT:-}" ]]; then
    echo "NAMMA_BACKUP_ROOT is not set. Configure it in .env." >&2
    exit 2
fi

BACKUP_ROOT="$(realpath -m "$NAMMA_BACKUP_ROOT")"
if [[ "$BACKUP_ROOT" == "$PROJECT_ROOT" || "$BACKUP_ROOT" == "$PROJECT_ROOT/"* ]]; then
    echo "Backup destination must be outside the project repository." >&2
    exit 2
fi

STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
DESTINATION="$BACKUP_ROOT/$STAMP"
mkdir -p "$DESTINATION"

LINK_DEST=()
if [[ -L "$BACKUP_ROOT/latest" ]]; then
    PREVIOUS="$(realpath "$BACKUP_ROOT/latest")"
    LINK_DEST=(--link-dest "$PREVIOUS")
fi

rsync -a "${LINK_DEST[@]}" \
    --exclude Binaries \
    --exclude DerivedDataCache \
    --exclude Intermediate \
    --exclude Saved \
    --exclude graphify-out \
    "$PROJECT_ROOT/" "$DESTINATION/"

ln -sfn "$STAMP" "$BACKUP_ROOT/latest"
echo "Backup completed: $DESTINATION"
