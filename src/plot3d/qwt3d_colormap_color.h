/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT3D_COLORMAP_COLOR_H
#define QWT3D_COLORMAP_COLOR_H

#include "qwt3d_global.h"
#include "qwt3d_color.h"

class QwtColorMap;
class QString;



class Plot3D;

/**
 * @brief Adapts a QwtColorMap (from qwt::core) for use as a Color.
 * @details Bridges the 2D colormap infrastructure into the 3D color functor system.
 */
class QWT3D_EXPORT ColorMapColor : public Color
{
public:
    explicit ColorMapColor(Plot3D* plot, const QString& presetName = QString("viridis"), unsigned size = 256);

    ColorMapColor(Plot3D* plot, ::QwtColorMap* colorMap, unsigned size = 256);

    ~ColorMapColor() override;

    RGBA operator()(double x, double y, double z) const override;
    ColorVector& createVector(ColorVector& vec) override;

    void setColorMap(::QwtColorMap* map);
    const ::QwtColorMap* colorMap() const;

    void setInterval(double min, double max);
    void reset(unsigned size = 256);
    void setAlpha(double a);

private:
    void rebuildColorVector(unsigned size);

    Plot3D* m_plot;
    ::QwtColorMap* m_colorMap;
    ColorVector m_colors;
    double m_manualMin;
    double m_manualMax;
    bool m_useManualInterval;
    double m_alpha;
};


#endif
