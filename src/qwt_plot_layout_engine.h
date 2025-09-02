#ifndef QWTPLOTLAYOUTENGINE_H
#define QWTPLOTLAYOUTENGINE_H
#include <QRectF>
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
 * @brief The QwtPlotLayoutEngine class
 *
 * 原来的QwtPlotLayout里的私有类，原来此类写在qwt_plot_layout.cpp中，class LayoutEngine，由于其它布局会用到，把它提取为公共类
 */
class QWT_EXPORT QwtPlotLayoutEngine
{
public:
    struct Dimensions
    {
        Dimensions();
        int dimAxis(QwtAxisId axisId) const;
        void setDimAxis(QwtAxisId axisId, int dim);
        int dimAxes(int axisPos) const;
        int dimYAxes() const;
        int dimXAxes() const;
        QRectF centered(const QRectF& rect, const QRectF& labelRect) const;
        QRectF innerRect(const QRectF& rect) const;
        int dimTitle;
        int dimFooter;

    private:
        int m_dimAxes[ QwtAxis::AxisPositions ];
    };

    class LayoutData
    {
    public:
        struct LegendData
        {
            void init(const QwtAbstractLegend* legend);
            QSize legendHint(const QwtAbstractLegend* legend, const QRectF& rect) const;
            int frameWidth;
            int hScrollExtent;
            int vScrollExtent;
            QSize hint;
        };

        struct LabelData
        {
            void init(const QwtTextLabel* label);
            QwtText text;
            int frameWidth;
        };

        struct ScaleData
        {
            void init(const QwtScaleWidget* axisWidget);
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

        struct CanvasData
        {
            void init(const QWidget* canvas);
            int contentsMargins[ QwtAxis::AxisPositions ];
        };

    public:
        enum Label
        {
            Title,
            Footer,

            NumLabels
        };

        LayoutData(const QwtPlot* plot);
        bool hasSymmetricYAxes() const;
        ScaleData& axisData(QwtAxisId axisId);
        const ScaleData& axisData(QwtAxisId axisId) const;
        double tickOffset(int axisPos) const;

        LegendData legendData;
        LabelData labelData[ NumLabels ];
        CanvasData canvasData;

    private:
        ScaleData m_scaleData[ QwtAxis::AxisPositions ];
    };

public:
    QwtPlotLayoutEngine();

    QRectF layoutLegend(int plotLayoutOptions,
                        const LayoutData::LegendData& legendData,
                        const QRectF& rect,
                        const QSize& legendHint) const;

    QRectF alignLegend(const QSize& legendHint, const QRectF& canvasRect, const QRectF& legendRect) const;

    void alignScales(int plotLayoutOptions,
                     const LayoutData& layoutData,
                     QRectF& canvasRect,
                     QRectF scaleRect[ QwtAxis::AxisPositions ]) const;
    void alignScalesToCanvas(int plotLayoutOptions,
                             const LayoutData& layoutData,
                             const QRectF& canvasRect,
                             QRectF scaleRect[ QwtAxis::AxisPositions ]) const;

    Dimensions layoutDimensions(int plotLayoutOptions, const LayoutData& layoutData, const QRectF& rect) const;

    void setSpacing(unsigned int spacing);
    unsigned int spacing() const;

    void setAlignCanvas(int axisPos, bool on);
    bool alignCanvas(int axisPos) const;

    void setCanvasMargin(int axisPos, int margin);
    int canvasMargin(int axisPos) const;

    void setLegendPos(QwtPlot::LegendPosition pos);
    QwtPlot::LegendPosition legendPos() const;

    void setLegendRatio(double ratio);
    double legendRatio() const;

private:
    int heightForWidth(LayoutData::Label labelType,
                       const LayoutData& layoutData,
                       int plotLayoutOptions,
                       double width,
                       int axesWidth) const;

    QwtPlot::LegendPosition m_legendPos;
    double m_legendRatio;

    unsigned int m_canvasMargin[ QwtAxis::AxisPositions ] = { 0, 0, 0, 0 };
    bool m_alignCanvas[ QwtAxis::AxisPositions ];

    unsigned int m_spacing;
};

#endif  // QWTPLOTLAYOUTENGINE_H
