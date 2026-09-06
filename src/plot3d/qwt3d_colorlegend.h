#ifndef QWT3D_COLORLEGEND_H
#define QWT3D_COLORLEGEND_H

#include "qwt3d_global.h"
#include "qwt3d_drawable.h"
#include "qwt3d_axis.h"
#include "qwt3d_color.h"

#include <QRectF>



/**
 * @brief A flat color legend
 * @details The class visualizes a ColorVector together with a scale (axis)
 *          and a caption. ColorLegends are vertical or horizontal.
 */
class QWT3D_EXPORT Qwt3DColorLegend : public Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DColorLegend)

public:
    // Possible anchor points for caption and axis
    enum SCALEPOSITION
    {
        Top,
        Bottom,
        Left,
        Right
    };

    // Orientation of the legend
    enum ORIENTATION
    {
        BottomTop,
        LeftRight
    };

    // Predefined screen positions for the legend
    enum Position
    {
        PosTopLeft,
        PosTopCenter,
        PosTopRight,
        PosLeftCenter,
        PosCenter,
        PosRightCenter,
        PosBottomLeft,
        PosBottomCenter,
        PosBottomRight,
        PosCustom
    };

    // Standard constructor
    Qwt3DColorLegend();
    ~Qwt3DColorLegend() override;

    // Draws the object - called by paintGL()
    void draw(const Qwt3DRenderContext& ctx) override;

    // Sets the relative position of the legend inside widget
    void setRelPosition(Tuple relMin, Tuple relMax);
    // Sets legend to a predefined screen position
    void setPosition(Position pos);
    // Sets legend position using absolute pixel coordinates (Qt coordinate system, origin top-left)
    void setAbsolutePosition(const QRectF& pixelRect);
    // Returns the current position mode
    Position position() const;
    // Returns the absolute pixel rect (only meaningful when useAbsolutePosition() is true)
    QRectF absolutePosition() const;
    // Returns true if absolute positioning is active
    bool useAbsolutePosition() const;
    // Returns the legend orientation
    ORIENTATION orientation() const;
    // Returns the scale position
    SCALEPOSITION scalePosition() const;
    // Returns whether the scale is drawn
    bool drawScale() const;
    // Returns the scale limits
    void limits(double& start, double& stop) const;
    // Returns the number of major tics
    int majors() const;
    // Returns the number of minor tics
    int minors() const;
    // Returns the legend caption string
    QString titleString() const;
    // Sets legend orientation and scale position
    void setOrientation(ORIENTATION, SCALEPOSITION);
    // Sets the limit of the scale
    void setLimits(double start, double stop);
    // Sets scale major tics
    void setMajors(int);
    // Sets scale minor tics
    void setMinors(int);
    // Sets whether a scale will be drawn
    void drawScale(bool val);
    // Sets whether the scale will have scale numbers
    void drawNumbers(bool val);
    // Sets whether the axis is autoscaled or not
    void setAutoScale(bool val);
    // Sets another scale
    void setScale(Qwt3DScale* scale);
    // Sets one of the predefined scale types
    void setScale(SCALETYPE);

    // Sets the legends caption string
    void setTitleString(QString const& s);

    // Sets the legends caption font
    void setTitleFont(QString const& family, int pointSize, int weight = QFont::Normal, bool italic = false);

    // The color vector
    ColorVector colors;

private:
    ParallelEpiped geometry() const;
    void setGeometryInternal(const Qwt3DRenderContext& ctx);
};


#endif
