# Widget Components - Dials and Sliders

Qwt provides a series of instrument widget components for numeric display and interactive control. These widgets are suitable for instrument panels, control systems, data monitoring, and similar scenarios.

## Widget Type Overview

| Widget Class | Description |
|--------|------|
| `QwtDial` | Dial/gauge |
| `QwtKnob` | Knob control |
| `QwtSlider` | Slider control |
| `QwtThermo` | Thermometer/progress bar |
| `QwtWheel` | Wheel control |
| `QwtCounter` | Numeric counter |
| `QwtCompass` | Compass |

## Usage

The widget demo is located at: `examples/2D/controls`. Screenshots:

![Dial Controls](../../assets/screenshots/controls-dial.png)

![Knob Controls](../../assets/screenshots/controls-knob.png)

![Slider Controls](../../assets/screenshots/controls-sliders.png)

### 1. QwtDial - Dial

`QwtDial` is a round instrument face with a needle indicating the current value — the classic gauge for angles, directions or cockpit-style displays. Its look is dominated by the needle object installed with `setNeedle()` (for instance a `QwtDialSimpleNeedle` in `Arrow` or `Ray` style), while the value itself is set through the inherited `setValue()` and announced by the `valueChanged()` signal. The example below shows a typical setup:

```cpp
#include <QwtDial>

QwtDial* dial = new QwtDial();

// Set range
dial->setRange(0.0, 360.0);  // 0-360 degrees

// Set current value
dial->setValue(90.0);

// Set scale step
dial->setScaleStep(30.0);  // One tick every 30 degrees

// Set scale position
dial->setScalePosition(QwtDial::Inside);  // Ticks on inside
// Other options: Outside (outside), NoScale (no scale)

// Set needle style
dial->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));

// Connect value change signal
connect(dial, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
```

### 2. QwtKnob - Knob Control

`QwtKnob` emulates a rotatable physical knob that you turn with the mouse — a familiar choice for tweaking parameters on audio or instrument panels. `setKnobStyle()` picks the surface shading (`Flat` / `Raised` / `Sunken` / `Styled`), `setKnobWidth()` sizes the knob, the current value is indicated by a marker styled with `setMarkerStyle()`, and `setTracking(true)` emits values continuously while dragging, as shown below:

```cpp
#include <QwtKnob>

QwtKnob* knob = new QwtKnob();

// Set range
knob->setRange(0, 100);

// Set current value
knob->setValue(50);

// Set scale style
knob->setScalePosition(QwtKnob::OutsideScale);

// Set knob appearance
knob->setKnobStyle(QwtKnob::RaisedStyle);  // Raised style
// Other options: FlatStyle (flat), SunkenStyle (sunken)

// Set knob width
knob->setKnobWidth(50);

// Set mouse tracking mode
knob->setTracking(true);  // Update value in real-time while dragging
```

### 3. QwtSlider - Slider Control

`QwtSlider` is Qwt's take on the familiar linear slider, ideal for adjusting a value within a bounded range. `setOrientation()` chooses between `Qt::Horizontal` and `Qt::Vertical`, `setScalePosition()` decides where the scale sits relative to the groove (`LeadingScale`, `TrailingScale` or `NoScale`), and `setHandleSize()` adjusts the grip, as in the example below:

```cpp
#include <QwtSlider>

QwtSlider* slider = new QwtSlider();

// Set range
slider->setRange(0.0, 100.0);

// Set current value
slider->setValue(30.0);

// Set orientation
slider->setOrientation(Qt::Horizontal);  // Horizontal slider
// or Qt::Vertical (vertical slider)

// Set scale position
slider->setScalePosition(QwtSlider::TrailingScale);  // Scale on trailing side
// Other options: LeadingScale, NoScale

// Set slider handle style
slider->setHandleSize(QSize(10, 20));  // Slider handle size

// Set step values
slider->setSingleStep(1.0);
slider->setPageStep(10.0);
```

### 4. QwtThermo - Thermometer

`QwtThermo` displays a value as a liquid column inside a pipe — the natural widget for temperatures, fill levels or progress. `setOrientation()` lays it out vertically or horizontally, `setFillBrush()` colors the column, `setPipeWidth()` sets the pipe thickness, and `setAlarmLevel()` together with `setAlarmBrush()` switches to a warning color once the value crosses a threshold:

```cpp
#include <QwtThermo>

QwtThermo* thermo = new QwtThermo();

// Set range
thermo->setRange(0.0, 100.0);

// Set current value
thermo->setValue(75.0);

// Set orientation
thermo->setOrientation(Qt::Vertical);

// Set fill color
thermo->setFillBrush(QBrush(Qt::red));

// Set pipe width
thermo->setPipeWidth(10);

// Set scale position
thermo->setScalePosition(QwtThermo::TrailingScale);

// Set alarm level
thermo->setAlarmLevel(80.0);  // Alarm at 80 degrees
thermo->setAlarmBrush(QBrush(Qt::darkRed));
```

### 5. QwtWheel - Wheel Control

`QwtWheel` emulates a mechanical thumb wheel for smooth, continuous value adjustment — handy wherever a slider would take too much panel space. `setRange()` defines the working interval, `setWheelWidth()` sizes the wheel, `setSingleStep()` sets the increment per notch, and `setTracking(true)` reports values while the user is still dragging, as the snippet below demonstrates:

```cpp
#include <QwtWheel>

QwtWheel* wheel = new QwtWheel();

// Set range
wheel->setRange(0, 360);

// Set current value
wheel->setValue(180);

// Set orientation
wheel->setOrientation(Qt::Horizontal);

// Set wheel width
wheel->setWheelWidth(30);

// Set border width
wheel->setBorderWidth(2);

// Set step values
wheel->setSingleStep(1);
wheel->setPageStep(15);

// Set mouse tracking
wheel->setTracking(true);
```

### 6. QwtCounter - Numeric Counter

`QwtCounter` pairs an editable numeric display with increment/decrement buttons for precise numeric entry. `setNumButtons()` decides how many step-button pairs flank the display, `setSingleStep()` sets the amount one click adds, and `setWrapping(true)` lets the value wrap around at the range boundaries:

```cpp
#include <QwtCounter>

QwtCounter* counter = new QwtCounter();

// Set range
counter->setRange(0, 9999);

// Set current value
counter->setValue(100);

// Set step buttons
counter->setNumButtons(3);  // 3 step buttons

// Set step values
counter->setSingleStep(1);
counter->setPageStep(100);

// Set wrap mode
counter->setWrap(true);  // Wrap around at boundaries
```

### 7. QwtCompass - Compass

`QwtCompass` is a direction indicator derived from `QwtDial` that draws a compass rose instead of a plain scale. `setRose()` installs the rose drawing (for example `QwtSimpleCompassRose`), `setLabelMap()` replaces numeric tick labels with cardinal letters such as N/E/S/W, and `setNeedle()` accepts a wind-arrow needle like `QwtCompassWindArrow`, as shown below:

```cpp
#include <QwtCompass>

QwtCompass* compass = new QwtCompass();

// Set range (angle)
compass->setRange(0.0, 360.0);

// Set current direction
compass->setValue(45.0);  // 45 degrees north-east

// Set rose style
compass->setRose(new QwtSimpleCompassRose());

// Set scale labels
compass->setLabelMap({
    {0, "N"}, {90, "E"}, {180, "S"}, {270, "W"}
});

// Set needle
compass->setNeedle(new QwtCompassWindArrow());
```

## Core Common Methods

All widgets inherit from `QwtAbstractSlider` and share the following methods:

| Method | Description |
|------|------|
| `setRange()` | Set numeric range |
| `setValue()` | Set current value |
| `value()` | Get current value |
| `setSingleStep()` | Set single step increment |
| `setPageStep()` | Set page step increment |
| `setTracking()` | Set mouse tracking |
| `setWrapping()` | Set wrap mode |

> **Tip — Widget Usage Recommendations**
>
> - Dials are suitable for angle or direction display
> - Knobs are suitable for parameter adjustment
> - Sliders are suitable for linear range adjustment
> - Thermometers are suitable for progress or temperature display
> - Wheels are suitable for continuous smooth adjustment

> **Example — Related Examples**
>
> - Widget demo: `examples/2D/controls`
> - Compass: `examples/2D/dials`

More screenshots of the widget demo:

![Wheel & Thermo Controls](../../assets/screenshots/controls-wheel-thermo.png)

![Dial Cockpit](../../assets/screenshots/dials-cockpit.png)

![Dial Compass](../../assets/screenshots/dials-compass.png)
