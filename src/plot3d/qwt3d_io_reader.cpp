#if defined(_MSC_VER) /* MSVC Compiler */
#pragma warning(disable : 4786)
#endif

#include "qwt3d_io_reader.h"

#include <qtextstream.h>

#include <cfloat>
#include <cstdio>

#include "qwt3d_surface.h"
#include "qwt3d_plot.h"


const char* NativeReader::magicstring = "jk:11051895-17021986";

class NativeReader::PrivateData
{
    QWT_DECLARE_PUBLIC(NativeReader)

public:
    PrivateData(NativeReader* q) : q_ptr(q), m_minZ(-DBL_MAX), m_maxZ(DBL_MAX)
    {
    }

    double m_minZ;
    double m_maxZ;
};

namespace
{
FILE* open(QString fname)
{
    FILE* file = fopen(QWT3DLOCAL8BIT(fname), "r");
    if (!file) {
        fprintf(stderr, "NativeReader::read: cannot open data file \"%s\"\n", QWT3DLOCAL8BIT(fname));
    }
    return file;
}

int read_char(FILE* fp, bool skipcomments = true)
{
    int c;

    if ((c = fgetc(fp)) == EOF)
        return (c);
    if (skipcomments) {
        if (c == '#') {
            do {
                if ((c = fgetc(fp)) == EOF)
                    return (c);
            } while (c != '\n' && c != '\r');
        }
    }
    return (c);
}

char* read_field(FILE* fp, bool skipcomments = true)
{
    static char buf[ 71 ];
    int c, i;

    do {
        if ((c = read_char(fp, skipcomments)) == EOF)
            return (nullptr);
    } while (isspace(c));
    for (i = 0; i < 70 && !isspace(c); ++i) {
        buf[ i ] = c;
        if ((c = read_char(fp, skipcomments)) == EOF)
            break;
    }
    buf[ i ] = '\0';
    return (buf);
}

// set to data begin
bool extract_info(FILE* fp, unsigned int& xmesh, unsigned int& ymesh, double& xmin, double& xmax, double& ymin, double& ymax)
{
    char* p;

    // find out the size
    if ((p = read_field(fp)) == nullptr)
        return false;
    xmesh = static_cast< unsigned int >(atoi(p));

    if ((p = read_field(fp)) == nullptr)
        return false;
    ymesh = static_cast< unsigned int >(atoi(p));

    if (xmesh < 1 || ymesh < 1)
        return false;

    // ... and the limits
    if ((p = read_field(fp)) == nullptr)
        return false;
    xmin = atof(p);

    if ((p = read_field(fp)) == nullptr)
        return false;
    xmax = atof(p);

    if ((p = read_field(fp)) == nullptr)
        return false;
    ymin = atof(p);

    if ((p = read_field(fp)) == nullptr)
        return false;
    ymax = atof(p);

    if (xmin > xmax || ymin > ymax)
        return false;

    return true;
}

// find out what the magic string is and compare
bool check_magic(FILE* fp, const char* val)
{
    char* p;
    if ((p = read_field(fp, false)) == nullptr)
        return false;

    if (strcmp(p, val) != 0)
        return false;
    return true;
}

// find out what the type is
bool check_type(FILE* fp, const char* val)
{
    char* p;
    if ((p = read_field(fp)) == nullptr)
        return false;

    if (strcmp(p, val) != 0)
        return false;
    return true;
}

double** allocateData(int columns, int rows)
{
    double** data = new double*[ columns ];

    for (int i = 0; i < columns; ++i) {
        data[ i ] = new double[ rows ];
    }
    return data;
}

void deleteData(double** data, int columns)
{
    for (int i = 0; i < columns; i++) {
        delete[] data[ i ];
    }
    delete[] data;
}
}

/**
 * @brief Default constructor
 */
NativeReader::NativeReader() : QWT_PIMPL_CONSTRUCT
{
}

NativeReader::~NativeReader() = default;

IO::Functor* NativeReader::clone() const
{
    auto* copy = new NativeReader();
    QWT_DC(d);
    auto* copyD   = copy->d_func();
    copyD->m_minZ = d->m_minZ;
    copyD->m_maxZ = d->m_maxZ;
    return copy;
}

/**
 * @brief Collects information about the data file
 * @param[out] file File pointer (opened on success)
 * @param fname File name to read
 * @param[out] xmesh Number of columns in mesh
 * @param[out] ymesh Number of rows in mesh
 * @param[out] minx Minimum x value
 * @param[out] maxx Maximum x value
 * @param[out] miny Minimum y value
 * @param[out] maxy Maximum y value
 * @return True on success, false if file cannot be opened or has invalid format
 */
bool NativeReader::collectInfo(FILE*& file,
                               QString const& fname,
                               unsigned& xmesh,
                               unsigned& ymesh,
                               double& minx,
                               double& maxx,
                               double& miny,
                               double& maxy)
{
    if (fname.isEmpty())
        return false;

    file = open(fname);

    if (!file)
        return false;

    if ((!check_magic(file, magicstring)) || (!check_type(file, "MESH"))
        || (!extract_info(file, xmesh, ymesh, minx, maxx, miny, maxy))) {
        fclose(file);
        return false;
    }

    return true;
}

/**
 * @brief Reads native format data into a Plot3D widget
 * @param plot Target Plot3D widget
 * @param fname File name to read
 * @return True on success, false on file error or data format error
 */
bool NativeReader::operator()(Qwt3DPlot* plot, QString const& fname)
{
    QWT_D(d);

    FILE* file;
    unsigned int xmesh, ymesh;
    double minx, maxx, miny, maxy;

    if (!collectInfo(file, fname, xmesh, ymesh, minx, maxx, miny, maxy))
        return false;

    /* allocate some space for the mesh */
    double** data = allocateData(xmesh, ymesh);

    for (unsigned int j = 0; j < ymesh; j++) {
        for (unsigned int i = 0; i < xmesh; i++) {
            if (fscanf(file, "%lf", &data[ i ][ j ]) != 1) {
                fprintf(stderr, "NativeReader::read: error in data file \"%s\"\n", QWT3DLOCAL8BIT(fname));
                return false;
            }

            if (data[ i ][ j ] > d->m_maxZ)
                data[ i ][ j ] = d->m_maxZ;
            else if (data[ i ][ j ] < d->m_minZ)
                data[ i ][ j ] = d->m_minZ;
        }
    }

    /* close the file */
    fclose(file);

    // Find the first Qwt3DSurface item attached to the plot
    Qwt3DSurface* surface = nullptr;
    for (Qwt3DPlotItem* item : plot->itemList()) {
        surface = dynamic_cast<Qwt3DSurface*>(item);
        if (surface)
            break;
    }
    if (!surface) {
        fprintf(stderr, "NativeReader::read: no Qwt3DSurface item attached to plot\n");
        deleteData(data, xmesh);
        return false;
    }

    surface->loadFromData(data, xmesh, ymesh, minx, maxx, miny, maxy);
    deleteData(data, xmesh);

    return true;
}
