# X-Racing

Laboratory racing simulator. Build, measure, test, fix, document, repeat.

## Table of Contents

- [Overview](#overview)
- [Project Status](#project-status)
- [Architecture](#architecture)
  - [Engine (Core Library)](#engine-core-library)
  - [Gameplay (Game Level)](#gameplay-game-level)
  - [Renderer (Win32 GDI)](#renderer-win32-gdi)
  - [Unity Integration](#unity-integration)
- [Physics Pipeline](#physics-pipeline)
- [Build and Test](#build-and-test)
- [Controls](#controls)
- [Data Structures](#data-structures)
- [Telemetry](#telemetry)
- [Unity Plugin](#unity-plugin)
- [Known Issues](#known-issues)
- [Design Principles](#design-principles)

---

## Overview

**X-Racing** is a C++20 racing driving simulator, designed as an iterative simulation lab. The goal is to build a realistic driving experience through a data-driven approach with continuous measurement and testing.

The project clearly separates physics simulation from rendering, allowing physics to be developed and validated independently of graphics. Unity 6000.x is used as the production renderer, connected to the C++ engine via a native DLL plugin.

**Key features:**

- **120 Hz physics engine** with iterative simulation pipeline
- **Pacejka Magic Formula** tire model with temperature and wear
- **4-corner suspensions** with load transfer
- **Full aerodynamics** (drag, downforce, ground effect)
- **Dynamic weather** (rain, temperature, grip)
- **Closed parametric track** with multiple surfaces
- **CSV telemetry** for analysis and validation
- **33+ unit tests** with Google Test
- **Track SVG exporter** with interactive diagrams
- **Unity plugin** for real-time rendering

---

## Project Status

| Milestone | Component | Status |
|-----------|-----------|-------|
| M0 | Environment / build system | ✅ |
| M1 | Track + vehicle state | ✅ |
| M2 | Steering (bicycle model) | ✅ |
| M3 | Grip / cornering (Pacejka) | ✅ |
| M4 | Tires (temp, wear, grip) | ✅ |
| M5 | Suspensions (4-corner loads) | ✅ |
| M6 | Aerodynamics (downforce, pitch/roll) | ✅ |
| M7 | Weather (rain, temperature, grip) | ✅ |
| M9 | Gameplay (input, lap timing) | ✅ |
| M8 | Rendering (Unity) | ✅ Unity 6000.0.82f1, operational project |
| M9 | Race management (pit, validation, race config) | ✅ |
| M10 | AI (racing line, opponents) | ⏳ next |

---

## Architecture

```
x-racing/
├── engine/                    # Core library (static + DLL plugin)
│   ├── common.h              # Math types, constants, utility
│   ├── physics/
│   │   └── types.h           # Pacejka tire model, vector projections
│   ├── vehicle/
│   │   ├── vehicle.h         # VehicleParams + VehicleState
│   │   ├── vehicle_generator.h/.cpp  # Procedural mesh generation
│   │   ├── mesh_exporter.h/.cpp      - OBJ export
│   │   └── glb_exporter.h/.cpp       - GLB export
│   ├── track/
│   │   ├── track.h/.cpp      # Closed parametric track
│   │   ├── track_data.h      # Track data (grid, checkpoint, pit)
│   │   ├── race_config.h     # Race configuration (type, penalties, compound)
│   │   ├── pit_lane.h/.cpp   # Runtime pit lane system
│   │   ├── pit_stop_fsm.h/.cpp # Vehicle pit stop FSM
│   │   ├── race_manager.h/.cpp # Race orchestrator
│   │   └── validation.h/.cpp  # Track and race validation
│   ├── input/
│   │   ├── input.h           # InputState definition
│   │   ├── input_manager.h   # Abstract input interface
│   │   └── platform/
│   │       ├── windows_input.h/.cpp  # Windows backend
│   │       ├── auto_input.h/.cpp     # Automatic input for testing
│   │       └── null_input.h          # Dummy backend for tests
│   ├── simulation/
│   │   ├── simulation.h/.cpp # 120 Hz physics loop
│   ├── telemetry/
│   │   ├── telemetry.h/.cpp  # Frame recording + CSV export
│   ├── weather/
│   │   ├── weather.h         # Weather parameters and state
│   └── plugin/
│       ├── sim_plugin.h/.cpp # Native Unity plugin (DLL)
├── game/                      # Game level
│   ├── gameplay.h/.cpp       # Console gameplay loop
│   ├── main.cpp              # Interactive gameplay entry point
│   ├── main_auto.cpp         # Automatic driving entry point
│   └── gen_telemetry.cpp     # Telemetry generator for Unity
├── renderer/                  # Win32 GDI renderer
│   ├── renderer.h/.cpp       # 2D/3D wireframe visualization
├── tests/                     # Google Test unit tests
│   ├── simulation_test.cpp   # 33+ physics test cases
│   ├── physics_test.cpp      # Pacejka model validation
│   ├── vehicle_test.cpp      # Vehicle parameter tests
│   ├── track_test.cpp        # Track generation tests
│   ├── track_diagram_test.cpp # Track SVG export tests
│   ├── telemetry_test.cpp    # Telemetry recording tests
│   └── gameplay_test.cpp     # Gameplay loop tests
├── experiments/               # Analysis and profiling tools
│   ├── track_analysis.cpp    # Track geometry analysis
│   ├── ai_experiments.cpp    - [planned] ML/AI experiments
│   └── performance.cpp       - [planned] Profiling, benchmarks
├── UnityProject/              # Unity 2022.3.x project
│   ├── Assets/
│   │   ├── Models/            # 3D vehicle models (FBX)
│   │   ├── Scripts/
│   │   │   ├── CarController.cs   # Vehicle controller + camera follow
│   │   │   ├── CarHUD.cs          # HUD speed/RPM/gear/lap
│   │   │   ├── HUDSetup.cs        # Auto-setup HUD utility
│   │   │   └── SimPlugin.cs       # P/Invoke bridge to sim_plugin.dll
│   │   ├── Editor/
│   │   │   ├── SceneSetup.cs      # Project0 > Setup Scene menu
│   │   │   ├── TrackGenerator.cs  # Project0 > Generate Track menu
│   │   │   └── GenerateTelemetry.cs
│   │   ├── Scenes/
│   │   │   ├── ok.unity           # Main gameplay scene
│   │   │   └── impostazioni.unity # Settings/config scene
│   │   ├── Materials/
│   │   │   ├── CarMaterial.mat
│   │   │   └── GroundMaterial.mat
│   │   ├── Plugins/x86_64/
│   │   │   └── sim_plugin.dll     # Native C++ plugin
│   │   └── Settings/
│   │       ├── UniversalRenderPipelineAsset.asset
│   │       └── ForwardRendererData.asset
│   ├── Packages/
│   │   └── manifest.json
│   └── ProjectSettings/
│       └── ProjectVersion.txt
├── tools/                     # Development tools
│   ├── fbx_to_obj/           # FBX→OBJ converters
│   ├── track_generator/      # Track generators
│   └── track_diagram/        # Track SVG diagram exporter
├── data/                      # Runtime data
│   ├── telemetry/            # Telemetry CSV output
│   └── models/               # 3D models
├── assets/                    # 3D assets, textures, audio
│   └── models/               # 3D models (OBJ, FBX)
├── vendor/                    # Third-party dependencies
│   ├── Eigen/                # Linear algebra
│   └── googletest/           # Unit testing framework
├── docs/                      # Project documentation
│   ├── README.md             # This file
│   └── ROADMAP.md            # Development plan
├── CMakeLists.txt             # Root CMake configuration
├── AGENTS.md                  # Project conventions
└── main.cpp                   # Win32 renderer entry point
```

---

## Engine (Core Library)

### `common.h` Module

Fundamental math types and utilities:

- **Vector types:** `Vec2`, `Vec3`, `Vec4` (Eigen wrappers)
- **Matrix types:** `Mat2`, `Mat3`, `Mat4`
- **Constants:** `kPi`, `kHalfPi`, `kGravity` (9.81 m/s²), `kEpsilon`
- **Utilities:** `clamp()`, `normalize_angle()`, `lerp()`, `smoothstep()`

### `physics/` Module

Pacejka Magic Formula tire model implementation:

- **`types.h`:** Vector projection/rejection functions, `cross2()`, centripetal/centrifugal force helpers
- **Pacejka Formula:** B, C, D, E coefficients for lateral force calculation
- **Combined grip ellipse:** Combines lateral and longitudinal force

### `vehicle/` Module

Vehicle definition and mesh generation:

- **`vehicle.h`:**
  - `VehicleParams` — mass, wheelbase, aerodynamics, suspensions, tires, engine
  - `VehicleState` — position, speed, RPM, gear, slip angles, tire temp/wear, aerodynamics, lap, pit lane
- **`vehicle_generator.h/.cpp`:** Procedural mesh generation from vehicle parameters
- **`mesh_exporter.h/.cpp`:** OBJ format export
- **`glb_exporter.h/.cpp`:** GLB format export (glTF 2.0)

### `track/` Module

Closed parametric track:

- **`track.h/.cpp`:**
  - `TrackPoint` — track point with curvature, slope, surface
  - `TrackParams` — configuration parameters
  - `SurfaceType` — enum: Asphalt, WetAsphalt, Kerb, Grass, Gravel, Sand, Ice
  - Two predefined layouts: `Default` (oval with pit lane) and `PitCircuit` (road circuit with pits)
  - Interpolation by distance, binary search, cumulative arc length

### `input/` Module

User input abstraction:

- **`input.h`:** `InputState` — throttle, brake, steering, upshift, downshift, reset, enter_exit_box
- **`input_manager.h`:** Abstract `InputManager` interface with `poll()` and `is_key_down()`
- **`platform/windows_input.h/.cpp`:** Win32 implementation via `GetAsyncKeyState`
- **`platform/auto_input.h/.cpp`:** Programmed input for automatic driving
- **`platform/null_input.h`:** Null input for tests

### `simulation/` Module

120 Hz physics simulation loop:

- **`simulation.h/.cpp`:** `Simulation` class with `step()` method
  - 4 sub-steps per frame (30 Hz × 4 = 120 Hz)
  - Each sub-step applies: engine → aero → weather → tire temp → suspension → tire forces → braking → steering → centripetal → integrate
  - Off-track management, pit lane speed limits, respawn

### `telemetry/` Module

Data recording and export:

- **`telemetry.h/.cpp`:**
  - `TelemetryFrame` — single data frame
  - `Telemetry` — 60 Hz recording, CSV export

### `weather/` Module

Dynamic weather model:

- **`weather.h`:**
  - `WeatherState` — current state (rain intensity, wind, temperature)
  - `WeatherParams` — configurable parameters
  - Rain effect: tire cooling, grip reduction
  - Track temperature dynamics

### `plugin/` Module

Native Unity plugin:

- **`sim_plugin.h/.cpp`:**
  - C ABI compatible (`SimPlugin_Initialize`, `SimPlugin_Update`, `SimPlugin_GetVehicleState`)
  - Exported as `sim_plugin.dll`

---

## Gameplay (Game Level)

### `game/` Module

- **`gameplay.h/.cpp`:** `Gameplay` class — input polling, simulation step, lap timing, console HUD rendering, telemetry recording
- **`main.cpp`:** Interactive gameplay entry point (Windows keyboard input)
- **`main_auto.cpp`:** Automatic driving entry point (programmed input, log to file)
- **`gen_telemetry.cpp`:** Telemetry generator (auto-pilot, CSV export for Unity)

---

## Renderer (Win32 GDI)

- **`renderer.h/.cpp`:** `Renderer` class — Win32 window, back buffer, track drawing (lines), pit lane (red), vehicle (2D rectangle or 3D wireframe from OBJ), HUD overlay. Keyboard input handling. Exposed via main `main.cpp`.

---

## Unity Integration

Unity 6000.0.82f1 with Universal Render Pipeline (URP) and TextMesh Pro.

### Unity Dependencies

| Script | Function |
|--------|----------|
| `CarController.cs` | Vehicle controller + camera follow |
| `CarHUD.cs` | HUD speed / RPM / gear / lap |
| `HUDSetup.cs` | Auto-setup HUD utility |
| `SimPlugin.cs` | P/Invoke bridge to `sim_plugin.dll` |
| `CheckpointTrigger.cs` | Unity checkpoint trigger |

### Editor Menu

- **Project0 → Setup Scene** — creates HUD canvas and binds `CarController`
- **Project0 → Generate Track** — generates parametric track mesh, borders and colliders

---

## Physics Pipeline

Each simulation sub-step applies these stages in order:

| # | Function | Description |
|---|----------|-------------|
| 1 | `update_engine_forces()` | Torque curve → longitudinal force |
| 2 | `update_aerodynamics()` | Drag, downforce, ground effect |
| 3 | `update_weather()` | Rain cooling, track temperature |
| 4 | `update_tire_temperature()` | Thermal model + wear |
| 5 | `update_suspension()` | 4-corner load transfer |
| 6 | `update_tire_forces()` | Lateral Pacejka grip |
| 7 | `update_braking()` | Brake deceleration |
| 8 | `update_steering()` | Bicycle model, slip angles |
| 9 | `update_centripetal_forces()` | Centripetal/centrifugal forces from track curvature |
| 10 | `integrate()` | Semi-implicit Euler integration |

**Frequency:** 120 Hz (4 sub-steps per frame at 30 Hz)

### Sub-step Details

#### 1. Engine Forces
- Engine torque curve vs RPM
- Gear ratio calculation → RPM → wheel torque
- Transmission losses
- Engine braking
- Automatic transmission

#### 2. Aerodynamics
- Aerodynamic drag (CdA)
- Lift
- Downforce
- Ground effect
- Wing contribution
- Front/rear balance
- Drag induced by downforce

#### 3. Weather
- Rain tire cooling
- Track temperature dynamics
- Weather grip factor
- Wind effect (placeholder)

#### 4. Tire Temperature
- Thermal model (heating from slip)
- Ambient cooling
- Wear accumulation
- Operating temperature window

#### 5. Suspension
- Longitudinal load transfer (acceleration/braking)
- Lateral load transfer (steering)
- Anti-roll bar
- Body roll/pitch

#### 6. Tire Forces
- Bicycle model for slip angles
- Pacejka formula (lateral force)
- Combined grip ellipse
- Tire loads
- Camber thrust
- Yaw moment

#### 7. Braking
- Brake deceleration
- Brake balance
- Tire lockup

#### 8. Steering
- Steering input → front wheel angle
- Behavior emerges from tire model

#### 9. Centripetal Forces
- Centripetal/centrifugal forces from track curvature

#### 10. Integrate
- Semi-implicit Euler integration
- Heading-first integration
- Space-velocity integration
- Lap counting

---

## Build and Test

### Prerequisites

- Visual Studio 2022 with C++ workload
- CMake 3.20+
- PowerShell 7+
- (Optional) Unity 6000.0.82f1

### Build Commands

```powershell
# Configure
cmake -G "Visual Studio 17 2022" -A x64 -B build -DPROJECT0_BUILD_TESTS=ON -DPROJECT0_BUILD_GAMEPLAY=ON

# Release build
cmake --build build --config Release --parallel

# Build with Win32 renderer
cmake -B build -G "Visual Studio 17 2022" -A x64 -DPROJECT0_BUILD_RENDERER=ON
```

### CMake Targets

| Target | Type | Description |
|--------|------|-------------|
| `project0_engine` | Static Lib | Core simulation library |
| `sim_plugin` | Shared DLL | Native Unity plugin |
| `generate_vehicle` | EXE | Vehicle mesh generator |
| `project0_gameplay_exe` | EXE | Gameplay entry point |
| `auto_drive` | EXE | Automatic driving |
| `gen_telemetry` | EXE | Telemetry generator |
| `project0_tests` | EXE | Unit tests (33+ cases) |
| `track_analysis` | EXE | Track geometry analysis |
| `track_svg` | EXE | Track SVG export |
| `track_diagram` | EXE | Track SVG diagram exporter (CLI) |
| `project0_renderer` | Static Lib | Win32 GDI renderer |

### Running

```powershell
# Unit tests
.\build\tests\Release\project0_tests.exe

# Interactive gameplay
.\build\game\Release\project0_gameplay_exe.exe

# Automatic driving
.\build\game\Release\auto_drive.exe

# Telemetry generator
.\build\game\Release\gen_telemetry.exe

# Track analysis
.\build\experiments\Release\track_analysis.exe

# Track SVG export
.\build\experiments\Release\track_svg.exe -o track.svg -t pit

# Track diagram (CLI)
.\build\tools\Release\track_diagram.exe -o diagram.svg -t default --no-chart

# Test with CTest
ctest --output-on-failure -C Release
```

---

## Controls

| Key | Action |
|-----|--------|
| W / ↑ | Throttle |
| S / ↓ | Brake |
| A / ← | Steer left |
| D / → | Steer right |
| Shift | Shift up |
| Ctrl | Shift down |
| R | Reset position |
| ESC | Exit |

---

## Data Structures

### VehicleParams

```cpp
struct VehicleParams {
    // Mass and dimensions
    float mass;              // kg (e.g. 1500)
    float wheelbase;         // m (e.g. 2.7)
    float track_width;       // m (e.g. 1.8)
    
    // Aerodynamics
    float frontal_area;      // m²
    float drag_coefficient;  // Cd
    float lift_coefficient;  // Cl
    float downforce_coefficient;
    
    // Engine
    float max_power;         // hp
    float max_torque;        // Nm
    float max_rpm;
    float idle_rpm;
    std::vector<float> gear_ratios;
    float final_drive;
    float drivetrain_loss;   // 0.0-1.0
    
    // Suspensions
    float spring_rate;       // N/m
    float damping_rate;      // Ns/m
    float anti_roll_bar;      // Nm/deg
    
    // Tires
    float tire_radius;       // m
    float tire_width;        // m
    PacejkaCoefficients pacejka;
    float thermal_conductivity;
    float wear_rate;
    
    // Brakes
    float brake_bias;        // 0.0-1.0 (front)
    float max_brake_force;   // N
};
```

### VehicleState

```cpp
struct VehicleState {
    // Position
    Vec3 position;
    float heading;           // radians
    float speed;             // m/s
    
    // Engine
    float rpm;
    int gear;
    
    // Tires
    float slip_angle[4];     // radians
    float tire_temp[4];      // Kelvin
    float tire_wear[4];      // 0.0-1.0
    
    // Aerodynamics
    float downforce;
    float drag;
    
    // Race
    int lap;
    float lap_time;
    float best_lap_time;
    
    // Pit lane
    bool in_pit_lane;
    float pit_lane_speed_limit;
    
    // Performance
    float lateral_g;
    float longitudinal_g;
};
```

### TrackParams

```cpp
struct TrackParams {
    float length;            // m
    int num_points;
    float track_width;       // m
    float kerb_width;        // m
    float banking;           // radians
    
    // Predefined layouts
    enum Layout { Default, PitCircuit };
};
```

---

## Telemetry

The telemetry system records data at 60 Hz for each simulation frame.

### CSV Format

```
frame,time,speed,rpm,gear,lap,lap_time,
position_x,position_y,position_z,
heading,lateral_g,longitudinal_g,
throttle,brake,steering,
tire_temp_fl,tire_temp_fr,tire_temp_rl,tire_temp_rr,
tire_wear_fl,tire_wear_fr,tire_wear_rl,tire_wear_rr,
slip_angle_fl,slip_angle_fr,slip_angle_rl,slip_angle_rr,
downforce,drag,track_temp,rain_intensity
```

### Usage

```cpp
// Create telemetry
Telemetry telemetry("output.csv");

// Record each frame
telemetry.record(frame_number, simulation_state, input_state, weather_state);

// Export at end
telemetry.save_csv();
```

---

## Unity Plugin

The Unity plugin allows Unity to query the vehicle state from the C++ simulator.

### C ABI Interface

```c
// Initialization
void SimPlugin_Initialize(const VehicleParams* params);

// Update
void SimPlugin_Update(float dt, const InputState* input);

// Read vehicle state
void SimPlugin_GetVehicleState(VehicleState* state);
```

### Unity Bridge (SimPlugin.cs)

```csharp
[DllImport("sim_plugin.dll")]
public static extern void SimPlugin_Initialize(ref VehicleParams params);

[DllImport("sim_plugin.dll")]
public static extern void SimPlugin_Update(float dt, ref InputState input);

[DllImport("sim_plugin.dll")]
public static extern void SimPlugin_GetVehicleState(out VehicleState state);
```

---

## Known Issues

- Unity 600.x `Bee.DotNet.dll` blocked by AppLocker/WDAC policy (`0x800711C7`):
  - Run PowerShell: `Get-ChildItem -Recurse "D:\Unity\Editors\6000.0.82f1\Editor\Data\Tools" -File | Unblock-File`
  - Add `D:\Unity` to Windows Defender Exclusion list if policy persists
- After first launch, if track/HUD objects are missing, re-run the Project0 menus
- `LegacyRuntime.ttf` is used for HUD text embedded in Unity 2022+
- Wind effect in `update_weather()` is a placeholder (`wind_speed = 0.0`); not yet implemented
- The vehicle mesh in the Win32 GDI renderer is a 2D rectangle; procedural generation is available but not integrated into gameplay
- The `track_diagram` tool generates SVG; requires a browser for viewing

---

## Design Principles

1. **Simulate → measure → test → fix → document → repeat**
2. Separation between simulation and rendering
3. Test every component
4. Use data and benchmarks
5. Prefer a simple verifiable model over a complex incomprehensible one
6. Do not sacrifice playability for realism
7. Do not sacrifice correctness for visual effects

---

## References

- [Pacejka, Hans B. "Tyre Vehicle Dynamics"](https://www.amazon.com/Tyre-Vehicle-Dynamics-Hans-Pacejka/dp/074751520X) — Magic Formula tire model
- [Milliken, William F. "Race Car Vehicle Dynamics"](https://www.amazon.com/Race-Car-Vehicle-Dynamics-Milliken/dp/1560915269) — Race car vehicle dynamics
- [CMake Documentation](https://cmake.org/documentation/) — Build system
- [Unity Manual](https://docs.unity3d.com/Manual/) — Unity 6000.x
- [Eigen Library](https://eigen.tuxfamily.org/) — Linear algebra
