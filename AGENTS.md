# X-Racing

C++20 racing simulation with CMake build system, Google Test unit tests, and optional Unity integration.

## Build

- Configure: `cmake -G "Visual Studio 17 2022" -A x64 -B build`
- Build: `cmake --build build --config Release --parallel`
- Test: `ctest --output-on-failure -C Release`

CMake options:
- `PROJECT0_BUILD_TESTS=ON` (default ON)
- `PROJECT0_BUILD_RENDERER=OFF` (default OFF)
- `PROJECT0_BUILD_EXPERIMENTS=ON` (default ON)
- `PROJECT0_BUILD_GAMEPLAY=ON` (default ON)

## Code Style

- C++20 standard
- Header files: `.h`, implementation: `.cpp`
- Tests use Google Test framework
- Track data in `data/` directory
- Keep build artifacts out of version control (`build/`, `build2/`, `build3/`)

## Worktree Guidelines

- Do not use `git stash` or autostash; worktree merge flows should resolve conflicts in the worktree
- Run setup before running build commands in a new worktree
- Avoid modifying shared global resources from multiple worktrees simultaneously
