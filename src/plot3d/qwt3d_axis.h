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
class QWT3D_EXPORT Axis : public Drawable
{
    QWT_DECLARE_PRIVATE(Axis)

public:
    // Constructs standard axis
    Axis();
    // Constructs a new axis with specified limits
    Axis(Triple beg, Triple end);
    // Destructor
    ~Axis() override;

    // Draws axis
    void draw() override;

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
    void setLabelPosition(const Triple& pos, ANCHOR);
    void setLabelColor(RGBA col);
    // Turns label drawing on or off
    void setLabel(bool val);
    // Shifts label in device coordinates dependent on anchor
    void adjustLabel(int val);

    // Turns scale drawing on or off
    void setScaling(bool val);
    // Returns, if scale drawing is on or off
    bool scaling() const;
    void setScale(SCALETYPE);
    void setScale(Scale* item);
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
    void drawBase();
    void drawTics();
    void drawTicLabel(Triple Pos, int mtic);
    Triple drawTic(Triple nadir, double length);
    void drawLabel();
    bool prepTicCalculation(Triple& startpoint);

    Triple biggestNumberString();
};


#endif
