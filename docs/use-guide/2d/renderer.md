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

```cpp
// Export to PDF
renderer.renderDocument(plot, "plot.pdf", QSizeF(200, 150), 300);  // 200x150 mm, 300 DPI

// PDF supports vector graphics, suitable for high-resolution printing
```

### 3. Export to SVG

```cpp
// Export to SVG vector format
renderer.renderDocument(plot, "plot.svg", QSizeF(200, 150));

// SVG advantages:
// - Vector graphics, lossless scaling
// - Smaller file size
// - Can be viewed in browsers
```

### 4. Export to QImage/QPixmap

```cpp
// Export as QImage
QSize size(800, 600);
QImage image = renderer.renderToImage(plot, size, 300);  // 300 DPI

// Export as QPixmap
QPixmap pixmap = renderer.renderToPixmap(plot, size, 300);

// Can be used for in-application display or further processing
```

### 5. Export Options

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