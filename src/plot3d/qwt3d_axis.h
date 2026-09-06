#ifndef QWT3D_AXIS_H
#define QWT3D_AXIS_H

#include <qwt_global.h>

#include "qwt3d_autoptr.h"
#include "qwt3d_label.h"
#include "qwt3d_scale.h"
#include "qwt3d_autoscaler.h"



/**
 * @brief Autoscalable axis with caption
 * @details Axes are highly customizable especially in terms
 *          of labeling and scaling.
 */
class QWT3D_EXPORT Qwt3DAxis : public Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DAxis)

public:
    // Constructs standard axis
    Qwt3DAxis();
    // Constructs a new axis with specified limits
    Qwt3DAxis(Triple beg, Triple end);
    // Destructor
    ~Qwt3DAxis() override;

    // Draws axis
    void draw(const Qwt3DRenderContext& ctx) override;

    // Positionate axis
    void setPosition(const Triple& beg, const Triple& end);
    // Returns axis' position
    void position(Triple& beg, Triple& end) const;
    // Returns axis' beginning position
    Triple begin() const;
    // Returns axis' ending position
    Triple end() const;
    // Returns axis' length
    double length() const;

    // Sets tics lengths in world coordinates
    void setTicLength(double majorl, double minorl);
    // Returns tics lengths
    void ticLength(double& majorl, double& minorl) const;
    // Sets tic orientation
    void setTicOrientation(double tx, double ty, double tz);
    // Same function as above
    void setTicOrientation(const Triple& val);
    // Returns tic orientation
    Triple ticOrientation() const;
    // Sets two-sided tics (default is false)
    void setSymmetricTics(bool b);

    // Sets font for axis label
    void setLabelFont(QString const& family, int pointSize, int weight = QFont::Normal, bool italic = false);
    // Sets font for axis label
    void setLabelFont(QFont const& font);
    // Returns current label font
    QFont const& labelFont() const;

    // Sets label content
    void setLabelString(QString const& name);
    // Returns label content
    QString labelString() const;
    void setLabelPosition(const Triple& pos, ANCHOR);
    void setLabelColor(RGBA col);
    // Returns the label color
    RGBA labelColor() const;
    // Turns label drawing on or off
    void setLabel(bool val);
    // Shifts label in device coordinates dependent on anchor
    void adjustLabel(int val);

    // Turns scale drawing on or off
    void setScaling(bool val);
    // Returns, if scale drawing is on or off
    bool scaling() const;
    void setScale(SCALETYPE);
    void setScale(Qwt3DScale* item);
    // Turns number drawing on or off
    void setNumbers(bool val);
    // Returns, if number drawing is on or off
    bool numbers() const;
    // Sets the color for axes numbers
    void setNumberColor(RGBA col);
    // Returns the color for axes numbers
    RGBA numberColor() const;
    // Sets font for numbering
    void setNumberFont(QString const& family, int pointSize, int weight = QFont::Normal, bool italic = false);
    // Overloaded member, works like the above function
    void setNumberFont(QFont const&);
    // Returns current numbering font
    QFont const& numberFont() const;
    // Sets anchor position for numbers
    void setNumberAnchor(ANCHOR a);
    // Shifts axis numbers in device coordinates dependent on anchor
    void adjustNumbers(int val);

    // Turns Autoscaling on or off
    void setAutoScale(bool val = true);
    // actual Autoscaling mode
    bool autoScale() const;

    // Requests major intervals (maybe changed, if autoscaling is present)
    void setMajors(int val);
    // Requests minor intervals
    void setMinors(int val);
    // Returns number of major intervals
    int majors() const;
    // Returns number of minor intervals
    int minors() const;
    // Returns positions for actual major tics (also if invisible)
    TripleField const& majorPositions() const;
    // Returns positions for actual minor tics (also if invisible)
    TripleField const& minorPositions() const;

    // Sets line width for axis components
    void setLineWidth(double val, double majfac = 0.9, double minfac = 0.5);
    // Returns line width for axis body
    double lineWidth() const;
    // Returns Line width for major tics
    double majLineWidth() const;
    // Returns Line width for minor tics
    double minLineWidth() const;

    // Sets interval
    void setLimits(double start, double stop);
    // Returns axis interval
    void limits(double& start, double& stop) const;
    // Enforces recalculation of ticmark positions
    void recalculateTics();

private:
    void init();
    void drawBase(const Qwt3DRenderContext& ctx);
    void drawTics(const Qwt3DRenderContext& ctx);
    void drawTicLabel(const Qwt3DRenderContext& ctx, Triple Pos, int mtic);
    Triple drawTic(Triple nadir, double length);
    void drawLabel(const Qwt3DRenderContext& ctx);
    bool prepTicCalculation(Triple& startpoint);

    Triple biggestNumberString(const Qwt3DRenderContext& ctx);

    void drawLines(const Qwt3DRenderContext& ctx, const QVector<float>& vertices, double lineWidth);
};


#endif
