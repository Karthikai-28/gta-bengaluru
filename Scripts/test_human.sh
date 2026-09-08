#!/usr/bin/env bash
source "$(dirname -- "${BASH_SOURCE[0]}")/common.sh"
require_unreal
exec "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" "$PROJECT_FILE" \
    -unattended -nocrashreports -nop4 -nosplash -NullRHI -nosound -stdout \
    '-ExecCmds=Automation RunTests NammaCity' '-TestExit=Automation Test Queue Empty' \
    "-ReportExportPath=$PROJECT_ROOT/Saved/Automation/Human"
