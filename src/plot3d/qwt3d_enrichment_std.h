#ifndef QWT3D_ENRICHMENT_STD_H
#define QWT3D_ENRICHMENT_STD_H

#include "qwt3d_enrichment.h"



class Qwt3DPlot;

/**
 * @brief The Cross Hair Style
 */
class QWT3D_EXPORT Qwt3DCrossHair : public Qwt3DVertexEnrichment
{
public:
    Qwt3DCrossHair();
    Qwt3DCrossHair(double rad, double linewidth, bool smooth, bool boxed);
    Qwt3DCrossHair(const Qwt3DCrossHair& other);
    ~Qwt3DCrossHair() override;

    Qwt3DEnrichment* clone() const override;

    void configure(double rad, double linewidth, bool smooth, bool boxed);
    void drawBegin() override;
    void drawEnd() override;
    void draw(Triple const&) override;

private:
    QWT_DECLARE_PRIVATE(Qwt3DCrossHair)
};

/**
 * @brief The Point Style
 */
class QWT3D_EXPORT Qwt3DDot : public Qwt3DVertexEnrichment
{
public:
    Qwt3DDot();
    Qwt3DDot(double pointsize, bool smooth);
    Qwt3DDot(const Qwt3DDot& other);
    ~Qwt3DDot() override;

    Qwt3DEnrichment* clone() const override;

    void configure(double pointsize, bool smooth);
    void drawBegin() override;
    void drawEnd() override;
    void draw(Triple const&) override;

private:
    QWT_DECLARE_PRIVATE(Qwt3DDot)
};

/**
 * @brief The Cone Style
 */
class QWT3D_EXPORT Qwt3DCone : public Qwt3DVertexEnrichment
{
public:
    Qwt3DCone();
    Qwt3DCone(double rad, unsigned quality);
    Qwt3DCone(const Qwt3DCone& other);
    ~Qwt3DCone() override;

    Qwt3DEnrichment* clone() const override;

    void configure(double rad, unsigned quality);
    void draw(Triple const&) override;

private:
    QWT_DECLARE_PRIVATE(Qwt3DCone)
};

/**
 * @brief 3D vector field
 * @details The class encapsulates a vector field including its OpenGL representation as arrow field.
 *          The arrows can be configured in different aspects (color, shape, painting quality).
 */
class QWT3D_EXPORT Qwt3DArrow : public Qwt3DVertexEnrichment
{
public:
    Qwt3DArrow();
    Qwt3DArrow(const Qwt3DArrow& other);
    ~Qwt3DArrow() override;

    Qwt3DEnrichment* clone() const override;

    void configure(int segs, double relconelength, double relconerad, double relstemrad);
    void setQuality(int val);
    void draw(Triple const&) override;

    void setTop(Triple t);
    void setColor(RGBA rgba);

private:
    QWT_DECLARE_PRIVATE(Qwt3DArrow)

    double calcRotation(Triple& axis, FreeVector const& vec);
};


#endif  // QWT3D_ENRICHMENT_STD_H
