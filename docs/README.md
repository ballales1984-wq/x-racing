# X-Racing

Simulatore automobilistico di laboratorio. Costruisci, misura, testa, correggi, documenta, ripeti.

## Indice

- [Panoramica](#panoramica)
- [Stato del Progetto](#stato-del-progetto)
- [Architettura](#architettura)
  - [Engine (Libreria Core)](#engine-libreria-core)
  - [Gameplay (Livello di Gioco)](#gameplay-livello-di-gioco)
  - [Renderer (Win32 GDI)](#renderer-win32-gdi)
  - [Unity Integration](#unity-integration)
- [Pipeline Fisica](#pipeline-fisica)
- [Build e Test](#build-e-test)
- [Controlli](#controlli)
- [Struttura Dati](#struttura-dati)
- [Telemetria](#telemetria)
- [Plugin Unity](#plugin-unity)
- [Problemi Noti](#problemi-noti)
- [Principi di Design](#principi-di-design)

---

## Panoramica

**X-Racing** è un simulatore di guida automobilistico scritto in C++20, progettato come laboratorio di simulazione iterativa. L'obiettivo è costruire un'esperienza di guida realistica attraverso un approccio basato su dati, misurazioni e test continui.

Il progetto separa chiaramente la simulazione fisica dal rendering, permettendo di sviluppare e validare la fisica indipendentemente dalla grafica. Unity 6000.x è utilizzato come renderer di produzione, collegato al motore C++ tramite un plugin nativo DLL.

**Caratteristiche principali:**

- **Motore fisico 120 Hz** con pipeline di simulazione iterativa
- **Modello pneumatici Pacejka** Magic Formula con temperatura e usura
- **Sospensioni 4 angoli** con trasferimento di carico
- **Aerodinamica completa** (drag, deportanza, effetto suolo)
- **Meteo dinamico** (pioggia, temperatura, grip)
- **Tracciato parametrico** chiuso con multiple superfici
- **Telemetria CSV** per analisi e validazione
- **33+ unit test** con Google Test
- **Esportatore SVG tracciati** con diagrammi interattivi
- **Plugin Unity** per rendering in tempo reale

---

## Stato del Progetto

| Milestone | Componente | Stato |
|-----------|-----------|-------|
| M0 | Ambiente / build system | ✅ |
| M1 | Tracciato + stato veicolo | ✅ |
| M2 | Sterzo (modello bicicletta) | ✅ |
| M3 | Aderenza / cornering (Pacejka) | ✅ |
| M4 | Pneumatici (temp, usura, grip) | ✅ |
| M5 | Sospensioni (carichi 4 angoli) | ✅ |
| M6 | Aerodinamica (deportanza, pitch/roll) | ✅ |
| M7 | Meteo (pioggia, temperatura, grip) | ✅ |
| M9 | Gameplay (input, timing giri) | ✅ |
| M8 | Rendering (Unity) | ✅ Unity 6000.0.82f1, progetto operativo |
| M10 | AI (traiettoria, avversari) | ⏳ prossimo |

---

## Architettura

```
x-racing/
├── engine/                    # Libreria core (static + DLL plugin)
│   ├── common.h              # Tipi matematici, costanti, utility
│   ├── physics/
│   │   └── types.h           # Modello pneumatici Pacejka, proiezioni vettoriali
│   ├── vehicle/
│   │   ├── vehicle.h         # VehicleParams + VehicleState
│   │   ├── vehicle_generator.h/.cpp  # Generazione mesh procedurale
│   │   ├── mesh_exporter.h/.cpp      # Esportazione OBJ
│   │   └── glb_exporter.h/.cpp       # Esportazione GLB
│   ├── track/
│   │   ├── track.h/.cpp      # Tracciato parametrico chiuso
│   ├── input/
│   │   ├── input.h           # Definizione InputState
│   │   ├── input_manager.h   # Interfaccia input astratta
│   │   └── platform/
│   │       ├── windows_input.h/.cpp  # Backend Windows
│   │       ├── auto_input.h/.cpp     # Input automatico per testing
│   │       └── null_input.h          # Backend dummy per test
│   ├── simulation/
│   │   ├── simulation.h/.cpp # Loop fisica 120 Hz
│   ├── telemetry/
│   │   ├── telemetry.h/.cpp  # Registrazione frame + export CSV
│   ├── weather/
│   │   ├── weather.h         # Parametri e stato meteo
│   └── plugin/
│       ├── sim_plugin.h/.cpp # Plugin nativo Unity (DLL)
├── game/                      # Livello di gioco
│   ├── gameplay.h/.cpp       # Loop gameplay console
│   ├── main.cpp              # Entry point gameplay
│   ├── main_auto.cpp         # Entry point guida automatica
│   └── gen_telemetry.cpp     # Generatore telemetria per Unity
├── renderer/                  # Renderer Win32 GDI
│   ├── renderer.h/.cpp       # Visualizzazione wireframe 2D/3D
├── tests/                     # Unit test Google Test
│   ├── simulation_test.cpp   # 33+ test casi fisica
│   ├── physics_test.cpp      # Validazione modello Pacejka
│   ├── vehicle_test.cpp      # Test parametri veicolo
│   ├── track_test.cpp        # Test generazione tracciato
│   ├── track_diagram_test.cpp # Test esportazione SVG tracciati
│   ├── telemetry_test.cpp    # Test registrazione telemetria
│   └── gameplay_test.cpp     # Test loop gameplay
├── experiments/               # Strumenti di analisi e profiling
│   ├── track_analysis.cpp    # Analisi geometria tracciato
│   ├── ai_experiments.cpp    # [pianificato] Esperimenti ML/AI
│   └── performance.cpp       # [pianificato] Profiling, benchmark
├── UnityProject/              # Progetto Unity 2022.3.x
│   ├── Assets/
│   │   ├── Scripts/
│   │   │   ├── CarController.cs   # Controller veicolo + camera follow
│   │   │   ├── CarHUD.cs          # HUD velocità/RPM/ marcia/giro
│   │   │   ├── HUDSetup.cs        # Utility auto-setup HUD
│   │   │   └── SimPlugin.cs       # Bridge P/Invoke a sim_plugin.dll
│   │   ├── Editor/
│   │   │   ├── SceneSetup.cs      # Menu Project0 > Setup Scene
│   │   │   ├── TrackGenerator.cs  # Menu Project0 > Generate Track
│   │   │   └── GenerateTelemetry.cs
│   │   ├── Scenes/
│   │   │   ├── ok.unity           # Scena gameplay principale
│   │   │   └── impostazioni.unity # Scena impostazioni/config
│   │   ├── Materials/
│   │   │   ├── CarMaterial.mat
│   │   │   └── GroundMaterial.mat
│   │   ├── Plugins/x86_64/
│   │   │   └── sim_plugin.dll     # Plugin nativo C++
│   │   └── Settings/
│   │       ├── UniversalRenderPipelineAsset.asset
│   │       └── ForwardRendererData.asset
│   ├── Packages/
│   │   └── manifest.json
│   └── ProjectSettings/
│       └── ProjectVersion.txt
├── tools/                     # Strumenti di sviluppo
│   ├── fbx_to_obj/           # Convertitori FBX→OBJ
│   ├── track_generator/      # Generatore tracciati
│   └── track_diagram/        # Esportatore diagrammi SVG tracciati
├── data/                      # Dati runtime
│   ├── telemetry/            # Output telemetria CSV
│   └── models/               # Modelli 3D
├── assets/                    # Asset 3D, texture, audio
├── vendor/                    # Dipendenze terze parti
│   ├── Eigen/                # Algebra lineare
│   └── googletest/           # Framework unit testing
├── docs/                      # Documentazione progetto
│   ├── README.md             # Questo file
│   └── ROADMAP.md            # Piano di sviluppo
├── CMakeLists.txt             # Configurazione CMake root
├── AGENTS.md                  # Convenzioni progetto
└── main.cpp                   # Entry point renderer Win32
```

---

## Engine (Libreria Core)

### Modulo `common.h`

Tipi matematici e utility fondamentali:

- **Tipi vettoriali:** `Vec2`, `Vec3`, `Vec4` (wrapper Eigen)
- **Tipi matriciali:** `Mat2`, `Mat3`, `Mat4`
- **Costanti:** `kPi`, `kHalfPi`, `kGravity` (9.81 m/s²), `kEpsilon`
- **Utility:** `clamp()`, `normalize_angle()`, `lerp()`, `smoothstep()`

### Modulo `physics/`

Implementazione del modello pneumatici Pacejka Magic Formula:

- **`types.h`:** Funzioni di proiezione/rifiuto vettoriale, `cross2()`, helper forze centripete/centrifughe
- **Formula di Pacejka:** Coefficienti B, C, D, E per calcolo forza laterale
- **Ellisse di aderenza combinata:** Combina forza laterale e longitudinale

### Modulo `vehicle/`

Definizione veicolo e generazione mesh:

- **`vehicle.h`:** 
  - `VehicleParams` — massa, passo, aerodinamica, sospensioni, pneumatici, motore
  - `VehicleState` — posizione, velocità, RPM, marcia, angoli slittamento, temp/consumo gomme, aerodinamica, giro, box lane
- **`vehicle_generator.h/.cpp`:** Generazione mesh procedurale da parametri veicolo
- **`mesh_exporter.h/.cpp`:** Esportazione formato OBJ
- **`glb_exporter.h/.cpp`:** Esportazione formato GLB (glTF 2.0)

### Modulo `track/`

Tracciato parametrico chiuso:

- **`track.h/.cpp`:**
  - `TrackPoint` — punto del tracciato con curvatura, pendenza, superficie
  - `TrackParams` — parametri di configurazione
  - `SurfaceType` — enum: Asphalt, WetAsphalt, Kerb, Grass, Gravel, Sand, Ice
  - Due layout predefiniti: `Default` (ovale con box lane) e `PitCircuit` (circuito stradale con box)
  - Interpolazione per distanza, ricerca binaria, lunghezza d'arco cumulativa

### Modulo `input/`

Astrazione input utente:

- **`input.h`:** `InputState` — throttle, brake, sterzo, upshift, downshift, reset, enter_exit_box
- **`input_manager.h`:** Interfaccia astratta `InputManager` con `poll()` e `is_key_down()`
- **`platform/windows_input.h/.cpp`:** Implementazione Win32 tramite `GetAsyncKeyState`
- **`platform/auto_input.h/.cpp`:** Input programmato per guida automatica
- **`platform/null_input.h`:** Input nullo per test

### Modulo `simulation/`

Loop di simulazione fisica a 120 Hz:

- **`simulation.h/.cpp`:** Classe `Simulation` con metodo `step()`
  - 4 sotto-passi per frame (30 Hz × 4 = 120 Hz)
  - Ogni sotto-passo applica: engine → aero → weather → tire temp → suspension → tire forces → braking → steering → centripetal → integrate
  - Gestione off-track, limiti velocità box lane, respawn

### Modulo `telemetry/`

Registrazione e export dati:

- **`telemetry.h/.cpp`:** 
  - `TelemetryFrame` — frame dati singolo
  - `Telemetry` — registrazione a 60 Hz, export CSV

### Modulo `weather/`

Modello meteo dinamico:

- **`weather.h`:** 
  - `WeatherState` — stato corrente (intensità pioggia, vento, temperatura)
  - `WeatherParams` — parametri configurabili
  - Effetto pioggia: raffreddamento pneumatici, riduzione grip
  - Dinamica temperatura pista

### Modulo `plugin/`

Plugin nativo Unity:

- **`sim_plugin.h/.cpp`:** 
  - ABI C compatibile (`SimPlugin_Initialize`, `SimPlugin_Update`, `SimPlugin_GetVehicleState`)
  - Esportato come `sim_plugin.dll`

---

## Gameplay (Livello di Gioco)

### Modulo `game/`

- **`gameplay.h/.cpp`:** Classe `Gameplay` — polling input, step simulazione, timing giri, rendering HUD console, registrazione telemetria
- **`main.cpp`:** Entry point gameplay interattivo (input tastiera Windows)
- **`main_auto.cpp`:** Entry point guida automatica (input programmato, log su file)
- **`gen_telemetry.cpp`:** Generatore telemetria (pilota automatico, export CSV per Unity)

---

## Renderer (Win32 GDI)

- **`renderer.h/.cpp`:** Classe `Renderer` — finestra Win32, back buffer, disegno tracciato (linee), box lane (rosso), vettura (rettangolo 2D o wireframe 3D da OBJ), overlay HUD. Gestione input tastiera. Esposto tramite `main.cpp` principale.

---

## Unity Integration

Unity 6000.0.82f1 con Universal Render Pipeline (URP) e TextMesh Pro.

### Dipendenze Unity

| Script | Funzione |
|--------|----------|
| `CarController.cs` | Controller veicolo + camera follow |
| `CarHUD.cs` | HUD velocità / RPM / marcia / giro |
| `HUDSetup.cs` | Utility auto-setup HUD |
| `SimPlugin.cs` | Bridge P/Invoke a `sim_plugin.dll` |
| `CheckpointTrigger.cs` | Trigger checkpoint Unity |

### Menu Editor

- **Project0 → Setup Scene** — crea canvas HUD e bind `CarController`
- **Project0 → Generate Track** — genera mesh tracciato parametrico, bordi e collider

---

## Pipeline Fisica

Ogni sotto-passo di simulazione applica questi stadi in ordine:

| # | Funzione | Descrizione |
|---|----------|-------------|
| 1 | `update_engine_forces()` | Curva coppia → forza longitudinale |
| 2 | `update_aerodynamics()` | Drag, deportanza, effetto suolo |
| 3 | `update_weather()` | Raffreddamento pioggia, temperatura pista |
| 4 | `update_tire_temperature()` | Modello termico + usura |
| 5 | `update_suspension()` | Trasferimento carico 4 angoli |
| 6 | `update_tire_forces()` | Aderenza laterale Pacejka |
| 7 | `update_braking()` | Decelerazione freni |
| 8 | `update_steering()` | Modello bicicletta, angoli slittamento |
| 9 | `update_centripetal_forces()` | Forze centripete/centrifughe da curvatura |
| 10 | `integrate()` | Integrazione semi-implicita di Eulero |

**Frequenza:** 120 Hz (4 sotto-passi per frame a 30 Hz)

### Dettaglio Sotto-Passi

#### 1. Engine Forces
- Curva coppia motore vs RPM
- Calcolo rapporto marcia → RPM → coppia ruota
- Perdite trasmissione
- Frenomotore
- Cambio automatico

#### 2. Aerodynamics
- Drag aerodinamico (CdA)
- Lift (sollevamento)
- Downforce deportanza
- Effetto suolo (ground effect)
- Contributo alettoni
- Bilanciamento anteriore/posteriore
- Drag indotto da deportanza

#### 3. Weather
- Raffreddamento pneumatici per pioggia
- Dinamica temperatura pista
- Fattore grip da meteo
- Effetto vento (placeholder)

#### 4. Tire Temperature
- Modello termico (riscaldamento per slittamento)
- Raffreddamento ambientale
- Accumulo usura
- Finestra operativa temperatura

#### 5. Suspension
- Trasferimento carico longitudinale (accelerazione/frenata)
- Trasferimento carico laterale (sterzo)
- Barra antirollio
- Rollio/cambio beccheggio telaio

#### 6. Tire Forces
- Modello bicicletta per angoli slittamento
- Formula di Pacejka (forza laterale)
- Ellisse aderenza combinata
- Carichi pneumatici
- Spinta camber
- Momento di yaw

#### 7. Braking
- Decelerazione freni
- Bilanciamento freni
- Bloccaggio pneumatici

#### 8. Steering
- Input sterzo → angolo ruote anteriori
- Comportamento emerge da modello pneumatici

#### 9. Centripetal Forces
- Forze centripete/centrifughe da curvatura tracciato

#### 10. Integrate
- Integrazione semi-implicita di Eulero
- Integrazione heading-first
- Integrazione spazio-velocità
- Conteggio giri

---

## Build e Test

### Prerequisiti

- Visual Studio 2022 con workload C++
- CMake 3.20+
- PowerShell 7+
- (Opzionale) Unity 6000.0.82f1

### Comandi Build

```powershell
# Configurazione
cmake -G "Visual Studio 17 2022" -A x64 -B build -DPROJECT0_BUILD_TESTS=ON -DPROJECT0_BUILD_GAMEPLAY=ON

# Build Release
cmake --build build --config Release --parallel

# Build con renderer Win32
cmake -B build -G "Visual Studio 17 2022" -A x64 -DPROJECT0_BUILD_RENDERER=ON
```

### Target CMake

| Target | Tipo | Descrizione |
|--------|------|-------------|
| `project0_engine` | Static Lib | Libreria core simulazione |
| `sim_plugin` | Shared DLL | Plugin nativo Unity |
| `generate_vehicle` | EXE | Generatore mesh veicolo |
| `project0_gameplay_exe` | EXE | Entry point gameplay |
| `auto_drive` | EXE | Guida automatica |
| `gen_telemetry` | EXE | Generatore telemetria |
| `project0_tests` | EXE | Unit test (33+ casi) |
| `track_analysis` | EXE | Analisi geometria tracciato |
| `track_svg` | EXE | Esportazione diagramma SVG tracciato |
| `track_diagram` | EXE | Esportatore SVG tracciato (CLI) |
| `project0_renderer` | Static Lib | Renderer Win32 GDI |

### Esecuzione

```powershell
# Test unitari
.\build\tests\Release\project0_tests.exe

# Gameplay interattivo
.\build\game\Release\project0_gameplay_exe.exe

# Guida automatica
.\build\game\Release\auto_drive.exe

# Generatore telemetria
.\build\game\Release\gen_telemetry.exe

# Analisi tracciato
.\build\experiments\Release\track_analysis.exe

# Esportazione SVG tracciato
.\build\experiments\Release\track_svg.exe -o track.svg -t pit

# Diagramma tracciato (CLI)
.\build\tools\Release\track_diagram.exe -o diagram.svg -t default --no-chart

# Test con CTest
ctest --output-on-failure -C Release
```

---

## Controlli

| Tasto | Azione |
|-------|--------|
| W / ↑ | Acceleratore |
| S / ↓ | Freno |
| A / ← | Sterzo sinistra |
| D / → | Sterzo destra |
| Shift | Marcia su |
| Ctrl | Marcia giù |
| R | Reset posizione |
| ESC | Esci |

---

## Struttura Dati

### VehicleParams

```cpp
struct VehicleParams {
    // Massa e dimensioni
    float mass;              // kg (es. 1500)
    float wheelbase;         // m (es. 2.7)
    float track_width;       // m (es. 1.8)
    
    // Aerodinamica
    float frontal_area;      // m²
    float drag_coefficient;  // Cd
    float lift_coefficient;  // Cl
    float downforce_coefficient;
    
    // Motore
    float max_power;         // hp
    float max_torque;        // Nm
    float max_rpm;
    float idle_rpm;
    std::vector<float> gear_ratios;
    float final_drive;
    float drivetrain_loss;   // 0.0-1.0
    
    // Sospensioni
    float spring_rate;       // N/m
    float damping_rate;      // Ns/m
    float anti_roll_bar;      // Nm/deg
    
    // Pneumatici
    float tire_radius;       // m
    float tire_width;        // m
    PacejkaCoefficients pacejka;
    float thermal_conductivity;
    float wear_rate;
    
    // Freni
    float brake_bias;        // 0.0-1.0 (anteriore)
    float max_brake_force;   // N
};
```

### VehicleState

```cpp
struct VehicleState {
    // Posizione
    Vec3 position;
    float heading;           // radianti
    float speed;             // m/s
    
    // Motore
    float rpm;
    int gear;
    
    // Pneumatici
    float slip_angle[4];     // radianti
    float tire_temp[4];      // Kelvin
    float tire_wear[4];      // 0.0-1.0
    
    // Aerodinamica
    float downforce;
    float drag;
    
    // Corsa
    int lap;
    float lap_time;
    float best_lap_time;
    
    // Box lane
    bool in_box_lane;
    float box_lane_speed_limit;
    
    // Prestazioni
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
    float banking;           // radianti
    
    // Layout predefiniti
    enum Layout { Default, PitCircuit };
};
```

---

## Telemetria

Il sistema di telemetria registra dati a 60 Hz per ogni frame di simulazione.

### Formato CSV

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

### Utilizzo

```cpp
// Creazione telemetria
Telemetry telemetry("output.csv");

// Registrazione ogni frame
telemetry.record(frame_number, simulation_state, input_state, weather_state);

// Export alla fine
telemetry.save_csv();
```

---

## Plugin Unity

Il plugin Unity permette a Unity di interrogare lo stato del veicolo dal simulatore C++.

### Interfaccia C ABI

```c
// Inizializzazione
void SimPlugin_Initialize(const VehicleParams* params);

// Aggiornamento
void SimPlugin_Update(float dt, const InputState* input);

// Lettura stato veicolo
void SimPlugin_GetVehicleState(VehicleState* state);
```

### Bridge Unity (SimPlugin.cs)

```csharp
[DllImport("sim_plugin.dll")]
public static extern void SimPlugin_Initialize(ref VehicleParams params);

[DllImport("sim_plugin.dll")]
public static extern void SimPlugin_Update(float dt, ref InputState input);

[DllImport("sim_plugin.dll")]
public static extern void SimPlugin_GetVehicleState(out VehicleState state);
```

---

## Problemi Noti

- Unity 600.x `Bee.DotNet.dll` bloccato da policy AppLocker/WDAC (`0x800711C7`):
  - Esegui PowerShell: `Get-ChildItem -Recurse "D:\Unity\Editors\6000.0.82f1\Editor\Data\Tools" -File | Unblock-File`
  - Aggiungi `D:\Unity` a Windows Defender Exclusion list se la policy persiste
- Dopo il primo avvio, se mancano oggetti tracciato/HUD, riesegui i menu Project0
- `LegacyRuntime.ttf` è usato per il testo HUD integrato in Unity 2022+
- Effetto vento in `update_weather()` è un placeholder (`wind_speed = 0.0`); non ancora implementato
- La mesh del veicolo nel renderer Win32 GDI è un rettangolo 2D; la generazione procedurale è disponibile ma non integrata nel gameplay
- Il tool `track_diagram` genera SVG; richiede un browser per la visualizzazione

---

## Principi di Design

1. **Simula → misura → testa → correggi → documenta → ripeti**
2. Separazione tra simulazione e rendering
3. Testa ogni componente
4. Usa dati e benchmark
5. Preferisci un modello semplice verificabile a uno complesso incompreso
6. Non sacrificare la giocabilità per il realismo
7. Non sacrificare la correttezza per gli effetti visivi

---

## Riferimenti

- [Pacejka, Hans B. "Tyre Vehicle Dynamics"](https://www.amazon.com/Tyre-Vehicle-Dynamics-Hans-Pacejka/dp/074751520X) — Modello pneumatici Magic Formula
- [Milliken, William F. "Race Car Vehicle Dynamics"](https://www.amazon.com/Race-Car-Vehicle-Dynamics-Milliken/dp/1560915269) — Dinamica veicolo da corsa
- [CMake Documentation](https://cmake.org/documentation/) — Build system
- [Unity Manual](https://docs.unity3d.com/Manual/) — Unity 6000.x
- [Eigen Library](https://eigen.tuxfamily.org/) — Algebra lineare
