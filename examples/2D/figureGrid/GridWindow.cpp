/******************************************************************************
 * Retained grid layout demo for QwtFigure.
 *
 * The example builds a 3 x 3 retained grid ( QwtFigure::enableGrid() ) and shows
 * what the grid model and its splitter can do:
 *
 *   - six members: four plots, one plot spanning two columns and one plain
 *     widget registered with QwtFigure::addGridItem()
 *   - the normalized track ratios ( QwtFigureGrid::rowSizes()/columnSizes() )
 *     can be preset or changed by dragging a divider
 *   - the gaps between the cells ( QwtFigureGrid::setSpacing() ) and the
 *     minimum track extent used to clamp a drag
 *     ( QwtFigureGrid::setMinimumTrackPixels() ) are exposed by the dock
 *   - one undo command per drag, pushed when
 *     QwtFigure::gridSizesChangeFinished() arrives
 *   - members can be removed and added back at runtime
 *   - leaving the grid ( QwtFigure::disableGrid() ) keeps the geometry of the
 *     members, re-entering it registers them again on their original cells
 *   - a real mouse press on a sub plot makes it the current axes
 *     ( QwtFigure::currentAxesChanged() )
 *
 * Usage:
 *   figureGrid                      interactive GUI
 *   figureGrid --screenshot <png>   unattended capture used by
 *                                   examples/update-screenshots.ps1
 *
 * Screenshot variants: default, drag (synthetic divider drags), spacing (wide
 * gaps plus a preset) and reenter (leave and re-enter the grid, then change the
 * ratios).
 *****************************************************************************/

#include "GridWindow.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QStringList>
#include <QToolBar>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

// STL
#include <cmath>

// qwt
#include "qwt_axis.h"
#include "qwt_colormap.h"
#include "qwt_figure.h"
#include "qwt_figure_grid.h"
#include "qwt_figure_grid_splitter.h"
#include "qwt_interval.h"
#include "qwt_legend.h"
#include "qwt_matrix_raster_data.h"
#include "qwt_plot.h"
#include "qwt_plot_barchart.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_grid.h"
#include "qwt_plot_spectrogram.h"
#include "qwt_symbol.h"
#include "qwt_text.h"

namespace
{
// Colors shared by the plots, kept in the muted palette of the other examples
const char* INK = "#22303f";
const char* MUTED = "#5b6b7c";
const char* GRID_LINE = "#d7dde5";

const double PI = 3.14159265358979323846;

// Font of the current style with an explicit point size
QFont pointFont(const QWidget* widget, qreal pointSize, bool bold = false)
{
    QFont font = widget->font();
    font.setPointSizeF(pointSize);
    font.setBold(bold);
    return font;
}

// Title with the compact font the cells need
void setPlotTitle(QwtPlot* plot, const QString& text)
{
    QwtText title(text);
    title.setFont(pointFont(plot, 9.0, true));
    title.setColor(QColor(INK));
    plot->setTitle(title);
}

// Axis title with the compact font the cells need
void setAxisTitle(QwtPlot* plot, QwtAxis::Position axis, const QString& text)
{
    QwtText title(text);
    title.setFont(pointFont(plot, 8.0));
    plot->setAxisTitle(axis, title);
}

// Common look of a demo plot: white canvas, dotted grid, compact fonts
void setupPlot(QwtPlot* plot, const QString& title)
{
    plot->setCanvasBackground(Qt::white);
    plot->setFrameStyle(QFrame::NoFrame);
    plot->setAxisFont(QwtAxis::YLeft, pointFont(plot, 8.0));
    plot->setAxisFont(QwtAxis::XBottom, pointFont(plot, 8.0));

    QwtPlotGrid* grid = new QwtPlotGrid();
    QPen gridPen{ QColor(GRID_LINE) };
    gridPen.setStyle(Qt::DotLine);
    gridPen.setWidthF(0.0);
    grid->setPen(gridPen);
    grid->attach(plot);

    setPlotTitle(plot, title);
}

// A chirp like spectrogram matrix, rows grow with the y (frequency) axis
QVector< double > makeSpectrogram(int rows, int columns)
{
    QVector< double > values;
    values.reserve(rows * columns);
    for (int row = 0; row < rows; ++row) {
        const double frequency = -1.5 + 3.0 * row / (rows - 1.0);
        for (int column = 0; column < columns; ++column) {
            const double time = -3.0 + 6.0 * column / (columns - 1.0);
            const double ridge = std::exp(-std::pow((frequency - 0.55 * std::sin(1.3 * time)) / 0.16, 2.0));
            const double echo =
                0.55 * std::exp(-std::pow((frequency + 0.85 * std::sin(0.65 * time + 0.8)) / 0.10, 2.0));
            const double ripple = 0.06 * (1.0 + std::sin(23.0 * time + 11.0 * frequency));
            values.append(qMin(1.2, ridge + echo + ripple));
        }
    }
    return values;
}

// Deterministic sample data for the demo plots
QVector< QPointF > makeWaveForm(int count)
{
    QVector< QPointF > samples;
    samples.reserve(count);
    for (int i = 0; i < count; ++i) {
        const double t = i * 0.01;
        const double y = std::sin(2.0 * PI * 1.2 * t) + 0.25 * std::sin(2.0 * PI * 9.0 * t + 0.7)
                         + 0.04 * std::sin(2.0 * PI * 41.0 * t);
        samples.append(QPointF(t, y));
    }
    return samples;
}

// A noisy cloud with a linear trend, produced by a small LCG so every run matches
QVector< QPointF > makeScatter(int count)
{
    QVector< QPointF > samples;
    samples.reserve(count);

    quint32 state = 0x9e3779b9u;
    auto next = [ &state ]() {
        state = state * 1664525u + 1013904223u;
        return static_cast< double >(state >> 8) / static_cast< double >(1u << 24);
    };

    for (int i = 0; i < count; ++i) {
        const double x = next() * 10.0 - 5.0;
        const double y = 0.6 * x + 4.0 * (next() - 0.5) + 1.2 * std::sin(x);
        samples.append(QPointF(x, y));
    }
    return samples;
}

// Two trends: the measured signal and its smoothed version
QVector< QPointF > makeTrend(int count, bool smoothed)
{
    QVector< QPointF > samples;
    samples.reserve(count);
    for (int i = 0; i < count; ++i) {
        const double t = i * 0.1;
        double y = 1.5 + 0.8 * std::sin(0.5 * t);
        if (!smoothed) {
            y += 0.25 * std::sin(5.3 * t) + 0.12 * std::sin(17.0 * t);
        }
        samples.append(QPointF(t, y));
    }
    return samples;
}

// Track ratios of the presets, applied through QwtFigureGrid::setRowSizes()
bool presetRatios(const QString& name, QVector< qreal >* rows, QVector< qreal >* columns)
{
    if (name == QLatin1String("equal")) {
        *rows = QVector< qreal >() << 1.0 << 1.0 << 1.0;
        *columns = QVector< qreal >() << 1.0 << 1.0 << 1.0;
        return true;
    }
    if (name == QLatin1String("focus-top")) {
        *rows = QVector< qreal >() << 0.56 << 0.27 << 0.17;
        *columns = QVector< qreal >() << 0.46 << 0.30 << 0.24;
        return true;
    }
    if (name == QLatin1String("golden")) {
        *rows = QVector< qreal >() << 0.44 << 0.34 << 0.22;
        *columns = QVector< qreal >() << 0.5 << 0.32 << 0.18;
        return true;
    }
    return false;
}

// Compares two ratio vectors with a tolerance suitable for normalized values
bool sameRatios(const QVector< qreal >& lhs, const QVector< qreal >& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (int i = 0; i < lhs.size(); ++i) {
        if (qAbs(lhs.at(i) - rhs.at(i)) > 1e-9) {
            return false;
        }
    }
    return true;
}

// Undo command for one track ratio change (one divider drag, one preset)
class GridRatiosCommand : public QUndoCommand
{
public:
    GridRatiosCommand(QwtFigureGrid* grid,
                      const QVector< qreal >& rows,
                      const QVector< qreal >& columns,
                      const QVector< qreal >& oldRows,
                      const QVector< qreal >& oldColumns,
                      const QString& text)
        : QUndoCommand(text)
        , m_grid(grid)
        , m_rows(rows)
        , m_columns(columns)
        , m_oldRows(oldRows)
        , m_oldColumns(oldColumns)
    {
    }

    void redo() override
    {
        apply(m_rows, m_columns);
    }

    void undo() override
    {
        apply(m_oldRows, m_oldColumns);
    }

private:
    void apply(const QVector< qreal >& rows, const QVector< qreal >& columns)
    {
        if (!m_grid) {
            return;
        }
        m_grid->setRowSizes(rows);
        m_grid->setColumnSizes(columns);
    }

    QwtFigureGrid* m_grid { nullptr };
    QVector< qreal > m_rows;
    QVector< qreal > m_columns;
    QVector< qreal > m_oldRows;
    QVector< qreal > m_oldColumns;
};
}  // namespace

GridWindow::GridWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(tr("QwtFigure retained grid layout example"));
    resize(1280, 820);

    // The status label lives in the status bar, it is updated from the grid signals
    m_statusLabel = new QLabel(this);
    m_statusLabel->setFont(pointFont(m_statusLabel, 8.5));
    statusBar()->addWidget(m_statusLabel, 1);

    m_undoStack = new QUndoStack(this);
    m_undoStack->setUndoLimit(50);
    connect(m_undoStack, &QUndoStack::indexChanged, this, [ this ]() {
        // Undo/redo applied a ratio vector: resync the committed snapshot
        if (m_grid && m_grid->isEnabled()) {
            m_committedRows = m_grid->rowSizes();
            m_committedColumns = m_grid->columnSizes();
        }
        updateStatus();
        updateNotesPanel();
    });

    createFigure();
    createToolBar();
    createControlPanel();

    updateStatus();
    updateNotesPanel();
}

GridWindow::~GridWindow()
{
    // The undo stack is a child of the window and is destroyed after the status label:
    // clear() then emits indexChanged() and a remaining connection would touch the
    // destroyed label. Drop the connections while the window is still alive.
    if (m_undoStack) {
        disconnect(m_undoStack, nullptr, this, nullptr);
    }
}

//----------------------------------------------------
// Figure and members
//----------------------------------------------------

/**
 * @brief Build the retained grid and register its members
 * @details The grid is enabled before any member is added: addAxes()/addGridItem()
 *          require the retained grid model to exist.
 */
void GridWindow::createFigure()
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(8, 6, 8, 4);
    centralLayout->setSpacing(4);

    QLabel* header = new QLabel(central);
    header->setTextFormat(Qt::RichText);
    header->setText(tr("<span style='font-size:11pt;color:%1'><b>QwtFigure retained grid layout</b></span><br/>"
                       "<span style='color:%2'>3 &times; 3 tracks &middot; members registered with addAxes() and "
                       "addGridItem() &middot; drag a divider to resize the adjacent tracks (Esc cancels)</span>")
                         .arg(INK, MUTED));
    centralLayout->addWidget(header);

    m_figure = new QwtFigure(central);
    m_figure->setFaceColor(Qt::white);
    m_figure->setEdgeColor(QColor("#b8c2cc"));
    m_figure->setEdgeLineWidth(1);
    centralLayout->addWidget(m_figure, 1);
    setCentralWidget(central);

    if (!m_figure->enableGrid(ROW_COUNT, COLUMN_COUNT)) {
        qWarning() << "Could not enable the retained grid";
        return;
    }
    m_grid = m_figure->grid();
    m_grid->setSpacing(0.006, 0.006);
    m_grid->setMinimumTrackPixels(40);

    connect(m_grid, &QwtFigureGrid::sizesChanged, this, &GridWindow::updateStatus);
    connect(m_grid, &QwtFigureGrid::sizesChanged, this, &GridWindow::updateNotesPanel);
    connect(m_grid, &QwtFigureGrid::sizesChangeFinished, this, &GridWindow::commitRatios);
    connect(m_figure, &QwtFigure::currentAxesChanged, this, &GridWindow::updateStatus);
    connect(m_figure, &QwtFigure::currentAxesChanged, this, &GridWindow::updateNotesPanel);

    m_committedRows = m_grid->rowSizes();
    m_committedColumns = m_grid->columnSizes();

    // Five plots and one plain widget share the 3 x 3 tracks of the grid
    m_members.append(Member(createSpectrumPlot(), 0, 0, 1, 3));
    m_members.append(Member(createWaveFormPlot(), 1, 0, 1, 1));
    m_members.append(Member(createScatterPlot(), 1, 1, 1, 1));
    m_members.append(Member(createBandPlot(), 1, 2, 1, 1));
    m_members.append(Member(createTrendPlot(), 2, 0, 1, 2));
    m_members.append(Member(createNotesPanel(), 2, 2, 1, 1));
    m_barChartCell = m_members.at(3).widget;

    registerMembers();

    // Draggable dividers: the splitter is created on the first setGridEditable()
    m_figure->setGridEditable(true);
    m_splitter = m_figure->findChild< QwtFigureGridSplitter* >();
    if (m_splitter) {
        m_splitter->setHitWidth(4);
        connect(m_splitter, &QwtFigureGridSplitter::hoverChanged, this, [ this ](Qt::CursorShape shape) {
            if (m_statusLabel && shape != Qt::ArrowCursor) {
                const bool horizontal = (shape == Qt::SplitHCursor);
                m_statusLabel->setText(
                    tr("%1 divider hit band: drag to resize the %2 tracks, Esc cancels")
                        .arg(horizontal ? tr("Column") : tr("Row"), horizontal ? tr("column") : tr("row")));
            } else {
                updateStatus();
            }
        });
    }
}

/**
 * @brief Register every member that belongs to the grid but is not registered yet
 * @details A member that is not part of the figure (first build, or removed with
 *          removeWidget()) is added with QwtFigure::addAxes() or
 *          QwtFigure::addGridItem(), which create its layout item as well. A member
 *          that stayed in the figure after disableGrid() only needs its grid
 *          registration back: QwtFigureGrid::addItem() is enough, because the layout
 *          resolves every registered member through the grid. Members marked with
 *          Member::inGrid == false are removed at runtime and stay out of the grid.
 */
void GridWindow::registerMembers()
{
    if (!m_figure || !m_figure->isGridEnabled() || !m_grid) {
        return;
    }

    const QList< QWidget* > registered = m_grid->items();

    for (int i = 0; i < m_members.size(); ++i) {
        const Member& member = m_members.at(i);
        QWidget* widget      = member.widget;
        if (!widget || !member.inGrid || registered.contains(widget)) {
            continue;
        }

        if (QwtPlot* plot = qobject_cast< QwtPlot* >(widget)) {
            if (m_figure->hasAxes(plot)) {
                // Still a free member of the figure after disableGrid(): register its cell
                m_grid->addItem(plot, member.row, member.column, member.rowSpan, member.columnSpan);
            } else {
                m_figure->addAxes(plot, member.row, member.column, member.rowSpan, member.columnSpan);
                plot->show();
            }
        } else if (widget->parentWidget() == m_figure) {
            m_grid->addItem(widget, member.row, member.column, member.rowSpan, member.columnSpan);
        } else if (m_figure->addGridItem(widget, member.row, member.column, member.rowSpan, member.columnSpan)) {
            widget->show();
        }
    }

    if (QLayout* lay = m_figure->layout()) {
        lay->invalidate();
        lay->activate();
    }
    m_figure->replotAll();
}

//----------------------------------------------------
// Plot items
//----------------------------------------------------

/**
 * @brief Spectrogram spanning the whole first row
 * @return The plot, not yet registered in the grid
 */
QwtPlot* GridWindow::createSpectrumPlot()
{
    QwtPlot* plot = new QwtPlot();
    setupPlot(plot, tr("Spectrogram - row 0, columns 0-2"));

    const int rows    = 128;
    const int columns = 256;

    QwtMatrixRasterData* rasterData = new QwtMatrixRasterData();
    rasterData->setValueMatrix(makeSpectrogram(rows, columns), columns);
    rasterData->setInterval(Qt::XAxis, QwtInterval(-3.0, 3.0));
    rasterData->setInterval(Qt::YAxis, QwtInterval(-1.5, 1.5));
    rasterData->setInterval(Qt::ZAxis, QwtInterval(0.0, 1.2));
    rasterData->setResampleMode(QwtMatrixRasterData::BilinearInterpolation);

    QwtLinearColorMap* colorMap = new QwtLinearColorMap(QColor("#0b1030"), QColor("#f6d746"));
    colorMap->addColorStop(0.15, QColor("#312f8f"));
    colorMap->addColorStop(0.35, QColor("#2a6fbb"));
    colorMap->addColorStop(0.55, QColor("#2bb673"));
    colorMap->addColorStop(0.75, QColor("#f0b422"));

    QwtPlotSpectrogram* spectrogram = new QwtPlotSpectrogram();
    spectrogram->setData(rasterData);
    spectrogram->setColorMap(colorMap);
    spectrogram->setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    spectrogram->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, -3.0, 3.0);
    plot->setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
    setAxisTitle(plot, QwtAxis::XBottom, tr("time [s]"));
    setAxisTitle(plot, QwtAxis::YLeft, tr("freq [kHz]"));
    return plot;
}

/**
 * @brief Time domain waveform in the middle row
 * @return The plot, not yet registered in the grid
 */
QwtPlot* GridWindow::createWaveFormPlot()
{
    QwtPlot* plot = new QwtPlot();
    setupPlot(plot, tr("Waveform - row 1, column 0"));

    QwtPlotCurve* curve = new QwtPlotCurve(tr("signal"));
    curve->setSamples(makeWaveForm(600));
    curve->setPen(QColor("#2c7fb8"), 1.2);
    curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    curve->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, 0.0, 6.0);
    plot->setAxisScale(QwtAxis::YLeft, -1.5, 1.5);
    setAxisTitle(plot, QwtAxis::XBottom, tr("t [s]"));
    setAxisTitle(plot, QwtAxis::YLeft, tr("amplitude"));
    return plot;
}

/**
 * @brief Scatter cloud in the middle row
 * @return The plot, not yet registered in the grid
 */
QwtPlot* GridWindow::createScatterPlot()
{
    QwtPlot* plot = new QwtPlot();
    setupPlot(plot, tr("Scatter - row 1, column 1"));

    QwtPlotCurve* dots = new QwtPlotCurve(tr("samples"));
    dots->setSamples(makeScatter(1500));
    dots->setStyle(QwtPlotCurve::Dots);
    dots->setSymbol(new QwtSymbol(QwtSymbol::Ellipse, QColor(31, 119, 180, 90), QPen(QColor(31, 119, 180, 150), 0.0),
                                  QSize(4, 4)));
    dots->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    dots->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, -5.0, 5.0);
    plot->setAxisScale(QwtAxis::YLeft, -6.0, 6.0);
    setAxisTitle(plot, QwtAxis::XBottom, tr("x"));
    setAxisTitle(plot, QwtAxis::YLeft, tr("y"));
    return plot;
}

/**
 * @brief Bar chart in the middle row
 * @return The plot, not yet registered in the grid
 */
QwtPlot* GridWindow::createBandPlot()
{
    QwtPlot* plot = new QwtPlot();
    setupPlot(plot, tr("Bands - row 1, column 2"));

    QVector< QPointF > bands;
    const double levels[] = { 0.35, 0.62, 0.48, 0.86, 0.71, 0.42, 0.55, 0.78, 0.30 };
    for (int i = 0; i < 9; ++i) {
        bands.append(QPointF(i, levels[ i ]));
    }

    QwtPlotBarChart* barChart = new QwtPlotBarChart(tr("level"));
    barChart->setSamples(bands);
    barChart->setPen(QPen(QColor("#1f6f8b"), 1.0));
    barChart->setBrush(QColor(31, 111, 139, 180));
    barChart->attach(plot);

    plot->setAxisScale(QwtAxis::XBottom, -0.5, 8.5);
    plot->setAxisScale(QwtAxis::YLeft, 0.0, 1.0);
    setAxisTitle(plot, QwtAxis::XBottom, tr("band"));
    setAxisTitle(plot, QwtAxis::YLeft, tr("level"));
    return plot;
}

/**
 * @brief Two curves and a legend, spanning two columns of the last row
 * @return The plot, not yet registered in the grid
 */
QwtPlot* GridWindow::createTrendPlot()
{
    QwtPlot* plot = new QwtPlot();
    setupPlot(plot, tr("Trend - row 2, columns 0-1"));

    QwtPlotCurve* measured = new QwtPlotCurve(tr("measured"));
    measured->setSamples(makeTrend(240, false));
    measured->setPen(QColor(228, 26, 28, 120), 0.8);
    measured->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    measured->attach(plot);

    QwtPlotCurve* smoothed = new QwtPlotCurve(tr("smoothed"));
    smoothed->setSamples(makeTrend(240, true));
    smoothed->setPen(QColor("#333333"), 1.6);
    smoothed->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    smoothed->attach(plot);

    QwtLegend* legend = new QwtLegend();
    legend->setFont(pointFont(plot, 8.0));
    plot->insertLegend(legend, QwtPlot::TopLegend);

    plot->setAxisScale(QwtAxis::XBottom, 0.0, 24.0);
    plot->setAxisScale(QwtAxis::YLeft, 0.4, 2.6);
    setAxisTitle(plot, QwtAxis::XBottom, tr("t [s]"));
    setAxisTitle(plot, QwtAxis::YLeft, tr("value"));
    return plot;
}

/**
 * @brief Plain widget member showing the live state of the grid
 * @return The panel, registered with QwtFigure::addGridItem()
 */
QWidget* GridWindow::createNotesPanel()
{
    QWidget* panel = new QWidget();
    panel->setAutoFillBackground(true);

    QPalette palette = panel->palette();
    palette.setColor(QPalette::Window, QColor("#f4f7fa"));
    panel->setPalette(palette);

    m_notesLabel = new QLabel(panel);
    m_notesLabel->setTextFormat(Qt::RichText);
    m_notesLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_notesLabel->setFont(pointFont(m_notesLabel, 8.5));
    m_notesLabel->setWordWrap(true);

    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->addWidget(m_notesLabel);
    return panel;
}

//----------------------------------------------------
// Grid controls and state
//----------------------------------------------------

void GridWindow::createToolBar()
{
    QToolBar* toolbar = addToolBar(tr("Figure"));
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction* saveAction = toolbar->addAction(QIcon(":/icons/save.svg"), tr("Save PNG..."));
    connect(saveAction, &QAction::triggered, this, [ this ]() {
        const QString file = QFileDialog::getSaveFileName(this, tr("Save figure"), QStringLiteral("figure_grid.png"),
                                                          tr("PNG image (*.png)"));
        if (file.isEmpty()) {
            return;
        }
        if (m_figure->saveFig(file, 300)) {
            qInfo() << "Figure written to" << file;
        }
    });

    toolbar->addSeparator();
    QAction* equalAction = toolbar->addAction(QIcon(":/icons/equal.svg"), tr("Equal"));
    connect(equalAction, &QAction::triggered, this, [ this ]() {
        applyPreset(QStringLiteral("equal"));
    });
    QAction* focusAction = toolbar->addAction(QIcon(":/icons/focus-top.svg"), tr("Focus top"));
    connect(focusAction, &QAction::triggered, this, [ this ]() {
        applyPreset(QStringLiteral("focus-top"));
    });
    QAction* goldenAction = toolbar->addAction(QIcon(":/icons/golden.svg"), tr("Golden"));
    connect(goldenAction, &QAction::triggered, this, [ this ]() {
        applyPreset(QStringLiteral("golden"));
    });

    toolbar->addSeparator();
    QAction* undoAction = m_undoStack->createUndoAction(this, tr("Undo"));
    undoAction->setIcon(QIcon(":/icons/undo.svg"));
    undoAction->setShortcut(QKeySequence::Undo);
    toolbar->addAction(undoAction);
    QAction* redoAction = m_undoStack->createRedoAction(this, tr("Redo"));
    redoAction->setIcon(QIcon(":/icons/redo.svg"));
    redoAction->setShortcut(QKeySequence::Redo);
    toolbar->addAction(redoAction);

    toolbar->addSeparator();
    QAction* logAction = toolbar->addAction(QIcon(":/icons/log.svg"), tr("Log layout"));
    connect(logAction, &QAction::triggered, this, &GridWindow::logLayout);
}

/**
 * @brief Build the dock with the knobs of the grid model and of the splitter
 */
void GridWindow::createControlPanel()
{
    QDockWidget* dock = new QDockWidget(tr("Grid controls"), this);
    dock->setObjectName(QStringLiteral("gridControls"));
    dock->setFeatures(QDockWidget::DockWidgetMovable);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QWidget* panel = new QWidget(dock);
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(10);

    // Dividers of the retained grid
    QGroupBox* interaction = new QGroupBox(tr("Dividers"), panel);
    QFormLayout* interactionForm = new QFormLayout(interaction);
    interactionForm->setLabelAlignment(Qt::AlignLeft);
    interactionForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_editableBox = new QCheckBox(tr("Draggable (setGridEditable)"), interaction);
    m_editableBox->setChecked(true);
    interactionForm->addRow(m_editableBox);

    m_hitWidthBox = new QSpinBox(interaction);
    m_hitWidthBox->setRange(1, 20);
    m_hitWidthBox->setValue(4);
    m_hitWidthBox->setSuffix(tr(" px"));
    m_hitWidthBox->setToolTip(tr("Half width of the hit band around a divider"));
    interactionForm->addRow(tr("Hit band +/-"), m_hitWidthBox);

    m_minTrackBox = new QSpinBox(interaction);
    m_minTrackBox->setRange(10, 200);
    m_minTrackBox->setSingleStep(5);
    m_minTrackBox->setValue(40);
    m_minTrackBox->setSuffix(tr(" px"));
    m_minTrackBox->setToolTip(tr("Minimum extent of a track while a divider is dragged"));
    interactionForm->addRow(tr("Min track"), m_minTrackBox);

    layout->addWidget(interaction);

    // Gaps between the cells
    QGroupBox* gaps = new QGroupBox(tr("Gaps (normalized)"), panel);
    QFormLayout* gapsForm = new QFormLayout(gaps);
    gapsForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_wspaceBox = new QDoubleSpinBox(gaps);
    m_wspaceBox->setDecimals(3);
    m_wspaceBox->setSingleStep(0.002);
    m_wspaceBox->setRange(0.0, 0.05);
    m_wspaceBox->setValue(0.006);
    gapsForm->addRow(tr("wspace"), m_wspaceBox);

    m_hspaceBox = new QDoubleSpinBox(gaps);
    m_hspaceBox->setDecimals(3);
    m_hspaceBox->setSingleStep(0.002);
    m_hspaceBox->setRange(0.0, 0.05);
    m_hspaceBox->setValue(0.006);
    gapsForm->addRow(tr("hspace"), m_hspaceBox);

    layout->addWidget(gaps);

    // Track ratio presets
    QGroupBox* ratios = new QGroupBox(tr("Track ratios"), panel);
    QHBoxLayout* ratiosLayout = new QHBoxLayout(ratios);
    const QStringList presetNames = { QStringLiteral("equal"), QStringLiteral("focus-top"), QStringLiteral("golden") };
    const QStringList presetTexts = { tr("Equal"), tr("Focus top"), tr("Golden") };
    for (int i = 0; i < presetNames.size(); ++i) {
        QPushButton* button = new QPushButton(presetTexts.at(i), ratios);
        const QString name = presetNames.at(i);
        connect(button, &QPushButton::clicked, this, [ this, name ]() {
            applyPreset(name);
        });
        ratiosLayout->addWidget(button);
    }
    layout->addWidget(ratios);

    // Members
    QGroupBox* members = new QGroupBox(tr("Members"), panel);
    QVBoxLayout* membersLayout = new QVBoxLayout(members);

    m_activeCellsButton = new QPushButton(tr("Bar chart member visible"), members);
    m_activeCellsButton->setCheckable(true);
    m_activeCellsButton->setChecked(true);
    m_activeCellsButton->setToolTip(tr("Remove it with QwtFigure::removeWidget(), add it back with addAxes()"));
    membersLayout->addWidget(m_activeCellsButton);

    m_leaveGridButton = new QPushButton(tr("Leave grid (keep geometry)"), members);
    m_leaveGridButton->setToolTip(tr("QwtFigure::disableGrid() freezes the current geometry of the members"));
    membersLayout->addWidget(m_leaveGridButton);

    layout->addWidget(members);

    QLabel* hint = new QLabel(panel);
    hint->setWordWrap(true);
    hint->setFont(pointFont(hint, 8.0));
    hint->setText(tr("Only the mouse events inside the divider band are consumed, so zooming, panning, picking "
                     "and hovering keep working. One undo command is pushed per drag, built from "
                     "QwtFigure::gridSizesChangeFinished()."));
    layout->addWidget(hint);
    layout->addStretch(1);

    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    // Connections of the controls
    connect(m_editableBox, &QCheckBox::toggled, this, [ this ](bool on) {
        if (!m_figure->isGridEnabled()) {
            return;
        }
        m_figure->setGridEditable(on);
        if (on) {
            m_splitter = m_figure->findChild< QwtFigureGridSplitter* >();
        }
        updateStatus();
    });

    connect(m_hitWidthBox, QOverload< int >::of(&QSpinBox::valueChanged), this, [ this ](int px) {
        if (m_splitter) {
            m_splitter->setHitWidth(px);
        }
    });

    connect(m_minTrackBox, QOverload< int >::of(&QSpinBox::valueChanged), this, [ this ](int px) {
        if (m_grid && m_grid->isEnabled()) {
            m_grid->setMinimumTrackPixels(px);
            updateStatus();
        }
    });

    const auto applySpacing = [ this ]() {
        if (m_grid && m_grid->isEnabled()) {
            m_grid->setSpacing(m_wspaceBox->value(), m_hspaceBox->value());
            updateStatus();
            updateNotesPanel();
        }
    };
    connect(m_wspaceBox, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, applySpacing);
    connect(m_hspaceBox, QOverload< double >::of(&QDoubleSpinBox::valueChanged), this, applySpacing);

    connect(m_activeCellsButton, &QPushButton::toggled, this, &GridWindow::setBarChartVisible);
    connect(m_leaveGridButton, &QPushButton::clicked, this, [ this ]() {
        setGridActive(!m_figure->isGridEnabled());
    });
}

/**
 * @brief Apply a track ratio preset and push it on the undo stack
 * @param name Preset name: "equal", "focus-top" or "golden"
 */
void GridWindow::applyPreset(const QString& name)
{
    QVector< qreal > rows;
    QVector< qreal > columns;
    if (!m_grid || !m_grid->isEnabled() || !presetRatios(name, &rows, &columns)) {
        return;
    }

    m_grid->setRowSizes(rows);
    m_grid->setColumnSizes(columns);
    commitRatios();
    updateStatus();
    updateNotesPanel();
}

/**
 * @brief Change the gaps of the grid and keep the control panel in sync
 * @param wspace Horizontal gap, normalized
 * @param hspace Vertical gap, normalized
 */
void GridWindow::setGridSpacing(qreal wspace, qreal hspace)
{
    if (m_wspaceBox) {
        m_wspaceBox->setValue(wspace);
    }
    if (m_hspaceBox) {
        m_hspaceBox->setValue(hspace);
    }
    if (m_grid && m_grid->isEnabled()) {
        m_grid->setSpacing(wspace, hspace);
    }
    updateStatus();
    updateNotesPanel();
}

/**
 * @brief Push the current ratios as one undo command when they differ from the last commit
 */
void GridWindow::commitRatios()
{
    if (!m_grid || !m_grid->isEnabled() || !m_undoStack) {
        return;
    }

    const QVector< qreal > rows    = m_grid->rowSizes();
    const QVector< qreal > columns = m_grid->columnSizes();
    if (sameRatios(rows, m_committedRows) && sameRatios(columns, m_committedColumns)) {
        return;
    }

    m_undoStack->push(new GridRatiosCommand(m_grid, rows, columns, m_committedRows, m_committedColumns,
                                            tr("Change the grid track ratios")));
}

/**
 * @brief Remove the bar chart member from the grid or add it back
 * @param on True to register the plot again on its cell
 */
void GridWindow::setBarChartVisible(bool on)
{
    if (!m_barChartCell || !m_figure) {
        return;
    }

    for (int i = 0; i < m_members.size(); ++i) {
        Member& member = m_members[ i ];
        if (member.widget != m_barChartCell) {
            continue;
        }

        if (on && !member.inGrid) {
            member.inGrid = true;
            registerMembers();
        } else if (!on && member.inGrid) {
            member.inGrid = false;
            m_figure->removeWidget(member.widget);
            // Keep the widget owned by the window while it is not a member
            member.widget->setParent(this);
            member.widget->hide();
            if (QLayout* lay = m_figure->layout()) {
                lay->invalidate();
                lay->activate();
            }
        }
    }

    updateStatus();
    updateNotesPanel();
}

/**
 * @brief Leave the retained grid or re-enter it
 * @details Leaving keeps the current geometry of the members
 *          ( QwtFigure::disableGrid() ), re-entering registers them again on
 *          their original cells with the ratios that were active before.
 * @param active True to enable the retained grid
 */
void GridWindow::setGridActive(bool active)
{
    if (!m_figure) {
        return;
    }
    if (active == m_figure->isGridEnabled()) {
        return;
    }

    if (active) {
        if (!m_figure->enableGrid(ROW_COUNT, COLUMN_COUNT)) {
            return;
        }
        m_grid = m_figure->grid();
        m_grid->setSpacing(m_wspaceBox ? m_wspaceBox->value() : 0.006, m_hspaceBox ? m_hspaceBox->value() : 0.006);
        m_grid->setMinimumTrackPixels(m_minTrackBox ? m_minTrackBox->value() : 40);

        // Restore the ratios of the grid that was left
        m_grid->setRowSizes(m_committedRows);
        m_grid->setColumnSizes(m_committedColumns);

        registerMembers();

        m_figure->setGridEditable(m_editableBox && m_editableBox->isChecked());
        m_splitter = m_figure->findChild< QwtFigureGridSplitter* >();
        if (m_splitter && m_hitWidthBox) {
            m_splitter->setHitWidth(m_hitWidthBox->value());
        }

        m_committedRows = m_grid->rowSizes();
        m_committedColumns = m_grid->columnSizes();
    } else {
        m_committedRows = m_grid ? m_grid->rowSizes() : QVector< qreal >();
        m_committedColumns = m_grid ? m_grid->columnSizes() : QVector< qreal >();
        m_figure->disableGrid();
        // The track model is gone: commands that restore ratios have nothing to apply
        m_undoStack->clear();
    }

    const bool gridEnabled = m_figure->isGridEnabled();
    if (m_editableBox) {
        m_editableBox->setEnabled(gridEnabled);
    }
    if (m_hitWidthBox) {
        m_hitWidthBox->setEnabled(gridEnabled);
    }
    if (m_minTrackBox) {
        m_minTrackBox->setEnabled(gridEnabled);
    }
    if (m_wspaceBox) {
        m_wspaceBox->setEnabled(gridEnabled);
    }
    if (m_hspaceBox) {
        m_hspaceBox->setEnabled(gridEnabled);
    }
    if (m_activeCellsButton) {
        m_activeCellsButton->setEnabled(gridEnabled);
    }
    if (m_leaveGridButton) {
        m_leaveGridButton->setText(gridEnabled ? tr("Leave grid (keep geometry)") : tr("Re-enter 3 x 3 grid"));
    }

    updateStatus();
    updateNotesPanel();
}

//----------------------------------------------------
// State readout
//----------------------------------------------------

/**
 * @brief Leave the retained grid while the members keep their geometry
 */
void GridWindow::leaveGrid()
{
    setGridActive(false);
}

/**
 * @brief Enable the retained grid again and register the members on their cells
 */
void GridWindow::reenterGrid()
{
    setGridActive(true);
}

QString GridWindow::ratiosText(const QVector< qreal >& ratios) const
{
    QStringList values;
    values.reserve(ratios.size());
    for (int i = 0; i < ratios.size(); ++i) {
        values << QString::number(ratios.at(i), 'f', 3);
    }
    return values.join(QStringLiteral(" / "));
}

/**
 * @brief Update the status bar with the state of the grid model
 */
void GridWindow::updateStatus()
{
    if (!m_statusLabel) {
        return;
    }

    if (!m_grid || !m_grid->isEnabled()) {
        m_statusLabel->setText(tr("Free layout: the members keep the geometry they had when the grid was left "
                                  "(QwtFigure::disableGrid)"));
        return;
    }

    const QList< QwtFigureGrid::Divider > dividers = m_grid->dividers();
    int columnDividers = 0;
    int rowDividers    = 0;
    for (const QwtFigureGrid::Divider& divider : dividers) {
        if (divider.orientation == Qt::Horizontal) {
            ++columnDividers;
        } else {
            ++rowDividers;
        }
    }

    QString active = tr("-");
    if (QwtPlot* plot = m_figure->currentAxes()) {
        active = plot->title().text();
    }

    QStringList parts;
    parts << tr("grid %1 x %2").arg(m_grid->rowCount()).arg(m_grid->columnCount());
    parts << tr("rows %1").arg(ratiosText(m_grid->rowSizes()));
    parts << tr("columns %1").arg(ratiosText(m_grid->columnSizes()));
    parts << tr("gap %1 / %2")
                 .arg(QString::number(m_grid->wspace(), 'f', 3), QString::number(m_grid->hspace(), 'f', 3));
    parts << tr("min track %1 px").arg(m_grid->minimumTrackPixels());
    parts << tr("dividers %1 + %2").arg(columnDividers).arg(rowDividers);
    parts << tr("active: %1").arg(active);
    parts << tr("undo %1").arg(m_undoStack ? m_undoStack->count() : 0);

    m_statusLabel->setText(parts.join(QStringLiteral("   \u00b7   ")));
}

/**
 * @brief Update the readout widget that is registered with addGridItem()
 */
void GridWindow::updateNotesPanel()
{
    if (!m_notesLabel) {
        return;
    }

    if (!m_grid || !m_grid->isEnabled()) {
        m_notesLabel->setText(tr("<div style='color:%1'><b>Free member</b><br/>"
                                 "<span style='color:%2'>The grid tracks are gone after disableGrid() and this widget "
                                 "keeps the geometry it had.</span></div>")
                                  .arg(INK, MUTED));
        return;
    }

    const QList< QwtFigureGrid::Divider > dividers = m_grid->dividers();
    int columnDividers = 0;
    int rowDividers    = 0;
    for (const QwtFigureGrid::Divider& divider : dividers) {
        if (divider.orientation == Qt::Horizontal) {
            ++columnDividers;
        } else {
            ++rowDividers;
        }
    }

    QString active = tr("-");
    if (QwtPlot* plot = m_figure->currentAxes()) {
        active = plot->title().text();
    }

    const QString mutedStyle = QStringLiteral("color:%1").arg(MUTED);
    QString html;
    html += QStringLiteral("<div><b>Plain widget member</b><br/>"
                           "<span style='%1'>This cell is registered with QwtFigure::addGridItem( 2, 2 ): "
                           "any QWidget can share the tracks of the plots.</span></div>")
                .arg(mutedStyle);
    html += QStringLiteral("<table cellpadding='2' cellspacing='0'>");
    html += QStringLiteral("<tr><td style='%1'>rows</td><td>%2</td></tr>")
                .arg(mutedStyle, ratiosText(m_grid->rowSizes()));
    html += QStringLiteral("<tr><td style='%1'>columns</td><td>%2</td></tr>")
                .arg(mutedStyle, ratiosText(m_grid->columnSizes()));
    html += QStringLiteral("<tr><td style='%1'>gap</td><td>%2 / %3</td></tr>")
                .arg(mutedStyle, QString::number(m_grid->wspace(), 'f', 3),
                     QString::number(m_grid->hspace(), 'f', 3));
    html += QStringLiteral("<tr><td style='%1'>min track</td><td>%2 px</td></tr>")
                .arg(mutedStyle)
                .arg(m_grid->minimumTrackPixels());
    html += QStringLiteral("<tr><td style='%1'>dividers</td><td>%2 columns + %3 rows</td></tr>")
                .arg(mutedStyle)
                .arg(columnDividers)
                .arg(rowDividers);
    html += QStringLiteral("<tr><td style='%1'>active axes</td><td>%2</td></tr>").arg(mutedStyle, active);
    html += QStringLiteral("</table>");

    m_notesLabel->setText(html);
}

void GridWindow::logLayout()
{
    if (!m_grid || !m_grid->isEnabled()) {
        qDebug() << "[figureGrid] the retained grid is disabled";
        return;
    }

    qDebug() << "[figureGrid] rows" << ratiosText(m_grid->rowSizes()) << "columns"
             << ratiosText(m_grid->columnSizes()) << "gap" << m_grid->wspace() << m_grid->hspace() << "min track px"
             << m_grid->minimumTrackPixels();

    const QList< QwtFigureGrid::Divider > dividers = m_grid->dividers();
    for (const QwtFigureGrid::Divider& divider : dividers) {
        qDebug() << "  divider" << (divider.orientation == Qt::Horizontal ? "columns" : "rows") << divider.index << "at"
                 << divider.position;
    }

    const QList< QWidget* > members = m_grid->items();
    for (QWidget* member : members) {
        int row = 0, column = 0, rowSpan = 0, columnSpan = 0;
        if (!m_grid->gridPosition(member, &row, &column, &rowSpan, &columnSpan)) {
            continue;
        }
        qDebug() << "  member" << (qobject_cast< QwtPlot* >(member) ? "plot" : "widget") << row << column << rowSpan
                 << columnSpan << "normRect" << m_grid->normRectFor(row, column, rowSpan, columnSpan) << "geometry"
                 << member->geometry();
    }
}

/**
 * @brief Drag a divider with synthetic mouse events
 * @details Used by the "drag" screenshot variant. The events go through the same
 *          path as real input: they are delivered to the widget under the
 *          divider (the canvas of a plot) and the splitter consumes the band.
 * @param dividerIndex Index in QwtFigureGrid::dividers(): the column dividers come
 *                     first, the row dividers follow
 * @param deltaPixels Signed offset along the axis of the divider
 */
void GridWindow::dragDivider(int dividerIndex, int deltaPixels)
{
    if (!m_grid || !m_grid->isEnabled() || !m_figure) {
        return;
    }

    // The variant is applied right after show(): make sure the figure was laid out
    QCoreApplication::processEvents();

    const QList< QwtFigureGrid::Divider > dividers = m_grid->dividers();
    if (dividerIndex < 0 || dividerIndex >= dividers.size()) {
        return;
    }

    const QwtFigureGrid::Divider divider = dividers.at(dividerIndex);
    const int width                      = m_figure->width();
    const int height                     = m_figure->height();
    if (width <= 0 || height <= 0) {
        return;
    }

    const bool horizontal = (divider.orientation == Qt::Horizontal);
    const QPoint start    = horizontal ? QPoint(qRound(divider.position * width), height / 2)
                                       : QPoint(width / 2, qRound(divider.position * height));
    const QPoint end      = start + (horizontal ? QPoint(deltaPixels, 0) : QPoint(0, deltaPixels));

    QWidget* target = m_figure->childAt(start);
    if (!target) {
        target = m_figure;
    }

    const auto sendMouse = [ this, target ](QEvent::Type type, const QPoint& figurePos, Qt::MouseButton button,
                                            Qt::MouseButtons buttons) {
        const QPoint local  = target->mapFrom(m_figure, figurePos);
        const QPoint global = target->mapToGlobal(local);
        QMouseEvent event(type, QPointF(local), QPointF(global), button, buttons, Qt::NoModifier);
        QCoreApplication::sendEvent(target, &event);
    };

    sendMouse(QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);

    QCoreApplication::processEvents();
}
