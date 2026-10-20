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

`QwtDial` is a round instrument face with a needle indicating the current value (the classic gauge for angles, directions or cockpit-style displays). The value range comes from the inherited `setScale()` and the tick spacing from `setScaleStepSize()`; its look is dominated by the needle object installed with `setNeedle()` (for instance a `QwtDialSimpleNeedle` in `Arrow` or `Ray` style), and `setMode()` decides whether the needle or the scale rotates. The value itself is set through the inherited `setValue()` and announced by the `valueChanged()` signal. The example below shows a typical setup:

```cpp
#include <QwtDial>
#include <QwtDialSimpleNeedle>

QwtDial* dial = new QwtDial();

// Set the value range (inherited from QwtAbstractScale)
dial->setScale(0.0, 360.0);  // 0-360 degrees

// Set current value
dial->setValue(90.0);

// Set the scale step size hint
dial->setScaleStepSize(30.0);  // One tick every 30 degrees

// Dial geometry: origin direction and arc covered by the scale
dial->setOrigin(90.0);          // Scale starts at the top
dial->setScaleArc(0.0, 360.0);  // Scale covers the full circle

// Rotation mode
dial->setMode(QwtDial::RotateNeedle);  // The needle rotates (default)
// Other option: QwtDial::RotateScale (needle fixed, the scale rotates)

// Set needle style
dial->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));

// Connect value change signal
connect(dial, SIGNAL(valueChanged(double)), this, SLOT(onValueChanged(double)));
```

### 2. QwtKnob - Knob Control

`QwtKnob` emulates a rotatable physical knob that you turn with the mouse; it is a familiar choice for tweaking parameters on audio or instrument panels. `setKnobStyle()` picks the surface shading (`Flat` / `Raised` / `Sunken` / `Styled`), `setKnobWidth()` sizes the knob, the current value is indicated by a marker styled with `setMarkerStyle()`, and `setTracking(true)` emits values continuously while dragging, as shown below:

```cpp
#include <QwtKnob>

QwtKnob* knob = new QwtKnob();

// Set the value range (inherited from QwtAbstractScale)
knob->setScale(0.0, 100.0);

// Set current value
knob->setValue(50.0);

// Scale tick spacing and rotation angle covered by the value range
knob->setScaleStepSize(10.0);  // One scale tick every 10 units
knob->setTotalAngle(270.0);    // Value range spans a 270 degree arc

// Set knob appearance
knob->setKnobStyle(QwtKnob::Raised);  // Raised style
// Other options: Flat (flat), Sunken (sunken), Styled (Qt-style gradient)

// Marker indicating the current value
knob->setMarkerStyle(QwtKnob::Notch);
// Other options: NoMarker, Tick, Triangle, Dot, Nub

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

// Set the value range (inherited from QwtAbstractScale)
slider->setScale(0.0, 100.0);

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

// Step counts for keyboard/wheel input (in number of steps, from QwtAbstractSlider)
slider->setSingleSteps(1);   // One single step per arrow key
slider->setPageSteps(10);    // One page step covers 10 single steps
```

### 4. QwtThermo - Thermometer

`QwtThermo` displays a value as a liquid column inside a pipe, which makes it natural for temperatures, fill levels or progress. `setOrientation()` lays it out vertically or horizontally, `setFillBrush()` colors the column, `setPipeWidth()` sets the pipe thickness, and `setAlarmLevel()` together with `setAlarmBrush()` switches to a warning color once the value crosses a threshold:

```cpp
#include <QwtThermo>

QwtThermo* thermo = new QwtThermo();

// Set the value range (inherited from QwtAbstractScale)
thermo->setScale(0.0, 100.0);

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

// Set alarm level (implicitly enables the alarm)
thermo->setAlarmLevel(80.0);  // Alarm at 80 degrees
thermo->setAlarmBrush(QBrush(Qt::darkRed));
```

### 5. QwtWheel - Wheel Control

`QwtWheel` emulates a mechanical thumb wheel for smooth, continuous value adjustment, and is handy wherever a slider would take too much panel space. `setRange()` defines the working interval, `setWheelWidth()` sizes the wheel, `setSingleStep()` sets the increment per notch, `setPageStepCount()` decides how many single steps one page step covers, and `setMass()` turns it into a flywheel that keeps spinning after release; `setTracking(true)` reports values while the user is still dragging, as the snippet below demonstrates:

```cpp
#include <QwtWheel>

QwtWheel* wheel = new QwtWheel();

// Set range
wheel->setRange(0.0, 360.0);

// Set current value
wheel->setValue(180.0);

// Set orientation
wheel->setOrientation(Qt::Horizontal);

// Set wheel width
wheel->setWheelWidth(30);

// Set border width
wheel->setBorderWidth(2);

// Set step values
wheel->setSingleStep(1.0);    // Increment per notch
wheel->setPageStepCount(15);  // One page step covers 15 single steps

// Flywheel effect: the wheel keeps spinning after being released
wheel->setMass(0.5);

// Set mouse tracking
wheel->setTracking(true);
```

### 6. QwtCounter - Numeric Counter

`QwtCounter` pairs an editable numeric display with increment/decrement buttons for precise numeric entry. `setNumButtons()` decides how many step-button pairs flank the display, `setSingleStep()` sets the amount one click of the first button adds, `setIncSteps()` assigns a larger multiplier to the other buttons, and `setWrapping(true)` lets the value wrap around at the range boundaries:

```cpp
#include <QwtCounter>

QwtCounter* counter = new QwtCounter();

// Set range
counter->setRange(0.0, 9999.0);

// Set current value
counter->setValue(100.0);

// Set step buttons
counter->setNumButtons(3);  // 3 step buttons on each side

// Set step values
counter->setSingleStep(1.0);                     // One click of button 1 adds 1.0
counter->setIncSteps(QwtCounter::Button2, 100);  // Button 2 adds 100 single steps

// Set wrap mode
counter->setWrapping(true);  // Wrap around at boundaries
```

### 7. QwtCompass - Compass

`QwtCompass` is a direction indicator derived from `QwtDial` that can draw a compass rose on top of the round scale. `setRose()` installs the rose drawing (for example `QwtSimpleCompassRose`), the cardinal letters N/E/S/W come from the label map of a `QwtCompassScaleDraw` installed with `setScaleDraw()`, and `setNeedle()` accepts a wind-arrow needle like `QwtCompassWindArrow`, as shown below:

```cpp
#include <QwtCompass>
#include <QwtCompassScaleDraw>
#include <QwtSimpleCompassRose>
#include <QwtCompassWindArrow>
#include <QMap>

QwtCompass* compass = new QwtCompass();

// Set range (angle)
compass->setScale(0.0, 360.0);

// Set current direction
compass->setValue(45.0);  // 45 degrees north-east

// Set rose style
compass->setRose(new QwtSimpleCompassRose());

// Replace numeric tick labels with cardinal letters
QMap<double, QString> labelMap;
labelMap.insert(0.0, "N");
labelMap.insert(90.0, "E");
labelMap.insert(180.0, "S");
labelMap.insert(270.0, "W");
compass->setScaleDraw(new QwtCompassScaleDraw(labelMap));

// Set needle (wind arrow showing where the wind comes from)
compass->setNeedle(new QwtCompassWindArrow(QwtCompassWindArrow::Style1));
```

## Core Common Methods

The seven widgets do not all share one base class: `QwtDial`, `QwtKnob` and `QwtSlider` derive from `QwtAbstractSlider` (itself a `QwtAbstractScale`), `QwtThermo` derives from `QwtAbstractScale` only, while `QwtWheel` and `QwtCounter` are standalone `QWidget` subclasses with a similar value API. The shared methods are available as listed below:

| Method | Available on | Description |
|------|------|------|
| `setScale(double lower, double upper)` | Dial / Knob / Slider / Thermo | Set value range and scale bounds (from `QwtAbstractScale`) |
| `setRange(double min, double max)` | Wheel / Counter | Set numeric range |
| `setValue(double)` / `value()` | All seven widgets | Set/get current value |
| `setSingleSteps(uint)` / `setPageSteps(uint)` | Dial / Knob / Slider | Number of single/page steps per keyboard or wheel input (`QwtAbstractSlider`) |
| `setSingleStep(double)` | Wheel / Counter | Single step size |
| `setPageStepCount(int)` | Wheel | Number of single steps covered by one page step |
| `setTracking(bool)` | Dial / Knob / Slider / Wheel | Report values continuously while dragging |
| `setWrapping(bool)` | Dial / Knob / Slider / Wheel / Counter | Wrap around at the range boundaries |

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
> - Wheel with flywheel effect: `examples/2D/radio`

More screenshots of the widget demo:

![Wheel & Thermo Controls](../../assets/screenshots/controls-wheel-thermo.png)

![Dial Cockpit](../../assets/screenshots/dials-cockpit.png)

![Dial Compass](../../assets/screenshots/dials-compass.png)
