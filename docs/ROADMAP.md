# X-Racing — Development Roadmap

> Motto: *simulate → measure → test → fix → document → repeat*
>
> Golden rule: **do not move to the next phase until the previous one is sufficiently working.**

---

## Current Status

| Component | Status |
|---|
| Build system (CMake) | ✅ |
| Math types + utility | ✅ |
| Tire model (Pacejka) | ✅ |
| Vehicle state + parameters | ✅ |
| Parametric track | ✅ |
| Input manager (Windows) | ✅ |
| 120 Hz simulation | ✅ |
| Telemetry + CSV export | ✅ |
| Weather (rain, temperature, grip) | ✅ |
| Gameplay (input, lap timing) | ✅ |
| Track SVG diagrams | ✅ |
| Unit tests (35+ GoogleTest) | ✅ |
| Rendering (Unity placeholder) | ✅ |
| Unity editor integration | ✅ |
| Race management (pit, validation, config) | ✅ |
| AI (racing line, opponents) | ⏳ next |

---

## Development Methodology

### Iterative Cycle

1. **Implement** the minimum necessary component
2. **Test** with unit tests + manual gameplay
3. **Document** behavior and limitations
4. **Measure** (telemetry, performance)
5. **Fix** bugs and edge cases
6. **Only then** move to the next component

### Completion Criteria

Every milestone has a verifiable completion criterion:

- **Playable:** a lap can be completed without crashes
- **Testable:** all components have unit tests
- **Measurable:** telemetry always active for validation

### Implementation Priority

| Priority | Component | Dependencies |
|---|---|---|
| P0 | Vehicle system | physics, input |
| P0 | Track system | spline, mesh |
| P1 | Collider + checkpoint | track system |
| P1 | Lap tracker | checkpoint |
| P1 | Camera system | vehicle state |
| P2 | Race manager | lap tracker, countdown |
| P2 | HUD | race manager, vehicle state |
| P3 | Racing line AI | track spline |
| P3 | Driver AI | racing line, vehicle |
| P3 | Opponents | driver AI, race manager |
| P4 | Procedural generation | track system |
| P4 | Vegetation / environment | procedural generation |
| P5 | Rendering pipeline | Unity / renderer |
| P5 | Audio | vehicle system |
| P5 | UI / menus | race manager |

---

## Phase 1 — Vertical Slice

**Goal:** Boot → car on track → drive → full lap → finish line.

No trees, no graphical details, no sophisticated AI.

### 1.1 Vehicle

- [x] Throttle (torque curve → longitudinal force)
- [x] Brake (deceleration, balance)
- [x] Steering (bicycle model, Ackermann)
- [x] Reverse
- [x] Gear / speed (gear → ratio → RPM)
- [x] Automatic transmission

**Completion criterion:** the vehicle accelerates, brakes, steers, and changes gears correctly.

### 1.2 Physics

- [x] Grip (Pacejka lateral/longitudinal)
- [x] Acceleration (engine map, gear ratios)
- [x] Braking (balance, lockup)
- [x] Weight / mass (CG height, mass distribution, weight transfer)
- [x] Suspensions (4-corner loads, weight transfer)
- [x] Collisions (track boundaries, barriers, off-track)
- [x] Tires (temperature, wear, grip)
- [x] Aerodynamics (downforce, drag, ground effect)
- [x] Weather (rain, temperature, grip)

**Completion criterion:** the car responds realistically to steering, throttle, and brake inputs.

### 1.3 Track

- [x] Closed parametric track
- [x] Multiple surfaces (asphalt, grass, gravel)
- [x] Pit lane
- [x] Vehicle position (respawn, reset)
- [x] Start line / grid (data model)
- [x] Checkpoint (data model)
- [x] Lap detection (runtime)
- [ ] Mesh-based collider

**Completion criterion:** a parametric track can be defined and the car drives it correctly.

### 1.4 Camera

- [x] Chase camera (basic)
- [x] Configurable distance
- [x] Rotation / look-ahead
- [x] Smoothing (lerp / spring)

**Completion criterion:** the camera follows the car with an acceptable view.

### 1.5 Minimum HUD

- [x] Speed (km/h)
- [x] RPM (bar or number)
- [x] Current gear
- [x] Current lap / total laps
- [x] Lap time / best lap

**Completion criterion:** a full lap can be completed without crashes, with a drivable car and readable HUD.

---

## Phase 2 — Track System

**Goal:** Build a system that allows creating parametric tracks without manual work.

- [x] Track spline (central path definition)
- [x] Variable width (per segment)
- [x] Asphalt mesh (generated from spline + width)
- [ ] Kerbs (optional elevation/banking)
- [ ] Guardrails (perimeter barriers)
- [ ] Run-off area (error zone)
- [x] Checkpoint system (data model, trigger volumes planned)
- [x] Track data export (SVG, JSON / binary for gameplay)

**Completion criterion:** a new track can be defined by modifying only spline parameters and generating the entire geometry automatically.

---

## Phase 3 — Procedural Environment

**Goal:** Populate the scene around the track procedurally.

- [ ] Track edge detection (terrain following)
- [ ] Generation zone (bounding box around track)
- [ ] Trees (Meshy assets, LOD, impostors)
- [ ] Rocks
- [ ] Grass (instancing, wind shader)
- [ ] Signs (racing line markers, signage)
- [ ] Barriers / fences
- [ ] Sky / atmosphere

**Completion criterion:** load a track and the environment is generated automatically, with sufficient visual variety.

---

## Phase 4 — Race

**Goal:** Transform free driving into a proper race.

- [x] Race config (RaceDefinition, CarAssignment, TeamDefinition)
- [x] Track data system (TrackData, GridDefinition, Checkpoints, StartFinishLine)
- [x] Pit lane system (PitLaneSystem, PitBox, SpeedDetectionZone)
- [x] Pit stop FSM (PitStopFSM, PitStopManager)
- [x] Race manager (RaceManager orchestrator)
- [x] Validation engine (validate geometry, direction, grid, pit, race, assignments)
- [ ] Countdown (3-2-1-GO)
- [ ] Starting grid (positions, spacing)
- [ ] Basic AI opponents (waypoint following, fixed speed)
- [ ] Standings (position, gap, best lap)
- [ ] Lap system (counter, lap validation)
- [ ] Penalties (track cut, false start)
- [ ] Flags (yellow, red, checkered)
- [ ] Race end (results screen, restart)

**Completion criterion:** an N-lap race can be run against opponents with countdown, standings, and race end.

---

## Phase 5 — Advanced AI

**Goal:** Opponents that drive in a realistic and adaptive way.

- [x] Racing line (optimal line optimization for the track)
- [x] Driver AI (acceleration, braking, steering based on racing line)
- [x] Opponents system (manager, multiple AI cars)
- [ ] Overtaking (decision making, gap analysis)
- [ ] Defense (corner positioning)
- [ ] Traffic adaptation
- [ ] Human errors (small imperfections, variance)
- [ ] ML experiments (engine behavior, sound, adaptive driving)

**Completion criterion:** opponents are competitive, make realistic mistakes, and offer variable difficulty.

---

## Phase 6 — Presentation

**Goal:** Graphics, audio, UI, optimization.

- [ ] Meshy assets (3D models, PBR textures)
- [ ] Scanned trees (photogrammetry)
- [ ] Lighting (HDR, GI, time of day)
- [ ] Materials (asphalt, metal, rubber)
- [ ] Particles (dust, rain, sparks)
- [ ] Effects (motion blur, DOF, lens flare)
- [ ] Audio engine (engine, exhaust, environment)
- [ ] UI (menus, advanced HUD, map)
- [ ] Optimization (LOD, instancing, draw calls)

**Completion criterion:** the game is visually competitive with similar titles.

---

## Code Structure (Planned)

```
x-racing/
├── engine/
│   ├── common.h               - Math types, constants, utility
│   ├── physics/
│   │   └── types.h           - Pacejka tire model, vector projections
│   ├── vehicle/
│   │   ├── vehicle.h         - VehicleParams + VehicleState
│   │   ├── vehicle_generator.h/.cpp - Procedural mesh generation
│   │   ├── mesh_exporter.h/.cpp     - OBJ export
│   │   └── glb_exporter.h/.cpp      - GLB export
│   ├── track/
│   │   ├── track.h/.cpp      - Closed parametric track
│   ├── input/
│   │   ├── input.h           - InputState definition
│   │   ├── input_manager.h   - Abstract input interface
│   │   └── platform/
│   │       ├── windows_input.h/.cpp  - Windows backend
│   │       ├── auto_input.h/.cpp     - Automatic input for testing
│   │       └── null_input.h          - Dummy backend for tests
│   ├── simulation/
│   │   ├── simulation.h/.cpp - 120 Hz physics loop
│   ├── telemetry/
│   │   ├── telemetry.h/.cpp  - Frame recording + CSV export
│   ├── weather/
│   │   └── weather.h         - Weather parameters and state
│   └── plugin/
│       ├── sim_plugin.h/.cpp - Native Unity plugin (DLL)
├── game/
│   ├── gameplay.h/.cpp       - Console gameplay loop
│   ├── main.cpp              - Gameplay entry point
│   ├── main_auto.cpp         - Automatic driving entry point
│   └── gen_telemetry.cpp     - Telemetry generator for Unity
├── renderer/
│   └── renderer.h/.cpp       - Win32 GDI renderer
├── UnityProject/
│   ├── Assets/
│   │   ├── Scripts/          - CarController, CarHUD, SimPlugin
│   │   ├── Editor/           - TrackGenerator, SceneSetup
│   │   ├── Scenes/           - MainScene.unity (placeholder), now.unity (active)
│   │   ├── Materials/        - CarMaterial, GroundMaterial
│   │   ├── Plugins/x86_64/   - sim_plugin.dll
│   │   └── Settings/         - URP asset, renderer data
│   ├── Packages/
│   │   └── manifest.json
│   └── ProjectSettings/
├── tests/
│   ├── simulation_test.cpp   - Physics tests (33+ cases)
│   ├── physics_test.cpp      - Pacejka model validation
│   ├── vehicle_test.cpp      - Vehicle tests
│   ├── track_test.cpp        - Track generation tests
│   ├── track_diagram_test.cpp - Track SVG export tests
│   ├── telemetry_test.cpp    - Telemetry tests
│   └── gameplay_test.cpp     - Gameplay tests
├── experiments/
│   ├── track_analysis.cpp    - Track geometry analysis
│   ├── track_svg.cpp         - Track SVG export
│   ├── ai_experiments.cpp    - [planned] ML/AI experiments
│   └── performance.cpp       - [planned] Profiling, benchmarks
├── tools/
│   ├── fbx_to_obj/           - FBX→OBJ converters
│   ├── track_generator/      - Track generators
│   └── track_diagram/        - Track SVG diagram exporter
├── data/
│   ├── telemetry/            - Telemetry CSV output
│   └── models/               - 3D models
├── assets/                   - 3D assets, textures, audio
├── vendor/                   - Third-party dependencies
│   ├── Eigen/                - Linear algebra
│   └── googletest/           - Unit testing framework
├── docs/                     - Project documentation
│   ├── README.md             - Project overview
│   └── ROADMAP.md            - This file
├── CMakeLists.txt             - Root CMake configuration
├── AGENTS.md                  - Project conventions
└── main.cpp                   - Win32 renderer entry point
```

---

## Development Notes

- Each phase produces a **playable build**
- Code is separated into independent modules
- Each module has its own tests
- Telemetry is always active for validation
- Rendering is decoupled from simulation
- Dependencies between modules are explicit and minimal
- P0 priority must be completed before moving to P1

---

## Technical Notes

### External Dependencies

| Library | Version | Usage |
|----------|----------|----------|
| Eigen | 3.4+ | Linear algebra (vectors, matrices) |
| Google Test | 1.14+ | Unit testing |
| Unity | 6000.5.9f1 | Production rendering |
| TextMesh Pro | 3.0+ | Unity HUD and text |
| CMake | 3.20+ | Build system |
| Visual Studio | 2022 | IDE / compiler |

### Code Conventions

- C++20 standard
- Headers: `.h`, implementation: `.cpp`
- Tests: Google Test framework
- Track data: `data/` directory
- Build artifacts excluded from version control (`build/`, `build2/`, `build3/`)

---

## Resources

- [Pacejka, Hans B. "Tyre Vehicle Dynamics"](https://www.amazon.com/Tyre-Vehicle-Dynamics-Hans-Pacejka/dp/074751520X) — Magic Formula tire model
- [Milliken, William F. "Race Car Vehicle Dynamics"](https://www.amazon.com/Race-Car-Vehicle-Dynamics-Milliken/dp/1560915269) — Race car vehicle dynamics
- [CMake Documentation](https://cmake.org/documentation/) — Build system
- [Unity Manual](https://docs.unity3d.com/Manual/) — Unity 6000.x
- [Eigen Library](https://eigen.tuxfamily.org/) — Linear algebra
