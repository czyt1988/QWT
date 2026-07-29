#ifndef QWT3D_PLOT_P_H
#define QWT3D_PLOT_P_H

#include "qwt3d_plot.h"
#include "qwt3d_portability.h"

#include <QPoint>
#include <QMatrix4x4>
#include <QList>
#include <QOpenGLShaderProgram>
#include <memory>


class Qwt3DPlotItem;

class Qwt3DPlot::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DPlot)

public:
    PrivateData(Qwt3DPlot* q);

    struct Light
    {
        Light() : unlit(true)
        {
        }
        bool unlit;
        Triple rot;
        Triple shift;
    };

    Qwt3DCoordinateSystem m_coordinates;

    std::vector< Light > m_lights;

    double m_xRot, m_yRot, m_zRot;
    double m_xShift, m_yShift, m_zShift;
    double m_zoom;
    double m_xScale, m_yScale, m_zScale;
    double m_xVPShift, m_yVPShift;

    RGBA m_bgColor;
    bool m_ortho;
    ASPECTRATIOMODE m_aspectRatioMode = AUTOFILL;

    bool m_displayLegend;

    ParallelEpiped m_hull;

    Qwt3DColorLegend m_legend;

    Qwt3DLabel m_title;
    Tuple m_titleRel;
    ANCHOR m_titleAnchor;

    QPoint m_lastMouseMovePosition;
    bool m_pressed;

    Qwt3DMouseState m_xrotMState, m_yrotMState, m_zrotMState;
    Qwt3DMouseState m_xscaleMState, m_yscaleMState, m_zscaleMState;
    Qwt3DMouseState m_zoomMState;
    Qwt3DMouseState m_xshiftMState, m_yshiftMState;

    bool m_mouseInputEnabled;

    Qwt3DKeyboardState m_xrotKState[ 2 ], m_yrotKState[ 2 ], m_zrotKState[ 2 ];
    Qwt3DKeyboardState m_xscaleKState[ 2 ], m_yscaleKState[ 2 ], m_zscaleKState[ 2 ];
    Qwt3DKeyboardState m_zoomKState[ 2 ];
    Qwt3DKeyboardState m_xshiftKState[ 2 ], m_yshiftKState[ 2 ];

    bool m_kPressed;
    bool m_kbdInputEnabled;
    double m_kbdRotSpeed, m_kbdScaleSpeed, m_kbdShiftSpeed;

    bool m_lightingEnabled;
    bool m_initializedGL;
    bool m_renderPixmapRequest;

    Qwt3DTheme m_theme;

    // CPU-side transformation matrices
    QMatrix4x4 m_modelView;
    QMatrix4x4 m_projection;

    // Viewport dimensions
    int m_viewportWidth = 0;
    int m_viewportHeight = 0;

    // Shared generic shaders
    std::unique_ptr< QOpenGLShaderProgram > m_lineShader;
    std::unique_ptr< QOpenGLShaderProgram > m_pointShader;
    std::unique_ptr< QOpenGLShaderProgram > m_polygonShader;
    std::unique_ptr< QOpenGLShaderProgram > m_textShader;

    // Attached items list (sorted by z-order)
    QList< Qwt3DPlotItem* > m_items;
};


#endif  // QWT3D_PLOT_P_H
