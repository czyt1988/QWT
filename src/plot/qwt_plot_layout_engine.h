#ifndef QWTPLOTLAYOUTENGINE_H
#define QWTPLOTLAYOUTENGINE_H
#include <QRectF>
#include <QSize>
#include <QFont>
class QWidget;
// qwt
#include "qwt_global.h"
#include "qwt_axis.h"
#include "qwt_axis_id.h"
#include "qwt_text.h"
#include "qwt_plot.h"
class QwtAbstractLegend;
class QwtTextLabel;
class QwtScaleWidget;
/**
 * @brief Layout engine for QwtPlot components
 * @details Originally a private class in QwtPlotLayout, previously written as class LayoutEngine in
 * qwt_plot_layout.cpp. It was extracted as a public class because other layouts need to use it.
 */
class QWT_EXPORT QwtPlotLayoutEngine
{
public:
    /**
     * @brief Structure holding dimension values for layout calculation
     * @details Contains width/height values for title, footer, and all four axes.
     */
    struct Dimensions
    {
        // Constructor
        Dimensions();

        // Get the dimension for a specific axis
        int dimAxis(QwtAxisId axisId) const;

        // Set the dimension for a specific axis
        void setDimAxis(QwtAxisId axisId, int dim);

        // Get the dimension for an axis position (YLeft, YRight, XTop, XBottom)
        int dimAxes(int axisPos) const;

        // Get the total width of left and right Y axes
        int dimYAxes() const;

        // Get the total height of top and bottom X axes
        int dimXAxes() const;

        // Center a label rectangle within available space after accounting for Y axes
        QRectF centered(const QRectF& rect, const QRectF& labelRect) const;

        // Calculate inner rectangle after reserving space for all axes
        QRectF innerRect(const QRectF& rect) const;

        int dimTitle;
        int dimFooter;

    private:
        int m_dimAxes[ QwtAxis::AxisPositions ];
    };

    /**
     * @brief Data structure for layout calculation
     * @details Contains cached data extracted from plot components for efficient layout calculation.
     */
    class LayoutData
    {
    public:
        /**
         * @brief Data for legend layout calculation
         * @details Contains frame width, scroll extents, and size hint for the legend.
         */
        struct LegendData
        {
            // Initialize legend data from a QwtAbstractLegend
            void init(const QwtAbstractLegend* legend);

            // Calculate optimal legend size for the given rectangle
            QSize legendHint(const QwtAbstractLegend* legend, const QRectF& rect) const;

            int frameWidth;
            int hScrollExtent;
            int vScrollExtent;
            QSize hint;
        };

        /**
         * @brief Data for title/footer label layout calculation
         * @details Contains text and frame width for a label widget.
         */
        struct LabelData
        {
            // Initialize label data from a QwtTextLabel
            void init(const QwtTextLabel* label);

            QwtText text;
            int frameWidth;
        };

        /**
         * @brief Data for axis scale widget layout calculation
         * @details Contains visibility, font, border distances, and tick offset for an axis.
         */
        struct ScaleData
        {
            // Initialize scale data from a QwtScaleWidget
            void init(const QwtScaleWidget* axisWidget);

            // Reset scale data to default values
            void reset();

            bool isVisible;
            const QwtScaleWidget* scaleWidget;
            QFont scaleFont;
            int start;
            int end;
            int baseLineOffset;
            double tickOffset;
            int dimWithoutTitle;
            int edgeMargin;
        };

        /**
         * @brief Data for canvas layout calculation
         * @details Contains content margins for the canvas widget.
         */
        struct CanvasData
        {
            // Initialize canvas data from a QWidget
            void init(const QWidget* canvas);

            int contentsMargins[ QwtAxis::AxisPositions ];
        };

        /**
         * @brief Label type enumeration
         * @details Identifies title or footer labels in the layout.
         */
        enum Label
        {
            Title,   //!< Title label
            Footer,  //!< Footer label

            NumLabels  //!< Number of label types
        };

        // Construct LayoutData from a QwtPlot
        LayoutData(const QwtPlot* plot);

        // Check if left and right Y axes have the same visibility state
        bool hasSymmetricYAxes() const;

        // Get mutable scale data for a specific axis
        ScaleData& axisData(QwtAxisId axisId);

        // Get const scale data for a specific axis
        const ScaleData& axisData(QwtAxisId axisId) const;

        // Get tick offset for a specific axis position
        double tickOffset(int axisPos) const;

        LegendData legendData;
        LabelData labelData[ NumLabels ];
        CanvasData canvasData;

    private:
        ScaleData m_scaleData[ QwtAxis::AxisPositions ];
    };

public:
    // Constructor
    QwtPlotLayoutEngine();

    // Calculate and return the legend rectangle within the available space
    QRectF layoutLegend(int plotLayoutOptions,
                        const LayoutData::LegendData& legendData,
                        const QRectF& rect,
                        const QSize& legendHint) const;

    // Align the legend rectangle relative to the canvas
    QRectF alignLegend(const QSize& legendHint, const QRectF& canvasRect, const QRectF& legendRect) const;

    // Align scale rectangles with the canvas, adjusting positions for proper layout
    void alignScales(int plotLayoutOptions,
                     const LayoutData& layoutData,
                     QRectF& canvasRect,
                     QRectF scaleRect[ QwtAxis::AxisPositions ]) const;

    // Align scale rectangles to canvas boundaries
    void alignScalesToCanvas(int plotLayoutOptions,
                             const LayoutData& layoutData,
                             const QRectF& canvasRect,
                             QRectF scaleRect[ QwtAxis::AxisPositions ]) const;

    // Calculate layout dimensions for title, footer, and all axes
    Dimensions layoutDimensions(int plotLayoutOptions, const LayoutData& layoutData, const QRectF& rect) const;

    // Set the spacing between plot components
    void setSpacing(unsigned int spacing);

    // Get the spacing between plot components
    unsigned int spacing() const;

    // Set whether the canvas should align to the scale at a given axis position
    void setAlignCanvas(int axisPos, bool on);

    // Check if the canvas is aligned to the scale at a given axis position
    bool alignCanvas(int axisPos) const;

    // Enable/disable fixed canvas size for an axis direction.
    // YLeft/YRight fix the canvas width; XBottom/XTop fix the canvas height.
    // When enabled, the canvas dimension is captured on the next layout and
    // held stable against axis label growth; overflowing labels are clipped.
    void setFixedCanvas(int axisPos, bool on);

    // Check if fixed canvas size is enabled for a given axis position
    bool isFixedCanvas(int axisPos) const;

    // Set a manual fixed canvas size, overriding the auto-captured value.
    // A component < 0 means "use auto-capture" for that direction.
    void setFixedCanvasSize(const QSize& size);

    // Get the manual fixed canvas size (-1 component = auto-capture)
    QSize fixedCanvasSize() const;

    // True if canvas width is fixed (any Y axis fixed or manual width set)
    bool isFixedCanvasWidth() const;

    // True if canvas height is fixed (any X axis fixed or manual height set)
    bool isFixedCanvasHeight() const;

    // Set the margin between canvas and scale at a given axis position
    void setCanvasMargin(int axisPos, int margin);

    // Get the margin between canvas and scale at a given axis position
    int canvasMargin(int axisPos) const;

    // Set the legend position
    void setLegendPos(QwtPlot::LegendPosition pos);

    // Get the legend position
    QwtPlot::LegendPosition legendPos() const;

    // Set the legend ratio for size calculation
    void setLegendRatio(double ratio);

    // Get the legend ratio for size calculation
    double legendRatio() const;

private:
    /**
     * @brief Get height for width
     */
    int heightForWidth(LayoutData::Label labelType,
                       const LayoutData& layoutData,
                       int plotLayoutOptions,
                       double width,
                       int axesWidth) const;

    QwtPlot::LegendPosition m_legendPos;
    double m_legendRatio;

    unsigned int m_canvasMargin[ QwtAxis::AxisPositions ] = { 0, 0, 0, 0 };
    bool m_alignCanvas[ QwtAxis::AxisPositions ];

    bool m_fixedCanvas[ QwtAxis::AxisPositions ] = { false, false, false, false };
    QSize m_fixedCanvasSize { -1, -1 };

    unsigned int m_spacing;
};

#endif  // QWTPLOTLAYOUTENGINE_H
