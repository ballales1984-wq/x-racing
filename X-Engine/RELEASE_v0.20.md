# X-Engine v0.20.0 — Release Notes

**Tag:** `v0.20.0`
**Date:** 2026-09-03
**Commit:** `444d781`

## What's in the box

A self-contained DirectX 12 + Win32 3D engine with an integrated 3D physics
sandbox.  One binary, one console, one set of cubes you can drag around.

![v0.20.0](https://placehold.co/640x360?text=X-Engine+v0.20.0) *(screenshot
goes here when the demo runs in CI)*

## Try it

```cmd
cmake -G "Visual Studio 17 2022" -A x64 -B build
cmake --build build --config Release --parallel
build\Release\xengine_runtime.exe
```

Then in the console (press `` ` `` to open):

```
physics on
gravity on
demo
```

You'll get a ground, two stacked boxes, a 20-bead rope, and a trigger sphere.
Press `F1` to see the trigger outline.  Click and drag a body with `LMB`,
right-click drag to spin, `MMB` to spawn a new sphere.

## Numbers

| Metric              | Value         |
|---------------------|---------------|
| Test cases          | 204 (all pass) |
| Lines of C++        | ~5,500        |
| Public headers      | 18            |
| Console commands    | 40+           |
| Versioned releases  | 20            |
| External libs       | 0 (DX12/Win32 only) |
| Build time (Release)| ~30 s         |
| Output binary       | 1 EXE + 1 DLL  |

## What you can build

The included `data/demo.xescript` is a working example.  You can also build
your own scenes:

```
spawn ground  0 -2 0 10 0.25 10
spawn box     0  0 0  0.5  0.5  0.5
spawn box     0  1 0  0.5  0.5  0.5
link 0 1
pin 1 2
spawn rope    0  6 0  20  0.25  0.1
```

Save with `save scene.scn`, then `load scene.scn` later.  Or put the above
into a `.xescript` file and `run my_scene.xescript`.

## Known issues

- No GPU line rendering; debug viz is GDI (CPU).
- No shadows.
- Single-threaded.
- `O(n²)` AABB broadphase (fine for ≤100 bodies).
- No continuous collision; very fast bodies may tunnel.

## What comes next

Candidates for V0.21+:
- Vehicle physics (chassis + 4 wheels + suspension)
- Soft body / cloth
- Particle systems
- GPU line rendering (replace GDI overlay)
- Multi-threading
- Shadow mapping

See `docs/ROADMAP.md` for the long-term plan.
