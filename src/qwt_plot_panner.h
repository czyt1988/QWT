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
    explicit QwtPlotPanner(QWidget* canvas);
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

public Q_SLOTS:
    void moveCanvas(int dx, int dy);

Q_SIGNALS:
    void panned(int dx, int dy);

protected:
    virtual void widgetMousePressEvent(QMouseEvent* mouseEvent) QWT_OVERRIDE;
    virtual void move(const QPoint&) QWT_OVERRIDE;
    virtual bool end(bool ok = true) QWT_OVERRIDE;

private:
    void init();
};
#endif  // QWT_PLOT_PANNER_H
