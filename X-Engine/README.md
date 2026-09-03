# X-Engine

A DirectX 12 + Win32 3D engine and physics sandbox.  Self-contained, no
external runtime dependencies, 204 unit tests.

## Quick start

```cmd
cmake -G "Visual Studio 17 2022" -A x64 -B build
cmake --build build --config Release --parallel
build\Release\xengine_runtime.exe
```

## Tests

```cmd
ctest --test-dir build -C Release --timeout 60 --output-on-failure
```

204 tests across `physics/`, `core/`, `rendering/`, `platform/`, `debug/`.

## What's in v0.20

See [CHANGELOG.md](CHANGELOG.md) and [RELEASE_v0.20.md](RELEASE_v0.20.md).

Highlights: OBB collision + SAT, AABB broadphase, raycast picking,
interactive drag (LMB move, RMB rotate, MMB spawn), gravity, sleeping,
distance constraints, ball/hinge joints (breakable), procedural rope,
scene save/load, trigger sensors, batch script, debug visualization.

## Controls

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `WASD`/arrows  | Move fly camera                         |
| Mouse          | Look (when captured)                    |
| `LMB` drag     | Move selected body                      |
| `RMB` drag     | Rotate selected body                    |
| `MMB`          | Spawn sphere at crosshair               |
| `` ` ``        | Open/close console                      |
| `ESC`          | Release / recapture mouse               |
| `F1`           | Toggle debug visualization              |
| `Space`/`LCtrl`| Up / down                               |

## Console essentials

```
help                  # list all 40+ commands
physics on            # enable physics
gravity on            # turn on gravity
demo                  # load the bundled demo scene
spawn rope 0 5 0 20 0.25 0.1   # 20-bead hanging rope
save my.scn           # serialize the world
load my.scn           # restore it
run script.xescript   # run a batch script
dbg aabb on           # show AABBs
triggers              # list trigger enter/exit events
```

## Layout

```
X-Engine/
├── core/              # math, logger, clock, application
├── platform/win32/    # window, input, mouse, HUD overlay
├── rendering/         # scene, meshes, DX12 renderer
├── physics/           # rigid bodies, joints, constraints, broadphase
├── debug/             # console, debug drawer
├── tests/             # Google Test suite
├── data/              # sample scripts (demo.xescript)
├── CMakeLists.txt     # top-level build
├── CHANGELOG.md
└── RELEASE_v0.20.md
```

## Build options

Default: `PROJECT0_BUILD_TESTS=ON`, `XENGINE_BUILD=ON` (when integrated with
parent project).  See root `AGENTS.md` for the cross-project conventions.
