#ifndef QWT3D_IO_H
#define QWT3D_IO_H

#include <vector>
#include <algorithm>

#include <qstring.h>
#include <qstringlist.h>
#include "qwt3d_global.h"



class Qwt3DPlot;

/**
 * @brief Generic interface for standard and user written I/O handlers
 * @details IO provides a generic interface for standard and user written I/O handlers.
 *          It also provides functionality for the registering of such handlers in the
 *          framework. The interface mimics roughly Qt's QImageIO functions for defining
 *          image input/output functions.
 */
class QWT3D_EXPORT IO
{

public:
    // The function type that can be processed by the define... members
    using Function = bool (*)(Qwt3DPlot*, QString const& fname);

    /**
     * @brief Functor class for more flexible IO handler implementation
     * @details This class gives more flexibility in implementing userdefined IO handlers
     *          than the simple IO::Function type.
     */
    class Functor
    {
    public:
        virtual ~Functor()
        {
        }
        // Must clone the content of *this for an object of a derived class
        virtual Functor* clone() const = 0;
        // The workhorse of the user-defined implementation
        virtual bool operator()(Qwt3DPlot* plot, QString const& fname) = 0;
    };

    // Define an input handler for a format with a function
    static bool defineInputHandler(QString const& format, Function func);
    // Define an output handler for a format with a function
    static bool defineOutputHandler(QString const& format, Function func);
    // Define an input handler for a format with a functor
    static bool defineInputHandler(QString const& format, Functor const& func);
    // Define an output handler for a format with a functor
    static bool defineOutputHandler(QString const& format, Functor const& func);
    // Save plot to file in specified format
    static bool save(Qwt3DPlot*, QString const& fname, QString const& format);
    // Load plot from file in specified format
    static bool load(Qwt3DPlot*, QString const& fname, QString const& format);
    // Returns list of available input formats
    static QStringList inputFormatList();
    // Returns list of available output formats
    static QStringList outputFormatList();
    // Returns output handler for a format
    static Functor* outputHandler(QString const& format);
    // Returns input handler for a format
    static Functor* inputHandler(QString const& format);

private:
    IO()
    {
    }

    // Lightweight Functor encapsulating an IO::Function
    class Wrapper : public Functor
    {
    public:
        // Performs actual input
        Functor* clone() const override
        {
            return new Wrapper(*this);
        }
        // Creates a Wrapper object from a function pointer
        explicit Wrapper(Function h) : hdl(h)
        {
        }
        // Returns a pointer to the wrapped function
        bool operator()(Qwt3DPlot* plot, QString const& fname) override
        {
            return (hdl) ? (*hdl)(plot, fname) : false;
        }

    private:
        Function hdl;
    };

    struct Entry
    {
        Entry();
        ~Entry();

        Entry(Entry const& e);
        void operator=(Entry const& e);

        Entry(QString const& s, Functor const& f);
        Entry(QString const& s, Function f);

        QString fmt;
        Functor* iofunc;
    };

    struct FormatCompare
    {
        explicit FormatCompare(Entry const& e);
        bool operator()(Entry const& e);

        Entry e_;
    };

    struct FormatCompare2
    {
        explicit FormatCompare2(QString s);
        bool operator()(Entry const& e);

        QString s_;
    };

    using Container = std::vector< Entry >;
    using IT        = Container::iterator;

    static bool add_unique(Container& l, Entry const& e);
    static IT find(Container& l, QString const& fmt);
    static Container& rlist();
    static Container& wlist();
    static void setupHandler();
};

/**
 * @brief Provides Qt's Pixmap output facilities
 */
class QWT3D_EXPORT PixmapWriter : public IO::Functor
{
    friend class IO;
    QWT_DECLARE_PRIVATE(PixmapWriter)

public:
    PixmapWriter();
    ~PixmapWriter() override;

    // Set output quality
    void setQuality(int val);

private:
    IO::Functor* clone() const override;
    bool operator()(Qwt3DPlot* plot, QString const& fname) override;
};


#endif