# Changelog

All notable changes to **X-Engine** (`X-Engine/` subproject) are recorded here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
X-Engine version numbers follow the in-app display (`X-Engine V0.X`).

## [v0.20.0] - 2026-09-03 — "First Demo"

First tagged release of X-Engine.  Working physics sandbox with 20 incremental
features built in eight days of focused development.  Runs as a single
`xengine_runtime.exe` with a Win32 window, a live console, and 204 unit tests.

### Added
- **Core engine** (V0.1–V0.8): Win32 window, GDI HUD overlay, DX12 forward
  renderer, scene graph, console with command history, fly camera controller.
- **Physics foundation** (V0.9–V0.20):
  - Sphere + OBB rigid bodies (V0.10, V0.11)
  - Sphere-sphere, OBB-OBB (SAT), OBB-sphere narrowphase (V0.11)
  - Linear and angular impulse with torque arm (V0.11)
  - AABB broadphase (V0.12)
  - Raycast picking + selection (V0.12)
  - Velocity-based interactive drag, mouse + raycast (V0.13)
  - Gravity vector, configurable per-axis (V0.14)
  - RMB-rotate drag (camera-relative axes) (V0.14)
  - Sleeping bodies with thresholds (V0.15)
  - Distance constraints (link) (V0.15)
  - Ball + hinge joints with break thresholds (V0.16)
  - Procedural rope via `BuildRope` (V0.17)
  - Scene serialize / deserialize, save / load to file (V0.18)
  - MMB-spawn sphere at crosshair (V0.18)
  - Trigger sensors with enter/exit events (V0.19)
  - Batch script interpreter (`run <file>`) (V0.19)
  - `DebugDrawer` (AABB, joint, trigger, contact, velocity lines) (V0.20)
  - Bundled `data/demo.xescript` (V0.20)
- **Console commands** (40+): `help`, `fps`, `physics on|off`, `gravity on|off|set`,
  `reset`, `reset_all`, `kick`, `spin`, `pick`, `link`, `unlink`, `pin`, `hinge`,
  `unpin`, `sleep on|off|all|list`, `spawn sphere|box|ground|rope`, `save`,
  `load`, `run`, `demo`, `dbg`, `triggers`, `echo`, `objects`, `clear`, `quit`.
- **Input bindings**: `WASD` + mouse for fly camera, `` ` `` for console,
  `ESC` to release cursor, `LMB` drag to move body, `RMB` drag to rotate,
  `MMB` to spawn, `F1` to toggle debug visualization.

### Stats
- 20 versioned releases
- 204 GoogleTest unit tests, 100% pass
- 5,500+ lines of C++20 across `physics/`, `rendering/`, `core/`, `debug/`,
  `platform/`, `tests/`
- 0 external runtime dependencies (DX12 + Win32 are part of the OS)

### Known limitations
- DX12 path is forward-only; no shadows, no post-processing
- No GPU-accelerated debug lines yet (overlays via GDI)
- Single-threaded physics
- Broadphase is `O(n²)` AABB overlap; SAP coming later
- No CCD; very fast bodies may tunnel

[Full release notes](RELEASE_v0.20.md)

## Pre-release history (in-development versions)

The git history carries every step.  Highlights:

| Ver | Date       | Theme                                    |
|----:|------------|------------------------------------------|
| V0.1| 2026-08    | Win32 window + logger skeleton           |
| V0.2| 2026-08    | First geometry: triangle                 |
| V0.3| 2026-08    | Clock, application loop, FPS counter      |
| V0.4| 2026-08    | Camera + view / projection matrices      |
| V0.5| 2026-08    | Mesh data: cube + quad                   |
| V0.6| 2026-08    | DX12 first frame, clear + present        |
| V0.7| 2026-08    | Scene graph, multiple objects, lighting  |
| V0.8| 2026-08    | HUD overlay + Win32 input                |
| V0.9| 2026-08    | Fly camera + console                     |
| V0.10| 2026-08   | Physics integration (sphere only)        |
| V0.11| 2026-08   | OBB collision + rotation + angular drag  |
| V0.12| 2026-08   | AABB broadphase + raycast picking        |
| V0.13| 2026-08   | Interactive drag (LMB)                   |
| V0.14| 2026-08   | Gravity + RMB-rotate drag                |
| V0.15| 2026-09   | Sleeping bodies + distance constraints   |
| V0.16| 2026-09   | Ball + hinge joints (breakable)          |
| V0.17| 2026-09   | Spawn factories + procedural rope        |
| V0.18| 2026-09   | Scene save/load + MMB-spawn              |
| V0.19| 2026-09   | Trigger sensors + batch script           |
| V0.20| 2026-09   | DebugDrawer + demo.xescript              |
