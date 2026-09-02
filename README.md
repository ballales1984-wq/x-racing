# X-Racing

C++20 racing simulation engine with Unity 6000.x integration.

## Documentazione

- [📖 **GAME.md**](docs/GAME.md) — Documentazione del gioco: come si gioca, fisica, tracciati, AI, pit stop, e i difetti che lo rendono unico
- [🔧 **README.md**](docs/README.md) — Architettura, build, pipeline fisica, telemetria
- [🗺️ **ROADMAP.md**](docs/ROADMAP.md) — Stato sviluppo e piani futuri

## Quick Build

```powershell
cmake -G "Visual Studio 17 2022" -A x64 -B build -DPROJECT0_BUILD_TESTS=ON -DPROJECT0_BUILD_GAMEPLAY=ON
cmake --build build --config Release --parallel
ctest --output-on-failure -C Release
```

## Quick Run

```powershell
.\build\game\Release\project0_gameplay_exe.exe   # Interactive driving
.\build\game\Release\auto_drive.exe               # AI driving
.\build\Release\project0_renderer.exe             # 2D/3D renderer
.\build\tests\Release\project0_tests.exe          # Unit tests
```
