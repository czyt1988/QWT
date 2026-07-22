#ifndef QWT3D_ENRICHMENT_STD_H
#define QWT3D_ENRICHMENT_STD_H

#include "qwt3d_enrichment.h"



class Plot3D;

/**
 * @brief The Cross Hair Style
 */
class QWT3D_EXPORT CrossHair : public VertexEnrichment
{
public:
    CrossHair();
    CrossHair(double rad, double linewidth, bool smooth, bool boxed);
    CrossHair(const CrossHair& other);
    ~CrossHair() override;

    Enrichment* clone() const override;

    void configure(double rad, double linewidth, bool smooth, bool boxed);
    void drawBegin() override;
    void drawEnd() override;
    void draw(Triple const&) override;

private:
    QWT_DECLARE_PRIVATE(CrossHair)
};

/**
 * @brief The Point Style
 */
class QWT3D_EXPORT Dot : public VertexEnrichment
{
public:
    Dot();
    Dot(double pointsize, bool smooth);
    Dot(const Dot& other);
    ~Dot() override;

    Enrichment* clone() const override;

    void configure(double pointsize, bool smooth);
    void drawBegin() override;
    void drawEnd() override;
    void draw(Triple const&) override;

private:
    QWT_DECLARE_PRIVATE(Dot)
};

/**
 * @brief The Cone Style
 */
class QWT3D_EXPORT Cone : public VertexEnrichment
{
public:
    Cone();
    Cone(double rad, unsigned quality);
    Cone(const Cone& other);
    ~Cone() override;

    Enrichment* clone() const override;

    void configure(double rad, unsigned quality);
    void draw(Triple const&) override;

private:
    QWT_DECLARE_PRIVATE(Cone)
};

/**
 * @brief 3D vector field
 * @details The class encapsulates a vector field including its OpenGL representation as arrow field.
 *          The arrows can be configured in different aspects (color, shape, painting quality).
 */
class QWT3D_EXPORT Arrow : public VertexEnrichment
{
public:
    Arrow();
    Arrow(const Arrow& other);
    ~Arrow() override;

    Enrichment* clone() const override;

    void configure(int segs, double relconelength, double relconerad, double relstemrad);
    void setQuality(int val);
    void draw(Triple const&) override;

    void setTop(Triple t);
    void setColor(RGBA rgba);

private:
    QWT_DECLARE_PRIVATE(Arrow)

    double calcRotation(Triple& axis, FreeVector const& vec);
};


#endif  // QWT3D_ENRICHMENT_STD_H
