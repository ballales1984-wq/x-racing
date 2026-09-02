# X-Racing — Documentazione del Gioco

> *Un laboratorio di guida che, anche con i suoi difetti, è sorprendentemente giocabile — e molto bello.*

---

## Indice

- [Cos'è X-Racing](#cosè-x-racing)
- [Come Si Gioca](#come-si-gioca)
- [Fase di Gioco](#fase-di-gioco)
- [La Macchina](#la-macchina)
- [Fisica e Sensazione di Guida](#fisica-e-sensazione-di-guida)
- [I Tracciati](#i-tracciati)
- [Gara e AI](#gara-e-ai)
- [Meteo e Superfici](#meteo-e-superfici)
- [Telemetria](#telemetaria)
- [Rendering: Console, GDI e Unity](#rendering-console-gdi-e-unity)
- [I Difetti che Rendono il Gioco Unico](#i-difetti-che-rendono-il-gioco-unico)
- [Problemi Noti](#problemi-noti)
- [Configurazione e Comandi](#configurazione-e-comandi)

---

## Cos'è X-Racing

X-Racing è un simulatore di guida da gara C++20 che separa netta la simulazione fisica dal rendering. Il cuore è un motore fisico a 120 Hz con modello gommario Pacejka, sospensioni 4 corner, aerodinamica completa e meteo dinamico. Il rendering può essere console testuale, Win3D GDI wireframe, o Unity 6000.x in 3D.

**Filosofia:** *simula → misura → testa → sistem documenta → ripeti*. Ogni componente è stato costruito, misurato con telemetria, testato, e solo allora documentato. Il risultato è un sistema che funziona — non perfettamente, ma in modo sorprendentemente soddisfacente.

---

## Come Si Gioca

### Avvio Rapido

```powershell
# Build
cmake -G "Visual Studio 17 2022" -A x64 -B build -DPROJECT0_BUILD_GAMEPLAY=ON
cmake --build build --config Release --parallel

# Gioca (console interattiva)
.\build\game\Release\project0_gameplay_exe.exe

# Guida automatica (AI al volante)
.\build\game\Release\auto_drive.exe

# Renderer GDI (finestra 2D/3D)
.\build\Release\project0_renderer.exe
```

### Controlli

| Tasto | Azione |
|-------|--------|
| W / ↑ | Acceleratore |
| S / ↓ | Freno |
| A / ← | Sterzo sinistro |
| D / → | Sterzo destro |
| Shift | Marcia superiore |
| Ctrl | Marcia inferiore |
| R | Reset posizione |
| ESC | Esci / Menu |
| M | (Renderer) Toggle 2D/3D wireframe |
| 1/2/3 | (Renderer) Cambia tracciato |
| C | (Unity) Telecamera chase/first-person |

---

## Fase di Gioco

### 1. Menu
Selezione tracciato (Default, Pit Circuit, Custom) e numero di giri (1, 3, 5, 10). Navigazione con tasti acceleratore/freno, conferma con Enter. I migliori tempi vengono salvati in `data/best_times.json` tra una sessione e l'altra.

### 2. Countdown
Secondi 3... 2... 1... GO! prima della partenza. ESC torna al menu. Se si tiene premuto l'acceleratore durante il countdown, la parte è immediata — un difetto che i giocatori più scaltri usano a proprio vantaggio.

### 3. Gara
Il cuore del gioco. HUD completo in console:
- Velocità (km/h), RPM, marcia
- Acceleratore/freno/sterzo percentuali
- Progresso giro, tempo giro, miglior giro
- Temperatura gomme (K) e usura (%)
- Aerodinamica (drag/downforce)
- Angoli di slip
- Avvisi off-track e collisioni

### 4. Risultati
Tempi per giro (validi/invalidi), miglior giro, tempo totale, giri validi. R per ripartire, M per tornare al menu.

---

## La Macchina

Il veicolo è definito da `VehicleParams` e il suo stato da `VehicleState`:

### Parametri Principali
- **Massa:** ~1500 kg
- **Passo:** 2.7 m
- **Motore:** curva di coppia realistica (picco gaussiano al 55% RPM), 6 marce, perdite trasmissione 12%
- **Aerodinamica:** drag, lift, downforce con ground effect, bilanciamento aloni anteriore/posteriore
- **Sospensioni:** molle-smortizzatori con barra anti-avvolgimento, trasferimento di carico
- **Gomme:** modello Pacejka con temperatura, usura e sensibilità al carico
- **Freni:** bilanciamento anteriore/posteriore, antibloccaggio

### Marce e Trasmissione
- Cambio automatico con isteresi: sale a 95% RPM max, scende a 160% RPM idle
- Manuale disponibile (Shift/Ctrl)
- Retrò sotto 1 m/s, limitata a 8 m/s, rilasciata automaticamente in accelerazione

---

## Fisica e Sensazione di Guida

### Ciclo di Simulazione (120 Hz = 4 sub-step per frame a 60 Hz)

| # | Stadio | Descrizione |
|---|--------|-------------|
| 1 | Motore | Curva di coppia → forza longitudinale |
| 2 | Aerodinamica | Drag, downforce, ground effect |
| 3 | Meteo | Raffreddamento pioggia, temperatura pista |
| 4 | Temperatura gomme | Modello termico + usura |
| 5 | Sospensioni | Trasferimento di carico 4 corner |
| 6 | Forze gomma | Pacejka laterale, ellipse grip |
| 7 | Frenata | Decelerazione, bilanciamento, lockup |
| 8 | Sterzata | Modello bicicletta, angoli di slip |
| 9 | Forze centripete | Dalla curvatura del tracciato |
| 10 | Integrazione | Euler semi-implicito |

### Cosa Si Sente al Volante

- **Sottosterzo in uscita di curva** quando si accelera troppo presto — le gomme posteriori perdono grip
- **Sovristerzo in inserimento** se si rilascia il freno troppo bruscamente
- ** Trasferimento di carico** si "sente" nel peso dello sterzo: la macchina si appiace sotto frenata, si alleggerisce in accelerazione
- **Degrado gomme** progressivo: dopo 5-6 giri intensi le temperature salgono e il grip cala
- **Pioggia improvvisa** riduce il grip del 35-40% — il tracciato diventa un lago
- **Gomme fredde** non abbracciano: il primo giro è sempre un riscaldamento

---

## I Tracciati

Tracciati parametrici chiusi generati da `TrackParams`, con interpolazione per distanza e ricerca binaria.

### Tracciati Disponibili

| Tracciato | Tipo | Caratteristiche |
|-----------|------|-----------------|
| **Default** | Ovalo | Con pit lane sul primo rettilineo |
| **PitCircuit** | Road course | Con pit lane e box |
| **CustomCircuit** | Road course personalizzato | Con direzione definita e box lane |

### Superfici

Ogni punto del tracciato ha un tipo di superficie:

| Superficie | Attrito | Effetto |
|------------|---------|---------|
| Asfalto | 1.00 | Grip nominale |
| Asfalto bagnato | 0.65 | -35% grip |
| Cordolo | 0.85 | Vibrazione, leggera perdita |
| Erba | 0.45 | Rallentamento, perdita grip |
| Ghiaia | 0.35 | Forte rallentamento |
| Sabbia | 0.25 | Impantanamento |
| Ghiaccio | 0.10 | Diversione totale |

---

## Gara e AI

### Sistema di Gara (`RaceManager`)

**Stati sessione:** PREGAME → FORMATION → GRID → GREEN_FLAG → GREEN_FLAG_RUNNING → SAFETY_CAR → CHECKERED_FLAG → POST_RACE

**Funzionalità:**
- Countdown configurabile (default 3s)
- Girata di formazione
- Conteggio giri con rilevamento direzione (attraversamento azzerra il giro)
- Limiti di pista: sistema 3 strike con penalità configurabile
- Rilevanto jump start
- Giri a vuoto con gap-to-leader, miglior giro, conteggio pit stop
- Sistema penalità: drive-through, stop-and-go, tempo, squalifica
- Bandiere: gialla, rossa, a scacchi

### Pit Stop

**FSM completo con 14 stati:** REQUESTED → APPROACHING_PIT_LANE → ENTERING_PIT_LANE → PIT_LANE_NAVIGATION → BOX_ASSIGNED → ALIGNING_BOX → STOPPED_AT_BOX → SERVICING → RELEASE_AUTHORIZED → EXITING_BOX → PIT_EXIT_NAVIGATION → TRACK_REENTRY → COMPLETE

**Unità di servizio (PSU):**
- Jack (sollevamento)
- Compressore (avvitamento bulloni)
- FuelDispenser (rifornimento)
- TireCambio (cambio gomme)

Ogni unità ha simulazione guasto/manutenzione — a volte il pit stop va storto.

### AI Avversari

**3 livelli di difficoltà:**

| Parametro | Easy | Medium | Hard |
|-----------|------|--------|------|
| Look-ahead | 25m | 35m | 45m |
| Guadagno sterzo | 0.7 | 0.9 | 1.0 |
| Acceleratore max | 85% | 95% | 100% |
| Ritardo reazione | 150ms | 50ms | 0ms |
| Errore percorso | 8% | 3% | 0% |
| Sorpassi | No | Sì | Aggressivo |

**Comportamenti AI:**
- Planning percorso su racing line precalcolata
- Consapevolezza del traffico: rileva macchine più lente, accumula urgenza sorpasso
- Difesa: blocca la racing line in curva se attaccata (Medium/Hard)
- Errori umani: jitter sterzo, varianza acceleratore, deviazione laterale
- Cambio marcia basato su RPM con soglie configurabili

---

## Meteo e Superfici

### Modello Meteo (`WeatherState`)

- **Pioggia** [0,1]: riduce grip 35-40%, aumenta resistenza rotolamento, raffredda gomme e pista
- **Vento:** influisce sulla velocità (placeholder — effetto minimo)
- **Temperatura pista:** evolve verso ambiente con riscaldamento solare
- **Grip factor:** combina tutti i fattori per il grip effettivo

### Dinamica Termica Gomme

- **Riscaldamento:** da slip e deformazione
- **Raffreddamento:** ambientale + pioggia
- **Finestra termica operativa:** troppo fredde = zero grip, troppo calde = usura rapida
- **Usura:** accumulato per giro, degrado progressivo prestazioni

---

## Telemetria

Registrazione 60 Hz per analisi e validazione.

### Formato CSV

```
frame,time,speed,rpm,gear,lap,lap_time,
position_x,position_y,position_z,
heading,lateral_g,longitudinal_g,
throttle,brake,steering,
tire_temp_fl,fr,rl,rr,
tire_wear_fl,fr,rl,rr,
slip_angle_fl,fr,rl,rr,
downforce,drag,track_temp,rain_intensity
```

### Utilizzo

```cpp
Telemetry telemetry("output.csv");
telemetry.record(frame, simulation_state, input_state, weather_state);
telemetry.save_csv();
```

I file vengono salvati in `data/telemetry/` per analisi con Python/Matplotlib.

---

## Rendering: Console, GDI e Unity

### Console (`game/`)
HUD testuale con barre di progresso ASCII. Leggero, sempre disponibile, telemetria completa.

### Renderer GDI (`renderer/`)
- Finestra Win32 1280×720 con double-buffer
- **Modo 2D:** vista dall'alto, tracciato con linee, pit lane in rosso, start/finish in oro, macchina come rettangolo ruotato, frecce direzione
- **Modo 3D wireframe (tasto M):** carica mesh OBJ/GLTF, proiezione prospettica con matrice look-at, telecamera chase con distanza/altezza/look-ahead regolabili
- HUD overlay: velocità, RPM, marcia, tempi giro
- Switch tracciato a runtime (tasti 1/2/3)

### Unity (`UnityProject/`)
**3 modalità di controllo:**
1. **Direct Control:** fisica arcade semplificata — accelerazione/freno lineare, sterzo dipendente dalla velocità, max 80 m/s
2. **SimPlugin:** ponte nativo vero simulatore C++ tramite `sim_plugin.dll`
3. **Telemetry Playback:** replay CSV con loop automatico

**Script Unity:**
- `CarController.cs` — controller veicolo + telecamera chase/first-person
- `CarHUD.cs` — HUD velocità/RPM/marcia/giro
- `SimPlugin.cs` — ponte P/Invoke verso DLL C++
- `XRRaceManager.cs` — porting C# del race manager
- `XRTrackGenerator.cs` — generazione mesh tracciato
- `TrackEditor.cs`, `TrackVisualizer.cs` — editing e visualizzazione

---

## I Difetti che Rendono il Gioco Unico

Queste "imperfezioni" non sono bug risolti — sono caratteristà che danno personalità al gioco:

### 1. Il Countdown Traditore
Se tieni premuto l'acceleratore durante il countdown, la gara parte istantaneamente. Niente anti-cheat: è un feature. I piloti scaltri partono con mezzo secondo di vantaggio.

### 2. Invalidazione Lap Tolleranza Zero
Un singolo frame off-track invalida tutto il giro. Nemmeno un attimo di sforatura. Questo trasforma ogni giro in una sfida di precisione estrema — e rende ogni giro valido incredibilmente soddisfacente.

### 3. Respawn Automatico dopo Collisione
Se rimani bloccato off-track per 2 secondi a velocità < 2 m/s, respawn automatico. Niente menu, niente conferma — sei rimandati sulla pista come se nulla fosse. Utile, ma a volte ti ritrovi nel posto sbagliato.

### 4. Lap Detection Inconsistente
Il sistema console usa `LapDetector`, il renderer usa `LapSystem`, Unity usa checkpoint trigger. Possono non essere d'accordo. È un po' come avere tre arbitri diversi — a volte contano cose diverse.

### 5. Il Menu Usa i Comandi di Guida
Navigazione menu con acceleratore/freno, partita con Enter. Ma durante il countdown stessi tasti controllano la macchina. Confuso? Sì. Una volta che l'hai imparato, è veloce.

### 6. Unity Due Fisiche, Nessuna Vera
Il controller Unity ha fisica arcade semplificata. La vera fisica C++ c'è ma richiede `sim_plugin.dll` compilato e deployato manualmente. Due mondi paralleli — uno accessibile, l'altro autentico.

### 7. Weather Senza Controllo
Il modello meteo esiste e funziona, ma nessuna UI per cambiarlo. La pioggia arriva quando vuole il codice — un elemento sorpresa che rende ogni giro potenzialmente diverso.

### 8. Pit Stop Senza Menu (Console)
Il FSM pit stop completo esiste nel motore C++, ma in console non c'è menu per richiederlo. Premi `enter_exit_box` e il sistema decide per te. A volte funziona, a volte ti ritrovi a fare il giro di pit lane a 60 km/h senza motivo.

### 9. Best Times JSON "Creativo"
Il salvataggio tempi usa parsing stringhe manuale. Se il JSON ha spazi extra o formattazione diversa, può silenziosamente fallire o dare risultati strani. Ma quando funziona, i tuoi record restano lì.

### 10. DX11 Renderer Fantasma
Esiste un renderer DirectX 11 con shader HLSL per mesh skinned, ma non è il percorso principale. È lì, funzionante, come una via secondaria che pochi conoscono.

---

## Problemi Noti

| Problema | Impatto | Soluzione |
|----------|---------|-----------|
| `Bee.DotNet.dll` bloccato da AppLocker | Unity non compila | `Get-ChildItem -Recurse "D:\Unity\Editors\6000.5.9f1\Editor\Data\Tools" -File \| Unblock-File` |
| Oggetti mancanti dopo avvio Unity | Manca HUD/tragliato | Ri-eseguire menu Project0 → Setup Scene |
| Wind effect placeholder | Vento nullo | Non ancora implementato |
| Mesh veicolo 2D in renderer GDI | Macchina = rettangolo | Mesh generation esiste ma non integrata |
| Nessun audio | Silenzio totale | Sistema audio non implementato |
| Nessuna intelligenza artificiale in console | Giocatore solo in console | AI esiste ma non collegata a Gameplay |

---

## Configurazione e Comandi

### CMake Build Options

```powershell
# Build completo
cmake -G "Visual Studio 17 2022" -A x64 -B build `
  -DPROJECT0_BUILD_TESTS=ON `
  -DPROJECT0_BUILD_GAMEPLAY=ON `
  -DPROJECT0_BUILD_RENDERER=ON

# Build + test
cmake --build build --config Release --parallel
ctest --output-on-failure -C Release
```

### Target CMake

| Target | Tipo | Descrizione |
|--------|------|-------------|
| `project0_engine` | Static Libreria | Motore simulazione |
| `sim_plugin` | DLL | Plugin Unity nativo |
| `generate_vehicle` | EXE | Generatore mesh veicolo |
| `project0_gameplay_exe` | EXE | Gameplay interattivo |
| `auto_drive` | EXE | Guida automatica |
| `gen_telemetry` | EXE | Generatore telemetria |
| `project0_tests` | EXE | Test unitari (33+) |
| `track_analysis` | EXE | Analisi geometria tracciato |
| `track_svg` | EXE | Export SVG tracciato |
| `track_diagram` | EXE | Export diagramma SVG (CLI) |
| `project0_renderer` | EXE | Renderer GDI |

### Esecuzione

```powershell
# Test unitari
.\build\tests\Release\project0_tests.exe

# Gameplay interattivo
.\build\game\Release\project0_gameplay_exe.exe

# Guida automatica (AI al volante, log su file)
.\build\game\Release\auto_drive.exe

# Telemetria per Unity
.\build\game\Release\gen_telemetry.exe

# Analisi tracciato
.\build\experiments\Release\track_analysis.exe

# Export SVG tracciato
.\build\experiments\Release\track_svg.exe -o track.svg -t pit

# Diagramma tracciato (CLI)
.\build\tools\Release\track_diagram.exe -o diagram.svg -t default --no-chart

# Renderer GDI
.\build\Release\project0_renderer.exe
```

---

## Struttura Dati di Gioco

### VehicleState (Runtime)

```cpp
struct VehicleState {
    Vec3 position;        // Posizione mondiale
    float heading;        // Direzione (radianti)
    float speed;          // Velocità (m/s)
    float rpm;            // Giri motore
    int gear;             // Marcia corrente
    float slip_angle[4];  // Angoli slip ruote (radianti)
    float tire_temp[4];   // Temperature gomme (Kelvin)
    float tire_wear[4];   // Usura gomme (0.0-1.0)
    float downforce;      // Downforce corrente
    float drag;           // Drag corrente
    int lap;              // Giro corrente
    float lap_time;       // Tempo giro (s)
    float best_lap_time;  // Miglior giro (s)
    bool in_pit_lane;     // In pit lane
    float lateral_g;      // Forza G laterale
    float longitudinal_g;  // Forza G longitudinale
};
```

---

## Riferimenti

- [Pacejka, Hans B. "Tyre Vehicle Dynamics"](https://www.amazon.com/Tyre-Vehicle-Dynamics-Hans-Pacejka/dp/074751520X) — Modello gommario Magic Formula
- [Milliken, William F. "Race Car Vehicle Dynamics"](https://www.amazon.com/Race-Car-Vehicle-Dynamics-Milliken/dp/1560915269) — Dinamica veicolo da gara
- [CMake Documentation](https://cmake.org/documentation/) — Build system
- [Unity Manual](https://docs.unity3d.com/Manual/) — Unity 6000.x
- [Eigen Library](https://eigen.tuxfamily.org/) — Algebra lineare

---

*X-Racing è un lavoro in progresso — un laboratorio dove la fisica incontra il divertimento. Non è perfetto, ed è esattamente per questo che è bello.*
