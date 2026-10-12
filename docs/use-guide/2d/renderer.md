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

All file exports go through `QwtPlotRenderer::renderDocument()`: you pass the plot, the target filename (whose extension selects the format), the output size, and the resolution in DPI (default 85). The example below writes the same plot as PNG, JPG, and BMP:

```cpp
#include <QwtPlotRenderer>

QwtPlotRenderer renderer;

// Export to PNG
renderer.renderDocument(plot, "plot.png", QSize(800, 600), 300);  // 300 DPI

// Export to JPG
renderer.renderDocument(plot, "plot.jpg", QSize(800, 600), 150);

// Export to BMP
renderer.renderDocument(plot, "plot.bmp", QSize(800, 600));

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

Instead of writing a file, the renderer can also draw into an in-memory paint device: the `renderTo(QwtPlot*, QPaintDevice&)` overload accepts any `QImage` or `QPixmap` prepared at the desired size and DPI, which is handy for embedding the result in an application or post-processing it:

```cpp
// Export as QImage
QSize size(800, 600);
QImage image = renderer.renderToImage(plot, size, 300);  // 300 DPI

// Export as QPixmap
QPixmap pixmap = renderer.renderToPixmap(plot, size, 300);

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
renderer.setDiscardFlag(QwtPlotRenderer::DiscardFrames, true);    // Ignore border frames

// Restore defaults
renderer.resetDiscardFlags();
```

### 6. Print Function

For printing, a configured `QPrinter` is handed to the renderer (`renderTo(QwtPlot*, QPrinter&)`), which lays the plot out across the page; the printer itself decides the target — a physical device or, as below, a PDF file:

```cpp
#include <QPrinter>

// Print to printer
QPrinter printer(QPrinter::HighResolution);
printer.setOutputFileName("print.pdf");  // Or use printer.printerName()

renderer.renderDocument(plot, &printer);

// Or directly use QwtPlot's print method
plot->print(printer);
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
| `DiscardTitle` | Ignore title |
| `DiscardLegend` | Ignore legend |
| `DiscardFooter` | Ignore footer |
| `DiscardCanvasBackground` | Ignore canvas background |
| `DiscardFrames` | Ignore borders |

## Core Method Summary

| Method | Description |
|------|------|
| `renderDocument()` | Export to file |
| `renderToImage()` | Export as QImage |
| `renderToPixmap()` | Export as QPixmap |
| `setDiscardFlag()` | Set discard option |
| `resetDiscardFlags()` | Reset discard options |

> **Tip — Export Recommendations**
>
> - Use PDF or high-resolution PNG (300 DPI) for printing
> - Use SVG or PNG for web pages
> - Use PDF or SVG for document embedding
> - Use PNG or JPG for screenshot sharing

> **Example — Related Examples**
>
> - Export functionality demo: `examples/2D/barchart`