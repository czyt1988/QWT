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
