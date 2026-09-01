# X-Racing — Placeholder Registry

This file tracks all known placeholders, TODOs, and incomplete implementations in the codebase.

## Legend
- `[x]` — Completed
- `[~]` — Partially implemented
- `[ ]` — Still a placeholder

---

## Simulation (`engine/simulation/`)

| Location | Description | Status |
|----------|-------------|--------|
| `simulation.h:19` | `use_abs` flag (placeholder) | `[x]` Implemented |
| `simulation.h:20` | `use_tcs` flag (placeholder) | `[x]` Implemented |
| `simulation.h:26` | `collision` field (placeholder comment) | `[x]` Implemented as off-track detection |
| `simulation.cpp:325` | Wind effect in `update_weather()` | `[x]` Implemented |
| `simulation.cpp:517` | Off-track grip reduction | `[x]` Implemented |
| `simulation.cpp:782` | Barrier push-back physics | `[x]` Implemented |

---

## Vehicle (`engine/vehicle/`)

| Location | Description | Status |
|----------|-------------|--------|
| `vehicle_generator.cpp:311` | Smooth normals calculation | `[x]` Implemented per-face normals |
| `vehicle.h:74-76` | Wind parameters (`wind_speed`, `wind_direction`, `wind_effect_on_speed`) | `[x]` Implemented |
| `vehicle.h:113` | `tire_compound` field | `[x]` Implemented |
| `vehicle.h:124-128` | Fuel fields (`current_fuel_l`, `fuel_capacity_l`, etc.) | `[x]` Implemented |

---

## Track (`engine/track/`)

| Location | Description | Status |
|----------|-------------|--------|
| `track.h:120` | Mesh-based collider | `[x]` Implemented |
| `track.h:45` | `banking` field (existed but unused) | `[x]` Used in physics |
| `track.h:50-51` | Box lane fields | `[x]` Fully implemented |

---

## Debug (`engine/debug/`)

| Location | Description | Status |
|----------|-------------|--------|
| `debug_console.cpp:417` | Save/load debug state | `[x]` Implemented JSON load |
| `debug_agent.h:109` | `load_snapshot_json` declaration | `[x]` Added |

---

## Audio (`engine/audio/`)

| Location | Description | Status |
|----------|-------------|--------|
| `audio_engine.h` | Full audio engine interface | `[x]` Implemented |
| `windows_audio_output.h/.cpp` | Windows audio backend | `[x]` Implemented |

---

## Rendering

| Location | Description | Status |
|----------|-------------|--------|
| `renderer/` | DX11 renderer | `[ ]` Phantom/implicit only |

---

## Remaining Placeholders

1. **Damage model** — No health/damage system for vehicle components
2. **DRS / slipstream** — Not implemented
3. **Track edge detection for procedural env** — Not implemented
4. **Generation zone** — Not implemented
5. **Trees, rocks, grass** — Not implemented
6. **Barriers/fences for procedural env** — Not implemented
7. **Sky/atmosphere** — Not implemented
8. **Meshy assets** — Not implemented
9. **Lighting (HDR, GI)** — Not implemented
10. **Materials (PBR)** — Not implemented
11. **Particles** — Not implemented
12. **Effects (motion blur, DOF)** — Not implemented
13. **Advanced UI** — Not implemented
14. **Optimization (LOD, instancing)** — Not implemented
15. **ML experiments** — Not implemented
16. **AI pit strategy** — Basic pit exists, advanced strategy not implemented
17. **AI wet weather adaptation** — Not implemented
18. **Integrated profiler** — Not implemented
19. **Telemetry batch validation tool** — Not implemented
20. **Unity plugin API docs for third parties** — Not documented
