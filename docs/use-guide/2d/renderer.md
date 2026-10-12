# Plot Export - QwtPlotRenderer

`QwtPlotRenderer` provides functionality to export plots to various file formats, supporting image files (PNG, JPG, etc.), PDF, SVG vector formats, and print output.

## Main Features

**Features**

- ✅ **Multiple export formats**: Supports PNG, JPG, BMP, PDF, SVG, PS, etc.
- ✅ **Resolution control**: Configurable DPI resolution for exported images
- ✅ **Size settings**: Customizable export dimensions or millimeter units
- ✅ **Layout options**: Option to ignore titles, legends, and other components during export

## Usage

### 1. Export to Image File

All file exports go through `QwtPlotRenderer::renderDocument()`: you pass the plot, the target filename (whose extension selects the format), the output size **in millimeters** (`QSizeF`), and the resolution in DPI (default 85). For image formats the pixel dimensions are derived as `mm / 25.4 * DPI`. The example below writes the same plot as PNG, JPG, and BMP:

```cpp
#include <QwtPlotRenderer>

QwtPlotRenderer renderer;

// Export to PNG: 200 x 150 mm at 300 DPI (about 2362 x 1772 pixels)
renderer.renderDocument(plot, "plot.png", QSizeF(200, 150), 300);

// Export to JPG: same physical size at 150 DPI (about 1181 x 886 pixels)
renderer.renderDocument(plot, "plot.jpg", QSizeF(200, 150), 150);

// Export to BMP with the default resolution of 85 DPI
renderer.renderDocument(plot, "plot.bmp", QSizeF(200, 150));

// File format is automatically determined by extension
```

### 2. Export to PDF

PDF output uses the same `renderDocument()` entry point — just give the file a `.pdf` extension. The result is a vector document that stays sharp at any zoom level, ideal for printing and document embedding:

```cpp
// Export to PDF
renderer.renderDocument(plot, "plot.pdf", QSizeF(200, 150), 300);  // 200x150 mm, 300 DPI

// PDF supports vector graphics, suitable for high-resolution printing
```

### 3. Export to SVG

With a `.svg` filename, `renderDocument()` produces a scalable vector graphic (Qwt must be built with SVG support, `QWT_CONFIG_QWTSVG`). SVG output remains editable and can be embedded directly into web pages:

```cpp
// Export to SVG vector format
renderer.renderDocument(plot, "plot.svg", QSizeF(200, 150));

// SVG advantages:
// - Vector graphics, lossless scaling
// - Smaller file size
// - Can be viewed in browsers
```

### 4. Export to QImage/QPixmap

Instead of writing a file, the renderer can also draw into an in-memory paint device: the `renderTo(QwtPlot*, QPaintDevice&)` overload accepts any `QImage` or `QPixmap` that you create yourself at the desired pixel size, which is handy for embedding the result in an application or post-processing it:

```cpp
// Export as QImage: create the image, then render into it
QSize size(800, 600);
QImage image(size, QImage::Format_ARGB32);
image.fill(Qt::white);
renderer.renderTo(plot, image);
image.save("plot.png");  // optional: write it to a file

// Export as QPixmap
QPixmap pixmap(size);
pixmap.fill(Qt::white);
renderer.renderTo(plot, pixmap);

// Can be used for in-application display or further processing
```

### 5. Export Options

The `DiscardFlag` enum strips selected components from the output: `DiscardBackground`, `DiscardTitle`, `DiscardLegend`, `DiscardCanvasBackground`, `DiscardFooter`, and `DiscardCanvasFrame`. Single flags are toggled with `setDiscardFlag()`, while `setDiscardFlags()` assigns the whole set at once:

```cpp
// Set export layout options
renderer.setDiscardFlag(QwtPlotRenderer::DiscardTitle, true);     // Ignore title
renderer.setDiscardFlag(QwtPlotRenderer::DiscardLegend, true);    // Ignore legend
renderer.setDiscardFlag(QwtPlotRenderer::DiscardFooter, true);    // Ignore footer
renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasBackground, true);  // Ignore canvas background

// Set frame options
renderer.setDiscardFlag(QwtPlotRenderer::DiscardCanvasFrame, true);  // Ignore the canvas frame

// Restore defaults: assign the empty flag set (render everything)
renderer.setDiscardFlags(QwtPlotRenderer::DiscardNone);
```

### 6. Print Function

For printing, a configured `QPrinter` is handed to the renderer (`renderTo(QwtPlot*, QPrinter&)`), which lays the plot out across the page; the printer itself decides the target — a physical device or, as below, a PDF file:

```cpp
#include <QPrinter>

// Print to a printer (or to a PDF file via setOutputFileName)
QPrinter printer(QPrinter::HighResolution);
printer.setOutputFileName("print.pdf");  // Leave unset to print on a physical printer

renderer.renderTo(plot, printer);

// Alternatively, exportTo() opens a file dialog and renders to the chosen file
renderer.exportTo(plot, "plot.pdf");
```

### 7. Using Millimeter Units for Size

The size argument of `renderDocument()` is a `QSizeF` interpreted in millimeters, so the exported document has well-defined physical dimensions; the `resolution` argument (default 85) sets the dots per inch used when rasterizing content:

```cpp
// Specify size in millimeter units
QSizeF sizeMm(200, 150);  // 200mm x 150mm
renderer.renderDocument(plot, "plot.pdf", sizeMm, 300);  // 300 DPI
```

## Export Format Comparison

| Format | Advantages | Disadvantages | Use Cases |
|------|------|------|----------|
| PNG | Lossless compression, transparency support | Larger file size | Web sharing, screenshots |
| JPG | Small file size, widely supported | Lossy compression, no transparency | Photo-like charts |
| PDF | Vector, high resolution | Requires PDF reader | Printing, documents |
| SVG | Vector, editable | Complex graphics render slowly | Web pages, editing |
| BMP | Lossless, simple | Very large file size | Specific software |

## Export Option Enum

| Option | Description |
|------|------|
| `DiscardBackground` | Ignore the plot background |
| `DiscardTitle` | Ignore title |
| `DiscardLegend` | Ignore legend |
| `DiscardFooter` | Ignore footer |
| `DiscardCanvasBackground` | Ignore canvas background |
| `DiscardCanvasFrame` | Ignore the canvas frame (no effect with style sheets, where the frame is part of the background) |

## Core Method Summary

| Method | Description |
|------|------|
| `renderDocument()` | Export to a file (format chosen by extension or passed explicitly) |
| `renderTo()` | Render into a `QPaintDevice` (`QImage`/`QPixmap`), a `QPrinter` or a `QSvgGenerator` |
| `exportTo()` | Open a file dialog and export to the selected file |
| `setDiscardFlag()` / `setDiscardFlags()` | Set/reset discard options (`DiscardNone` renders everything) |

> **Tip — Export Recommendations**
>
> - Use PDF or high-resolution PNG (300 DPI) for printing
> - Use SVG or PNG for web pages
> - Use PDF or SVG for document embedding
> - Use PNG or JPG for screenshot sharing

> **Example — Related Examples**
>
> - Export functionality demo: `examples/2D/barchart`