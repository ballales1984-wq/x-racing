# Project 0

Automotive simulation laboratory. Build, measure, test, correct, document, repeat.

## Status

| Milestone | Component | Status |
|-----------|-----------|--------|
| M0 | Environment / build system | ✅ |
| M1 | Track + vehicle state | ✅ |
| M2 | Steering (bicycle model) | ✅ |
| M3 | Grip / cornering (Pacejka) | ✅ |
| M4 | Tires (temp, wear, grip) | ✅ |
| M5 | Suspension (4-corner loads) | ✅ |
| M6 | Aerodynamics (downforce, pitch/roll) | ✅ |
| M7 | Weather (rain, temp, grip) | ✅ |
| M9 | Gameplay (input, lap timing) | ✅ |
| M8 | Rendering (Unity) | ✅ Unity 6000.0.82f1, project operational |
| M10 | AI (trajectory, opponents) | ⏳ next |

## Architecture

```
engine/
  common.h            - Math types, constants, utilities
  physics/
    types.h           - Pacejka tire model, vector projections
  vehicle/
    vehicle.h         - VehicleParams + VehicleState
  track/
    track.h/.cpp      - Parametric closed-loop track
  input/
    input.h           - InputState definition
    input_manager.h   - Abstract input interface
    platform/
      windows_input.h/.cpp - Windows backend
      null_input.h    - Dummy backend for tests
  simulation/
    simulation.h/.cpp - 120 Hz physics loop
  telemetry/
    telemetry.h/.cpp  - Frame recording + CSV export
  weather/
    weather.h         - Weather parameters + state
game/
  gameplay.h/.cpp     - Console gameplay loop
  main.cpp            - Gameplay entry point
tests/
  simulation_test.cpp - 18 GoogleTest cases
experiments/
  track_analysis.cpp  - Track geometry analysis
renderer/
  renderer.h/.cpp     - Placeholder for Unity integration
vendor/
  Eigen/              - Linear algebra
  googletest/         - Unit testing framework
```

## Physics Pipeline

Each simulation sub-step applies these stages in order:

1. `update_engine_forces()` — torque curve → longitudinal force
2. `update_aerodynamics()` — drag, lift, downforce
3. `update_weather()` — rain cooling, track temperature
4. `update_tire_temperature()` — thermal model + wear
5. `update_suspension()` — 4-corner weight transfer
6. `update_tire_forces()` — Pacejka lateral grip
7. `update_braking()` — brake deceleration
8. `update_steering()` — bicycle model slip angles
9. `integrate()` — velocity-space integration

## Building

```powershell
# Configure
cmake -B build -G "Visual Studio 17 2022" -DPROJECT0_BUILD_TESTS=ON -DPROJECT0_BUILD_GAMEPLAY=ON

# Build Release
cmake --build build --config Release

# Run tests
.\build\tests\Release\project0_tests.exe

# Run gameplay
.\build\game\Release\project0_gameplay_exe.exe

# Run track analysis
.\build\experiments\Release\track_analysis.exe
```

## Controls (Gameplay)

| Key | Action |
|-----|--------|
| W / ↑ | Throttle |
| S / ↓ | Brake |
| A / ← | Steer left |
| D / → | Steer right |
| Shift | Upshift |
| Ctrl | Downshift |
| R | Reset |
| ESC | Quit |

## Unity Integration

| Item | Value |
|------|-------|
| Editor | Unity 6000.0.82f1 |
| Project | `D:\x-racing\UnityProject\` |
| Render pipeline | Universal Render Pipeline |
| Native plugin | `Assets/Plugins/x86_64/sim_plugin.dll` |

### Project structure

```
UnityProject/
├── Assets/
│   ├── Scripts/
│   │   ├── CarController.cs   - Vehicle controller + camera follow
│   │   ├── CarHUD.cs          - Speed / RPM / gear / lap HUD
│   │   └── SimPlugin.cs       - P/Invoke bridge to sim_plugin.dll
│   ├── Editor/
│   │   ├── SceneSetup.cs      - Project0 > Setup Scene menu
│   │   ├── TrackGenerator.cs  - Project0 > Generate Track menu
│   │   └── GenerateTelemetry.cs
│   ├── Scenes/
│   │   └── MainScene.unity    - Main gameplay scene
│   ├── Materials/
│   │   ├── CarMaterial.mat
│   │   └── GroundMaterial.mat
│   ├── Plugins/x86_64/
│   │   └── sim_plugin.dll     - Native C++ simulation
│   └── Settings/
│       ├── UniversalRenderPipelineAsset.asset
│       └── ForwardRendererData.asset
├── Packages/
│   └── manifest.json
└── ProjectSettings/
    └── ProjectVersion.txt
```

### Editor menus

- **Project0 → Setup Scene** — creates HUD canvas and binds `CarController`
- **Project0 → Generate Track** — generates parametric track mesh, borders and collider

### Known issues

- Unity 6000.5.9f1 was blocked by a WDAC policy on `Bee.DotNet.dll`; use **Unity 6000.0.82f1** instead
- After first launch, if missing track/HUD objects, re-run the Project0 menus
- `LegacyRuntime.ttf` is used for built-in HUD text in Unity 2022+

## Design Principles

1. Simulate → measure → test → correct → document → repeat
2. Separation of simulation and rendering
3. Test every component
4. Use data and benchmarks
5. Prefer a simple verifiable model over a complex misunderstood one
6. Do not sacrifice playability for realism
7. Do not sacrifice correctness for visual effects
