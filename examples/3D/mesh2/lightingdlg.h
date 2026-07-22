#ifndef lightingdlg_h
#define lightingdlg_h

#include <math.h>
#include <qapplication.h>
#include "qwt3d_parametricsurface.h"
#include "qwt3d_plot.h"
#include "qwt3d_surface.h"
#include "qwt3d_enrichment.h"
#include "qwt3d_color.h"

#include "ui_lightingdlgbase.h"

class Pointer : public Qwt3DVertexEnrichment
{
public:
    Pointer(double rad);
    ~Pointer() override;

    Qwt3DEnrichment *clone() const override { return new Pointer(*this); }

    void configure(double rad);
    void drawBegin() override;
    void draw(Triple const &) override { }
    void setPos(double x, double y, double z) { pos_ = Triple(x, y, z); }

private:
    double radius_;
    Triple pos_;
};

struct SColor : public Qwt3DColor
{
    RGBA operator()(double, double, double) const override { return RGBA(0.8, 0, 0, 0.5); }
};

class Plot : public Qwt3DPlot
{
    Q_OBJECT

public:
    Plot(QWidget *parent);
    Pointer *stick;
    Qwt3DSurface *surface;
    void reset();
};

// MOC_SKIP_BEGIN
class LightingBase : public QDialog, protected Ui::Dialog
{
public:
    LightingBase(QWidget *parent = 0) : QDialog(parent) { }
};
// MOC_SKIP_END

class LightingDlg : public LightingBase
{
    Q_OBJECT

public:
    LightingDlg(QWidget *parent = 0);
    ~LightingDlg();

    void assign(Qwt3DPlot *pl);

    Plot *plot;
    Qwt3DPlot *dataPlot;

public slots:
    void setDistance(int);
    void setEmission(int);
    void setDiff(int);
    void setSpec(int);
    void setShin(int);
    void reset();
    void setRotation(double x, double y, double z);
};

#endif /* include guarded */
