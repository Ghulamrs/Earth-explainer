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

Each frame is an `AxisFrame`. **Frame1** is the main frame above; it rotates
and moves about its own current axes. **Frame2** is a 40% copy fixed to the
Earth's surface at −Z (Greenwich, facing the viewer): X north, Y east,
Z down. It rotates and moves rigidly with the Earth.

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
| hold X / Y / Z + ←/→ | rotate about the held axis; hold two or three for their combined axis |
| ←/→, ↑/↓, R/F | move ±X, ±Y, ±Z (when no X/Y/Z is held) |
| Tab | next Frame2 turn: about Frame1's X +74° and Y −34° (through the Earth's centre, so Frame2 travels over the surface), then its own Z +35° (animated, permanent) |
| L | Frame1's own axes (default) / world axes |
| Drag | free rotation |
| Scroll, pinch | zoom |
| Space | reset |

## Tests

```bash
cd Tests && make check
```
