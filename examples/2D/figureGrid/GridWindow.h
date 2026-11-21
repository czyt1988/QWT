#ifndef GRIDWINDOW_H
#define GRIDWINDOW_H

// Qt
#include <QMainWindow>
#include <QPointer>
#include <QVector>

class QCheckBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QUndoStack;
class QwtFigure;
class QwtFigureGrid;
class QwtFigureGridSplitter;
class QwtPlot;

/**
 * @class GridWindow
 * @brief Example of the retained grid layout of QwtFigure and of its draggable dividers
 *
 * @details
 * The window shows a 3 x 3 retained grid ( QwtFigure::enableGrid() ) with six members:
 * four plots, one plot spanning two columns and one plain widget registered with
 * QwtFigure::addGridItem(). Everything the grid model offers is exposed by the
 * controls of the window:
 *
 *  - track ratios: the "Equal / Focus top / Golden" presets and the divider drag
 *  - gaps: the column/row gap spin boxes ( QwtFigureGrid::setSpacing() )
 *  - the minimum track extent that clamps a drag ( QwtFigureGrid::setMinimumTrackPixels() )
 *  - the divider hit band ( QwtFigureGridSplitter::setHitWidth() )
 *  - one undo command per drag, built from QwtFigure::gridSizesChangeFinished()
 *  - members added/removed at runtime ( QwtFigureGrid::addItem()/removeItem() through
 *    QwtFigure::addAxes()/addGridItem()/removeWidget() )
 *  - the automatic plot area alignment of the grid members
 *    ( QwtFigure::setGridAxisAlignment() ): the plots of a column share the
 *    left and right canvas edges, the plots of a row share the top and bottom ones
 *  - leaving the grid while the members keep their geometry ( QwtFigure::disableGrid() )
 *  - the current axes tracked from real mouse presses ( QwtFigure::currentAxesChanged() )
 *
 * The class also exposes a few hooks used by the unattended screenshot variants
 * ( see examples/screenshots-manifest.json ).
 */
class GridWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GridWindow(QWidget* parent = nullptr);
    ~GridWindow() override;

    // Apply one of the track ratio presets: "equal", "focus-top" or "golden"
    void applyPreset(const QString& name);

    // Change the gaps of the retained grid and sync the control panel
    void setGridSpacing(qreal wspace, qreal hspace);

    // Drag a divider by a number of pixels, using synthetic mouse events
    void dragDivider(int dividerIndex, int deltaPixels);

    // Leave the retained grid (the members keep their geometry) and re-enter it
    void leaveGrid();
    void reenterGrid();

    // Enable/disable the automatic plot area alignment and sync the control panel
    void setAxisAlignment(bool on);

private:
    // Cell of one member of the retained grid
    struct Member
    {
        // A member belongs to the grid until it is removed at runtime
        Member(QWidget* memberWidget, int memberRow, int memberColumn, int memberRowSpan, int memberColumnSpan)
            : widget(memberWidget)
            , row(memberRow)
            , column(memberColumn)
            , rowSpan(memberRowSpan)
            , columnSpan(memberColumnSpan)
        {
        }

        QWidget* widget { nullptr };
        int row { 0 };
        int column { 0 };
        int rowSpan { 1 };
        int columnSpan { 1 };
        bool inGrid { true };
    };

    void createFigure();
    void createToolBar();
    void createControlPanel();

    QwtPlot* createSpectrumPlot();
    QwtPlot* createWaveFormPlot();
    QwtPlot* createScatterPlot();
    QwtPlot* createBandPlot();
    QwtPlot* createTrendPlot();
    QWidget* createNotesPanel();

    void registerMembers();
    void setBarChartVisible(bool on);
    void setGridActive(bool on);

    // Push the current ratios on the undo stack when they differ from the last commit
    void commitRatios();
    void updateStatus();
    void updateNotesPanel();
    void logLayout();

    QString ratiosText(const QVector< qreal >& ratios) const;

private:
    static const int ROW_COUNT = 3;
    static const int COLUMN_COUNT = 3;

    QwtFigure* m_figure { nullptr };
    QwtFigureGrid* m_grid { nullptr };
    QwtFigureGridSplitter* m_splitter { nullptr };
    QVector< Member > m_members;
    QWidget* m_barChartCell { nullptr };

    QUndoStack* m_undoStack { nullptr };
    QVector< qreal > m_committedRows;
    QVector< qreal > m_committedColumns;

    // QPointer: the labels are destroyed with the window, a late signal must not touch them
    QPointer< QLabel > m_statusLabel;
    QPointer< QLabel > m_notesLabel;
    QCheckBox* m_editableBox { nullptr };
    QCheckBox* m_alignBox { nullptr };
    QSpinBox* m_hitWidthBox { nullptr };
    QSpinBox* m_minTrackBox { nullptr };
    QDoubleSpinBox* m_wspaceBox { nullptr };
    QDoubleSpinBox* m_hspaceBox { nullptr };
    QPushButton* m_activeCellsButton { nullptr };
    QPushButton* m_leaveGridButton { nullptr };
};

#endif  // GRIDWINDOW_H
