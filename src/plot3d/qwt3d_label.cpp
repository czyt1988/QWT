#include <qbitmap.h>
#include "qwt3d_label.h"


namespace
{
bool deviceFonts = false;
}

class Label::PrivateData
{
    QWT_DECLARE_PUBLIC(Label)

public:
    PrivateData(Label* q)
        : q_ptr(q)
        , m_beg(0.0, 0.0, 0.0)
        , m_end(0.0, 0.0, 0.0)
        , m_pos(0.0, 0.0, 0.0)
        , m_pm(0, 0)
        , m_font()
        , m_anchor(BottomLeft)
        , m_gap(0)
        , m_flagForUpdate(true)
    {
    }

    Triple m_beg;
    Triple m_end;
    Triple m_pos;
    QPixmap m_pm;
    QImage m_buf;
    QImage m_tex;
    QFont m_font;
    QString m_text;
    ANCHOR m_anchor;
    int m_gap;
    bool m_flagForUpdate;
};

/**
 * @brief Default constructor
 */
Label::Label() : QWT_PIMPL_CONSTRUCT
{
    init();
}

/**
 * @brief Constructs a Label with specified font parameters
 * @param family Font family name
 * @param pointSize Font point size
 * @param weight Font weight
 * @param italic Whether font is italic
 */
Label::Label(const QString& family, int pointSize, int weight, bool italic) : QWT_PIMPL_CONSTRUCT
{
    init(family, pointSize, weight, italic);
}

/**
 * @brief Destructor
 */
Label::~Label() = default;

/**
 * @brief Copy constructor
 */
Label::Label(const Label& other) : Drawable(), QWT_PIMPL_CONSTRUCT
{
    QWT_D(d);
    const PrivateData* od = other.d_func();
    d->m_beg              = od->m_beg;
    d->m_end              = od->m_end;
    d->m_pos              = od->m_pos;
    d->m_pm               = od->m_pm;
    d->m_buf              = od->m_buf;
    d->m_tex              = od->m_tex;
    d->m_font             = od->m_font;
    d->m_text             = od->m_text;
    d->m_anchor           = od->m_anchor;
    d->m_gap              = od->m_gap;
    d->m_flagForUpdate    = od->m_flagForUpdate;
    color                 = other.color;
}

/**
 * @brief Move constructor
 */
Label::Label(Label&& other) noexcept : Drawable(std::move(other)), m_data(std::move(other.m_data))
{
}

/**
 * @brief Copy assignment operator
 */
Label& Label::operator=(const Label& other)
{
    if (this != &other) {
        QWT_D(d);
        const PrivateData* od = other.d_func();
        d->m_beg              = od->m_beg;
        d->m_end              = od->m_end;
        d->m_pos              = od->m_pos;
        d->m_pm               = od->m_pm;
        d->m_buf              = od->m_buf;
        d->m_tex              = od->m_tex;
        d->m_font             = od->m_font;
        d->m_text             = od->m_text;
        d->m_anchor           = od->m_anchor;
        d->m_gap              = od->m_gap;
        d->m_flagForUpdate    = od->m_flagForUpdate;
        color                 = other.color;
    }
    return *this;
}

/**
 * @brief Move assignment operator
 */
Label& Label::operator=(Label&& other) noexcept
{
    if (this != &other) {
        Drawable::operator=(std::move(other));
        m_data = std::move(other.m_data);
    }
    return *this;
}

void Label::init(const QString& family, int pointSize, int weight, bool italic)
{
    init();
    QWT_D(d);
    d->m_font = QFont(family, pointSize, weight, italic);
}

void Label::init()
{
    QWT_D(d);
    d->m_beg = Triple(0.0, 0.0, 0.0);
    d->m_end = d->m_beg;
    d->m_pos = d->m_beg;
    setColor(0, 0, 0);
    d->m_pm            = QPixmap(0, 0);
    d->m_font          = QFont();
    d->m_anchor        = BottomLeft;
    d->m_gap           = 0;
    d->m_flagForUpdate = true;
}

/**
 * @brief Enables or disables device font rendering for all labels
 * @param val True to use device fonts, false to use Qt-based rendering
 */
void Label::useDeviceFonts(bool val)
{
    deviceFonts = val;
}

/**
 * @brief Sets the label font
 * @param family Font family name
 * @param pointSize Font point size
 * @param weight Font weight
 * @param italic Whether font is italic
 */
void Label::setFont(const QString& family, int pointSize, int weight, bool italic)
{
    QWT_D(d);
    d->m_font          = QFont(family, pointSize, weight, italic);
    d->m_flagForUpdate = true;
}

/**
 * @brief Sets the label text string
 * @param s Text string to display
 */
void Label::setString(QString const& s)
{
    QWT_D(d);
    d->m_text          = s;
    d->m_flagForUpdate = true;
}

/**
 * @brief Sets the label color from RGBA components
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param a Alpha component
 */
void Label::setColor(double r, double g, double b, double a)
{
    Drawable::setColor(r, g, b, a);
    QWT_D(d);
    d->m_flagForUpdate = true;
}

/**
 * @brief Sets the label color from an RGBA object
 * @param rgba RGBA color value
 */
void Label::setColor(RGBA rgba)
{
    Drawable::setColor(rgba);
    QWT_D(d);
    d->m_flagForUpdate = true;
}

/**
 * @brief Sets the label position and anchor point
 * @param pos Position triple in world coordinates
 * @param a Anchor type defining how the label aligns relative to pos
 * @details Anchor example:
 *          TopCenter (*) resp. BottomRight (X):
 *          +----*----+
 *          |  Pixmap |
 *          +---------X
 */
void Label::setPosition(Triple pos, ANCHOR a)
{
    QWT_D(d);
    d->m_anchor = a;
    d->m_pos    = pos;
}

/**
 * @brief Sets the label position relative to the viewport
 * @param rpos Relative position tuple (x,y)
 * @param a Anchor type defining how the label aligns
 */
void Label::setRelPosition(Tuple rpos, ANCHOR a)
{
    QWT_D(d);
    double ot = 0.99;

    getMatrices(modelMatrix, projMatrix, viewport);
    d->m_beg = relativePosition(Triple(rpos.x, rpos.y, ot));
    setPosition(d->m_beg, a);
}

void Label::update()
{
    QWT_D(d);
    QPainter p;
    QFontMetrics fm(d->m_font);

    QFontInfo info(d->m_font);

    QRect r = QRect(QPoint(0, 0), fm.size(SingleLine, d->m_text));  // fm.boundingRect(text_)  misbehaviour under linux;

    r.translate(0, -r.top());

    d->m_pm = QPixmap(r.width(), r.bottom());

    if (d->m_pm.isNull())  // else crash under linux
    {
        r = QRect(QPoint(0, 0), fm.size(SingleLine, QString(" ")));  // draw empty space else //todo
        r.translate(0, -r.top());
        d->m_pm = QPixmap(r.width(), r.bottom());
    }

    QBitmap bm(d->m_pm.width(), d->m_pm.height());
    bm.fill(Qt::color0);
    p.begin(&bm);
    p.setPen(Qt::color1);
    p.setFont(d->m_font);
    p.drawText(0, r.height() - fm.descent() - 1, d->m_text);
    p.end();

    d->m_pm.setMask(bm);

    p.begin(&d->m_pm);
    p.setFont(d->m_font);
    p.setPen(Qt::SolidLine);
    p.setPen(GL2Qt(color.r, color.g, color.b));

    p.drawText(0, r.height() - fm.descent() - 1, d->m_text);
    p.end();
    d->m_buf = d->m_pm.toImage();
    d->m_tex = d->m_buf.mirrored();
}

/**
 * @brief Adds an additional shift to the anchor point
 * @param gap Gap value in pixels
 * @details The shift direction depends on the anchor type:
 *          left aligned -->, right aligned <--, top aligned top-down,
 *          bottom aligned bottom-up. The unit is user space dependent
 *          (one pixel on screen - play around to get satisfying results).
 */
void Label::adjust(int gap)
{
    QWT_D(d);
    d->m_gap = gap;
}

void Label::convert2screen()
{
    QWT_D(d);
    Triple start = World2ViewPort(d->m_pos);

    switch (d->m_anchor) {
    case BottomLeft:
        d->m_beg = d->m_pos;
        break;
    case BottomRight:
        d->m_beg = ViewPort2World(start - Triple(width() + d->m_gap, 0, 0));
        break;
    case BottomCenter:
        d->m_beg = ViewPort2World(start - Triple(width() / 2, -d->m_gap, 0));
        break;
    case TopRight:
        d->m_beg = ViewPort2World(start - Triple(width() + d->m_gap, height(), 0));
        break;
    case TopLeft:
        d->m_beg = ViewPort2World(start - Triple(-d->m_gap, height(), 0));
        break;
    case TopCenter:
        d->m_beg = ViewPort2World(start - Triple(width() / 2, height() + d->m_gap, 0));
        break;
    case CenterLeft:
        d->m_beg = ViewPort2World(start - Triple(-d->m_gap, height() / 2, 0));
        break;
    case CenterRight:
        d->m_beg = ViewPort2World(start - Triple(width() + d->m_gap, height() / 2, 0));
        break;
    case Center:
        d->m_beg = ViewPort2World(start - Triple(width() / 2, height() / 2, 0));
        break;
    default:
        break;
    }
    start    = World2ViewPort(d->m_beg);
    d->m_end = ViewPort2World(start + Triple(width(), height(), 0));
}

/**
 * @brief Draws the label
 */
void Label::draw()
{
    QWT_D(d);
    if (d->m_flagForUpdate) {
        update();
        d->m_flagForUpdate = false;
    }

    if (d->m_buf.isNull())
        return;

    GLboolean b;
    GLint func;
    GLdouble v;
    glGetBooleanv(GL_ALPHA_TEST, &b);
    glGetIntegerv(GL_ALPHA_TEST_FUNC, &func);
    glGetDoublev(GL_ALPHA_TEST_REF, &v);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_NOTEQUAL, 0.0);

    convert2screen();
    glRasterPos3d(d->m_beg.x, d->m_beg.y, d->m_beg.z);

    int w = d->m_tex.width();
    int h = d->m_tex.height();

    if (deviceFonts) {
        drawDeviceText(QWT3DLOCAL8BIT(d->m_text), "Courier", d->m_font.pointSize(), d->m_pos, color, d->m_anchor, d->m_gap);
    } else {
        drawDevicePixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, d->m_tex.bits());
    }

    glAlphaFunc(func, v);
    Enable(GL_ALPHA_TEST, b);
}

/**
 * @brief Returns the label width in pixels
 * @return Label pixmap width
 */
double Label::width() const
{
    QWT_DC(d);
    return d->m_pm.width();
}

/**
 * @brief Returns the label height in pixels
 * @return Label pixmap height
 */
double Label::height() const
{
    QWT_DC(d);
    return d->m_pm.height();
}

double Label::gap() const
{
    QWT_DC(d);
    return d->m_gap;
}

Triple Label::first() const
{
    QWT_DC(d);
    return d->m_beg;
}

Triple Label::second() const
{
    QWT_DC(d);
    return d->m_end;
}

ANCHOR Label::anchor() const
{
    QWT_DC(d);
    return d->m_anchor;
}
