# X-Racing

C++20 racing simulation engine with Unity 6000.x integration.

See [docs/README.md](docs/README.md) for full documentation, build instructions, architecture overview, and physics pipeline details.

## Quick Build

```powershell
cmake -G "Visual Studio 17 2022" -A x64 -B build -DPROJECT0_BUILD_TESTS=ON -DPROJECT0_BUILD_GAMEPLAY=ON
cmake --build build --config Release --parallel
ctest --output-on-failure -C Release
```

## Quick Run

```powershell
.\build\game\Release\project0_gameplay_exe.exe   # Interactive driving
.\build\tests\Release\project0_tests.exe          # Unit tests
```
