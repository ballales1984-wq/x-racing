# X-Racing

C++20 racing simulation with CMake build system, Google Test unit tests, optional Unity integration, and a proprietary in-house engine (X-Engine).

## Build

- Configure: `cmake -G "Visual Studio 17 2022" -A x64 -B build`
- Build: `cmake --build build --config Release --parallel`
- Test: `ctest --output-on-failure -C Release`

CMake options:
- `PROJECT0_BUILD_TESTS=ON` (default ON)
- `PROJECT0_BUILD_RENDERER=OFF` (default OFF)
- `PROJECT0_BUILD_EXPERIMENTS=ON` (default ON)
- `PROJECT0_BUILD_GAMEPLAY=ON` (default ON)
- `PROJECT0_BUILD_ASSIMP=OFF` (default OFF)
- `XENGINE_BUILD=OFF` (default OFF) — build X-Engine, the proprietary runtime

The canonical build directory is `build/`. Do not create `build2/`, `build3/`, etc. — they are leftovers and will be deleted on cleanup.

## Project Structure

```
x-racing/
├── CMakeLists.txt            # Root CMake — Project0 (Unity + native sim)
├── engine/                   # project0_engine: simulation core (physics, AI, telemetry, track, vehicle)
├── renderer/                 # project0_renderer: Win32 GDI/DX11 renderer (Project0 only)
├── tests/                    # Google Test suite for project0_engine
├── experiments/              # Standalone tools/benchmarks (track_analysis, tracking_demo, brake_measure, ...)
├── game/                     # Gameplay executable (Project0 native)
├── tools/                    # Python/build utilities (track diagram, FBX→OBJ, TrackSetupManager for Unity)
├── UnityProject/             # Unity 6 project that drives the simulation through sim_plugin (Unity integration is optional)
├── X-Engine/                 # xe_core / xengine_runtime — proprietary engine (DX12, Win32, scene/scene_loader, console). Built only with XENGINE_BUILD=ON
├── data/                     # Track layouts, telemetry output, SVG diagrams, best-times
├── assets/                   # 3D source assets (FBX/GLB) — exported via tools/ to UnityProject/Assets/Models
├── docs/                     # ROADMAP.md, GAME.md, README.md, PLACEHOLDERS.md
└── vendor/                   # Third-party (googletest, Eigen, assimp [optional], unsupported)
```

### engine/ vs X-Engine/

- **`engine/`** is `project0_engine`: the racing simulation domain (track, vehicle, telemetry, AI, physics, replay, tracking). Exposed to Unity through `sim_plugin.dll`. Renderer-agnostic.
- **`X-Engine/`** is the proprietary standalone engine runtime (`xe_core`, `xengine_runtime`). DirectX 12 renderer, Win32 platform layer, scene graph, console/HUD overlay. Built only when `-DXENGINE_BUILD=ON` (default OFF). It is the long-term replacement for Unity/Unreal.

The two are **separate projects** with separate CMake roots; do not link them together.

## Code Style

- C++20 standard
- Header files: `.h`, implementation: `.cpp`
- Tests use Google Test framework
- Track data in `data/` directory
- Keep build artifacts out of version control (`build/`, `build2/`, `build3/`)

## Worktree Guidelines

- Do not use `git stash` or autostash; worktree merge flows should resolve conflicts in the worktree
- Run setup before running build commands in a new worktree
- Avoid modifying shared global resources from multiple worktrees simultaneously
