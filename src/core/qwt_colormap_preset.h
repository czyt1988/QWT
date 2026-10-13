/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_COLORMAP_PRESET_H
#define QWT_COLORMAP_PRESET_H

#include "qwtcore_global.h"
#include "qwt_colormap.h"

#include <qcolor.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qpair.h>
#include <qvector.h>

#include <memory>

/**
 * @brief Registry of named color map presets for scientific visualization.
 * @details QwtColorMapPreset provides factory methods to create QwtLinearColorMap
 *          instances from well-known colormap names used in scientific computing
 *          and data visualization (matplotlib, MATLAB, etc.).
 *
 *          Each preset is defined by a set of color stops at specific positions
 *          in the [0.0, 1.0] range, enabling smooth multi-stop gradient interpolation.
 *
 * @par Example
 * @code
 *   QwtLinearColorMap cmap = QwtColorMapPreset::create("viridis");
 *   // Use cmap->rgb(0.0, 100.0, 50.0) to get the color at the midpoint
 * @endcode
 *
 * @sa QwtColorMap, QwtLinearColorMap
 */
class QWTCORE_EXPORT QwtColorMapPreset
{
public:
    /**
     * @brief Built-in colormap presets
     */
    enum Preset
    {
        //! Perceptually uniform purple-green-yellow (matplotlib default)
        Viridis,

        //! Perceptually uniform purple-orange-yellow
        Plasma,

        //! Perceptually uniform black-red-orange-yellow
        Inferno,

        //! Perceptually uniform black-purple-orange-yellow
        Magma,

        //! Perceptually uniform blue-gray-yellow (colorblind-friendly)
        Cividis,

        //! Blue-cyan-green-yellow-red (MATLAB default)
        Jet,

        //! Black-red-yellow-white
        Hot,

        //! Cyan-white
        Cool,

        //! Magenta-yellow
        Spring,

        //! Green-white
        Summer,

        //! Red-yellow-white
        Autumn,

        //! Blue-white
        Winter,

        //! Black-white grayscale
        Gray,

        //! Black-gray-white with blue tint
        Bone,

        //! Black-copper
        Copper,

        //! Red-yellow-green-cyan-blue-magenta-red
        Rainbow,

        //! Full HSV cycle
        Hsv,

        //! Improved rainbow (Google's turbo)
        Turbo,

        //! Blue-white-red diverging
        CoolWarm,

        //! Red-yellow-blue diverging
        RdYlBu,

        //! Red-yellow-green diverging
        RdYlGn,

        //! Multi-hue diverging
        Spectral
    };

    /// Create a QwtLinearColorMap from a preset enum
    static std::unique_ptr< QwtLinearColorMap > create(Preset preset);

    /// Create a QwtLinearColorMap from a preset name (case-insensitive)
    static std::unique_ptr< QwtLinearColorMap > create(const QString& name);

    /// Get a list of all available preset names
    static QStringList availablePresets();

    /// Get the color stops for a preset (position, color pairs)
    static QVector< QPair< double, QColor > > colorStops(Preset preset);

    /// Convert a preset enum to its string name
    static QString presetName(Preset preset);

    /// Convert a string name to a preset enum (unknown names fall back to Viridis)
    static Preset presetFromName(const QString& name);
};

#endif
