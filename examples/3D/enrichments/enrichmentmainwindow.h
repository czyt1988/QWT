#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_function.h"
#include "enrichments.h"

#include "ui_enrichmentmainwindowbase.h"

// MOC_SKIP_BEGIN
class DummyBase : public QMainWindow, protected Ui::MainWindow
{
public:
    DummyBase(QWidget *parent = 0) : QMainWindow(parent) { }
};
// MOC_SKIP_END

class EnrichmentMainWindow : public DummyBase
{
    Q_OBJECT

public:
    EnrichmentMainWindow(QWidget *parent = 0);
    ~EnrichmentMainWindow();
    void setColor();
    Bar *bar;
    Qwt3DPlot *plot;
    Qwt3DSurface *surface;

public slots:
    void setLevel(int);
    void setWidth(int);
    void barSlot();

private:
    double level_, width_;
};
