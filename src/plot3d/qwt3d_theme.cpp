/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt3d_theme.h"
#include "qwt3d_plot.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_colormap_color.h"
#include "qwt_colormap_preset.h"
#include "qwt_colormap.h"
#include "qwt3d_plotitem.h"
#include "qwt3d_surface.h"
#include "qwt3d_bar.h"
#include "qwt3d_line3d.h"

#include <qfont.h>

// GL constants for material/light properties (used in setMaterialComponent/setLightComponent)
#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif


Qwt3DTheme::Qwt3DTheme()
    : m_backgroundColor(1.0, 1.0, 1.0, 1.0)
    , m_meshColor(0.0, 0.0, 0.0, 1.0)
    , m_meshLineWidth(1.0)
    , m_smoothMesh(false)
    , m_dataColorPreset("viridis")
    , m_axesColor(0.0, 0.0, 0.0, 1.0)
    , m_numberColor(0.0, 0.0, 0.0, 1.0)
    , m_labelColor(0.0, 0.0, 0.0, 1.0)
    , m_gridLinesColor(0.2, 0.2, 0.2, 1.0)
    , m_interiorGridLinesColor(0.6, 0.6, 0.6, 0.5)
    , m_interiorGridMajorWidth(0.5)
    , m_interiorGridMinorWidth(0.3)
    , m_titleColor(0.0, 0.0, 0.0, 1.0)
    , m_titleFontFamily("Courier")
    , m_titleFontSize(16)
    , m_titleFontBold(true)
    , m_lightingPreset(NoLighting)
    , m_shading(GOURAUD)
    , m_plotStyle(FILLEDMESH)
    , m_shininess(5.0)
    , m_specularIntensity(0.3)
{
}

Qwt3DTheme::Qwt3DTheme(Preset preset) : Qwt3DTheme()
{
    *this = create(preset);
}

Qwt3DTheme::~Qwt3DTheme() = default;

Qwt3DTheme::Qwt3DTheme(const Qwt3DTheme&)            = default;
Qwt3DTheme& Qwt3DTheme::operator=(const Qwt3DTheme&) = default;

Qwt3DTheme Qwt3DTheme::create(Preset preset)
{
    Qwt3DTheme theme;

    switch (preset) {
    case Default:
        theme.m_dataColorPreset = "viridis";
        break;

    case Dark:
        theme.m_backgroundColor   = RGBA(0.15, 0.15, 0.15, 1.0);
        theme.m_meshColor         = RGBA(0.5, 0.5, 0.5, 1.0);
        theme.m_meshLineWidth     = 0.5;
        theme.m_smoothMesh        = true;
        theme.m_dataColorPreset   = "viridis";
        theme.m_axesColor         = RGBA(0.8, 0.8, 0.8, 1.0);
        theme.m_numberColor       = RGBA(0.8, 0.8, 0.8, 1.0);
        theme.m_labelColor        = RGBA(0.9, 0.9, 0.9, 1.0);
        theme.m_gridLinesColor    = RGBA(0.35, 0.35, 0.35, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.25, 0.25, 0.25, 0.5);
        theme.m_titleColor        = RGBA(0.95, 0.95, 0.95, 1.0);
        theme.m_lightingPreset    = Soft;
        theme.m_shininess         = 5.0;
        theme.m_specularIntensity = 0.1;
        break;

    case Scientific:
        theme.m_dataColorPreset   = "jet";
        theme.m_meshLineWidth     = 0.8;
        theme.m_titleFontFamily   = "Helvetica";
        theme.m_titleFontSize     = 14;
        theme.m_lightingPreset    = Studio;
        theme.m_shininess         = 10.0;
        theme.m_specularIntensity = 0.3;
        break;

    case Warm:
        theme.m_backgroundColor   = RGBA(0.98, 0.95, 0.90, 1.0);
        theme.m_meshColor         = RGBA(0.3, 0.15, 0.0, 1.0);
        theme.m_meshLineWidth     = 0.5;
        theme.m_dataColorPreset   = "hot";
        theme.m_axesColor         = RGBA(0.3, 0.15, 0.0, 1.0);
        theme.m_numberColor       = RGBA(0.3, 0.15, 0.0, 1.0);
        theme.m_labelColor        = RGBA(0.3, 0.15, 0.0, 1.0);
        theme.m_gridLinesColor    = RGBA(0.7, 0.55, 0.4, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.8, 0.65, 0.5, 0.5);
        theme.m_titleColor        = RGBA(0.3, 0.15, 0.0, 1.0);
        theme.m_lightingPreset    = FlatLight;
        theme.m_shininess         = 3.0;
        theme.m_specularIntensity = 0.1;
        break;

    case Cool:
        theme.m_backgroundColor   = RGBA(0.93, 0.96, 1.0, 1.0);
        theme.m_meshColor         = RGBA(0.0, 0.15, 0.35, 1.0);
        theme.m_meshLineWidth     = 0.5;
        theme.m_dataColorPreset   = "cool";
        theme.m_axesColor         = RGBA(0.0, 0.15, 0.35, 1.0);
        theme.m_numberColor       = RGBA(0.0, 0.15, 0.35, 1.0);
        theme.m_labelColor        = RGBA(0.0, 0.15, 0.35, 1.0);
        theme.m_gridLinesColor    = RGBA(0.5, 0.6, 0.75, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.6, 0.7, 0.85, 0.5);
        theme.m_titleColor        = RGBA(0.0, 0.15, 0.35, 1.0);
        theme.m_lightingPreset    = FlatLight;
        theme.m_shininess         = 3.0;
        theme.m_specularIntensity = 0.1;
        break;

    case Matplotlib:
        theme.m_meshColor         = RGBA(0.2, 0.2, 0.2, 1.0);
        theme.m_meshLineWidth     = 0.5;
        theme.m_dataColorPreset   = "viridis";
        theme.m_axesColor         = RGBA(0.2, 0.2, 0.2, 1.0);
        theme.m_numberColor       = RGBA(0.2, 0.2, 0.2, 1.0);
        theme.m_labelColor        = RGBA(0.2, 0.2, 0.2, 1.0);
        theme.m_gridLinesColor    = RGBA(0.75, 0.75, 0.75, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.85, 0.85, 0.85, 0.4);
        theme.m_titleColor        = RGBA(0.2, 0.2, 0.2, 1.0);
        theme.m_titleFontFamily   = "sans-serif";
        theme.m_titleFontSize     = 14;
        theme.m_lightingPreset    = Soft;
        theme.m_shininess         = 5.0;
        theme.m_specularIntensity = 0.2;
        break;

    case EarthTones:
        theme.m_backgroundColor   = RGBA(0.96, 0.94, 0.88, 1.0);
        theme.m_meshColor         = RGBA(0.35, 0.25, 0.15, 1.0);
        theme.m_meshLineWidth     = 0.5;
        theme.m_dataColorPreset   = "autumn";
        theme.m_axesColor         = RGBA(0.35, 0.25, 0.15, 1.0);
        theme.m_numberColor       = RGBA(0.35, 0.25, 0.15, 1.0);
        theme.m_labelColor        = RGBA(0.35, 0.25, 0.15, 1.0);
        theme.m_gridLinesColor    = RGBA(0.65, 0.55, 0.45, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.75, 0.65, 0.55, 0.5);
        theme.m_titleColor        = RGBA(0.35, 0.25, 0.15, 1.0);
        theme.m_lightingPreset    = Outdoor;
        theme.m_shininess         = 5.0;
        theme.m_specularIntensity = 0.2;
        break;

    case Ocean:
        theme.m_backgroundColor   = RGBA(0.90, 0.95, 1.0, 1.0);
        theme.m_meshColor         = RGBA(0.0, 0.1, 0.3, 1.0);
        theme.m_meshLineWidth     = 0.5;
        theme.m_dataColorPreset   = "winter";
        theme.m_axesColor         = RGBA(0.0, 0.1, 0.3, 1.0);
        theme.m_numberColor       = RGBA(0.0, 0.1, 0.3, 1.0);
        theme.m_labelColor        = RGBA(0.0, 0.1, 0.3, 1.0);
        theme.m_gridLinesColor    = RGBA(0.5, 0.65, 0.8, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.6, 0.75, 0.9, 0.5);
        theme.m_titleColor        = RGBA(0.0, 0.1, 0.3, 1.0);
        theme.m_lightingPreset    = FlatLight;
        theme.m_shininess         = 3.0;
        theme.m_specularIntensity = 0.1;
        break;

    case HighContrast:
        theme.m_backgroundColor = RGBA(0.0, 0.0, 0.0, 1.0);
        theme.m_meshColor       = RGBA(1.0, 1.0, 1.0, 1.0);
        theme.m_dataColorPreset = "gray";
        theme.m_axesColor       = RGBA(1.0, 1.0, 1.0, 1.0);
        theme.m_numberColor     = RGBA(1.0, 1.0, 1.0, 1.0);
        theme.m_labelColor      = RGBA(1.0, 1.0, 1.0, 1.0);
        theme.m_gridLinesColor  = RGBA(0.5, 0.5, 0.5, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.3, 0.3, 0.3, 0.5);
        theme.m_titleColor      = RGBA(1.0, 1.0, 1.0, 1.0);
        break;

    case Presentation:
        theme.m_meshLineWidth     = 1.5;
        theme.m_dataColorPreset   = "plasma";
        theme.m_gridLinesColor    = RGBA(0.6, 0.6, 0.6, 1.0);
        theme.m_interiorGridLinesColor = RGBA(0.7, 0.7, 0.7, 0.4);
        theme.m_titleFontFamily   = "Arial";
        theme.m_titleFontSize     = 20;
        theme.m_lightingPreset    = Studio;
        theme.m_shininess         = 10.0;
        theme.m_specularIntensity = 0.4;
        break;
    }
    return theme;
}

Qwt3DTheme Qwt3DTheme::create(const QString& name)
{
    const QString lower = name.toLower();
    if (lower == "default")
        return create(Default);
    if (lower == "dark")
        return create(Dark);
    if (lower == "scientific")
        return create(Scientific);
    if (lower == "warm")
        return create(Warm);
    if (lower == "cool")
        return create(Cool);
    if (lower == "matplotlib")
        return create(Matplotlib);
    if (lower == "earthtones" || lower == "earth")
        return create(EarthTones);
    if (lower == "ocean")
        return create(Ocean);
    if (lower == "highcontrast" || lower == "contrast")
        return create(HighContrast);
    if (lower == "presentation")
        return create(Presentation);
    return create(Default);
}

QStringList Qwt3DTheme::availablePresets()
{
    return { "Default",    "Dark",       "Scientific", "Warm",         "Cool",
             "Matplotlib", "EarthTones", "Ocean",      "HighContrast", "Presentation" };
}

// Getters/Setters
RGBA Qwt3DTheme::backgroundColor() const
{
    return m_backgroundColor;
}
void Qwt3DTheme::setBackgroundColor(RGBA c)
{
    m_backgroundColor = c;
}

RGBA Qwt3DTheme::meshColor() const
{
    return m_meshColor;
}
void Qwt3DTheme::setMeshColor(RGBA c)
{
    m_meshColor = c;
}

double Qwt3DTheme::meshLineWidth() const
{
    return m_meshLineWidth;
}
void Qwt3DTheme::setMeshLineWidth(double w)
{
    m_meshLineWidth = w;
}

bool Qwt3DTheme::smoothMesh() const
{
    return m_smoothMesh;
}
void Qwt3DTheme::setSmoothMesh(bool v)
{
    m_smoothMesh = v;
}

QString Qwt3DTheme::dataColorPreset() const
{
    return m_dataColorPreset;
}
void Qwt3DTheme::setDataColorPreset(const QString& name)
{
    m_dataColorPreset = name;
}

QwtColorMap* Qwt3DTheme::createColorMap() const
{
    return QwtColorMapPreset::create(m_dataColorPreset).release();
}

RGBA Qwt3DTheme::axesColor() const
{
    return m_axesColor;
}
void Qwt3DTheme::setAxesColor(RGBA c)
{
    m_axesColor = c;
}

RGBA Qwt3DTheme::numberColor() const
{
    return m_numberColor;
}
void Qwt3DTheme::setNumberColor(RGBA c)
{
    m_numberColor = c;
}

RGBA Qwt3DTheme::labelColor() const
{
    return m_labelColor;
}
void Qwt3DTheme::setLabelColor(RGBA c)
{
    m_labelColor = c;
}

RGBA Qwt3DTheme::gridLinesColor() const
{
    return m_gridLinesColor;
}
void Qwt3DTheme::setGridLinesColor(RGBA c)
{
    m_gridLinesColor = c;
}

RGBA Qwt3DTheme::interiorGridLinesColor() const
{
    return m_interiorGridLinesColor;
}
void Qwt3DTheme::setInteriorGridLinesColor(RGBA c)
{
    m_interiorGridLinesColor = c;
}

double Qwt3DTheme::interiorGridMajorWidth() const
{
    return m_interiorGridMajorWidth;
}
void Qwt3DTheme::setInteriorGridMajorWidth(double w)
{
    m_interiorGridMajorWidth = w;
}

double Qwt3DTheme::interiorGridMinorWidth() const
{
    return m_interiorGridMinorWidth;
}
void Qwt3DTheme::setInteriorGridMinorWidth(double w)
{
    m_interiorGridMinorWidth = w;
}

RGBA Qwt3DTheme::titleColor() const
{
    return m_titleColor;
}
void Qwt3DTheme::setTitleColor(RGBA c)
{
    m_titleColor = c;
}

QString Qwt3DTheme::titleFontFamily() const
{
    return m_titleFontFamily;
}
void Qwt3DTheme::setTitleFontFamily(const QString& f)
{
    m_titleFontFamily = f;
}

int Qwt3DTheme::titleFontSize() const
{
    return m_titleFontSize;
}
void Qwt3DTheme::setTitleFontSize(int s)
{
    m_titleFontSize = s;
}

bool Qwt3DTheme::titleFontBold() const
{
    return m_titleFontBold;
}
void Qwt3DTheme::setTitleFontBold(bool b)
{
    m_titleFontBold = b;
}

Qwt3DTheme::LightingPreset Qwt3DTheme::lightingPreset() const
{
    return m_lightingPreset;
}
void Qwt3DTheme::setLightingPreset(LightingPreset p)
{
    m_lightingPreset = p;
}

SHADINGSTYLE Qwt3DTheme::shading() const
{
    return m_shading;
}
void Qwt3DTheme::setShading(SHADINGSTYLE s)
{
    m_shading = s;
}

PLOTSTYLE Qwt3DTheme::plotStyle() const
{
    return m_plotStyle;
}
void Qwt3DTheme::setPlotStyle(PLOTSTYLE s)
{
    m_plotStyle = s;
}

double Qwt3DTheme::shininess() const
{
    return m_shininess;
}
void Qwt3DTheme::setShininess(double v)
{
    m_shininess = v;
}

double Qwt3DTheme::specularIntensity() const
{
    return m_specularIntensity;
}
void Qwt3DTheme::setSpecularIntensity(double v)
{
    m_specularIntensity = v;
}

void Qwt3DTheme::apply(Qwt3DPlot* plot) const
{
    if (!plot)
        return;

    plot->setBackgroundColor(m_backgroundColor);

    // Mesh color, line width, smooth mesh, data color, plot style, shading
    // are now item-level properties — they will be set on Qwt3DSurface items
    // in future versions. For now, only plot-level properties are applied.

    Qwt3DCoordinateSystem* coords = plot->coordinates();
    if (coords) {
        coords->setAxesColor(m_axesColor);
        coords->setNumberColor(m_numberColor);
        coords->setLabelColor(m_labelColor);
        coords->setGridLinesColor(m_gridLinesColor);
        coords->setInteriorGridLinesColor(m_interiorGridLinesColor);
        coords->setInteriorGridLinesWidth(m_interiorGridMajorWidth, m_interiorGridMinorWidth);
    }

    plot->setTitleColor(m_titleColor);
    plot->setTitleFont(m_titleFontFamily, m_titleFontSize, m_titleFontBold ? QFont::Bold : QFont::Normal);

    switch (m_lightingPreset) {
    case NoLighting:
        plot->disableLighting();
        break;
    case FlatLight:
        plot->enableLighting();
        plot->setLightComponent(GL_AMBIENT, 0.6, 0.6, 0.6);
        plot->setLightComponent(GL_DIFFUSE, 0.4, 0.4, 0.4);
        plot->setLightComponent(GL_SPECULAR, 0.0, 0.0, 0.0);
        break;
    case Studio:
        plot->enableLighting();
        plot->setLightComponent(GL_AMBIENT, 0.2, 0.2, 0.2);
        plot->setLightComponent(GL_DIFFUSE, 0.8, 0.8, 0.8);
        plot->setLightComponent(GL_SPECULAR, m_specularIntensity, m_specularIntensity, m_specularIntensity);
        plot->setLightRotation(45.0, 0.0, 0.0, 0);
        break;
    case Outdoor:
        plot->enableLighting();
        plot->setLightComponent(GL_AMBIENT, 0.3, 0.3, 0.35);
        plot->setLightComponent(GL_DIFFUSE, 1.0, 0.95, 0.8);
        plot->setLightComponent(GL_SPECULAR, m_specularIntensity, m_specularIntensity, m_specularIntensity);
        plot->setLightRotation(90.0, 0.0, 0.0, 0);
        break;
    case Soft:
        plot->enableLighting();
        plot->setLightComponent(GL_AMBIENT, 0.5, 0.5, 0.5);
        plot->setLightComponent(GL_DIFFUSE, 0.5, 0.5, 0.5);
        plot->setLightComponent(GL_SPECULAR, m_specularIntensity, m_specularIntensity, m_specularIntensity);
        break;
    }

    // Shading, plot style, shininess, material components are stored on CPU
    // for future shader uniform upload
    plot->setShininess(m_shininess);
    plot->setMaterialComponent(GL_SPECULAR, m_specularIntensity, m_specularIntensity, m_specularIntensity);
    plot->setMaterialComponent(GL_DIFFUSE, 1.0, 1.0, 1.0);

    // Apply item-level properties to each attached item
    for (auto* item : plot->itemList())
        applyToItem(item);

    plot->update();
}

/**
 * @brief Applies item-level visual properties to a single plot item
 * @param item The target item
 * @details Dispatches by concrete item type. Qwt3DSurface receives mesh color,
 *          line width, smooth mesh, data color (from preset), plot style, and
 *          shading. Qwt3DBar receives mesh color/line width, bar style (mapped
 *          from the theme's plot style), and the data color functor. Qwt3DLine
 *          receives the solid color and the data color functor.
 */
void Qwt3DTheme::applyToItem(Qwt3DPlotItem* item) const
{
    if (auto* surface = dynamic_cast<Qwt3DSurface*>(item)) {
        surface->setMeshColor(m_meshColor);
        surface->setMeshLineWidth(m_meshLineWidth);
        surface->setSmoothMesh(m_smoothMesh);
        surface->setPlotStyle(m_plotStyle);
        surface->setShading(m_shading);

        auto* colorMap = new Qwt3DColorMapColor(m_dataColorPreset);
        surface->setDataColor(colorMap);
        return;
    }

    if (auto* bar = dynamic_cast<Qwt3DBar*>(item)) {
        bar->setMeshColor(m_meshColor);
        bar->setMeshLineWidth(m_meshLineWidth);
        // Map the theme's PLOTSTYLE onto the closest bar style
        switch (m_plotStyle) {
        case WIREFRAME:
            bar->setBarStyle(Qwt3DBar::Wireframe);
            break;
        case FILLED:
            bar->setBarStyle(Qwt3DBar::Filled);
            break;
        default: // FILLEDMESH, HIDDENLINE, etc.
            bar->setBarStyle(Qwt3DBar::FilledMesh);
            break;
        }
        auto* colorMap = new Qwt3DColorMapColor(m_dataColorPreset);
        bar->setDataColor(colorMap);
        return;
    }

    if (auto* line = dynamic_cast<Qwt3DLine*>(item)) {
        line->setColor(m_meshColor);
        auto* colorMap = new Qwt3DColorMapColor(m_dataColorPreset);
        line->setDataColor(colorMap);
        return;
    }
}
