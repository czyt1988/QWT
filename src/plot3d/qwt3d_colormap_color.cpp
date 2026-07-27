/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt3d_colormap_color.h"
#include "qwt3d_plot.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"

#include <qcolor.h>
#include <qstring.h>


Qwt3DColorMapColor::Qwt3DColorMapColor(Qwt3DPlot* plot, const QString& presetName, unsigned size)
    : m_plot(plot)
    , m_colorMap(QwtColorMapPreset::create(presetName).release())
    , m_manualMin(0.0)
    , m_manualMax(1.0)
    , m_useManualInterval(false)
    , m_alpha(1.0)
{
    rebuildColorVector(size);
}

Qwt3DColorMapColor::Qwt3DColorMapColor(Qwt3DPlot* plot, ::QwtColorMap* colorMap, unsigned size)
    : m_plot(plot), m_colorMap(colorMap), m_manualMin(0.0), m_manualMax(1.0), m_useManualInterval(false), m_alpha(1.0)
{
    rebuildColorVector(size);
}

Qwt3DColorMapColor::~Qwt3DColorMapColor()
{
    delete m_colorMap;
}

RGBA Qwt3DColorMapColor::operator()(double, double, double z) const
{
    double zMin, zMax;
    if (m_useManualInterval) {
        zMin = m_manualMin;
        zMax = m_manualMax;
    } else if (m_plot) {
        const ParallelEpiped hull = m_plot->hull();
        zMin                      = hull.minVertex.z;
        zMax                      = hull.maxVertex.z;
    } else {
        zMin = 0.0;
        zMax = 1.0;
    }

    const QRgb rgb = m_colorMap->rgb(zMin, zMax, z);
    RGBA rgba;
    rgba.r = qRed(rgb) / 255.0;
    rgba.g = qGreen(rgb) / 255.0;
    rgba.b = qBlue(rgb) / 255.0;
    rgba.a = qAlpha(rgb) / 255.0 * m_alpha;
    return rgba;
}

ColorVector& Qwt3DColorMapColor::createVector(ColorVector& vec)
{
    rebuildColorVector(static_cast< unsigned >(m_colors.size()));
    vec = m_colors;
    return vec;
}

void Qwt3DColorMapColor::setColorMap(::QwtColorMap* map)
{
    if (map != m_colorMap) {
        delete m_colorMap;
        m_colorMap = map;
    }
    rebuildColorVector(static_cast< unsigned >(m_colors.size()));
    notifyColorChanged();
}

const ::QwtColorMap* Qwt3DColorMapColor::colorMap() const
{
    return m_colorMap;
}

void Qwt3DColorMapColor::setInterval(double min, double max)
{
    m_manualMin         = min;
    m_manualMax         = max;
    m_useManualInterval = true;
    rebuildColorVector(static_cast< unsigned >(m_colors.size()));
    notifyColorChanged();
}

void Qwt3DColorMapColor::reset(unsigned size)
{
    rebuildColorVector(size);
    notifyColorChanged();
}

void Qwt3DColorMapColor::setAlpha(double a)
{
    if (a < 0.0 || a > 1.0)
        return;
    m_alpha = a;
    rebuildColorVector(static_cast< unsigned >(m_colors.size()));
    notifyColorChanged();
}

void Qwt3DColorMapColor::rebuildColorVector(unsigned size)
{
    m_colors.resize(size);

    double zMin, zMax;
    if (m_useManualInterval) {
        zMin = m_manualMin;
        zMax = m_manualMax;
    } else if (m_plot) {
        const ParallelEpiped hull = m_plot->hull();
        zMin                      = hull.minVertex.z;
        zMax                      = hull.maxVertex.z;
    } else {
        zMin = 0.0;
        zMax = 1.0;
    }

    if (!m_colorMap)
        return;

    for (unsigned i = 0; i < size; ++i) {
        const double t = (size > 1) ? static_cast< double >(i) / (size - 1) : 0.0;
        const double z = zMin + t * (zMax - zMin);
        const QRgb rgb = m_colorMap->rgb(zMin, zMax, z);

        RGBA& rgba = m_colors[ i ];
        rgba.r     = qRed(rgb) / 255.0;
        rgba.g     = qGreen(rgb) / 255.0;
        rgba.b     = qBlue(rgb) / 255.0;
        rgba.a     = qAlpha(rgb) / 255.0 * m_alpha;
    }
}
