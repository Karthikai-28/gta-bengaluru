# Internal SSD storage review

Inventory and authorized cleanup on 2026-09-08. **The 14 archive cache folders listed below, pip cache, and Chrome cache have been removed following explicit user authorization.** Free space increased to 45.76 GiB (about 24 GiB reclaimed). Disk usage can change while other applications run.

The table records the exact archive-manager cache folders removed. No personal Downloads, project files, or active Codex/browser runtime installations were removed. This is a historical cleanup record, not a request for further deletion.

| Exact candidate | Logical file size |
|---|---:|
| `/home/karthi/.cache/.fr-PWBx0X` | 2.03 GiB |
| `/home/karthi/.cache/.fr-5NUGM4` | 2.03 GiB |
| `/home/karthi/.cache/.fr-CYnQzM` | 2.01 GiB |
| `/home/karthi/.cache/.fr-R9ePG7` | 1.73 GiB |
| `/home/karthi/.cache/.fr-YYPDYn` | 1.72 GiB |
| `/home/karthi/.cache/.fr-HedCDE` | 1.64 GiB |
| `/home/karthi/.cache/.fr-2dmquH` | 1.53 GiB |
| `/home/karthi/.cache/.fr-kt34EP` | 1.47 GiB |
| `/home/karthi/.cache/.fr-MpjTW1` | 1.47 GiB |
| `/home/karthi/.cache/.fr-yVCPrj` | 1.19 GiB |
| `/home/karthi/.cache/.fr-W0Gy8h` | 1.15 GiB |
| `/home/karthi/.cache/.fr-OPmzmM` | 1.14 GiB |
| `/home/karthi/.cache/.fr-bye1LH` | 1.13 GiB |
| `/home/karthi/.cache/.fr-WLkeH2` | 1.10 GiB |

Additional cache categories observed: `~/.cache/google-chrome` about 2.3 GiB and `~/.cache/pip` about 458 MiB. Prefer their applications' own cache-clearing controls. Do not delete active Codex/browser runtimes or personal Downloads to make space implicitly.

## Capacity decision

Use Epic's installed Linux distribution. Its authenticated download/version and archive size are not yet available. Before downloading, budget the compressed archive + advertised unpacked size + 40 GiB working reserve on the destination filesystem. After download, run:

```bash
python3 Scripts/check_workstation.py --archive /path/to/Linux_UnrealEngine.zip --install-parent /existing/install/parent
```

The checker inspects ZIP metadata without extraction and accounts for the archive already occupying disk. It may also report an unconfigured engine; that remains expected before installation. Freeing these candidates alone is not a guarantee that Unreal will fit. Review the selected archive's exact requirements before installation.

The listed cache deletion is complete and authorized. Further cleanup outside these paths is not assumed. Epic account access and the selected installed-build archive metadata remain necessary to check whether the remaining 45.76 GiB can accommodate installation. No engine download or extraction has started.
