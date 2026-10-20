# Earth Explainer

A macOS app (Objective-C++ + Metal) showing the Earth with a right-handed
body frame of three arrows joined at their tails at its centre. Press
**Enter** to turn the wireframe into a solid, shiny, textured Earth with
metal arrows. Earth and arrows move and rotate together as one body.

| Axis | Arrow | Meaning |
|---|---|---|
| X | red | Earth's rotation axis (North Pole) — up on screen |
| Y | green | equator at 90° E — to the right |
| Z | black | equator at 180° — mostly away from the viewer |

X × Y = Z. Greenwich (0°, 0°) faces the viewer at start.

All CPU maths uses only `CVector`, `CMatrix`, `CQuat` and `CRotrix` from
`~/Documents/Claude Overflow/C++ Vector Exercise`, referenced in place through
the `CVECTOR_LIB_DIR` build setting.

## Build

Open `ArrowsMetal.xcodeproj`, scheme **Arrows**, ⌘R.

For the real Earth, put an equirectangular image at
`ArrowsMetal/Resources/earth.jpg` (e.g. NASA Blue Marble); otherwise a
placeholder globe is shown.

## Controls

| Input | Action |
|---|---|
| Enter | wireframe ⇄ solid |
| Q/A, W/S, E/D | rotate ±X, ±Y, ±Z (hold several to combine) |
| ←/→, ↑/↓, R/F | move ±X, ±Y, ±Z |
| L | world / body axes |
| Drag | free rotation |
| Scroll, pinch | zoom |
| Space | reset |

## Tests

```bash
cd Tests && make check
```
