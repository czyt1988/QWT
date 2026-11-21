# Example toolbars and their icons

Qwt 7 beautified the default widget style (flat instead of the legacy embossed
look), but the example applications still used text-only tool buttons. This
page records the toolbar audit of all example programs and the flat SVG icons
that were added to their toolbar buttons.

The icons themselves, their design rules, palette and file naming are
documented in [`common/icons/README.md`](common/icons/README.md). All examples
share the same resource file, `common/icons/icons.qrc`, which is compiled into
an example by adding it to the target sources ( `AUTORCC` is on ):

```cmake
set(APP_ICON_RESOURCES "${CMAKE_CURRENT_LIST_DIR}/../../common/icons/icons.qrc")
add_executable(${QWT_APP_NAME} ... ${APP_ICON_RESOURCES})
```

The buttons then use `QIcon( ":/icons/<name>.svg" )`; no example links Qt SVG
explicitly because QIcon resolves the SVG through Qt's SVG image plugin at
runtime.

## 2D examples

"Toolbar" refers to a real `QToolBar` added to the main window. Controls next
to a toolbar (combo boxes, sliders, check boxes) are configuration widgets and
carry a label, so they are not iconified.

| Example | Toolbar | Toolbar buttons | Icons |
|---------|---------|-----------------|-------|
| `animation` | no | - | - |
| `barchart` | yes | Export | `export` |
| `bode` | yes | Zoom, Print, Export | `zoom`, `print`, `export` |
| `boxchart` | yes | Export | `export` |
| `colormap` | yes | Save Screenshots... | `camera` |
| `controls` | no | - | - |
| `cpuplot` | no | - | - |
| `curvedemo` | no | - | - |
| `curvesmoothing` | no | benchmark/undo buttons live in the side panel | - |
| `dials` | no | - | - |
| `distrowatch` | yes | Export, Screenshot | `export`, `camera` |
| `downsample_demo` | yes | save PNG | `save` |
| `figure` | yes | Save, Clear All, Resize, Grid layout (draggable) | `save`, `clear`, `resize`, `grid-layout` |
| `figureGrid` | yes | Save PNG..., Equal, Focus top, Golden, Undo, Redo, Log layout | `save`, `equal`, `focus-top`, `golden`, `undo`, `redo`, `log` |
| `friedberg` | yes | Export | `export` |
| `itemeditor` | yes | Export | `export` |
| `legends` | yes | Export | `export` |
| `nanperf` | no | action buttons live in the dock | - |
| `oscilloscope` | no | - | - |
| `outsideTitle` | yes | Y Titles Outside, X Titles Outside, Export PNG, Export SVG | `title-y-outside`, `title-x-outside`, `export-png`, `export-svg` |
| `parasitePlot` | yes | 18 actions (pickers, zoomer, panner, magnifier, axis wheel, title axis/position/alignment, fixed canvas, long labels) | already had a flat SVG set ( `icons/` of that example ), kept as the reference |
| `polardemo` | yes | Zoom, Print, Export | `zoom`, `print`, `export` |
| `polarspectrogram` | yes | Export, Grid, Rotate, Mirror | `export`, `grid`, `rotate`, `mirror` |
| `pyplot` | no | - | - |
| `radio` | no | - | - |
| `rasterview` | yes | Export | `export` |
| `realtime` | yes | Start/Stop, Clear, Help | `play`/`pause` (swapped with the running state), `clear`, `help` |
| `refreshtest` | no | - | - |
| `scatterplot` | no | - | - |
| `simpleplot` | no | - | - |
| `sinusplot` | no | - | - |
| `spectrogram` | yes | Print | `print` |
| `splineeditor` | yes | Print, Overlay, Closed | `print`, `overlay`, `closed` |
| `stockchart` | yes | Export | `export` |
| `stylesheets` | no | CSS-only demo, not a CMake target | - |
| `sysinfo` | no | - | - |
| `ticks_inside` | no | two panel buttons (All Inside / All Outside) | - |
| `tvplot` | yes | Export | `export` |

`bode`, `polardemo` and `realtime` used the old XPM bitmaps of the original Qwt
examples; those bitmaps (and their `Pixmaps.h` / `*.xpm` files) were replaced
by the shared SVG icons.

## 3D examples

The 3D toolbars are wider, so they keep `Qt::ToolButtonIconOnly` with the
action text as tooltip / accessible name; the 2D toolbars show the icon
together with the label.

| Example | Toolbar | Toolbar buttons | Icons |
|---------|---------|-----------------|-------|
| `bar3D` | yes | Coord Style (menu), Lighting, Color Legend, Reset View | `coord-style`, `lighting`, `legend`, `reset-view` |
| `figureSurface3D` | yes | Animate Surface, Auto Rotate, Plot Style (menu), Floor Style (menu), Coord Style (menu), Lighting, Color Legend, Orthographic, Reset View, Save Figure | `play`/`pause`, `auto-rotate`, `plot-style`, `floor-style`, `coord-style`, `lighting`, `legend`, `ortho`, `reset-view`, `save` |
| `line3D` | yes | Coord Style (menu), Lighting, Color Legend, Reset View | `coord-style`, `lighting`, `legend`, `reset-view` |
| `qwtplot3d` | yes | Animate, Box, Frame, None, Lighting, Legend, Ortho, Grid, Reset View, Save... | `play`/`pause`, `coord-box`, `coord-frame`, `coord-none`, `lighting`, `legend`, `ortho`, `grid`, `reset-view`, `save` |
| `waterfall3D` | yes | Pause, Reset, Reset view, Screenshot | `pause`/`play`, `reset-data`, `reset-view`, `camera` |

The animation/run toggles swap their icon with the checked state
( `play.svg` when stopped, `pause.svg` when running ).

## Other example programs

| Target | Toolbar | Note |
|--------|---------|------|
| `staticExample` | no | Save / Clear / Replot are plain buttons in a side panel, outside this change |
| `qmakeExample` | no | single-file qmake integration demo without a toolbar |
| `bench/renderbench` | no | benchmark control panel only |
| `bench/chartbench` | - | embeds the third-party QIM application, whose toolbar is not part of the Qwt examples |

## Regenerating the documentation screenshots

The screenshots of the examples in `docs/assets/screenshots{,-3D}` are
generated from the examples themselves, see
[`SCREENSHOTS.md`](SCREENSHOTS.md). The toolbars now contain icons, so the
affected entries were regenerated with the same script:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File examples\update-screenshots.ps1 -BuildDir build
```

Note that a toolbar with `Qt::ToolButtonTextUnderIcon` is slightly taller than
a text-only one, so a few of the regenerated captures are taller than before
( the `--size` argument is a request; the window's minimum size hint wins ).
