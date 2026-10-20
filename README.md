# Arrows — wireframe ⇄ metal triad on Metal

A macOS app (Objective-C++ + Metal) that draws three arrows whose tails meet
at the origin — red +X, green +Y, blue +Z — first as a wireframe, then, on
**Enter**, as solid shiny metal arrows of the same shape and colour.

All CPU-side maths uses **only** the `CVector` / `CMatrix` / `CQuat` /
`CRotrix` headers from
`~/Documents/Claude Overflow/C++ Vector Exercise` — no `simd`, no GLKit, no
other matrix library. Xcode reads those headers in place through the
`CVECTOR_LIB_DIR` build setting (change it there if the library moves); they
appear in the navigator under *CVector Library*.

## Build & run

Open `ArrowsMetal.xcodeproj` in Xcode, pick the **Arrows** scheme, ⌘R.
Signing is ad-hoc ("Sign to Run Locally"), so no team is needed.

## Controls

| Input | Action |
|---|---|
| Enter | wireframe ⇄ solid metal (0.6 s cross-fade) |
| Q / A, W / S, E / D | rotate ±X, ±Y, ±Z — hold several to rotate about the combined axis |
| ← / →, ↑ / ↓, R / F | move ±X, ±Y, ±Z |
| L | toggle world axes / the arrows' own (local) axes |
| Mouse drag | free rotation about any axis in the screen plane |
| Scroll / pinch | zoom |
| Space | reset |

## Where the library is used

| Job | Library call |
|---|---|
| Orientation state | `CQuat`; world rotation `dq * q`, local `q * dq`, renormalised |
| Any / combined axis | held keys build a `CVector<3>` axis → `CQuat::fromAxisAngle` |
| Model matrix `[R|t]` | `CRotrix(q)` rows, each `resized<4>(t_i)` into a `CMatrix<4,4>` |
| View matrix | `lookAt` from `cross`, `dot`, `normalized`, `resized<4>` |
| Projection | `CMatrix<4,4>` perspective, Metal depth range 0…1 |
| MVP | `CMatrix<4,4> * CMatrix<4,4>` once per frame |
| Arrow geometry | `CMatrix<2,2>::rotation` for the circle, `CRotrix::between(+Z, axis)` to aim each arrow |
| Mouse trackball | axis = `right * -dy + up * dx` in camera space |

## Efficiency choices

- Geometry is built once and uploaded once (675 vertices, 16-bit indices).
- Colour is baked per vertex, so the whole triad is **one draw call** per mode.
- Per frame the CPU does a single 4×4 product; uniforms (176 bytes) go by
  `setVertexBytes`, so no buffer ring or CPU/GPU sync.
- View × projection is cached and rebuilt only on zoom or resize.
- The view is **paused when nothing moves**: the display link runs only while
  a key is held or the cross-fade is running; drags and scrolls draw single frames.
- Release builds use `-O3`, LTO and Metal fast-math.

## Layout

```
ArrowsMetal/            app: main, AppDelegate, ArrowView (input), Renderer (Metal)
ArrowsMetal/Shaders.metal
ArrowsMetal/ShaderTypes.h   the CPU/GPU boundary — plain floats only
ArrowsMetal/Core/       pure C++: Scene, RigidTransform, Projection, ArrowMesh
Tests/                  core_tests.cpp + Makefile (no Cocoa/Metal needed)
```

## Tests

```bash
cd Tests && make check
```

Checks the model matrix against `CQuat::rotate`, no drift after 200 000
rotations, local ≡ world-about-`R·a`, combined X+Y axis, the projection's
depth range, `lookAt`, the column-major hand-off, and the mesh (tips on the
axes, unit normals, every triangle facing outward).
