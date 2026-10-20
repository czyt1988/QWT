# Legend Widget - QwtLegend

`QwtLegend` is the display widget for plot legends, used to show titles and icons of each plot item. Through the legend, users can identify the meaning of different curves or data series.

## Main Features

- Automatic legend item generation: legend entries are generated from the plot items
- Interactive legend: clicking an entry can hide or show the corresponding plot item (opt-in via `setDefaultItemMode()`)
- Layout: supports multiple legend layout modes
- Style customization: icon and text styles of legend items can be customized

## Basic Concepts

### Legend Position

QwtPlot supports four legend positions:

| Position | Enum Value | Description |
|------|--------|------|
| Left | `LeftLegend` | Left of YLeft axis |
| Right | `RightLegend` | Right of YRight axis |
| Bottom | `BottomLegend` | Bottom of plot |
| Top | `TopLegend` | Top of plot |

### Legend Item Composition

Each legend item pairs a small icon with the plot item's title, and the icon is rendered from that item's line, symbol or brush style. The sketch below shows the composition of a typical legend:

```text
┌───────────────────────┐
│  [Icon]  Curve Title  │  ← Legend item
│  ────  Data Curve A   │
│  ▬▬▬  Data Curve B   │
│  ○○○  Scatter Data   │
└───────────────────────┘
```

## Usage

The legend demo is located at: `examples/2D/legends`. Screenshot:

![Legends Demo](../../assets/screenshots/legends.png)

### 1. Inserting a Legend

A legend is created as a `QwtLegend` widget and handed to the plot with `QwtPlot::insertLegend()`, whose second argument picks one of the four `QwtPlot::LegendPosition` values (`LeftLegend`, `RightLegend`, `BottomLegend`, `TopLegend`, defaulting to `RightLegend`). The example below inserts a legend on the right side:

```cpp
#include <QwtPlot>
#include <QwtLegend>

QwtPlot* plot = new QwtPlot();

// Create legend and insert
QwtLegend* legend = new QwtLegend();
plot->insertLegend(legend, QwtPlot::RightLegend);  // Right-side legend

// Other position options:
// QwtPlot::LeftLegend   - Left
// QwtPlot::BottomLegend - Bottom
// QwtPlot::TopLegend    - Top
```

### 2. Legend Interaction

Legend items are read-only by default (`QwtLegendData::ReadOnly`). Interaction is enabled with `QwtLegend::setDefaultItemMode()`; a `Checkable` legend then emits the `checked()` signal, which is typically connected to toggle the visibility of the corresponding plot item via `QwtPlot::infoToItem()`:

```cpp
#include <QwtLegendData>

// Make legend entries checkable (modes: ReadOnly, Clickable, Checkable)
legend->setDefaultItemMode(QwtLegendData::Checkable);

// Toggle the visibility of the plot item behind the legend entry
QObject::connect(legend, &QwtLegend::checked, plot,
    [plot](const QVariant& itemInfo, bool on, int) {
        if (QwtPlotItem* plotItem = plot->infoToItem(itemInfo))
            plotItem->setVisible(on);
    });

// Clickable mode emits clicked(itemInfo, index) instead,
// leaving the reaction completely to the application.
```

### 3. Setting Legend Position and Ratio

The legend ratio controls how much of the plot's width or height the legend may occupy; it can be passed directly as the third argument of `insertLegend()`, or adjusted later through `QwtPlotLayout::setLegendPosition()` and `setLegendRatio()`. The example below reserves 20% for a right-side legend and then moves it to the bottom with a 15% ratio:

```cpp
// Set legend position and ratio
plot->insertLegend(legend, QwtPlot::RightLegend, 0.2);  // Right side, 20% width

// Adjust via layout
QwtPlotLayout* layout = plot->plotLayout();
layout->setLegendPosition(QwtPlot::BottomLegend);
layout->setLegendRatio(0.15);  // Legend occupies 15%
```

### 4. Legend Style Configuration

Since `QwtLegend` is a regular Qt widget, its font can be set with `setFont()` and its background and border with a style sheet. The individual legend entries are `QwtLegendLabel` widgets (a `QwtTextLabel` subclass) living inside `contentsWidget()`; they can be found with `legendWidget()`/`legendWidgets()` for a single plot item, or styled in bulk. The example below shows these three levels of customization:

```cpp
#include <QwtLegendLabel>

// Set legend font
legend->setFont(QFont("Arial", 9));

// Set legend background
legend->setStyleSheet("background-color: white; border: 1px solid gray;");

// Set legend item style: the entries are children of contentsWidget()
const QList<QwtLegendLabel*> labels =
    legend->contentsWidget()->findChildren<QwtLegendLabel*>();
for (QwtLegendLabel* label : labels)
    label->setFont(QFont("Arial", 10));

// A single entry can be looked up via the item info of its plot item
// (curve being any QwtPlotItem attached to the plot)
QwtLegendLabel* label = qobject_cast<QwtLegendLabel*>(
    legend->legendWidget(plot->itemToInfo(curve)));
```

### 5. Plot Item Legend Properties

Control the display style of plot items in the legend:

```cpp
#include <QwtPlotCurve>

QwtPlotCurve* curve = new QwtPlotCurve("Curve A");

// Set legend properties
curve->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);    // Show line icon
curve->setLegendAttribute(QwtPlotCurve::LegendShowSymbol, true);  // Show symbol icon
curve->setLegendAttribute(QwtPlotCurve::LegendShowBrush, true);   // Show fill icon

// Set legend icon size
curve->setLegendIconSize(QSize(20, 15));
```

### 6. Custom Legend Icons

Legend icons are produced by the virtual method `QwtPlotItem::legendIcon()`, so derived classes can override it to draw completely custom icons; their size is controlled by `setLegendIconSize()`. The snippet below enlarges the icon size and notes where to hook in the custom drawing:

```cpp
// Fully custom legend icon
curve->setLegendIconSize(QSize(30, 20));

// Legend icon is generated by QwtPlotItem::legendIcon() method
// Can override this method in derived classes to implement custom icons
```

### 7. External Legend Widget

The legend can be placed outside the plot. There is no dedicated "connect external legend" call: the plot's `legendDataChanged()` signal is connected to the `updateLegend()` slot of the external `QwtLegend`, and `QwtPlot::updateLegend()` populates it with the current items:

```cpp
// Create external legend
QwtLegend* externalLegend = new QwtLegend(parentWidget);

// Connect the plot's legendDataChanged() signal with the
// updateLegend() slot of the external legend
QObject::connect(plot, &QwtPlot::legendDataChanged,
    externalLegend, &QwtLegend::updateLegend);

// Populate the new legend with the current plot items
plot->updateLegend();

externalLegend->show();

// The legend now updates automatically without occupying plot space
```

### 8. Embedded Legend Item

Use `QwtPlotLegendItem` to display legend inside the canvas. Its position is chosen with `setAlignmentInCanvas()` using standard `Qt::Alignment` flags:

```cpp
#include <QwtPlotLegendItem>

// Create embedded legend (drawn on canvas)
QwtPlotLegendItem* legendItem = new QwtPlotLegendItem();
legendItem->attach(plot);

// Align the legend in the canvas (top-right corner here)
legendItem->setAlignmentInCanvas(Qt::AlignRight | Qt::AlignTop);
// Other combinations: Qt::AlignLeft|Qt::AlignTop (top-left),
// Qt::AlignLeft|Qt::AlignBottom, Qt::AlignRight|Qt::AlignBottom, ...

// Optional pixel offset from the canvas border
legendItem->setOffsetInCanvas(Qt::Horizontal | Qt::Vertical, 10);

// Set background
legendItem->setBackgroundBrush(QBrush(Qt::white));

// Set border
legendItem->setBorderPen(QPen(Qt::gray, 1));

// Set maximum columns
legendItem->setMaxColumns(1);
```

## Core Method Summary

### QwtPlot Legend Methods

| Method | Description |
|------|------|
| `insertLegend()` | Insert legend (position and ratio as arguments) |
| `legend()` | Return the current legend |
| `legendDataChanged()` | Signal to feed an external legend |
| `updateLegend()` | Emit legend data for all plot items |
| `itemToInfo()` / `infoToItem()` | Translate between plot item and legend item info |

### QwtLegend Methods

| Method | Description |
|------|------|
| `setDefaultItemMode()` | Set interaction mode (`QwtLegendData::ReadOnly`/`Clickable`/`Checkable`) |
| `setMaxColumns()` | Set maximum number of columns |
| `legendWidget()` / `legendWidgets()` | Return the entry widget(s) of a plot item |
| `itemInfo()` | Return the item info of an entry widget |
| `contentsWidget()` | Return the widget holding all entries |
| `clicked()` / `checked()` | Interaction signals |

### QwtPlotItem Legend Methods

| Method | Description |
|------|------|
| `setItemAttribute()` | Show/hide the item on the legend (`QwtPlotItem::Legend`) |
| `setLegendIconSize()` | Set icon size |
| `legendIcon()` | Return (and in derived classes override) the legend icon |
| `setTitle()` | Set title (legend text) |

> **Note** `setLegendAttribute()` (`LegendShowLine`/`LegendShowSymbol`/`LegendShowBrush`) is a method of `QwtPlotCurve`, controlling how the curve renders its legend icon.

> **Tip — Legend Usage Recommendations**
>
> - Few curves (<5): Right-side legend
> - Many curves (5-10): Bottom legend
> - Very many curves (>10): Consider using embedded legend or grouped display

> **Example — Related Examples**
>
> - Legend demo: `examples/2D/legends`