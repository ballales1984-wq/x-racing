# X-Racing — Piano di Sviluppo

> Motto: *simulate → measure → test → correct → document → repeat*
>
> Regola d'oro: **non passare alla fase successiva finché quella precedente non è abbastanza funzionante.**

---

## Stato Attuale

| Componente | Stato |
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
| Unit tests (18 GoogleTest) | ✅ |
| Rendering (Unity placeholder) | ⏳ |
| AI (trajectory, opponents) | ⏳ |

---

## Fase 1 — Vertical Slice

Obiettivo: **Avvio → macchina sulla pista → guida → giro completo → arrivo.**

Niente alberi, niente dettagli grafici, niente AI sofisticata.

### 1.1 Macchina

- [ ] Acceleratore (torque curve → longitudinal force)
- [ ] Freno (brake deceleration, balance)
- [ ] Sterzo (bicycle model, Ackermann)
- [ ] Retromarcia
- [ ] Cambio / velocità (marcia → rapporto → RPM)

### 1.2 Fisica

- [ ] Aderenza (Pacejka lat/long)
- [ ] Accelerazione (engine map, gear ratios)
- [ ] Frenata (brake bias, lock-up)
- [ ] Peso / massa
- [ ] Sospensioni (4-corner loads, weight transfer)
- [ ] Collisioni (track boundaries, barriers)

### 1.3 Pista

- [ ] Collider corretto (mesh-based o spline-based)
- [ ] Linea di partenza / griglia
- [ ] Checkpoint (trigger zones)
- [ ] Rilevamento giro (lap detection)
- [ ] Posizione macchina (respawn, reset)

### 1.4 Camera

- [ ] Camera inseguimento (chase cam)
- [ ] Distanza configurable
- [ ] Rotazione / look-ahead
- [ ] Smoothing (lerp / spring)

### 1.5 HUD minimo

- [ ] Velocità (km/h)
- [ ] RPM (bar o numero)
- [ ] Marcia corrente
- [ ] Giro corrente / totale
- [ ] Tempo giro / best lap

**Criterio di completamento:** riesci a fare un giro completo di pista senza crash, con macchina guidabile e HUD leggibile.

---

## Fase 2 — Sistema della Pista

Obiettivo: **costruire un sistema che permetta di creare piste parametriche senza lavoro manuale.**

- [ ] Spline pista (definizione percorso centrale)
- [ ] Larghezza variabile (segment-by-segment)
- [ ] Mesh asfalto (generata da spline + larghezza)
- [ ] Cordoli (kerbs, elevation banking opzionale)
- [ ] Guardrail (barriere perimetrali)
- [ ] Runoff area (zona di errore)
- [ ] Checkpoint system (trigger volumes sulla spline)
- [ ] Track data export (JSON / binary per gameplay)

**Criterio di completamento:** puoi definire una nuova pista modificando solo i parametri della spline e generi l'intera geometria automaticamente.

---

## Fase 3 — Ambiente Procedurale

Obiettivo: **popolare la scena intorno alla pista in modo procedurale.**

- [ ] Bordo pista (edge detection, terrain following)
- [ ] Zona generazione (bounding box attorno alla pista)
- [ ] Alberi (Meshy assets, LOD, impostor)
- [ ] Rocce
- [ ] Erba (instancing, wind shader)
- [ ] Cartelli (racing line markers, track signage)
- [ ] Barriere / recinzioni
- [ ] Cielo / atmosfera

**Criterio di completamento:** carichi una pista e l'ambiente si genera automaticamente, con varietà visiva sufficiente.

---

## Fase 4 — Gara

Obiettivo: **trasformare la guida libera in una gara.**

- [ ] Countdown (3-2-1-GO)
- [ ] Griglia di partenza (posizioni, spacing)
- [ ] Race manager (stato: countdown → racing → finished)
- [ ] Avversari AI basilari (waypoint following, fixed speed)
- [ ] Classifiche (posizione, gap, best lap)
- [ ] Sistema giri (lap counter, lap validation)
- [ ] Penalità (cutting track, false start)
- [ ] Bandiere (yellow, red, checkered)
- [ ] Fine gara (results screen, restart)

**Criterio di completamento:** puoi correre una gara di N giri contro avversari con countdown, classifica e fine gara.

---

## Fase 5 — AI Avanzata

Obiettivo: **avversari che guidano in modo realistico e adattivo.**

- [ ] Racing line (ottimizzazione della linea ideale per la pista)
- [ ] AI driver (accelerazione, frenata, sterzo basati su racing line)
- [ ] Sorpasso (decision making, gap analysis)
- [ ] Difesa (posizione in curva)
- [ ] Adattamento al traffico
- [ ] Errori umani (piccole imperfezioni, varianza)
- [ ] ML experiments (comportamento motore, sound, guida adattiva)

**Criterio di completamento:** gli avversari sono competitivi, commettono errori realistici e offrono una sfida variabile per difficoltà.

---

## Fase 6 — Presentazione

Obiettivo: **grafica, audio, UI, ottimizzazione.**

- [ ] Asset Meshy (modelli 3D, texture PBR)
- [ ] Alberi scansionati (photogrammetry)
- [ ] Illuminazione (HDR, GI, time of day)
- [ ] Materiali (asfalto, metallo, gomma)
- [ ] Particelle (polvere, pioggia, scintille)
- [ ] Effetti (motion blur, DOF, lens flare)
- [ ] Sound engine (motore, scarico, ambiente)
- [ ] UI (menu, HUD avanzato, mappa)
- [ ] Ottimizzazione (LOD, instancing, draw calls)

**Criterio di completamento:** il gioco è visivamente competitivo con titoli del genere.

---

## Struttura Codice

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
├── tests/
│   ├── simulation_test.cpp   - Physics tests
│   ├── track_test.cpp        - Track tests
│   ├── ai_test.cpp           - AI tests
│   └── race_test.cpp         - Race system tests
├── experiments/
│   ├── track_analysis.cpp    - Track geometry analysis
│   ├── ai_experiments.cpp    - ML / AI experiments
│   └── performance.cpp       - Profiling, benchmarks
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

## Priorità di Implementazione

| Priorità | Componente | Dipendenze |
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

## Workflow di Sviluppo

1. **Implementa** il componente minimo necessario
2. **Testa** con unit test + gameplay manuale
3. **Documenta** comportamento e limiti
4. **Misura** (telemetry, performance)
5. **Correggi** bug e edge cases
6. **Solo allora** passa al prossimo componente

---

## Note

- Ogni fase produce un **build giocabile**
- Il codice è separato in moduli indipendenti
- Ogni modulo ha i suoi test
- La telemetry è sempre abilitata per validazione
- Il rendering è disaccoppiato dalla simulazione
