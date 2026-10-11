# Scientific Colormap Presets

`QwtColorMapPreset` (in `qwt::core`) ships 22 well-known scientific colormaps — perceptually uniform ramps (viridis, plasma…), MATLAB classics (jet, hot, cool…), rainbow ramps (rainbow, hsv, turbo) and diverging ramps (coolwarm, rdylbu…). The names match matplotlib / MATLAB, so existing palettes can be swapped in place without touching the stop definitions. 2D spectrograms, 3D surfaces and axis color bars share the same presets, and the color bar always uses the very same map as the plot itself.

## Key Features

- ✅ **22 presets**: matplotlib / MATLAB compatible names, created by name without any stop definitions
- ✅ **Perceptually uniform**: viridis, plasma, inferno, magma, cividis vary monotonically in lightness — colorblind friendly and print safe
- ✅ **Diverging ramps**: coolwarm, rdylbu, rdylgn, spectral for signed data around a center value
- ✅ **Reusable everywhere**: 2D spectrograms, 3D surfaces, axis color bars and plain `rgb()` sampling share one factory
- ✅ **Discoverable at runtime**: list all preset names, inspect color stops, convert name ↔ enum
- ✅ **Example included**: `examples/2D/colormap` browses all presets interactively and exports them via `--screenshot`

## Basic Usage

### 1. Create a colormap by name

```cpp
#include <qwt_colormap.h>
#include <qwt_colormap_preset.h>

// Names are case insensitive
std::unique_ptr< QwtLinearColorMap > cmap = QwtColorMapPreset::create("viridis");
```

`create()` returns a `QwtLinearColorMap`, whose base class `QwtColorMap` provides `rgb(vMin, vMax, value)` for sampling colors. All preset names are available at runtime:

```cpp
for (const QString& name : QwtColorMapPreset::availablePresets())
    qInfo() << name;   // viridis plasma inferno magma cividis jet hot cool
                       // spring summer autumn winter gray bone copper rainbow
                       // hsv turbo coolwarm rdylbu rdylgn spectral
```

### 2. Use it in a 2D spectrogram

The spectrogram image and the color bar of the axis each own their color map instance:

```cpp
QwtPlotSpectrogram* spectrogram = new QwtPlotSpectrogram();
spectrogram->setColorMap(QwtColorMapPreset::create("plasma").release());
spectrogram->attach(plot);

// The right axis acts as a color bar, painted with the same color map
QwtScaleWidget* rightAxis = plot->axisWidget(QwtAxis::YRight);
rightAxis->setTitle("Intensity");
rightAxis->setColorBarEnabled(true);
rightAxis->setColorMap(dataInterval, QwtColorMapPreset::create("plasma").release());
plot->setAxisScale(QwtAxis::YRight, dataInterval.minValue(), dataInterval.maxValue());
plot->setAxisVisible(QwtAxis::YRight);
```

Before QWT7.3 the sampling API was `QwtColorMap::rgb(interval, value)`; it has been unified to `rgb(double vMin, double vMax, double value)` since QWT7.3.1.

### 3. Sample colors without plotting

```cpp
const std::unique_ptr< QwtLinearColorMap > cmap = QwtColorMapPreset::create("inferno");
const QRgb color = cmap->rgb(0.0, 100.0, 42.0);   // the color of the value 42 in [0, 100]
```

### 4. Use it on a 3D surface

The 3D module bridges the core colormaps through `Qwt3DColorMapColor`, so all 22 presets can be applied to surfaces:

```cpp
Qwt3DTheme theme(Qwt3DTheme::Scientific);
theme.setDataColorPreset("plasma");   // internally created via QwtColorMapPreset
theme.apply(plot);
```

### 5. Introspection

```cpp
// Name and enum conversion
QwtColorMapPreset::Preset preset = QwtColorMapPreset::Viridis;
QString name = QwtColorMapPreset::presetName(preset);          // "viridis"
QwtColorMapPreset::Preset from = QwtColorMapPreset::presetFromName("TURBO");

// The color stops (position, color), useful for painting your own color bar
QVector< QPair< double, QColor > > stops = QwtColorMapPreset::colorStops(QwtColorMapPreset::Jet);
```

## Running the Example

The example lives in `examples/2D/colormap` and looks like this:

![colormap example](../assets/screenshots/colormap.png)

What the window offers:

- **Preset list (left)**: all 22 presets, each item shows a gradient strip of its colors; click to switch
- **Backdrop**: switches between a light and a dark plot area
- **Color Table**: color table size (Smooth / 256 / 1024 / 16384) to inspect banding artifacts
- **Contours**: overlays contour lines of the demonstration field
- **Save Screenshots...**: exports all renders into a chosen directory

All renders show the classic MATLAB `peaks` demonstration field (a sharp peak next to a deep well on concentric rings, normalized to [0, 1]) with a color bar on the right axis.

### Command line option: --screenshot

The example accepts a `--screenshot <directory>` option that renders every preset without any GUI, and exits with 0 on success:

```bash
./colormap --screenshot ./colormap-shots
```

What is written:

| File | Description |
|------|-------------|
| `<preset>.png` × 22 | one render per preset (1280×800, field + color bar + axes) |
| `overview.png` | a 2×11 grid of all gradients |
| `colormap.png` | a screenshot of the window itself |

Screenshot mode renders through the same window system as the GUI, so the text in the exported images (titles, tick labels, color bar labels) matches what is seen in the window. Without a display (headless Linux CI) it falls back to the `offscreen` platform plugin — the images are still written, but every character degenerates to a box. Setting the `QT_QPA_PLATFORM` environment variable overrides this choice. The `Save Screenshots...` toolbar button does the same from within the GUI.

## All 22 Presets at a Glance

Below are the gradients of all 22 presets (sampled from `create("name")`, ordered like `availablePresets()`):

![overview of all 22 colormap presets](../assets/screenshots/colormap/overview.png)

| Name | Category | Characteristics | Typical use |
|------|----------|-----------------|-------------|
| `viridis` | Sequential · uniform | purple-green-yellow, matplotlib default | first choice, papers |
| `plasma` | Sequential · uniform | purple-pink-yellow | emphasize high values |
| `inferno` | Sequential · uniform | black-purple-orange-yellow | dark backgrounds, glow |
| `magma` | Sequential · uniform | black-purple-pink-white | astronomy, dark data |
| `cividis` | Sequential · uniform | blue-gray-yellow | color vision deficiency, grayscale print |
| `jet` | Sequential · MATLAB rainbow | blue-cyan-yellow-red | legacy compatibility only |
| `hot` | Sequential · thermal | black-red-yellow-white | temperature fields |
| `cool` | Sequential | cyan-magenta | math demos |
| `spring` | Sequential | magenta-yellow | math demos |
| `summer` | Sequential | greens | math demos |
| `autumn` | Sequential | red-yellow | math demos |
| `winter` | Sequential | blue-green | math demos |
| `gray` | Sequential · no hue | black-white | printing, grayscale docs |
| `bone` | Sequential · no hue | black-gray-white with blue tint | medical imagery |
| `copper` | Sequential · single hue | black-copper | geology, metallic look |
| `rainbow` | Cyclic | full hue circle | periodic quantities (phase, angle) |
| `hsv` | Cyclic | full HSV hue circle | data whose min equals its max |
| `turbo` | Sequential · rainbow, improved | Google turbo, more uniform than jet | rainbow look without the artifacts |
| `coolwarm` | Diverging | blue-white-red | signed difference fields (anomalies, errors) |
| `rdylbu` | Diverging | red-yellow-blue (ColorBrewer) | anomalies, elevation fills |
| `rdylgn` | Diverging | red-yellow-green (ColorBrewer) | signed ratios |
| `spectral` | Diverging · multi hue | red-yellow-green-blue | distinct qualitative bands |

The comparison renders below were exported with `--screenshot`. Every image shows the same data field with a different colormap, so they can be compared side by side.

### Perceptually uniform

`viridis`, `plasma`, `inferno`, `magma` and `cividis` keep a monotonic perceptual lightness: once converted to grayscale they still read as a smooth ramp, so no artificial structures appear. They are the safe first choice for scientific figures.

| viridis | plasma |
|---------|--------|
| ![viridis](../assets/screenshots/colormap/viridis.png) | ![plasma](../assets/screenshots/colormap/plasma.png) |

| inferno | magma |
|---------|-------|
| ![inferno](../assets/screenshots/colormap/inferno.png) | ![magma](../assets/screenshots/colormap/magma.png) |

| cividis |
|---------|
| ![cividis](../assets/screenshots/colormap/cividis.png) |

### MATLAB classics and single hue ramps

`jet` is the best-known MATLAB default. Note the artificial contour line where the cyan-yellow transition crosses, and that its lightness is no longer monotonic after a grayscale conversion — the reason new code should prefer turbo or viridis. `hot`, `cool`, `spring`, `summer`, `autumn`, `winter`, `gray`, `bone` and `copper` are the traditional single/dual hue ramps, valued for their simplicity.

| jet | hot | gray |
|-----|-----|------|
| ![jet](../assets/screenshots/colormap/jet.png) | ![hot](../assets/screenshots/colormap/hot.png) | ![gray](../assets/screenshots/colormap/gray.png) |

| cool | bone | copper |
|------|------|--------|
| ![cool](../assets/screenshots/colormap/cool.png) | ![bone](../assets/screenshots/colormap/bone.png) | ![copper](../assets/screenshots/colormap/copper.png) |

| spring | summer | autumn | winter |
|--------|--------|--------|--------|
| ![spring](../assets/screenshots/colormap/spring.png) | ![summer](../assets/screenshots/colormap/summer.png) | ![autumn](../assets/screenshots/colormap/autumn.png) | ![winter](../assets/screenshots/colormap/winter.png) |

### Rainbow family

`rainbow` and `hsv` wrap around to their first color, which makes them suitable for periodic data. `turbo` is Google's balanced rainbow: far more perceptually linear than `jet` while keeping the rainbow look.

| rainbow | hsv | turbo |
|---------|-----|-------|
| ![rainbow](../assets/screenshots/colormap/rainbow.png) | ![hsv](../assets/screenshots/colormap/hsv.png) | ![turbo](../assets/screenshots/colormap/turbo.png) |

### Diverging

Whenever the data has a meaningful center (a zero value or a reference), a two-sided ramp with a light center encodes the direction of the deviation with the hue:

| coolwarm | rdylbu | rdylgn | spectral |
|---------|--------|--------|----------|
| ![coolwarm](../assets/screenshots/colormap/coolwarm.png) | ![rdylbu](../assets/screenshots/colormap/rdylbu.png) | ![rdylgn](../assets/screenshots/colormap/rdylgn.png) | ![spectral](../assets/screenshots/colormap/spectral.png) |

## Choosing a Ramp

- **Sequential data**: use a perceptually uniform ramp — `viridis` by default, `plasma` for a warmer gradient, `inferno` / `magma` on dark backdrops
- **Signed or deviating data**: use a diverging ramp — `coolwarm` for errors, `rdylbu` for anomalies
- **Periodic data**: use `rainbow` / `hsv`, whose first and last colors match; scale the data to exactly one period, otherwise a false boundary appears where red meets red
- **Avoid `jet` / `rainbow` for sequential data**: their lightness is not monotonic, which invents contour lines that are not in the data — especially after grayscale printing
- **Printing and grayscale**: `cividis` or `gray` keep the ordering of the values readable
- **Color vision deficiency**: avoid encoding the magnitude in red-vs-green alone; `viridis`, `cividis` and `coolwarm` are safe options

!!! tip "Quick self check"
    Save your figure as grayscale (or switch the preset to `gray` in the example): if the lightness no longer follows the data, the colormap is misleading the reader.

## Notes

- `setColorMap()` takes ownership: the `QwtColorMap*` is owned by the item / scale widget, so hand over the `unique_ptr` with `release()`. Do **not** set the same pointer on the spectrogram and the color bar at once — the example calls `create()` for both
- The sampling signature is `rgb(double vMin, double vMax, double value)` (unified since QWT7.3.1); the earlier `rgb(const QwtInterval&, double)` has been removed
- Presets are `QwtLinearColorMap`s with `RGB` format (continuous interpolation). For maximum rendering speed a quantized color table (e.g. `setColorTableSize(256)`) can be used, at the price of visible banding — directly observable with the `Color Table` combobox of the example

## Related Classes and Source Files

| Class / file | Description |
|--------------|-------------|
| `QwtColorMapPreset` (`src/core/qwt_colormap_preset.h`) | factory and introspection for the 22 presets |
| `QwtLinearColorMap` (`src/core/qwt_colormap.h`) | linear gradient colormap |
| `QwtPlotSpectrogram` (`src/plot/qwt_plot_spectrogram.h`) | 2D spectrogram / heatmap |
| `QwtScaleWidget::setColorMap()` (`src/plot/qwt_scale_widget.h`) | axis color bar |
| `Qwt3DColorMapColor` (`src/plot3d/qwt3d_colormap_color.h`) | bridges core colormaps to 3D |
| `Qwt3DTheme::setDataColorPreset()` (`src/plot3d/qwt3d_theme.h`) | preset selection inside 3D themes |

!!! example "Related Examples"
    - Colormap preset gallery: `examples/2D/colormap` (with `--screenshot` export)
    - Spectrogram: `examples/2D/spectrogram`
    - Polar spectrogram: `examples/2D/polarspectrogram`
    - 3D surface: `examples/3D/figureSurface3D`
