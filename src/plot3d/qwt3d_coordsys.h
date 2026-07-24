#ifndef QWT3D_COORDSYS_H
#define QWT3D_COORDSYS_H

#include "qwt3d_axis.h"
#include "qwt3d_colorlegend.h"



/**
 * @brief A coordinate system with different styles (BOX, FRAME)
 */
class QWT3D_EXPORT Qwt3DCoordinateSystem : public Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DCoordinateSystem)

public:
    explicit Qwt3DCoordinateSystem(Triple blb = Triple(0, 0, 0),
                              Triple ftr = Triple(0, 0, 0),
                              COORDSTYLE = BOX);
    ~Qwt3DCoordinateSystem() override;

    void init(Triple beg = Triple(0, 0, 0), Triple end = Triple(0, 0, 0));
    // Set style for the coordinate system (NOCOORD, FRAME or BOX)
    void setStyle(COORDSTYLE s,
                  AXIS frame_1 = X1,
                  AXIS frame_2 = Y1,
                  AXIS frame_3 = Z1);
    // Return style of the coordinate system
    COORDSTYLE style() const;
    // first == front_left_bottom, second == back_right_top
    void setPosition(Triple first, Triple second);

    // Set common color for all axes
    void setAxesColor(RGBA val);
    // Set common font for all axis numberings
    void setNumberFont(QString const& family, int pointSize, int weight = QFont::Normal, bool italic = false);
    // Set common font for all axis numberings
    void setNumberFont(QFont const& font);
    // Set common color for all axis numberings
    void setNumberColor(RGBA val);
    // Sets an linear axis with real number items
    void setStandardScale();

    // Fine tunes distance between axis numbering and axis body
    void adjustNumbers(int val);
    // Fine tunes distance between axis label and axis body
    void adjustLabels(int val);

    // Sets color for the grid lines
    void setGridLinesColor(RGBA val);

    // Set common font for all axis labels
    void setLabelFont(QString const& family, int pointSize, int weight = QFont::Normal, bool italic = false);
    // Set common font for all axis labels
    void setLabelFont(QFont const& font);
    // Set common color for all axis labels
    void setLabelColor(RGBA val);

    // Set line width for tic marks and axes
    void setLineWidth(double val, double majfac = 0.9, double minfac = 0.5);
    // Set length for tic marks
    void setTicLength(double major, double minor);

    // Switch autoscaling of axes
    void setAutoScale(bool val = true);

    Triple first() const;
    Triple second() const;

    void setAutoDecoration(bool val = true);
    bool autoDecoration() const;

    // Set tick position preference for auto-decorated axes
    // TICK_BOTTOM (default): ticks on visually lower axis; TICK_TOP: ticks on visually upper axis
    void setTickPosition(TICKPOSITION val);
    // Returns tick position preference
    TICKPOSITION tickPosition() const;

    // Draw smooth axes
    void setLineSmooth(bool val = true);
    // Smooth axes enabled?
    bool lineSmooth() const;

    void draw() override;

    // Defines whether a grid between the major and/or minor tics should be drawn
    void setGridLines(bool majors, bool minors, int sides = NOSIDEGRID);
    // Returns grids switched on
    int grids() const;

    // The vector of all 12 axes - use them to set axis properties individually
    std::vector< Qwt3DAxis > axes;

private:
    void destroy();
    void chooseAxes();
    void autoDecorateExposedAxis(Qwt3DAxis& ax, bool left);
    void drawMajorGridLines();
    void drawMinorGridLines();
    void drawMajorGridLines(Qwt3DAxis&, Qwt3DAxis&, QVector<float>& verts);
    void drawMinorGridLines(Qwt3DAxis&, Qwt3DAxis&, QVector<float>& verts);
    void recalculateAxesTics();
    void drawGridLines(const QVector<float>& vertices, double lineWidth, const RGBA& color);
};


#endif
