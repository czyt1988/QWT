#include <qmainwindow.h>

#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_function.h"

#include "ui_axesmainwindowbase.h"

// MOC_SKIP_BEGIN
class DummyBase : public QMainWindow, protected Ui::MainWindow
{
public:
    DummyBase(QWidget *parent = 0) : QMainWindow(parent) { }
};
// MOC_SKIP_END

class AxesMainWindow : public DummyBase
{
    Q_OBJECT

public:
    AxesMainWindow(QWidget *parent = 0);
    ~AxesMainWindow();
    Qwt3DPlot *plot;
    Qwt3DSurface *surface;
    Qwt3DFunction *rosenbrock;
    void resetTics();

public slots:
    void setNumberGap(int gap);
    void setLabelGap(int gap);

    void setSmoothLines(bool);
    void setTicLength(int val);
    void setTicNumber(int degree);

    void standardItems();
    void complexItems();
    void letterItems();
    void timeItems();
    void customScale();

private:
    int tics;
};
