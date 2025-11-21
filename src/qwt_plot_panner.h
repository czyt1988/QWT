#ifndef QWT_PLOT_PANNER_H
#define QWT_PLOT_PANNER_H
#include "qwt_global.h"
#include "qwt_picker.h"

// qt
class QWidget;

// qwt
class QwtPlot;

class QWT_EXPORT QwtPlotPanner : public QwtPicker
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtPlotPanner)
public:
    explicit QwtPlotPanner(QWidget* parent);
    virtual ~QwtPlotPanner();

    QWidget* canvas();
    const QWidget* canvas() const;

    QwtPlot* plot();
    const QwtPlot* plot() const;

    void setOrientations(Qt::Orientations);
    Qt::Orientations orientations() const;
    bool isOrientationEnabled(Qt::Orientation) const;

    void setMouseButton(Qt::MouseButton button, Qt::KeyboardModifiers modifiers = Qt::NoModifier);
    void getMouseButton(Qt::MouseButton& button, Qt::KeyboardModifiers& modifiers) const;

    void setEnabled(bool);

public Q_SLOTS:
    void moveCanvas(int dx, int dy);

Q_SIGNALS:
    void panned(int dx, int dy);
    void moved(int dx, int dy);

protected:
    virtual bool accept(QPolygon&) const QWT_OVERRIDE;
    virtual void begin() QWT_OVERRIDE;
    virtual void move(const QPoint&) QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;

    virtual void widgetMousePressEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void widgetMouseMoveEvent(QMouseEvent*) QWT_OVERRIDE;
    virtual void widgetMouseReleaseEvent(QMouseEvent*) QWT_OVERRIDE;

private:
    void init();
};
#endif  // QWT_PLOT_PANNER_H
