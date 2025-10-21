#ifndef QWT_FIGURE_H
#define QWT_FIGURE_H
// stl
#include <memory>

// Qt
#include <QFrame>
class QResizeEvent;
class QPaintEvent;

// qwt
#include "qwt_global.h"
#include "qwt_axis.h"
class QwtPlot;

/**
 * @class QwtFigure
 * @brief A figure container for organizing Qwt plots with flexible layout options
 * @brief 用于组织Qwt绘图的图形容器，提供灵活的布局选项
 *
 * This class provides a figure-like container similar to matplotlib's Figure class,
 * supporting both normalized coordinate positioning and grid layouts for Qwt plots.
 * It uses Qt's standard top-left coordinate system for intuitive positioning.
 *
 * 此类提供类似于matplotlib的Figure类的图形容器，支持Qwt绘图的归一化坐标定位和网格布局。
 * 它使用Qt的标准左上角坐标系，使定位更加直观。
 *
 * @code
 * // Example usage:
 * // 使用示例：
 * QwtFigure figure;
 *
 * // Add a plot using normalized coordinates (Qt top-left coordinate system)
 * // 使用归一化坐标添加绘图（Qt左上角坐标系）
 * QwtPlot* plot1 = new QwtPlot;
 * figure.addAxes(plot1, QRectF(0.1, 0.1, 0.8, 0.4)); // Left: 10%, Top: 10%, Width: 80%, Height: 40%
 *
 * // Add plots using grid layout
 * // 使用网格布局添加绘图
 * // Create a 2x2 grid:
 * // 创建2x2网格：
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |      (0,0)        |       (0,1)       |
 * // |                   |                   |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |      (1,0)        |       (1,1)       |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * QwtPlot* plot2 = new QwtPlot;
 * figure.addAxes(plot2, 2, 2, 0, 1); // 2x2 grid, row 0, column 1
 * // Result:
 * // 结果：
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |                   |      plot2        |
 * // |                   |                   |
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |                   |                   |
 * // |                   |                   |
 * // +-------------------+-------------------+
 *
 * QwtPlot* plot3 = new QwtPlot;
 * figure.addAxes(plot3, 2, 2, 1, 0, 1, 2); // 2x2 grid, row 1, columns 0-1 (span 2 columns)
 * // Result:
 * // 结果：
 * // +-------------------+-------------------+
 * // |                   |                   |
 * // |                   |      plot2        |
 * // |                   |                   |
 * // +---------------------------------------+
 * // |                                       |
 * // |              plot3 (span 2 cols)      |
 * // |                                       |
 * // +---------------------------------------+
 *
 * // Adjust layout parameters
 * // 调整布局参数
 * figure.adjustLayout(0.1, 0.1, 0.9, 0.9, 0.2, 0.2);
 *
 * // Save the figure
 * // 保存图形
 * figure.saveFig("output.png", 300);
 * @endcode
 */
class QWT_EXPORT QwtFigure : public QFrame
{
    Q_OBJECT
public:
    QwtFigure(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~QwtFigure();
    // Add a plot with normalized coordinates/使用归一化坐标添加绘图
    void addAxes(QwtPlot* plot, const QRectF& rect);

    // Add a plot with normalized coordinates using separate parameters/使用分离参数和归一化坐标添加窗口部件
    void addAxes(QwtPlot* plot, qreal left, qreal top, qreal width, qreal height);

    // Add a plot by grid layout/添加窗口部件到网格布局
    void addAxes(QwtPlot* plot,
                 int rowCnt,
                 int colCnt,
                 int row,
                 int col,
                 int rowSpan  = 1,
                 int colSpan  = 1,
                 qreal wspace = 0.0,
                 qreal hspace = 0.0);

    // Update layout parameters/更新布局参数
    void adjustLayout(qreal left, qreal bottom, qreal right, qreal top);

    // Get all axes (plots) in the figure（not contain parasite axes）/获取图形中的所有坐标轴（绘图）(不包含寄生轴)
    QList< QwtPlot* > allAxes() const;

    // Check if the figure has any axes/检查图形是否有任意绘图
    bool hasAxes() const;

    // Check if the figure has plot/检查图形是否存在plot
    bool hasAxes(QwtPlot* plot) const;

    // Remove a specific axes (plot) from the figure/从图形中移除特定的坐标轴（绘图）/This function does not destroy the QwtPlot object
    void removeAxes(QwtPlot* plot);

    //  Take a specific axes (plot) from the figure without deleting it/从图形中取出特定的坐标轴（绘图）但不删除它
    bool takeAxes(QwtPlot* plot);

    // Clear all axes from the figure/清除图形中的所有坐标轴
    void clear();

    //  Get the size of the figure in inches/获取图形的英寸尺寸
    QSize getSizeInches() const;

    // Set the size of the figure in inches/设置图形的英寸尺寸
    void setSizeInches(float width, float height);
    void setSizeInches(const QSizeF& size);

    //  Set/Get the face color of the figure/设置图形的背景颜色
    void setFaceColor(const QColor& color);
    QColor faceColor() const;

    // Set/Get the face brush of the figure/设置图形的背景画刷
    void setFaceBrush(const QBrush& brush);
    QBrush faceBrush() const;

    // Set/Get the edge color of the figure/设置图形的边缘颜色
    void setEdgeColor(const QColor& color);
    QColor edgeColor() const;

    // Set/Get the edge line width of the figure/设置图形的边缘线宽
    void setEdgeLineWidth(int width);
    int edgeLineWidth() const;

    // Parasite Axes
    // Create parasite axes for a host plot/为宿主绘图创建寄生轴
    QwtPlot* createParasiteAxes(QwtPlot* hostPlot, QwtAxis::Position enableAxis, bool shareX = true, bool shareY = false);
    // Get all parasite axes for a host plot/获取宿主绘图的所有寄生轴
    QList< QwtPlot* > getParasiteAxes(QwtPlot* hostPlot) const;

    // Save methods / 保存方法
    // Save the figure to a QPixmap with specified DPI/使用指定DPI将图形保存为QPixmap
    QPixmap saveFig(int dpi = -1) const;

    // Save the figure to a QPixmap with specified size in inches/使用指定英寸尺寸将图形保存为QPixmap
    QPixmap saveFig(QSizeF& inchesSize) const;

    // Save the figure to a file with specified DPI/使用指定DPI将图形保存到文件
    bool saveFig(const QString& filename, int dpi = -1) const;

    // Set the current axes (plot)/设置当前坐标轴（绘图）
    void setCurrentAxes(QwtPlot* plot);
    void sca(QwtPlot* plot);

    // Get the current axes (plot)/获取当前坐标轴（绘图）
    QwtPlot* currentAxes() const;
    QwtPlot* gca() const;

    // Get the normalized rectangle for a axes/获取绘图的归一化矩形
    QRectF axesNormRect(QwtPlot* plot) const;
Q_SIGNALS:
    /**
     * @brief Signal emitted when axes are added to the figure/当坐标轴添加到图形时发出的信号
     * @param newAxes Pointer to the newly added QwtPlot / 指向新添加的QwtPlot的指针
     * @note 寄生轴的添加也会触发此信号
     */
    void axesAdded(QwtPlot* newAxes);

    /**
     * @brief Signal emitted when axes are removed from the figure/当坐标轴从图形中移除时发出的信号
     * @param removedAxes Pointer to the removed QwtPlot / 指向被移除的QwtPlot的指针
     * @note 寄生轴的移除也会触发此信号
     */
    void axesRemoved(QwtPlot* removedAxes);

    /**
     * @brief Signal emitted when the figure is cleared/当图形被清除时发出的信号
     */
    void figureCleared();

    /**
     * @brief 当前激活的坐标系发生了改变的信号
     * @param current
     * @note 寄生轴不能作为当前axes
     * @note 此信号会携带空指针，说明没有设置任何有效的激活坐标系
     */
    void currentAxesChanged(QwtPlot* current);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    class PrivateData;
    std::unique_ptr< PrivateData > m_data;
};

#endif  // QWT_FIGURE_H
