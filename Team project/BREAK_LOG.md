# Break Log

| Date | Commit hash | What broke | What changed |
|---|---|---|---|
| 2026-10-26 | Unavailable (this project folder is not a Git repository) | PlatformIO targeted `btatmega328`, which did not match the requested Arduino Nano hardware. | Changed the board target to `nanoatmega328` and verified the firmware builds for the Nano. |