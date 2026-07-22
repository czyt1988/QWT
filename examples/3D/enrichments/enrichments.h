#ifndef enrichments_h
#define enrichments_h

#include "qwt3d_plot.h"
#include "qwt3d_enrichment.h"

class Bar : public Qwt3DVertexEnrichment
{
public:
    Bar();
    Bar(double rad, double level);

    Qwt3DEnrichment *clone() const override { return new Bar(*this); }

    void configure(double rad, double level);
    void drawBegin() override;
    void drawEnd() override;
    void draw(Triple const &) override;

private:
    double level_, radius_;
    double diag_;
};

class Label3D
{
public:
    void draw(Triple const &, double w, double h);
};

#endif
