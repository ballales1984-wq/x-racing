# X-Racing — Piano di Sviluppo

> Motto: *simula → misura → testa → correggi → documenta → ripeti*
>
> Regola d'oro: **non passare alla fase successiva finché la precedente non è sufficientemente funzionante.**

---

## Stato Attuale

| Componente | Stato |
|---|
| Build system (CMake) | ✅ |
| Tipi matematici + utility | ✅ |
| Modello pneumatici (Pacejka) | ✅ |
| Stato veicolo + parametri | ✅ |
| Tracciato parametrico | ✅ |
| Input manager (Windows) | ✅ |
| Simulazione 120 Hz | ✅ |
| Telemetria + export CSV | ✅ |
| Meteo (pioggia, temperatura, grip) | ✅ |
| Gameplay (input, timing giri) | ✅ |
| Diagrammi tracciato SVG | ✅ |
| Unit test (35+ GoogleTest) | ✅ |
| Rendering (Unity placeholder) | ✅ |
| Integrazione editor Unity | ✅ |
| Race management (pit, validation, config) | ✅ |
| AI (traiettoria, avversari) | ⏳ prossimo |

---

## Metodologia di Sviluppo

### Ciclo Iterativo

1. **Implementa** il componente minimo necessario
2. **Testa** con unit test + gameplay manuale
3. **Documenta** comportamento e limiti
4. **Misura** (telemetria, prestazioni)
5. **Correggi** bug e casi limite
6. **Solo allora** passa al componente successivo

### Criteri di Completamento

Ogni milestone ha un criterio di completamento verificabile:

- **Playable:** si può completare un giro senza crash
- **Testabile:** tutti i componenti hanno unit test
- **Misurabile:** telemetria sempre attiva per validazione

### Priorità di Implementazione

| Priorità | Componente | Dipendenze |
|---|---|---|
| P0 | Sistema veicolo | fisica, input |
| P0 | Sistema tracciato | spline, mesh |
| P1 | Collider + checkpoint | sistema tracciato |
| P1 | Lap tracker | checkpoint |
| P1 | Sistema camera | stato veicolo |
| P2 | Race manager | lap tracker, countdown |
| P2 | HUD | race manager, stato veicolo |
| P3 | Racing line AI | spline tracciato |
| P3 | Driver AI | racing line, veicolo |
| P3 | Avversari | driver AI, race manager |
| P4 | Generazione procedurale | sistema tracciato |
| P4 | Vegetazione / ambiente | generazione procedurale |
| P5 | Pipeline rendering | Unity / renderer |
| P5 | Audio | sistema veicolo |
| P5 | UI / menu | race manager |

---

## Fase 1 — Vertical Slice

**Obiettivo:** Avvio → vettura su tracciato → guida → giro completo → arrivo.

Niente alberi, niente dettagli grafici, niente AI sofisticata.

### 1.1 Veicolo

- [x] Acceleratore (curva coppia → forza longitudinale)
- [x] Freno (decelerazione, bilanciamento)
- [x] Sterzo (modello bicicletta, Ackermann)
- [ ] Retromarcia
- [x] Cambio / velocità (marcia → rapporto → RPM)
- [x] Cambio automatico

**Criterio di completamento:** il veicolo accelera, frena, sterza e cambia marcia correttamente.

### 1.2 Fisica

- [x] Aderenza (Pacejka laterale/longitudinale)
- [x] Accelerazione (mappa motore, rapporti marce)
- [x] Frenata (bilanciamento, bloccaggio)
- [ ] Peso / massa
- [x] Sospensioni (carichi 4 angoli, trasferimento peso)
- [x] Collisioni (limitazioni tracciato, barriere, off-track)
- [x] Pneumatici (temperatura, usura, grip)
- [x] Aerodinamica (deportanza, drag, effetto suolo)
- [x] Meteo (pioggia, temperatura, grip)

**Criterio di completamento:** la vettura risponde realisticamente a input di sterzo, acceleratore e freno.

### 1.3 Tracciato

- [x] Tracciato parametrico chiuso
- [x] Superfici multiple (asfalto, erba, ghiaia)
- [x] Box lane
- [x] Posizione veicolo (respawn, reset)
- [x] Start line / griglia (data model)
- [x] Checkpoint (data model)
- [ ] Rilevamento giri (runtime)
- [ ] Collider mesh-based

**Criterio di completamento:** si può definire un tracciato parametrico e la vettura lo percorre correttamente.

### 1.4 Camera

- [x] Camera chase (base)
- [ ] Distanza configurabile
- [ ] Rotazione / look-ahead
- [ ] Smoothing (lerp / spring)

**Criterio di completamento:** la camera segue la vettura con visuale accettabile.

### 1.5 HUD Minimo

- [x] Velocità (km/h)
- [x] RPM (bar o numero)
- [x] Marcia corrente
- [x] Giro corrente / totale
- [x] Tempo giro / miglior giro

**Criterio di completamento:** si può completare un giro completo senza crash, con vettura guidabile e HUD leggibile.

---

## Fase 2 — Sistema Tracciato

**Obiettivo:** Costruire un sistema che permetta di creare tracciati parametrici senza lavoro manuale.

- [x] Spline tracciato (definizione percorso centrale)
- [x] Larghezza variabile (segmento per segmento)
- [x] Mesh asfalto (generata da spline + larghezza)
- [ ] Cordoli (elevazione/banking opzionale)
- [ ] Guardrail (barriere perimetrali)
- [ ] Area di run-off (zona errore)
- [x] Sistema checkpoint (data model, volumi trigger in pianificazione)
- [x] Export dati tracciato (SVG, JSON / binario per gameplay)

**Criterio di completamento:** si può definire un nuovo tracciato modificando solo i parametri spline e generare l'intera geometria automaticamente.

---

## Fase 3 — Ambiente Procedurale

**Obiettivo:** Popolare la scena attorno al tracciato in modo procedurale.

- [ ] Edge detection tracciato (terrain following)
- [ ] Zona di generazione (bounding box attorno al tracciato)
- [ ] Alberi (asset Meshy, LOD, impostor)
- [ ] Rocce
- [ ] Erba (instancing, shader vento)
- [ ] Segnali (marcatori racing line, segnaletica)
- [ ] Barriere / recinzioni
- [ ] Cielo / atmosfera

**Criterio di completamento:** carichi un tracciato e l'ambiente si genera automaticamente, con sufficiente varietà visiva.

---

## Fase 4 — Corsa

**Obiettivo:** Trasformare la guida libera in una corsa vera e propria.

- [x] Race config (RaceDefinition, CarAssignment, TeamDefinition)
- [x] Track data system (TrackData, GridDefinition, Checkpoints, StartFinishLine)
- [x] Pit lane system (PitLaneSystem, PitBox, SpeedDetectionZone)
- [x] Pit stop FSM (PitStopFSM, PitStopManager)
- [x] Race manager (RaceManager orchestrator)
- [x] Validation engine (validate geometry, direction, grid, pit, race, assignments)
- [ ] Countdown (3-2-1-VIA)
- [ ] Griglia di partenza (posizioni, spacing)
- [ ] Avversari AI base (waypoint following, velocità fissa)
- [ ] Classifica (posizione, distacco, miglior giro)
- [ ] Sistema giri (contatore, validazione giri)
- [ ] Penalità (taglio tracciato, falsa partenza)
- [ ] Bandiere (gialla, rossa, a scacchi)
- [ ] Fine corsa (schermata risultati, restart)

**Criterio di completamento:** si può correre una gara N-giri contro avversari con countdown, classifica e fine gara.

---

## Fase 5 — AI Avanzata

**Obiettivo:** Avversari che guidano in modo realistico e adattivo.

- [ ] Racing line (ottimizzazione linea ideale per il tracciato)
- [ ] Driver AI (accelerazione, frenata, sterzo basati su racing line)
- [ ] Sorpasso (decision making, analisi distacchi)
- [ ] Difesa (posizionamento in curve)
- [ ] Adattamento traffico
- [ ] Errori umani (piccole imperfezioni, varianza)
- [ ] Esperimenti ML (comportamento motore, suono, guida adattiva)

**Criterio di completamento:** gli avversari sono competitivi, commettono errori realistici e offrono difficoltà variabile.

---

## Fase 6 — Presentazione

**Obiettivo:** Grafica, audio, UI, ottimizzazione.

- [ ] Asset Meshy (modelli 3D, texture PBR)
- [ ] Alberi scansionati (fotogrammetria)
- [ ] Illuminazione (HDR, GI, ora del giorno)
- [ ] Materiali (asfalto, metallo, gomma)
- [ ] Particelle (polvere, pioggia, scintille)
- [ ] Effetti (motion blur, DOF, lens flare)
- [ ] Motore audio (motore, scarico, ambiente)
- [ ] UI (menu, HUD avanzato, mappa)
- [ ] Ottimizzazione (LOD, instancing, draw calls)

**Criterio di completamento:** il gioco è visivamente competitivo con titoli simili.

---

## Struttura Codice (Pianificata)

```
x-racing/
├── engine/
│   ├── common.h               - Tipi matematici, costanti, utility
│   ├── physics/
│   │   └── types.h           - Modello pneumatici Pacejka, proiezioni vettoriali
│   ├── vehicle/
│   │   ├── vehicle.h         - VehicleParams + VehicleState
│   │   ├── vehicle_generator.h/.cpp - Generazione mesh procedurale
│   │   ├── mesh_exporter.h/.cpp     - Esportazione OBJ
│   │   └── glb_exporter.h/.cpp      - Esportazione GLB
│   ├── track/
│   │   ├── track.h/.cpp      - Tracciato parametrico chiuso
│   ├── input/
│   │   ├── input.h           - InputState definition
│   │   ├── input_manager.h   - Interfaccia input astratta
│   │   └── platform/
│   │       ├── windows_input.h/.cpp  - Backend Windows
│   │       ├── auto_input.h/.cpp     - Input automatico per testing
│   │       └── null_input.h          - Backend dummy per test
│   ├── simulation/
│   │   ├── simulation.h/.cpp - Loop fisica 120 Hz
│   ├── telemetry/
│   │   ├── telemetry.h/.cpp  - Registrazione frame + export CSV
│   ├── weather/
│   │   └── weather.h         - Parametri e stato meteo
│   └── plugin/
│       ├── sim_plugin.h/.cpp - Plugin nativo Unity (DLL)
├── game/
│   ├── gameplay.h/.cpp       - Loop gameplay console
│   ├── main.cpp              - Entry point gameplay
│   ├── main_auto.cpp         - Entry point guida automatica
│   └── gen_telemetry.cpp     - Generatore telemetria per Unity
├── renderer/
│   └── renderer.h/.cpp       - Renderer Win32 GDI
├── UnityProject/
│   ├── Assets/
│   │   ├── Scripts/          - CarController, CarHUD, SimPlugin
│   │   ├── Editor/           - TrackGenerator, SceneSetup
│   │   ├── Scenes/           - ok.unity, impostazioni.unity
│   │   ├── Materials/        - CarMaterial, GroundMaterial
│   │   ├── Plugins/x86_64/   - sim_plugin.dll
│   │   └── Settings/         - URP asset, renderer data
│   ├── Packages/
│   │   └── manifest.json
│   └── ProjectSettings/
├── tests/
│   ├── simulation_test.cpp   - Test fisica (33+ casi)
│   ├── physics_test.cpp      - Validazione Pacejka
│   ├── vehicle_test.cpp      - Test veicolo
│   ├── track_test.cpp        - Test tracciato
│   ├── track_diagram_test.cpp - Test esportazione SVG tracciati
│   ├── telemetry_test.cpp    - Test telemetria
│   └── gameplay_test.cpp     - Test gameplay
├── experiments/
│   ├── track_analysis.cpp    - Analisi geometria tracciato
│   ├── track_svg.cpp         - Esportazione SVG tracciati
│   ├── ai_experiments.cpp    - [pianificato] Esperimenti ML/AI
│   └── performance.cpp       - [pianificato] Profiling, benchmark
├── tools/
│   ├── fbx_to_obj/           - Convertitori FBX→OBJ
│   ├── track_generator/      - Generatore tracciati
│   └── track_diagram/        - Esportatore diagrammi SVG tracciati
├── data/
│   ├── telemetry/            - Output telemetria CSV
│   └── models/               - Modelli 3D
├── assets/                   - Asset 3D, texture, audio
├── vendor/                   - Dipendenze terze parti
│   ├── Eigen/                - Algebra lineare
│   └── googletest/           - Framework unit testing
├── docs/                     - Documentazione
│   ├── README.md             - Panoramica progetto
│   └── ROADMAP.md            - Questo file
├── CMakeLists.txt             - Configurazione CMake root
├── AGENTS.md                  - Convenzioni progetto
└── main.cpp                   - Entry point renderer Win32
```

---

## Note di Sviluppo

- Ogni fase produce una **build giocabile**
- Il codice è separato in moduli indipendenti
- Ogni modulo ha i propri test
- La telemetria è sempre attiva per validazione
- Il rendering è disaccoppiato dalla simulazione
- Le dipendenze tra moduli sono esplicite e minime
- La priorità P0 deve essere completata prima di passare a P1

---

## Note Tecniche

### Dipendenze Esterne

| Libreria | Versione | Utilizzo |
|----------|----------|----------|
| Eigen | 3.4+ | Algebra lineare (vettori, matrici) |
| Google Test | 1.14+ | Unit testing |
| Unity | 6000.0.82f1 | Rendering produzione |
| TextMesh Pro | 3.0+ | HUD e testo Unity |
| CMake | 3.20+ | Build system |
| Visual Studio | 2022 | IDE / compilatore |

### Convenzioni Codice

- C++20 standard
- Header: `.h`, implementazione: `.cpp`
- Test: Google Test framework
- Dati tracciato: directory `data/`
- Artefatti build esclusi da version control (`build/`, `build2/`, `build3/`)

---

## Risorse

- [Pacejka, Hans B. "Tyre Vehicle Dynamics"](https://www.amazon.com/Tyre-Vehicle-Dynamics-Hans-Pacejka/dp/074751520X) — Modello pneumatici Magic Formula
- [Milliken, William F. "Race Car Vehicle Dynamics"](https://www.amazon.com/Race-Car-Vehicle-Dynamics-Milliken/dp/1560915269) — Dinamica veicolo da corsa
- [CMake Documentation](https://cmake.org/documentation/) — Build system
- [Unity Manual](https://docs.unity3d.com/Manual/) — Unity 6000.x
- [Eigen Library](https://eigen.tuxfamily.org/) — Algebra lineare
