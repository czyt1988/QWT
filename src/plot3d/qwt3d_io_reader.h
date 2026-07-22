#ifndef QWT3D_IO_READER_H
#define QWT3D_IO_READER_H

#include "qwt3d_io.h"



/**
 * @brief Functor for reading of native files containing grid data
 * @details As a standard input functor associated with "mes" and "MES" file extensions.
 */
class QWT3D_EXPORT NativeReader : public IO::Functor
{
    friend class IO;
    QWT_DECLARE_PRIVATE(NativeReader)

public:
    NativeReader();
    ~NativeReader() override;

private:
    IO::Functor* clone() const override;
    bool operator()(Qwt3DPlot* plot, QString const& fname) override;
    static const char* magicstring;
    bool collectInfo(FILE*& file,
                     QString const& fname,
                     unsigned& xmesh,
                     unsigned& ymesh,
                     double& minx,
                     double& maxx,
                     double& miny,
                     double& maxy);
};


#endif  // QWT3D_IO_READER_H