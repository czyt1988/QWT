# Comment Standards

This document defines the standards for writing code comments in the Qwt project, ensuring consistency and completeness between documentation and code.

## Language Requirements

> **Warning — Important: English Only**
>
> Qwt is an international project. **All comments in source code (including Doxygen documentation comments) must be written in English; Chinese is not permitted.**
> Chinese documentation is only allowed in the `docs/` directory.

## Key Requirements

**Features**

- ✅ **Doxygen Format**: Use Doxygen standard format for comments
- ✅ **English-Only Comments**: All comments must be written in English
- ✅ **Placement Rules**: Detailed comments go in cpp files; brief comments go in header files
- ✅ **Completeness**: Classes, functions, and signals all require complete documentation comments

## Doxygen Keyword Standards

Doxygen keywords must use the `@` prefix (e.g., `@brief`, `@param`, `@return`, `@details`):

```cpp
/**
 * @brief Brief description here
 * @param name Parameter description
 * @return Return value description
 */
```

## Comment Placement Principles

> **Info — Core Principles**
>
> - **Detailed comments** go in `.cpp` files
> - **Brief comments** go in `.h` files
> - Keep header files clean and avoid excessive comment content

## Source File Comment Standards

All new code must use the **Doxygen English format** for comments.

### Function Comment Template

```cpp
/**
 * @brief [Brief description]
 * @param param_name [Parameter description]
 * @return [Return value description]
 * @details [Detailed explanation]
 */
void MyClass::myFunction(int param_name)
{
    // ...
}
```

### Parameter Descriptions

Parameter direction annotations are **recommended but not mandatory**. For value and const ref parameters, the direction is obvious and `[in]` can be omitted.

However, direction **must** be annotated in the following cases:

- `@param[out]` — Non-const reference/pointer used for output results
- `@param[in,out]` — Non-const reference/pointer that is both read and written

```cpp
/**
 * @brief Calculate the intersection point of two curves
 * @param curve1 First curve to intersect
 * @param curve2 Second curve to intersect
 * @param[out] intersectionPoint The calculated intersection point
 * @return true if intersection found, false otherwise
 */
bool calculateIntersection(const QwtPlotCurve* curve1,
                           const QwtPlotCurve* curve2,
                           QPointF& intersectionPoint);
```

### Detailed Description Example

```cpp
/**
 * @brief Set the samples for the curve
 * @param xData Array of x coordinates
 * @param yData Array of y coordinates
 * @param size Number of samples
 * @details This method creates a new sample series from the provided arrays.
 *          The data is copied internally, so the original arrays can be
 *          modified or deleted after calling this method.
 */
void QwtPlotCurve::setSamples(const double* xData,
                              const double* yData,
                              int size);
```

## Header File Comment Standards

`public` function declarations in header files should only have **single-line English brief comments**.

### Brief Comment Example

```cpp
class MyClass {
public:
    // Constructor for MyClass
    MyClass();

    // Set the title text
    void setTitle(const QString& title);

    // Get the current title
    QString title() const;
};
```

> **Warning — Note**
>
> **Do not** write detailed Doxygen comment blocks in header files. Detailed content should be placed in the corresponding `.cpp` file.

## Class Comment Standards

Class Doxygen comments should be added in header files.

### Class Comment Template

```cpp
/**
 * @brief 2D plotting widget for displaying curves and other plot items
 * @details QwtPlot is the main widget for displaying 2D data. It manages
 *          axes, legends, and plot items like curves, markers, and grids.
 */
class QwtPlot : public QWidget
{
    QWT_DECLARE_PRIVATE(QwtPlot)

public:
    // Constructor
    QwtPlot(QWidget* parent = nullptr);

    // ...
};
```

### Class Comments with Examples

For classes with rich functionality, usage examples should be included:

```cpp
/**
 * @brief Container widget for organizing multiple QwtPlot instances
 * @details QwtFigure provides a layout container similar to matplotlib's Figure,
 *          allowing multiple plots to be arranged in a single window.
 *
 * @code
 * QwtFigure* figure = new QwtFigure();
 * figure->setSizeInches(8, 6);
 *
 * QwtPlot* plot1 = new QwtPlot();
 * figure->addAxes(plot1, 0.0, 0.0, 0.5, 0.5);  // Left-top quadrant
 * @endcode
 */
class QwtFigure : public QWidget
{
    // ...
};
```

## Signal Comment Standards

Qt signals are declared in header files and have no corresponding cpp definition, so signal comments must be fully added in the header file.

### Signal Comment Example

```cpp
class QwtPlot : public QWidget
{
Q_SIGNALS:
    /**
     * @brief Signal emitted when the plot scale changes
     * @param axisId The axis that changed its scale
     */
    void scaleChanged(QwtAxis::Position axisId);
};
```

## Comment Style Checklist

When writing comments, please verify the following items:

| Check Item | Location | Requirement |
|--------|------|------|
| Class comments | Header file `.h` | English Doxygen, including usage examples |
| Public function detailed comments | Source file `.cpp` | English Doxygen |
| Public function brief comments | Header file `.h` | Single-line English |
| Signal comments | Header file `.h` | English Doxygen |
| Private/protected functions | Source file `.cpp` | Optional, English recommended |
| Inline comments | Anywhere | English |

## Common Mistakes

### ❌ Wrong: Detailed Comments in Header Files

```cpp
// Wrong example - too many comments in header file
class MyClass {
public:
    /**
     * @brief Constructor for MyClass
     * @param parent Parent widget pointer
     * @details This constructor initializes the class with...
     */
    MyClass(QWidget* parent = nullptr);
};
```

### ❌ Wrong: Chinese Comments in Source Code

```cpp
// Wrong example - Chinese comment in source code
void QwtPlotCurve::setSamples(const double* data, int size)
{
    m_data = data;  // Wrong: Chinese comment
    m_size = size;
}
```

### ✅ Correct: Brief Header Comments + Detailed cpp Comments

Header file `MyClass.h`:

```cpp
class MyClass {
public:
    // Constructor
    MyClass(QWidget* parent = nullptr);
};
```

Source file `MyClass.cpp`:

```cpp
/**
 * @brief Constructor for MyClass
 * @param parent Parent widget pointer
 * @details This constructor initializes the widget and sets up
 *          the internal data structures.
 */
MyClass::MyClass(QWidget* parent)
    : QWidget(parent)
{
}
```

## Related Documentation

- [Coding Standards](coding-standards.md)
- [PIMPL Pattern Guide](pimpl-pattern.md)
