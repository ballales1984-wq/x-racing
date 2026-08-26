# X-Racing — Development Plan

> Motto: *simulate → measure → test → correct → document → repeat*
>
> Golden rule: **do not move to the next phase until the previous one is sufficiently working.**

---

## Current Status

| Component | Status |
|---|---|
| Build system (CMake) | ✅ |
| Math types + utilities | ✅ |
| Tire model (Pacejka) | ✅ |
| Vehicle state + params | ✅ |
| Parametric track | ✅ |
| Input manager (Windows) | ✅ |
| Simulation 120 Hz | ✅ |
| Telemetry + CSV export | ✅ |
| Weather (rain, temp, grip) | ✅ |
| Gameplay (input, lap timing) | ✅ |
| Unit tests (33 GoogleTest) | ✅ |
| Rendering (Unity placeholder) | ⏳ |
| Unity editor integration | ⏳ |
| AI (trajectory, opponents) | ⏳ |

---

## Phase 1 — Vertical Slice

Goal: **Startup → car on track → driving → full lap → finish.**

No trees, no graphical details, no sophisticated AI.

### 1.1 Car

- [ ] Accelerator (torque curve → longitudinal force)
- [ ] Brake (brake deceleration, balance)
- [ ] Steering (bicycle model, Ackermann)
- [ ] Reverse gear
- [ ] Gearbox / speed (gear → ratio → RPM)

### 1.2 Physics

- [ ] Grip (Pacejka lateral/longitudinal)
- [ ] Acceleration (engine map, gear ratios)
- [ ] Braking (brake bias, lock-up)
- [ ] Weight / mass
- [ ] Suspension (4-corner loads, weight transfer)
- [x] Collisions (track boundaries, barriers, off-track terrain drag)

### 1.3 Track

- [ ] Correct collider (mesh-based or spline-based)
- [ ] Start line / grid
- [ ] Checkpoints (trigger zones)
- [ ] Lap detection
- [x] Car position (respawn, reset)

### 1.4 Camera

- [ ] Chase camera
- [ ] Configurable distance
- [ ] Rotation / look-ahead
- [ ] Smoothing (lerp / spring)

### 1.5 Minimal HUD

- [ ] Speed (km/h)
- [ ] RPM (bar or number)
- [ ] Current gear
- [ ] Current / total lap
- [ ] Lap time / best lap

**Completion criterion:** you can complete a full lap without crashing, with a drivable car and a readable HUD.

---

## Phase 2 — Track System

Goal: **build a system that lets you create parametric tracks without manual work.**

- [ ] Track spline (central path definition)
- [ ] Variable width (segment-by-segment)
- [ ] Asphalt mesh (generated from spline + width)
- [ ] Kerbs (elevation/banking optional)
- [ ] Guardrail (perimeter barriers)
- [ ] Runoff area (error zone)
- [ ] Checkpoint system (trigger volumes on the spline)
- [ ] Track data export (JSON / binary for gameplay)

**Completion criterion:** you can define a new track by changing only the spline parameters and generate the entire geometry automatically.

---

## Phase 3 — Procedural Environment

Goal: **populate the scene around the track procedurally.**

- [ ] Track edge (edge detection, terrain following)
- [ ] Generation zone (bounding box around the track)
- [ ] Trees (Meshy assets, LOD, impostor)
- [ ] Rocks
- [ ] Grass (instancing, wind shader)
- [ ] Signs (racing line markers, track signage)
- [ ] Barriers / fences
- [ ] Sky / atmosphere

**Completion criterion:** you load a track and the environment generates automatically, with sufficient visual variety.

---

## Phase 4 — Race

Goal: **turn free driving into a race.**

- [ ] Countdown (3-2-1-GO)
- [ ] Starting grid (positions, spacing)
- [ ] Race manager (state: countdown → racing → finished)
- [ ] Basic AI opponents (waypoint following, fixed speed)
- [ ] Standings (position, gap, best lap)
- [ ] Lap system (lap counter, lap validation)
- [ ] Penalties (cutting track, false start)
- [ ] Flags (yellow, red, checkered)
- [ ] Race end (results screen, restart)

**Completion criterion:** you can run an N-lap race against opponents with countdown, standings, and race end.

---

## Phase 5 — Advanced AI

Goal: **opponents that drive realistically and adaptively.**

- [ ] Racing line (optimal line optimization for the track)
- [ ] AI driver (acceleration, braking, steering based on the racing line)
- [ ] Overtaking (decision making, gap analysis)
- [ ] Defense (positioning in corners)
- [ ] Traffic adaptation
- [ ] Human errors (small imperfections, variance)
- [ ] ML experiments (engine behavior, sound, adaptive driving)

**Completion criterion:** opponents are competitive, make realistic mistakes, and offer variable difficulty.

---

## Phase 6 — Presentation

Goal: **graphics, audio, UI, optimization.**

- [ ] Meshy assets (3D models, PBR textures)
- [ ] Scanned trees (photogrammetry)
- [ ] Lighting (HDR, GI, time of day)
- [ ] Materials (asphalt, metal, rubber)
- [ ] Particles (dust, rain, sparks)
- [ ] Effects (motion blur, DOF, lens flare)
- [ ] Sound engine (engine, exhaust, environment)
- [ ] UI (menus, advanced HUD, map)
- [ ] Optimization (LOD, instancing, draw calls)

**Completion criterion:** the game is visually competitive with similar titles.

---

## Code Structure

```
x-racing/
├── engine/
│   ├── common.h               - Math types, constants, utilities
│   ├── physics/
│   │   └── types.h           - Pacejka tire model, vector projections
│   ├── vehicle/
│   │   ├── vehicle.h         - VehicleParams + VehicleState
│   │   └── vehicle_system.h  - Vehicle lifecycle management
│   ├── track/
│   │   ├── track.h/.cpp      - Parametric closed-loop track
│   │   ├── track_spline.h    - Spline-based track definition
│   │   ├── track_mesh.h      - Mesh generation from spline
│   │   └── track_physics.h   - Collider + checkpoint system
│   ├── input/
│   │   ├── input.h           - InputState definition
│   │   └── input_manager.h   - Abstract input interface
│   ├── simulation/
│   │   ├── simulation.h/.cpp - 120 Hz physics loop
│   │   └── integrator.h      - Velocity-space integration
│   ├── camera/
│   │   └── camera.h          - Chase camera, smoothing
│   ├── ai/
│   │   ├── racing_line.h     - Optimal line computation
│   │   ├── driver.h          - AI driver behavior
│   │   └── opponent.h        - Opponent management
│   ├── race/
│   │   ├── race_manager.h    - Countdown, state machine
│   │   ├── lap_tracker.h     - Lap detection, timing
│   │   └── checkpoint.h      - Checkpoint triggers
│   ├── telemetry/
│   │   ├── telemetry.h/.cpp  - Frame recording + CSV export
│   │   └── analyzer.h        - Telemetry analysis tools
│   ├── weather/
│   │   └── weather.h         - Weather parameters + state
│   └── procedural/
│       ├── generator.h       - Track generation system
│       ├── vegetation.h      - Tree/rock placement
│       └── environment.h     - Sky, atmosphere
├── game/
│   ├── gameplay.h/.cpp       - Console gameplay loop
│   ├── gameplay_race.h/.cpp  - Race-specific gameplay
│   └── main.cpp              - Entry point
├── renderer/
│   └── renderer.h/.cpp       - Unity / renderer integration
├── UnityProject/
│   ├── Assets/
│   │   ├── Scripts/          - CarController, CarHUD, SimPlugin
│   │   ├── Editor/           - TrackGenerator, SceneSetup
│   │   ├── Scenes/           - MainScene.unity
│   │   ├── Materials/        - CarMaterial, GroundMaterial
│   │   ├── Plugins/x86_64/   - sim_plugin.dll
│   │   └── Settings/         - URP asset, renderer data
│   ├── Packages/
│   │   └── manifest.json
│   └── ProjectSettings/
├── tests/
│   ├── simulation_test.cpp   - Physics tests
│   ├── track_test.cpp        - Track tests
│   ├── ai_test.cpp           - AI tests
│   └── race_test.cpp         - Race system tests
├── experiments/
│   ├── track_analysis.cpp    - Track geometry analysis
│   ├── ai_experiments.cpp    - [planned] ML / AI experiments
│   └── performance.cpp       - [planned] Profiling, benchmarks
├── tools/
│   ├── track_editor/         - Spline editor tool
│   ├── telemetry_viewer/     - CSV visualization
│   └── asset_pipeline/       - Asset conversion scripts
├── data/
│   ├── tracks/               - Track definitions
│   ├── vehicles/             - Vehicle configs
│   └── telemetry/            - Telemetry output
├── docs/
│   ├── ROADMAP.md            - This file
│   ├── PHYSICS.md            - Physics documentation
│   └── API.md                - API reference
├── assets/                   - 3D models, textures, audio
├── build/                    - CMake build directory
├── vendor/                   - Third-party dependencies
├── CMakeLists.txt            - Root CMake config
└── README.md                 - Project overview
```

---

## Implementation Priority

| Priority | Component | Dependencies |
|---|---|---|
| P0 | Vehicle system | physics, input |
| P0 | Track system | spline, mesh |
| P1 | Collider + checkpoint | track system |
| P1 | Lap tracker | checkpoint |
| P1 | Camera system | vehicle state |
| P2 | Race manager | lap tracker, countdown |
| P2 | HUD | race manager, vehicle state |
| P3 | AI racing line | track spline |
| P3 | AI driver | racing line, vehicle |
| P3 | Opponents | AI driver, race manager |
| P4 | Procedural generation | track system |
| P4 | Vegetation / environment | procedural generation |
| P5 | Rendering pipeline | Unity / renderer |
| P5 | Audio | vehicle system |
| P5 | UI / menus | race manager |

---

## Development Workflow

1. **Implement** the minimum necessary component
2. **Test** with unit tests + manual gameplay
3. **Document** behavior and limits
4. **Measure** (telemetry, performance)
5. **Fix** bugs and edge cases
6. **Only then** move to the next component

---

## Notes

- Each phase produces a **playable build**
- Code is separated into independent modules
- Each module has its own tests
- Telemetry is always enabled for validation
- Rendering is decoupled from simulation
