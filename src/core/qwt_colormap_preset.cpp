/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_colormap_preset.h"

// Color stop data sourced from matplotlib colormaps.
// Each preset defines key color stops at normalized positions [0.0, 1.0].
//
// The stops of viridis/plasma/inferno/magma/cividis/turbo are sampled from the
// official matplotlib 256 entry tables at exactly these positions - regenerate
// them with docs/assets/algorithms/colormap_stops_gen.py.
// The remaining presets are approximations with a few stops (their deviation
// from the matplotlib tables is documented in
// docs/zh/tech-report/colormap-color-science.md, section 5.2), and "rainbow"
// is a QWT specific closed hue ring, not the matplotlib table of that name.

static QVector< QPair< double, QColor > > viridisStops()
{
    return { { 0.000, QColor(68, 1, 84) },   { 0.125, QColor(71, 45, 123) },  { 0.250, QColor(59, 82, 139) },
             { 0.375, QColor(44, 114, 142) }, { 0.500, QColor(33, 145, 140) }, { 0.625, QColor(40, 174, 128) },
             { 0.750, QColor(94, 201, 98) },  { 0.875, QColor(173, 220, 48) }, { 1.000, QColor(253, 231, 37) } };
}

static QVector< QPair< double, QColor > > plasmaStops()
{
    return { { 0.000, QColor(13, 8, 135) },   { 0.125, QColor(75, 3, 161) },   { 0.250, QColor(125, 3, 168) },
             { 0.375, QColor(168, 34, 150) }, { 0.500, QColor(203, 70, 121) }, { 0.625, QColor(229, 107, 93) },
             { 0.750, QColor(248, 148, 65) }, { 0.875, QColor(253, 195, 40) }, { 1.000, QColor(240, 249, 33) } };
}

static QVector< QPair< double, QColor > > infernoStops()
{
    return { { 0.000, QColor(0, 0, 4) },      { 0.125, QColor(33, 12, 74) },   { 0.250, QColor(87, 16, 110) },
             { 0.375, QColor(138, 34, 106) }, { 0.500, QColor(188, 55, 84) },  { 0.625, QColor(228, 90, 49) },
             { 0.750, QColor(249, 142, 9) },  { 0.875, QColor(249, 203, 53) }, { 1.000, QColor(252, 255, 164) } };
}

static QVector< QPair< double, QColor > > magmaStops()
{
    return { { 0.000, QColor(0, 0, 4) },      { 0.125, QColor(29, 17, 71) },   { 0.250, QColor(81, 18, 124) },
             { 0.375, QColor(131, 38, 129) }, { 0.500, QColor(183, 55, 121) }, { 0.625, QColor(231, 82, 99) },
             { 0.750, QColor(252, 137, 97) }, { 0.875, QColor(254, 196, 136) }, { 1.000, QColor(252, 253, 191) } };
}

static QVector< QPair< double, QColor > > cividisStops()
{
    return { { 0.000, QColor(0, 34, 78) },    { 0.125, QColor(26, 56, 111) },  { 0.250, QColor(67, 78, 108) },
             { 0.375, QColor(97, 101, 111) }, { 0.500, QColor(125, 124, 120) }, { 0.625, QColor(155, 148, 118) },
             { 0.750, QColor(188, 174, 108) }, { 0.875, QColor(222, 201, 88) }, { 1.000, QColor(254, 232, 56) } };
}

static QVector< QPair< double, QColor > > jetStops()
{
    return { { 0.000, QColor(0, 0, 128) },   { 0.125, QColor(0, 0, 255) },     { 0.250, QColor(0, 128, 255) },
             { 0.375, QColor(0, 255, 255) }, { 0.500, QColor(128, 255, 128) }, { 0.625, QColor(255, 255, 0) },
             { 0.750, QColor(255, 128, 0) }, { 0.875, QColor(255, 0, 0) },     { 1.000, QColor(128, 0, 0) } };
}

static QVector< QPair< double, QColor > > hotStops()
{
    return { { 0.000, QColor(0, 0, 0) },
             { 0.333, QColor(255, 0, 0) },
             { 0.667, QColor(255, 255, 0) },
             { 1.000, QColor(255, 255, 255) } };
}

static QVector< QPair< double, QColor > > coolStops()
{
    return { { 0.0, QColor(0, 255, 255) }, { 1.0, QColor(255, 0, 255) } };
}

static QVector< QPair< double, QColor > > springStops()
{
    return { { 0.0, QColor(255, 0, 255) }, { 1.0, QColor(255, 255, 0) } };
}

static QVector< QPair< double, QColor > > summerStops()
{
    return { { 0.0, QColor(0, 128, 102) }, { 1.0, QColor(255, 255, 102) } };
}

static QVector< QPair< double, QColor > > autumnStops()
{
    return { { 0.0, QColor(255, 0, 0) }, { 1.0, QColor(255, 255, 0) } };
}

static QVector< QPair< double, QColor > > winterStops()
{
    return { { 0.0, QColor(0, 0, 255) }, { 1.0, QColor(0, 255, 128) } };
}

static QVector< QPair< double, QColor > > grayStops()
{
    return { { 0.0, QColor(0, 0, 0) }, { 1.0, QColor(255, 255, 255) } };
}

static QVector< QPair< double, QColor > > boneStops()
{
    return { { 0.000, QColor(0, 0, 0) },
             { 0.333, QColor(54, 54, 82) },
             { 0.667, QColor(128, 155, 155) },
             { 1.000, QColor(255, 255, 255) } };
}

static QVector< QPair< double, QColor > > copperStops()
{
    return { { 0.0, QColor(0, 0, 0) }, { 1.0, QColor(255, 199, 127) } };
}

static QVector< QPair< double, QColor > > rainbowStops()
{
    return { { 0.000, QColor(255, 0, 0) },   { 0.167, QColor(255, 255, 0) }, { 0.333, QColor(0, 255, 0) },
             { 0.500, QColor(0, 255, 255) }, { 0.667, QColor(0, 0, 255) },   { 0.833, QColor(255, 0, 255) },
             { 1.000, QColor(255, 0, 0) } };
}

static QVector< QPair< double, QColor > > hsvStops()
{
    return { { 0.000, QColor(255, 0, 0) },   { 0.167, QColor(255, 255, 0) }, { 0.333, QColor(0, 255, 0) },
             { 0.500, QColor(0, 255, 255) }, { 0.667, QColor(0, 0, 255) },   { 0.833, QColor(255, 0, 255) },
             { 1.000, QColor(255, 0, 0) } };
}

static QVector< QPair< double, QColor > > turboStops()
{
    return { { 0.000, QColor(48, 18, 59) },   { 0.125, QColor(70, 107, 227) }, { 0.250, QColor(40, 188, 235) },
             { 0.375, QColor(50, 242, 152) }, { 0.500, QColor(164, 252, 60) }, { 0.625, QColor(238, 207, 58) },
             { 0.750, QColor(251, 126, 33) }, { 0.875, QColor(208, 47, 5) },  { 1.000, QColor(122, 4, 3) } };
}

static QVector< QPair< double, QColor > > coolWarmStops()
{
    return { { 0.000, QColor(59, 76, 192) },
             { 0.250, QColor(124, 159, 249) },
             { 0.500, QColor(221, 221, 221) },
             { 0.750, QColor(237, 133, 104) },
             { 1.000, QColor(180, 4, 38) } };
}

static QVector< QPair< double, QColor > > rdYlBuStops()
{
    return { { 0.000, QColor(165, 0, 38) },    { 0.100, QColor(215, 48, 39) },   { 0.200, QColor(244, 109, 67) },
             { 0.300, QColor(253, 174, 97) },  { 0.400, QColor(254, 224, 144) }, { 0.500, QColor(255, 255, 191) },
             { 0.600, QColor(224, 243, 248) }, { 0.700, QColor(171, 217, 233) }, { 0.800, QColor(116, 173, 209) },
             { 0.900, QColor(69, 117, 180) },  { 1.000, QColor(49, 54, 149) } };
}

static QVector< QPair< double, QColor > > rdYlGnStops()
{
    return { { 0.000, QColor(165, 0, 38) },    { 0.100, QColor(215, 48, 39) },   { 0.200, QColor(244, 109, 67) },
             { 0.300, QColor(253, 174, 97) },  { 0.400, QColor(254, 224, 139) }, { 0.500, QColor(255, 255, 191) },
             { 0.600, QColor(217, 239, 139) }, { 0.700, QColor(166, 217, 106) }, { 0.800, QColor(102, 189, 99) },
             { 0.900, QColor(26, 152, 80) },   { 1.000, QColor(0, 104, 55) } };
}

static QVector< QPair< double, QColor > > spectralStops()
{
    return { { 0.000, QColor(158, 1, 66) },    { 0.100, QColor(213, 62, 79) },   { 0.200, QColor(244, 109, 67) },
             { 0.300, QColor(253, 174, 97) },  { 0.400, QColor(254, 224, 139) }, { 0.500, QColor(255, 255, 191) },
             { 0.600, QColor(230, 245, 152) }, { 0.700, QColor(171, 221, 164) }, { 0.800, QColor(102, 194, 165) },
             { 0.900, QColor(50, 136, 189) },  { 1.000, QColor(94, 79, 162) } };
}

/**
 * @brief Get the color stops for a preset
 * @param preset Preset selector
 * @return Vector of (position, color) pairs
 */
QVector< QPair< double, QColor > > QwtColorMapPreset::colorStops(Preset preset)
{
    switch (preset) {
    case Viridis:
        return viridisStops();
    case Plasma:
        return plasmaStops();
    case Inferno:
        return infernoStops();
    case Magma:
        return magmaStops();
    case Cividis:
        return cividisStops();
    case Jet:
        return jetStops();
    case Hot:
        return hotStops();
    case Cool:
        return coolStops();
    case Spring:
        return springStops();
    case Summer:
        return summerStops();
    case Autumn:
        return autumnStops();
    case Winter:
        return winterStops();
    case Gray:
        return grayStops();
    case Bone:
        return boneStops();
    case Copper:
        return copperStops();
    case Rainbow:
        return rainbowStops();
    case Hsv:
        return hsvStops();
    case Turbo:
        return turboStops();
    case CoolWarm:
        return coolWarmStops();
    case RdYlBu:
        return rdYlBuStops();
    case RdYlGn:
        return rdYlGnStops();
    case Spectral:
        return spectralStops();
    }
    return viridisStops();
}

/**
 * @brief Create a QwtLinearColorMap from a preset
 * @param preset Preset selector
 * @return A QwtLinearColorMap with the preset's color stops applied
 */
std::unique_ptr< QwtLinearColorMap > QwtColorMapPreset::create(Preset preset)
{
    auto map         = qwt_make_unique< QwtLinearColorMap >();
    const auto stops = colorStops(preset);
    if (stops.isEmpty())
        return map;

    map->setColorInterval(stops.first().second, stops.last().second);
    for (int i = 1; i < stops.size() - 1; ++i)
        map->addColorStop(stops[ i ].first, stops[ i ].second);

    return map;
}

/**
 * @brief Create a QwtLinearColorMap from a preset name (case-insensitive)
 * @param name Preset name string
 * @return A QwtLinearColorMap with the preset's color stops applied
 */
std::unique_ptr< QwtLinearColorMap > QwtColorMapPreset::create(const QString& name)
{
    return create(presetFromName(name));
}

/**
 * @brief Get a list of all available preset names
 * @return List of preset name strings
 */
QStringList QwtColorMapPreset::availablePresets()
{
    return { "viridis", "plasma", "inferno",  "magma",  "cividis", "jet",     "hot",    "cool",
             "spring",  "summer", "autumn",   "winter", "gray",    "bone",    "copper", "rainbow",
             "hsv",     "turbo",  "coolwarm", "rdylbu", "rdylgn",  "spectral" };
}

/**
 * @brief Convert a preset enum to its string name
 * @param preset Preset selector
 * @return Lowercase name string
 */
QString QwtColorMapPreset::presetName(Preset preset)
{
    switch (preset) {
    case Viridis:
        return "viridis";
    case Plasma:
        return "plasma";
    case Inferno:
        return "inferno";
    case Magma:
        return "magma";
    case Cividis:
        return "cividis";
    case Jet:
        return "jet";
    case Hot:
        return "hot";
    case Cool:
        return "cool";
    case Spring:
        return "spring";
    case Summer:
        return "summer";
    case Autumn:
        return "autumn";
    case Winter:
        return "winter";
    case Gray:
        return "gray";
    case Bone:
        return "bone";
    case Copper:
        return "copper";
    case Rainbow:
        return "rainbow";
    case Hsv:
        return "hsv";
    case Turbo:
        return "turbo";
    case CoolWarm:
        return "coolwarm";
    case RdYlBu:
        return "rdylbu";
    case RdYlGn:
        return "rdylgn";
    case Spectral:
        return "spectral";
    }
    return "viridis";
}

/**
 * @brief Convert a string name to a preset enum
 * @param name Case-insensitive preset name
 * @return Preset enum value, or Viridis if not found
 */
QwtColorMapPreset::Preset QwtColorMapPreset::presetFromName(const QString& name)
{
    const QString lower = name.toLower();
    if (lower == "viridis")
        return Viridis;
    if (lower == "plasma")
        return Plasma;
    if (lower == "inferno")
        return Inferno;
    if (lower == "magma")
        return Magma;
    if (lower == "cividis")
        return Cividis;
    if (lower == "jet")
        return Jet;
    if (lower == "hot")
        return Hot;
    if (lower == "cool")
        return Cool;
    if (lower == "spring")
        return Spring;
    if (lower == "summer")
        return Summer;
    if (lower == "autumn")
        return Autumn;
    if (lower == "winter")
        return Winter;
    if (lower == "gray")
        return Gray;
    if (lower == "bone")
        return Bone;
    if (lower == "copper")
        return Copper;
    if (lower == "rainbow")
        return Rainbow;
    if (lower == "hsv")
        return Hsv;
    if (lower == "turbo")
        return Turbo;
    if (lower == "coolwarm")
        return CoolWarm;
    if (lower == "rdylbu")
        return RdYlBu;
    if (lower == "rdylgn")
        return RdYlGn;
    if (lower == "spectral")
        return Spectral;
    return Viridis;
}
