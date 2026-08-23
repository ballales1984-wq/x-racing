# Project 0

Automotive simulation laboratory. Build, measure, test, correct, document, repeat.

## Current Milestone

A mathematical car that travels a parametric track whose state can be observed and verified.

### Input
- Throttle
- Brake
- Steering

### State
- Position
- Velocity
- Acceleration
- Heading
- Slip ratio / slip angle

### Output
- New position
- New velocity
- Telemetry

## Building

```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

## Running Tests

```bash
.\build\tests\Release\project0_tests.exe
```

## Running Experiments

```bash
.\build\experiments\Release\track_analysis.exe
```

## Project Structure

```
engine/
  physics/     - Math utilities and tire models
  vehicle/     - Vehicle parameters and state
  track/       - Parametric track model
  input/       - Input state definitions
  simulation/  - Core simulation loop
  telemetry/   - Telemetry recording and CSV export
tests/         - GoogleTest unit tests
experiments/   - Analysis tools and scripts
renderer/      - Visualization (future)
vendor/        - Third-party dependencies (Eigen, GoogleTest)
```
