#include "qwt3d_io.h"

#include <QImageWriter>

#include <ctime>

#include "qwt3d_plot.h"
#include "qwt3d_io_gl2ps.h"
#include "qwt3d_io_reader.h"


class Qwt3DPixmapWriter::PrivateData
{
    QWT_DECLARE_PUBLIC(Qwt3DPixmapWriter)

public:
    PrivateData(Qwt3DPixmapWriter* q) : q_ptr(q), m_quality(-1)
    {
    }

    QString m_fmt;
    int m_quality;
};

Qwt3DPixmapWriter::Qwt3DPixmapWriter() : QWT_PIMPL_CONSTRUCT
{
}

Qwt3DPixmapWriter::~Qwt3DPixmapWriter() = default;

Qwt3DIO::Functor* Qwt3DPixmapWriter::clone() const
{
    auto* copy = new Qwt3DPixmapWriter();
    QWT_DC(d);
    auto* copyD      = copy->d_func();
    copyD->m_fmt     = d->m_fmt;
    copyD->m_quality = d->m_quality;
    return copy;
}

Qwt3DIO::Entry::Entry() : iofunc(nullptr)
{
}

Qwt3DIO::Entry::~Entry()
{
    delete iofunc;
}

Qwt3DIO::Entry::Entry(Qwt3DIO::Entry const& e)
{
    if (this == &e)
        return;

    fmt    = e.fmt;
    iofunc = e.iofunc->clone();
}

void Qwt3DIO::Entry::operator=(Qwt3DIO::Entry const& e)
{
    if (this == &e)
        return;

    delete iofunc;
    fmt    = e.fmt;
    iofunc = e.iofunc->clone();
}

Qwt3DIO::Entry::Entry(QString const& s, Functor const& f) : fmt(s)
{
    iofunc = f.clone();
}

Qwt3DIO::Entry::Entry(QString const& s, Function f) : fmt(s)
{
    Wrapper w(f);
    iofunc = w.clone();
}

Qwt3DIO::FormatCompare::FormatCompare(Qwt3DIO::Entry const& e)
{
    e_ = e;
}

bool Qwt3DIO::FormatCompare::operator()(Qwt3DIO::Entry const& e)
{
    return (e.fmt == e_.fmt);
}

Qwt3DIO::FormatCompare2::FormatCompare2(QString s)
{
    s_ = s;
}

bool Qwt3DIO::FormatCompare2::operator()(Qwt3DIO::Entry const& e)
{
    return (e.fmt == s_);
}

bool Qwt3DIO::add_unique(Container& l, Entry const& e)
{
    FormatCompare comp(e);
    l.erase(std::remove_if(l.begin(), l.end(), comp), l.end());
    l.push_back(e);

    return true;
}

Qwt3DIO::IT Qwt3DIO::find(Container& l, QString const& fmt)
{
    FormatCompare2 comp(fmt);
    return std::find_if(l.begin(), l.end(), comp);
}

Qwt3DIO::Container& Qwt3DIO::rlist()
{
    static Container rl = Container();
    static bool rfirst  = true;
    if (rfirst) {
        rfirst = false;
        setupHandler();
    }
    return rl;
}

Qwt3DIO::Container& Qwt3DIO::wlist()
{
    static Container wl = Container();
    static bool wfirst  = true;
    if (wfirst) {
        wfirst = false;
        setupHandler();
    }
    return wl;
}

/**
 * @brief Registers a new Qwt3DIO::Function for data input
 * @param format Format string identifier
 * @param func Input handler function
 * @return True on successful registration
 * @details Every call overwrites a formerly registered handler for the same format string (case sensitive).
 */
bool Qwt3DIO::defineInputHandler(QString const& format, Qwt3DIO::Function func)
{
    return add_unique(rlist(), Entry(format, func));
}

/**
 * @brief Registers a new Functor for data input
 * @param format Format string identifier
 * @param func Input handler functor
 * @return True on successful registration
 * @details Every call overwrites a formerly registered handler for the same format string (case sensitive).
 */
bool Qwt3DIO::defineInputHandler(QString const& format, Qwt3DIO::Functor const& func)
{
    return add_unique(rlist(), Entry(format, func));
}

/**
 * @brief Registers a new Qwt3DIO::Function for data output
 * @param format Format string identifier
 * @param func Output handler function
 * @return True on successful registration
 * @details Every call overwrites a formerly registered handler for the same format string (case sensitive).
 */
bool Qwt3DIO::defineOutputHandler(QString const& format, Qwt3DIO::Function func)
{
    return add_unique(wlist(), Entry(format, func));
}

/**
 * @brief Registers a new Functor for data output
 * @param format Format string identifier
 * @param func Output handler functor
 * @return True on successful registration
 * @details Every call overwrites a formerly registered handler for the same format string (case sensitive).
 */
bool Qwt3DIO::defineOutputHandler(QString const& format, Qwt3DIO::Functor const& func)
{
    return add_unique(wlist(), Entry(format, func));
}

/**
 * @brief Applies a reading Qwt3DIO::Function or Qwt3DIO::Functor
 * @param plot Plot with the content that should be loaded
 * @param fname File name
 * @param format Input format
 * @return The return value from the called Function/Functor.
 *         Returns false if no registered handler could be found.
 */
bool Qwt3DIO::load(Qwt3DPlot* plot, QString const& fname, QString const& format)
{
    IT it = Qwt3DIO::find(rlist(), format);

    if (it == rlist().end())
        return false;

    return (*it->iofunc)(plot, fname);
}

/**
 * @brief Applies a writing Qwt3DIO::Function or Qwt3DIO::Functor
 * @param plot Plot with the content that should be saved
 * @param fname File name
 * @param format Output format
 * @return The return value from the called Function/Functor.
 *         Returns false if no registered handler could be found.
 */
bool Qwt3DIO::save(Qwt3DPlot* plot, QString const& fname, QString const& format)
{
    IT it = Qwt3DIO::find(wlist(), format);

    if (it == wlist().end())
        return false;

    return (*it->iofunc)(plot, fname);
}

/**
 * @brief Returns a list of currently registered input formats
 * @return List of input format strings
 */
QStringList Qwt3DIO::inputFormatList()
{
    QStringList list;
    for (IT it = rlist().begin(); it != rlist().end(); ++it)
        list.append(it->fmt);

    return list;
}

/**
 * @brief Returns a list of currently registered output formats
 * @return List of output format strings
 */
QStringList Qwt3DIO::outputFormatList()
{
    QStringList list;
    for (IT it = wlist().begin(); it != wlist().end(); ++it)
        list.append(it->fmt);

    return list;
}

/**
 * @brief Returns the input functor in charge for format
 * @param format Format string identifier
 * @return Pointer to the input functor, or 0 if non-existent
 */
Qwt3DIO::Functor* Qwt3DIO::inputHandler(QString const& format)
{
    Qwt3DIO::IT it = Qwt3DIO::find(rlist(), format);

    if (it == rlist().end())
        return nullptr;

    return it->iofunc;
}

/**
 * @brief Returns the output functor in charge for format
 * @param format Format string identifier
 * @return Pointer to the output functor, or 0 if non-existent
 */
Qwt3DIO::Functor* Qwt3DIO::outputHandler(QString const& format)
{
    Qwt3DIO::IT it = Qwt3DIO::find(wlist(), format);

    if (it == wlist().end())
        return nullptr;

    return it->iofunc;
}

bool Qwt3DPixmapWriter::operator()(Qwt3DPlot* plot, QString const& fname)
{
    QWT_D(d);
    QImage im = plot->grabFramebuffer();

    QImageWriter iio;
    iio.setFormat(QWT3DLOCAL8BIT(d->m_fmt));
    iio.setQuality(d->m_quality);
    iio.setFileName(fname);
    return iio.write(im);
}

/**
 * @brief Calls Qt's QImageIO::setQuality() function
 * @param val Quality value
 */
void Qwt3DPixmapWriter::setQuality(int val)
{
    QWT_D(d);
    d->m_quality = val;
}

void Qwt3DIO::setupHandler()
{
    const QList< QByteArray > list = QImageWriter::supportedImageFormats();
    Qwt3DPixmapWriter qtw;
    for (const auto& fmt : list) {
        qtw.d_func()->m_fmt = fmt;
        defineOutputHandler(fmt, qtw);
    }
    Qwt3DVectorWriter vecfunc;
    vecfunc.setCompressed(false);
    vecfunc.setFormat("EPS");
    defineOutputHandler("EPS", vecfunc);
    vecfunc.setFormat("PS");
    defineOutputHandler("PS", vecfunc);

#ifdef GL2PS_HAVE_ZLIB
    vecfunc.setCompressed(true);
    vecfunc.setFormat("EPS_GZ");
    defineOutputHandler("EPS_GZ", vecfunc);
    vecfunc.setFormat("PS_GZ");
    defineOutputHandler("PS_GZ", vecfunc);
#endif
    vecfunc.setFormat("PDF");
    defineOutputHandler("PDF", vecfunc);
    vecfunc.setFormat("SVG");
    defineOutputHandler("SVG", vecfunc);
    vecfunc.setFormat("PGF");
    defineOutputHandler("PGF", vecfunc);

    defineInputHandler("mes", Qwt3DNativeReader());
    defineInputHandler("MES", Qwt3DNativeReader());
}

/**
 * @brief Writes vector data supported by gl2ps
 * @param fileName Output file name
 * @param format Output format ("EPS", "PS", "PDF", "SVG", or "PGF")
 * @param text Text handling mode
 * @param sortmode Sort mode for polygon ordering
 * @return True on success
 * @deprecated Use Qwt3DPlot::save or Qwt3DIO::save instead.
 * @details If zlib has been configured, format types will be extended by "EPS_GZ" and "PS_GZ".
 *          Beware: BSPSORT turns out to behave very slowly and memory consuming, especially in cases
 *          where many polygons appear. It is still more exact than SIMPLESORT.
 */
bool Qwt3DPlot::saveVector(QString const& fileName, QString const& format, Qwt3DVectorWriter::TEXTMODE text, Qwt3DVectorWriter::SORTMODE sortmode)
{
    if (format == "EPS" || format == "EPS_GZ" || format == "PS" || format == "PS_GZ" || format == "PDF"
        || format == "SVG" || format == "PGF") {
        Qwt3DVectorWriter* gl2ps = static_cast< Qwt3DVectorWriter* >(Qwt3DIO::outputHandler(format));
        if (gl2ps) {
            gl2ps->setSortMode(sortmode);
            gl2ps->setTextMode(text);
        }
        return Qwt3DIO::save(this, fileName, format);
    }
    return false;
}
/**
 * @brief Saves the framebuffer to an image file
 * @param fileName Output file name
 * @param format Image file format supported by Qt
 * @return True on success
 * @deprecated Use Qwt3DPlot::save or Qwt3DIO::save instead.
 */
bool Qwt3DPlot::savePixmap(QString const& fileName, QString const& format)
{
    if (format == "EPS" || format == "EPS_GZ" || format == "PS" || format == "PS_GZ" || format == "PDF"
        || format == "SVG" || format == "PGF")
        return false;

    return Qwt3DIO::save(this, fileName, format);
}

/**
 * @brief Saves content in one of the registered output formats
 * @param fileName Output file name
 * @param format Output format string
 * @return True on success
 * @details To modify the behaviour for more complex output handling use Qwt3DIO::outputHandler.
 */
bool Qwt3DPlot::save(QString const& fileName, QString const& format)
{
    return Qwt3DIO::save(this, fileName, format);
}
