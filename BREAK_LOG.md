# Break Log

| Date | Commit hash | What broke | What changed |
|---|---|---|---|
| 2026-10-26 | 1c913a5a07dfea2014e49b03e6215f876e1c4122 (HEAD before uncommitted fixes) | The OLED sketch used 30 cm/120 cm thresholds and NEAR/HERE/EMPTY labels; it showed no sideways thumb, so the requested detection behavior was absent. | Set the bands to under 10 cm, 10–50 cm, and over 50 cm; changed labels to CLOSE/DETECTED/NOBODY and added up/sideways/down thumb orientations. |
| 2026-10-26 | 1c913a5a07dfea2014e49b03e6215f876e1c4122 (HEAD before uncommitted fixes) | The first AVR compile after renaming the threshold failed because the unused sonar helper still referenced `NEAR_CM`. | Changed the helper to use `CLOSE_CM`; the updated sketch then compiled to AVR object code. |
| 2026-10-26 | 1c913a5a07dfea2014e49b03e6215f876e1c4122 (HEAD before uncommitted fixes) | State changes were based on the smoothed distance, delaying the requested response when crossing a cutoff. | Use the latest raw reading for labels and thumbs; keep smoothing only for the displayed distance and graph. |