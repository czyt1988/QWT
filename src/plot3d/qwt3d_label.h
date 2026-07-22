#ifndef QWT3D_LABEL_H
#define QWT3D_LABEL_H

#include <qpixmap.h>
#include <qimage.h>
#include <qfont.h>
#include <qpainter.h>
#include <qfontmetrics.h>

#include "qwt3d_drawable.h"
#include "qwt3d_io_gl2ps.h"



/**
 * @brief A Qt string or an output device dependent string
 * @details Qwt3DLabel provides text rendering on 3D plots, supporting both Qt string
 *          representation and device-dependent string output.
 */
class QWT3D_EXPORT Qwt3DLabel : public Qwt3DDrawable
{
    QWT_DECLARE_PRIVATE(Qwt3DLabel)

public:
    Qwt3DLabel();
    ~Qwt3DLabel() override;
    Qwt3DLabel(const Qwt3DLabel& other);
    Qwt3DLabel(Qwt3DLabel&& other) noexcept;
    Qwt3DLabel& operator=(const Qwt3DLabel& other);
    Qwt3DLabel& operator=(Qwt3DLabel&& other) noexcept;
    // Construct label and initialize with font
    Qwt3DLabel(const QString& family, int pointSize, int weight = QFont::Normal, bool italic = false);

    // Sets the labels font
    void setFont(QString const& family, int pointSize, int weight = QFont::Normal, bool italic = false);

    // Fine tunes label
    void adjust(int gap);
    // Returns the gap caused by adjust()
    double gap() const;
    // Sets the labels position
    void setPosition(Triple pos, ANCHOR a = BottomLeft);
    // Sets the labels position relative to screen
    void setRelPosition(Tuple rpos, ANCHOR a);
    // Receives bottom left label position
    Triple first() const;
    // Receives top right label position
    Triple second() const;
    // Defines an anchor point for the labels surrounding rectangle
    ANCHOR anchor() const;
    virtual void setColor(double r, double g, double b, double a = 1) override;
    virtual void setColor(RGBA rgba) override;

    // Sets the labels string
    void setString(QString const& s);
    // Actual drawing
    virtual void draw() override;

    // Decides about use of PDF standard fonts for PDF output
    static void useDeviceFonts(bool val);

private:
    void init();
    void init(const QString& family, int pointSize, int weight = QFont::Normal, bool italic = false);
    void update();
    void convert2screen();
    double width() const;
    double height() const;
};


#endif  // QWT3D_LABEL_H
