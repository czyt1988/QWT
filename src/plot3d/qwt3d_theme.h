/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT3D_THEME_H
#define QWT3D_THEME_H

#include "qwt3d_global.h"
#include "qwt3d_types.h"

#include <qstring.h>
#include <qstringlist.h>

class QwtColorMap;



class Plot3D;

/**
 * @brief Complete visual theme descriptor for Plot3D widgets.
 * @details Qwt3DTheme encapsulates all visual properties of a 3D plot:
 *          background color, mesh styling, data colormap, coordinate system colors,
 *          title styling, lighting preset, shading mode, plot style, and material
 *          properties.
 */
class QWT3D_EXPORT Qwt3DTheme
{
public:
    enum Preset
    {
        Default,
        Dark,
        Scientific,
        Warm,
        Cool,
        Matplotlib,
        EarthTones,
        Ocean,
        HighContrast,
        Presentation
    };

    enum LightingPreset
    {
        NoLighting,
        FlatLight,
        Studio,
        Outdoor,
        Soft
    };

    Qwt3DTheme();
    explicit Qwt3DTheme(Preset preset);
    ~Qwt3DTheme();

    Qwt3DTheme(const Qwt3DTheme&);
    Qwt3DTheme& operator=(const Qwt3DTheme&);

    static Qwt3DTheme create(Preset preset);
    static Qwt3DTheme create(const QString& name);
    static QStringList availablePresets();

    // Background
    RGBA backgroundColor() const;
    void setBackgroundColor(RGBA);

    // Mesh
    RGBA meshColor() const;
    void setMeshColor(RGBA);
    double meshLineWidth() const;
    void setMeshLineWidth(double);
    bool smoothMesh() const;
    void setSmoothMesh(bool);

    // Qwt3DData Color
    QString dataColorPreset() const;
    void setDataColorPreset(const QString&);
    QwtColorMap* createColorMap() const;

    // Coordinate System
    RGBA axesColor() const;
    void setAxesColor(RGBA);
    RGBA numberColor() const;
    void setNumberColor(RGBA);
    RGBA labelColor() const;
    void setLabelColor(RGBA);
    RGBA gridLinesColor() const;
    void setGridLinesColor(RGBA);

    // Title
    RGBA titleColor() const;
    void setTitleColor(RGBA);
    QString titleFontFamily() const;
    void setTitleFontFamily(const QString&);
    int titleFontSize() const;
    void setTitleFontSize(int);
    bool titleFontBold() const;
    void setTitleFontBold(bool);

    // Lighting
    LightingPreset lightingPreset() const;
    void setLightingPreset(LightingPreset);

    // Shading
    SHADINGSTYLE shading() const;
    void setShading(SHADINGSTYLE);

    // Plot Style
    PLOTSTYLE plotStyle() const;
    void setPlotStyle(PLOTSTYLE);

    // Material
    double shininess() const;
    void setShininess(double);
    double specularIntensity() const;
    void setSpecularIntensity(double);

    // Apply to a plot
    void apply(Plot3D* plot) const;

private:
    RGBA m_backgroundColor;
    RGBA m_meshColor;
    double m_meshLineWidth;
    bool m_smoothMesh;
    QString m_dataColorPreset;
    RGBA m_axesColor;
    RGBA m_numberColor;
    RGBA m_labelColor;
    RGBA m_gridLinesColor;
    RGBA m_titleColor;
    QString m_titleFontFamily;
    int m_titleFontSize;
    bool m_titleFontBold;
    LightingPreset m_lightingPreset;
    SHADINGSTYLE m_shading;
    PLOTSTYLE m_plotStyle;
    double m_shininess;
    double m_specularIntensity;
};


#endif
