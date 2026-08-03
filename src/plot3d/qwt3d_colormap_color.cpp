/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt3d_colormap_color.h"
#include "qwt_colormap.h"
#include "qwt_colormap_preset.h"

#include <qcolor.h>
#include <qstring.h>


Qwt3DColorMapColor::Qwt3DColorMapColor(const QString& presetName, unsigned size)
    : m_colorMap(QwtColorMapPreset::create(presetName).release())
    , m_manualMin(0.0)
    , m_manualMax(1.0)
    , m_useManualInterval(false)
    , m_alpha(1.0)
    , m_presetName(presetName)
{
    rebuildColorVector(size);
}

Qwt3DColorMapColor::Qwt3DColorMapColor(::QwtColorMap* colorMap, unsigned size)
    : m_colorMap(colorMap), m_manualMin(0.0), m_manualMax(1.0), m_useManualInterval(false), m_alpha(1.0)
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
    } else {
        zMin = activeZMin();
        zMax = activeZMax();
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
    m_presetName.clear();
    if (map != m_colorMap) {
        delete m_colorMap;
        m_colorMap = map;
    }
    rebuildColorVector(static_cast< unsigned >(m_colors.size()));
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
}

void Qwt3DColorMapColor::reset(unsigned size)
{
    rebuildColorVector(size);
}

void Qwt3DColorMapColor::setAlpha(double a)
{
    if (a < 0.0 || a > 1.0)
        return;
    m_alpha = a;
    rebuildColorVector(static_cast< unsigned >(m_colors.size()));
}

void Qwt3DColorMapColor::rebuildColorVector(unsigned size)
{
    m_colors.resize(size);

    double zMin, zMax;
    if (m_useManualInterval) {
        zMin = m_manualMin;
        zMax = m_manualMax;
    } else {
        zMin = activeZMin();
        zMax = activeZMax();
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

/**
 * @brief 返回构造时传入的 preset 名称
 * @return preset 名称，如果通过 QwtColorMap* 构造则返回空字符串
 */
QString Qwt3DColorMapColor::presetName() const
{
    return m_presetName;
}

/**
 * @brief 返回颜色向量中的颜色数量
 * @return 颜色数量
 */
unsigned Qwt3DColorMapColor::colorCount() const
{
    return static_cast< unsigned >(m_colors.size());
}

/**
 * @brief 返回 alpha 值
 * @return alpha 值（0.0 ~ 1.0），默认 1.0
 */
double Qwt3DColorMapColor::alpha() const
{
    return m_alpha;
}

/**
 * @brief 返回是否使用了手动区间
 * @return true 如果通过 setInterval() 设置了手动区间
 */
bool Qwt3DColorMapColor::useManualInterval() const
{
    return m_useManualInterval;
}

/**
 * @brief 返回手动区间的最小值
 * @return 手动最小值（仅当 useManualInterval() 为 true 时有意义）
 */
double Qwt3DColorMapColor::manualMin() const
{
    return m_manualMin;
}

/**
 * @brief 返回手动区间的最大值
 * @return 手动最大值（仅当 useManualInterval() 为 true 时有意义）
 */
double Qwt3DColorMapColor::manualMax() const
{
    return m_manualMax;
}
