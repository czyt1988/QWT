#include "PlotAdapter.h"

#include "QCustomPlotAdapter.h"
#include "QwtAdapter.h"

#ifdef CHARTBENCH_HAVE_QTCHARTS
#include "QtChartsAdapter.h"
#endif

#ifdef CHARTBENCH_HAVE_QIM
#include "QImAdapter.h"
#endif

namespace chartbench
{

QStringList PlotAdapter::downsamplingAlgorithms() const
{
    return QStringList { QStringLiteral("none") };
}

bool PlotAdapter::setDownsamplingAlgorithm(const QString& name)
{
    return name == QLatin1String("none");
}

std::unique_ptr<PlotAdapter> createAdapter(const QString& library, Backend backend, QString& error)
{
    if (library == QLatin1String("qwt"))
        return std::make_unique<QwtAdapter>(backend);

    if (library == QLatin1String("qcustomplot"))
        return std::make_unique<QCustomPlotAdapter>(backend);

#ifdef CHARTBENCH_HAVE_QTCHARTS
    if (library == QLatin1String("qtcharts"))
        return std::make_unique<QtChartsAdapter>(backend);
#endif

#ifdef CHARTBENCH_HAVE_QIM
    if (library == QLatin1String("qim"))
        return std::make_unique<QImAdapter>(backend);
#endif

    error = QStringLiteral("unknown or unavailable library '%1' (available: %2)")
                .arg(library, availableLibraries().join(QLatin1Char(',')));
    return nullptr;
}

QStringList availableLibraries()
{
    QStringList libraries;
    libraries << QStringLiteral("qwt") << QStringLiteral("qcustomplot");
#ifdef CHARTBENCH_HAVE_QTCHARTS
    libraries << QStringLiteral("qtcharts");
#endif
#ifdef CHARTBENCH_HAVE_QIM
    libraries << QStringLiteral("qim");
#endif
    return libraries;
}

} // namespace chartbench
