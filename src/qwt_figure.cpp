/******************************************************************************
 * Qwt Widget Library
 * Copyright (C) 2024   ChenZongYan <czy.t@163.com>
 *****************************************************************************/
#include "qwt_figure.h"
// Qt
#include <QPainter>
#include <QPointer>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QWidgetItem>
#include <QMap>
#include <QDebug>
// qwt
#include "qwt_figure_layout.h"
#include "qwt_scale_engine.h"
#include "qwt_scale_widget.h"
#include "qwt_plot_layout.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_transparent_canvas.h"
#include "qwt_parasite_plot_layout.h"

#ifndef QWTFIGURE_SAFEGET_LAY
#define QWTFIGURE_SAFEGET_LAY(lay)                                                                                     \
    QwtFigureLayout* lay = qobject_cast< QwtFigureLayout* >(layout());                                                 \
    if (!lay) {                                                                                                        \
        return;                                                                                                        \
    }
#endif

#ifndef QWTFIGURE_SAFEGET_LAY_RET
#define QWTFIGURE_SAFEGET_LAY_RET(lay, ret)                                                                            \
    QwtFigureLayout* lay = qobject_cast< QwtFigureLayout* >(layout());                                                 \
    if (!lay) {                                                                                                        \
        return ret;                                                                                                    \
    }
#endif

class QwtFigure::PrivateData
{
    QWT_DECLARE_PUBLIC(QwtFigure)
public:
    PrivateData(QwtFigure* p);

public:
    QBrush faceBrush { Qt::white };   ///< Background color of the figure / 图形背景颜色
    QColor edgeColor { Qt::black };   ///< Border color of the figure / 图形边框颜色
    int edgeLineWidth { 0 };          ///< Border line width / 边框线宽
    QPointer< QwtPlot > currentAxes;  ///< Current active axes / 当前活动坐标轴
};

QwtFigure::PrivateData::PrivateData(QwtFigure* p) : q_ptr(p)
{
}

//----------------------------------------------------
// QwtFigure
//----------------------------------------------------

/**
 * @brief Constructor
 * @brief 构造函数
 * @param parent Parent widget / 父窗口部件
 * @param f Window flags / 窗口标志
 */
QwtFigure::QwtFigure(QWidget* parent, Qt::WindowFlags f) : QFrame(parent, f), QWT_PIMPL_CONSTRUCT
{
    setLayout(new QwtFigureLayout());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QwtFigure::~QwtFigure()
{
}

/**
 * @brief Add a widget with normalized coordinates/使用归一化坐标添加窗口
 * @param widget QWidget to add / 要添加的QWidget
 * @param left Normalized coordinates left in range [0,1]
 * @param top Normalized coordinates top in range [0,1]
 * @param width Normalized coordinates width in range [0,1]
 * @param height Normalized coordinates height in range [0,1]
 *
 * @note 即使添加的窗口是qwtplot,此函数也不会发射@ref axesAdded 信号，因此，如果你需要添加QwtPlot窗口，
 * 你应该使用@ref addAxes 函数,此函数是为了在figure窗口添加除QwtPlot以外的窗口使用的
 *
 * @sa addAxes
 */
void QwtFigure::addWidget(QWidget* widget, qreal left, qreal top, qreal width, qreal height)
{
    QWTFIGURE_SAFEGET_LAY(lay)
    if (widget && widget->parentWidget() != this) {
        widget->setParent(this);
    }
    lay->addAxes(widget, left, top, width, height);
}

/**
 * @brief Add a widget by grid layout/添加窗口部件到网格布局
 *
 * This method adds a widget to the grid layout at the specified position with optional row and column spans.
 *
 * 此方法将widget添加到网格布局中的指定位置，可选择跨行和跨列。
 *
 * @param plot widget to add / 要添加的widget
 * @param rowCnt Number of rows in the grid / 网格行数
 * @param colCnt Number of columns in the grid / 网格列数
 * @param row Grid row position (0-based) / 网格行位置（从0开始）
 * @param col Grid column position (0-based) / 网格列位置（从0开始）
 * @param rowSpan Number of rows to span (default: 1) / 跨行数（默认：1）
 * @param colSpan Number of columns to span (default: 1) / 跨列数（默认：1）
 * @param wspace Horizontal space between subplots [0,1] / 子图之间的水平间距 [0,1]
 * @param hspace Vertical space between subplots [0,1] / 子图之间的垂直间距 [0,1]
 *
 * @note 即使添加的窗口是qwtplot,此函数也不会发射@ref axesAdded 信号，因此，如果你需要添加QwtPlot窗口，
 * 你应该使用@ref addAxes 函数,此函数是为了在figure窗口添加除QwtPlot以外的窗口使用的
 *
 * @sa addAxes
 */
void QwtFigure::addWidget(QWidget* widget, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
    QWTFIGURE_SAFEGET_LAY(lay)
    if (widget && widget->parentWidget() != this) {
        widget->setParent(this);
    }
    lay->addGridAxes(widget, rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);
}

/**
 * @brief Add a plot with normalized coordinates/使用归一化坐标添加绘图
 *
 * This method adds a QwtPlot to the figure using normalized coordinates in the range [0,1].
 * The coordinates are specified as [left, bottom, width, height].
 *
 * 此方法使用[0,1]范围内的归一化坐标将QwtPlot添加到图形中。
 * 坐标指定为[左, 下, 宽, 高]。
 *
 * @param plot QwtPlot to add / 要添加的QwtPlot
 * @param rect Normalized coordinates [left, bottom, width, height] in range [0,1]
 *              归一化坐标 [左, 下, 宽, 高]，范围 [0,1]
 *
 * @code
 * // Add a plot that occupies the top-left quarter of the figure
 * // 添加一个占据图形左上角四分之一的绘图
 * QwtPlot* plot = new QwtPlot;
 * figure.addAxes(plot, QRectF(0.0, 0.5, 0.5, 0.5));
 * @endcode
 *
 * @note 此函数会发射@ref axesAdded 信号，此信号发射后发射@ref currentAxesChanged 信号
 */
void QwtFigure::addAxes(QwtPlot* plot, const QRectF& rect)
{
    addAxes(plot, rect.x(), rect.y(), rect.width(), rect.height());
}

/**
 * @brief Add a plot with normalized coordinates/使用归一化坐标添加绘图
 *
 * This method adds a QwtPlot to the figure using normalized coordinates in the range [0,1].
 * The coordinates are specified as [left, bottom, width, height].
 *
 * @param plot QwtPlot to add / 要添加的QwtPlot
 * @param left Normalized coordinates left in range [0,1]
 * @param top Normalized coordinates top in range [0,1]
 * @param width Normalized coordinates width in range [0,1]
 * @param height Normalized coordinates height in range [0,1]
 *
 * @note 此函数会发射@ref axesAdded 信号，此信号发射后发射@ref currentAxesChanged 信号
 */
void QwtFigure::addAxes(QwtPlot* plot, qreal left, qreal top, qreal width, qreal height)
{
    addWidget(plot, left, top, width, height);
    Q_EMIT axesAdded(plot);
    setCurrentAxes(plot);
}

/**
 * @brief Add a plot by grid layout/添加窗口部件到网格布局
 *
 * This method adds a QwtPlot to the grid layout at the specified position with optional row and column spans.
 *
 * 此方法将QwtPlot添加到网格布局中的指定位置，可选择跨行和跨列。
 *
 * @param plot QwtPlot to add / 要添加的QwtPlot
 * @param rowCnt Number of rows in the grid / 网格行数
 * @param colCnt Number of columns in the grid / 网格列数
 * @param row Grid row position (0-based) / 网格行位置（从0开始）
 * @param col Grid column position (0-based) / 网格列位置（从0开始）
 * @param rowSpan Number of rows to span (default: 1) / 跨行数（默认：1）
 * @param colSpan Number of columns to span (default: 1) / 跨列数（默认：1）
 * @param wspace Horizontal space between subplots [0,1] / 子图之间的水平间距 [0,1]
 * @param hspace Vertical space between subplots [0,1] / 子图之间的垂直间距 [0,1]
 *
 * @code
 * // Create a 2x2 grid and add plots
 * // 创建一个2x2网格并添加绘图
 *
 * // Add a plot that spans the entire top row (row 0, columns 0-1)
 * // 添加一个占据整个顶行（第0行，第0-1列）的绘图
 * QwtPlot* topPlot = new QwtPlot;
 * figure.addAxes(topPlot, 2, 2, 0, 0, 1, 2);
 *
 * // Add a plot to the bottom-left cell (row 1, column 0)
 * // 添加一个到底部左侧单元格（第1行，第0列）的绘图
 * QwtPlot* bottomLeftPlot = new QwtPlot;
 * figure.addAxes(bottomLeftPlot, 2, 2, 1, 0);
 * @endcode
 *
 * @note 此函数会发射@ref axesAdded 信号，此信号发射后发射@ref currentAxesChanged 信号
 */
void QwtFigure::addGridAxes(QwtPlot* plot, int rowCnt, int colCnt, int row, int col, int rowSpan, int colSpan, qreal wspace, qreal hspace)
{
    addWidget(plot, rowCnt, colCnt, row, col, rowSpan, colSpan, wspace, hspace);
    Q_EMIT axesAdded(plot);
    setCurrentAxes(plot);
}

/**
 * @brief 改变已经添加的窗口的位置占比,如果窗口还没添加，此函数无效
 * @param widget
 * @param rect
 */
void QwtFigure::setWidgetNormPos(QWidget* widget, const QRectF& rect)
{
    QWTFIGURE_SAFEGET_LAY(lay)
    lay->setAxesNormPos(widget, rect);
    lay->invalidate();
}

/**
 * @brief Get all axes (plots) in the figure（not contain parasite axes）/获取图形中的所有坐标轴（绘图）(不包含寄生轴)
 *
 * This method returns a list of all QwtPlot objects added to the figure.（not contain parasite axes）
 *
 * 此方法返回添加到图形中的所有QwtPlot对象的列表。(不包含寄生轴)
 *
 * @param byZOrder 是否按zorder排序，如果按zorder排序，按按 z 序从高到低排序
 *
 * @return List of all QwtPlot objects / 所有QwtPlot对象的列表(不包含寄生轴)
 *
 * @note 此方法获取的绘图不包含寄生轴
 *
 * @code
 * // Get all plots and update their titles
 * // 获取所有绘图并更新它们的标题
 * QList<QwtPlot*> plots = figure.allAxes();
 * for (int i = 0; i < plots.size(); ++i) {
 *     plots[i]->setTitle(QString("Plot %1").arg(i + 1));
 * }
 * @endcode
 */
QList< QwtPlot* > QwtFigure::allAxes(bool byZOrder) const
{
    QList< QwtPlot* > plots;
    QLayout* lay = layout();
    if (lay) {
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (item && item->widget()) {
                if (QwtPlot* plot = qobject_cast< QwtPlot* >(item->widget())) {
                    if (plot->isHostPlot()) {
                        plots.append(plot);
                    }
                }
            }
        }
    }
    if (!byZOrder || plots.isEmpty()) {
        return plots;  // 原顺序直接返回
    }

    /* 按 z-order 从高到低重新排（children() 越靠后 z 越高） */
    const QObjectList& oc = children();
    QList< QwtPlot* > zOrdered;
    zOrdered.reserve(plots.size());

    // 倒序扫一次，命中就搬
    for (auto it = oc.crbegin(); it != oc.crend(); ++it) {
        if (QwtPlot* p = qobject_cast< QwtPlot* >(*it)) {
            if (plots.contains(p)) {  // 只有 O(n) 小列表查找
                zOrdered.append(p);
            }
        }
    }

    return zOrdered;  // 已经是从顶到底
}

/**
 * @brief Check if the figure has any axes/检查图形是否有坐标轴
 *
 * This method returns true if the figure contains at least one QwtPlot.
 *
 * 如果图形包含至少一个QwtPlot，则此方法返回true。
 *
 * @return true if the figure has axes, false otherwise / 如果图形有坐标轴返回true，否则返回false
 *
 * @code
 * // Check if figure has axes before performing operations
 * // 在执行操作前检查图形是否有坐标轴
 * if (figure.hasAxes()) {
 *     // Do something with the plots
 *     // 对绘图进行操作
 * }
 * @endcode
 */
bool QwtFigure::hasAxes() const
{
    QLayout* lay = layout();
    if (!lay) {
        return false;
    }

    for (int i = 0; i < lay->count(); ++i) {
        QLayoutItem* item = lay->itemAt(i);
        if (item && item->widget() && qobject_cast< QwtPlot* >(item->widget())) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Check if the figure has any axes/检查图形是否有坐标轴
 *
 * This method returns true if the figure contains at least one QwtPlot.
 *
 * 如果图形包含至少一个QwtPlot，则此方法返回true。
 * @param plot QwtPlot to check / 要检测的QwtPlot
 * @return true if the figure has axes, false otherwise / 如果图形有坐标轴返回true，否则返回false
 *
 * @code
 * // Check if figure has axes before performing operations
 * // 在执行操作前检查图形是否有坐标轴
 * QwtPlot* plot;
 * ...
 * if (figure.hasAxes(plot)) {
 *     // Do something with the plots
 *     // 对绘图进行操作
 * }
 * @endcode
 */
bool QwtFigure::hasAxes(QwtPlot* plot) const
{
    QLayout* lay = layout();
    if (!lay || !plot) {
        return false;
    }

    for (int i = 0; i < lay->count(); ++i) {
        QLayoutItem* item = lay->itemAt(i);
        if (item) {
            if (QwtPlot* ax = qobject_cast< QwtPlot* >(item->widget())) {
                if (ax == plot) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief Remove a specific axes (plot) from the figure/从图形中移除特定的坐标轴（绘图）
 *
 * This method removes the specified QwtPlot from the figure.
 *
 * 此方法从图形中移除指定的QwtPlot。
 *
 * @param plot QwtPlot to remove / 要移除的QwtPlot
 *
 * @note This function will destroy the QwtPlot object
 * 此函数会销毁QwtPlot对象
 *
 * @code
 * // Remove a specific plot from the figure
 * // 从图形中移除特定的绘图
 * QwtPlot* plotToRemove = figure.getAllAxes().first();
 * figure.removeAxes(plotToRemove);
 * // 你需要手动删除它
 * plotToRemove->deletelater();
 * @endcode
 */
void QwtFigure::removeAxes(QwtPlot* plot)
{
    takeAxes(plot);
}

/**
 * @brief Take a specific axes (plot) from the figure without deleting it/从图形中取出特定的坐标轴（绘图）但不删除它
 * @param plot Pointer to the QwtPlot to take / 要取出的QwtPlot指针
 * @return Pointer to the taken QwtPlot, or nullptr if not found / 取出的QwtPlot指针，如果未找到则返回nullptr
 *
 * @note 如果当前的绘图是选择的激活坐标系，在移除时，会先发射@ref currentAxesChanged 信号，再发射@ref axesRemoved 信号
 *
 * @note 如果只有一个绘图，在移除后，整个figure没有绘图的情况下，也会发射@ref currentAxesChanged 信号，信号携带的内容为nullptr
 *
 * @note 如果一个绘图有寄生轴，再takeAxes后，它的寄生轴会设置为隐藏，并把parent widget设置为nullptr
 */
bool QwtFigure::takeAxes(QwtPlot* plot)
{
    if (!plot) {
        return false;
    }

    // Remove from layout
    bool isRemove = false;
    // Check if the plot to remove is the current axes
    // 检查要移除的绘图是否是当前坐标轴
    bool removingCurrent = (plot == currentAxes());
    QLayout* lay         = layout();
    if (lay) {
        for (int i = 0; i < lay->count(); ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (!item) {
                continue;
            }
            QWidget* w = item->widget();
            if (!w) {
                continue;
            }
            if (w == plot) {
                lay->removeItem(item);
                delete item;
                isRemove = true;
                break;
            }
        }
        if (removingCurrent) {
            // 说明移除了当前axes，需要更新currentAxes
            const int count = lay->count();
            if (count == 0) {
                // 如果figure已经清空，也发射currentAxesChanged，携带nullptr
                setCurrentAxes(nullptr);
            } else {
                for (int i = 0; i < count; ++i) {
                    QLayoutItem* item = lay->itemAt(i);
                    if (!item) {
                        continue;
                    }
                    if (QwtPlot* w = qobject_cast< QwtPlot* >(item->widget())) {
                        setCurrentAxes(w);
                    }
                }
            }
        }
    }
    if (isRemove) {
        // 处理寄生轴
        const QList< QwtPlot* > parasites = plot->parasitePlots();
        for (QwtPlot* para : parasites) {
            para->setParent(nullptr);
            para->hide();
        }
        Q_EMIT axesRemoved(plot);
    }
    return isRemove;
}

/**
 * @brief Clear all axes from the figure/清除图形中的所有坐标轴
 *
 * This method removes all QwtPlot objects from the figure and deletes them.
 *
 * 此方法从图形中移除所有QwtPlot对象并删除它们。
 *
 * @note 此方法在移除过程中会发射@ref axesRemoved 信号，
 * axesRemoved携带的绘图指针不应该被保存
 *
 * @note 此方法还会发射2个信号，先发射@ref currentAxesChanged 信号，此信号参数会携带nullptr，
 * 最后发射@ref figureCleared 信号
 *
 * @note 此方法会删除已经持有的所有plot窗口
 *
 * @code
 * // Clear all plots from the figure
 * // 清除图形中的所有绘图
 * figure.clear();
 * @endcode
 */
void QwtFigure::clear()
{
    // Remove from layout
    QLayout* lay  = layout();
    int removeCnt = 0;
    if (lay) {
        // lay->count()不能放到for循环里面，每次循环会变化
        const int itemCnt = lay->count();
        // 先删除窗口，最后再统一删除item，这个循环里面不能调用removeItem，否则每次都改变队列大小，就不能正常遍历
        for (int i = 0; i < itemCnt; ++i) {
            QLayoutItem* item = lay->itemAt(i);
            if (item) {
                if (QwtPlot* plot = qobject_cast< QwtPlot* >(item->widget())) {
                    Q_EMIT axesRemoved(plot);
                }
                if (QWidget* w = item->widget()) {
                    w->hide();
                    w->deleteLater();
                }
                ++removeCnt;
            }
        }
        // 最后再统一删除item
        for (int i = 0; i < itemCnt; ++i) {
            QLayoutItem* item = lay->itemAt(i);
            lay->removeItem(item);
            delete item;
        }
    }
    setCurrentAxes(nullptr);
    if (removeCnt > 0) {
        Q_EMIT figureCleared();
    }
}

/**
 * @brief Get the size of the figure in inches/获取图形的英寸尺寸
 *
 * This method calculates the physical size of the figure in inches based on
 * the current pixel size and screen DPI.
 *
 * 此方法基于当前像素尺寸和屏幕DPI计算图形的物理尺寸（英寸）。
 *
 * @return Size of the figure in inches / 图形的英寸尺寸
 *
 * @code
 * // Get the size of the figure in inches
 * // 获取图形的英寸尺寸
 * QSize sizeInInches = figure.getSizeInches();
 * qDebug() << "Figure size:" << sizeInInches.width() << "x" << sizeInInches.height() << "inches";
 * @endcode
 */
QSize QwtFigure::getSizeInches() const
{
    QScreen* screen = QGuiApplication::primaryScreen();
    int dpi         = screen ? screen->logicalDotsPerInch() : 96;

    QSize size = this->size();
    return QSize(size.width() / dpi, size.height() / dpi);
}

/**
 * @brief Set the size of the figure in inches/设置图形的英寸尺寸
 *
 * This method sets the size of the figure in inches, converting to pixels
 * based on the screen DPI.
 *
 * 此方法设置图形的英寸尺寸，基于屏幕DPI转换为像素。
 *
 * @param width Width in inches / 宽度（英寸）
 * @param height Height in inches / 高度（英寸）
 *
 * @code
 * // Set the figure size to 6x4 inches
 * // 将图形尺寸设置为6x4英寸
 * figure.setSizeInches(6.0, 4.0);
 * @endcode
 */
void QwtFigure::setSizeInches(float width, float height)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    int dpi         = screen ? screen->logicalDotsPerInch() : 96;

    int pixelWidth  = width * dpi;
    int pixelHeight = height * dpi;

    resize(pixelWidth, pixelHeight);
}

/**
 * @brief Set the size of the figure in inches/设置图形的英寸尺寸
 *
 * This method sets the size of the figure in inches, converting to pixels
 * based on the screen DPI.
 *
 * 此方法设置图形的英寸尺寸，基于屏幕DPI转换为像素。
 *
 * @param size Size in inches / 英寸尺寸
 *
 * @code
 * // Set the figure size to 6x4 inches
 * // 将图形尺寸设置为6x4英寸
 * figure.setSizeInches(QSizeF(6.0, 4.0));
 * @endcode
 */
void QwtFigure::setSizeInches(const QSizeF& size)
{
    setSizeInches(size.width(), size.height());
}

/**
 * @brief Set the face color of the figure/设置图形的背景颜色
 *
 * This method sets the background color of the figure.
 *
 * 此方法设置图形的背景颜色。
 *
 * @param color Background color / 背景颜色
 *
 * @code
 * // Set the figure background to light gray
 * // 将图形背景设置为浅灰色
 * figure.setFaceColor(Qt::lightGray);
 * @endcode
 */
void QwtFigure::setFaceColor(const QColor& color)
{
    m_data->faceBrush = color;
}

/**
 * @brief Get the face color of the figure/获取图形的表面颜色
 *
 * This method returns the background color of the figure.
 *
 * 此方法返回图形的背景颜色。
 *
 * @return Background color / 背景颜色
 *
 * @code
 * // Get the current background color
 * // 获取当前背景颜色
 * QColor bgColor = figure.faceColor();
 * @endcode
 */
QColor QwtFigure::faceColor() const
{
    return m_data->faceBrush.color();
}

/**
 * @brief Set the face brush of the figure/设置图形的背景画刷
 *
 * This method sets the background brush of the figure, allowing for
 * more complex backgrounds (gradients, textures, etc.).
 *
 * 此方法设置图形的背景画刷，允许更复杂的背景（渐变、纹理等）。
 *
 * @param brush Background brush / 背景画刷
 *
 * @code
 * // Set a gradient background
 * // 设置渐变背景
 * QLinearGradient gradient(0, 0, 0, 1);
 * gradient.setColorAt(0, Qt::white);
 * gradient.setColorAt(1, Qt::lightGray);
 * figure.setFaceBrush(QBrush(gradient));
 * @endcode
 */
void QwtFigure::setFaceBrush(const QBrush& brush)
{
    m_data->faceBrush = brush;
}

/**
 * @brief Get the face brush of the figure/获取图形的表面画刷
 *
 * This method returns the background brush of the figure.
 *
 * 此方法返回图形的背景画刷。
 *
 * @return Background brush / 背景画刷
 *
 * @code
 * // Get the current background brush
 * // 获取当前背景画刷
 * QBrush bgBrush = figure.faceBrush();
 * @endcode
 */
QBrush QwtFigure::faceBrush() const
{
    return m_data->faceBrush;
}

/**
 * @brief Set the edge color of the figure/设置图形的边缘颜色
 *
 * This method sets the border color of the figure.
 *
 * 此方法设置图形的边框颜色。
 *
 * @param color Border color / 边框颜色
 *
 * @code
 * // Set the figure border to black
 * // 将图形边框设置为黑色
 * figure.setEdgeColor(Qt::black);
 * @endcode
 */
void QwtFigure::setEdgeColor(const QColor& color)
{
    m_data->edgeColor = color;
}

/**
 * @brief Get the edge color of the figure/获取图形的边缘颜色
 *
 * This method returns the border color of the figure.
 *
 * 此方法返回图形的边框颜色。
 *
 * @return Border color / 边框颜色
 *
 * @code
 * // Get the current border color
 * // 获取当前边框颜色
 * QColor borderColor = figure.edgeColor();
 * @endcode
 */
QColor QwtFigure::edgeColor() const
{
    return m_data->edgeColor;
}

/**
 * @brief Set the edge line width of the figure/设置图形的边缘线宽
 *
 * This method sets the border line width of the figure.
 *
 * 此方法设置图形的边框线宽。
 *
 * @param width Border line width in pixels / 边框线宽（像素）
 *
 * @code
 * // Set the figure border width to 2 pixels
 * // 将图形边框宽度设置为2像素
 * figure.setEdgeLineWidth(2);
 * @endcode
 */
void QwtFigure::setEdgeLineWidth(int width)
{
    m_data->edgeLineWidth = width;
}

/**
 * @brief Get the edge line width of the figure/获取图形的边缘线宽
 *
 * This method returns the border line width of the figure.
 *
 * 此方法返回图形的边框线宽。
 *
 * @return Border line width in pixels / 边框线宽（像素）
 *
 * @code
 * // Get the current border width
 * // 获取当前边框宽度
 * int borderWidth = figure.edgeLineWidth();
 * @endcode
 */
int QwtFigure::edgeLineWidth() const
{
    return m_data->edgeLineWidth;
}

/**
 * @brief Create parasite axes for a host plot/为宿主绘图创建寄生轴
 *
 * This method creates a parasite axes that shares the same plotting area as the host plot
 * but with independent axis scaling and labeling. The parasite axes will be positioned
 * exactly on top of the host plot and will automatically synchronize its geometry.
 *
 * 此方法创建一个寄生轴，它与宿主绘图共享相同的绘图区域，但具有独立的轴缩放和标签。
 * 寄生轴将精确定位在宿主绘图之上，并自动同步其几何形状。
 *
 * @param hostPlot Pointer to the host QwtPlot/指向宿主QwtPlot的指针
 * @param enableAxis The axis position to enable on the parasite axes/在寄生轴上启用的轴位置
 * @param shareX If true, share X-axis scale with host plot/如果为true，与宿主绘图共享X轴刻度
 * @param shareY If true, share Y-axis scale with host plot/如果为true，与宿主绘图共享Y轴刻度
 * @return Pointer to the created parasite QwtPlot/指向创建的寄生QwtPlot的指针
 * @retval nullptr if hostPlot is invalid or not in the figure/如果hostPlot无效或不在图形中则返回nullptr
 *
 * @note The parasite axes will have a transparent background and only the specified axis will be visible.
 *       /寄生轴将具有透明背景，只有指定的轴可见。
 * @note The parasite axes will automatically be deleted when the host plot is removed from the figure.
 *       /当宿主绘图从图形中移除时，寄生轴将自动被删除。
 * @note Parasitic axes are not stored in QwtFigureLayout, but are separately controlled by QwtFigure for layout
 * management /寄生轴不会存入QwtFigureLayout中，单独由QwtFigure进行布局控制
 *
 * @code
 * // Create a host plot
 * // 创建宿主绘图
 * QwtPlot* hostPlot = new QwtPlot(figure);
 * figure->addAxes(hostPlot, 0.1, 0.1, 0.8, 0.8);
 *
 * // Create parasite axes with YRight axis enabled and sharing Y-axis scale
 * // 创建寄生轴，启用YRight轴并共享X轴刻度
 * QwtPlot* parasiteYRight = figure->createParasiteAxes(hostPlot, QwtAxis::YRight, true, false);
 *
 * // Add curves to both plots
 * // 向两个绘图添加曲线
 * QwtPlotCurve* curve1 = new QwtPlotCurve("Host Curve");
 * curve1->setSamples(xData, yData1);
 * curve1->attach(hostPlot);
 *
 * QwtPlotCurve* curve2 = new QwtPlotCurve("Parasite Curve");
 * curve2->setSamples(xData, yData2);
 * curve2->attach(parasiteYRight);
 *
 * // Set different axis titles
 * // 设置不同的轴标题
 * hostPlot->setAxisTitle(QwtAxis::YLeft, "Primary Y");
 * parasiteYRight->setAxisTitle(QwtAxis::YRight, "Secondary Y");
 * @endcode
 *
 * @see getParasiteAxes()
 *
 * @note 寄生轴必须有figure来管理，这是因为寄生轴仅仅是绘图区域和宿主重叠，坐标窗口的位置都和宿主不一样
 */
QwtPlot* QwtFigure::createParasiteAxes(QwtPlot* hostPlot, QwtAxis::Position enableAxis)
{
    if (!hostPlot || !hasAxes(hostPlot)) {
        qWarning() << "Invalid host plot or host plot not in figure";
        return nullptr;
    }
    if (hostPlot->isParasitePlot()) {
        // 不是宿主，切换为宿主
        hostPlot = hostPlot->hostPlot();
    }
    // 创建寄生轴
    QwtPlot* parasitePlot = hostPlot->createParasitePlot(enableAxis);

    return parasitePlot;
}

/**
 * @brief Get all parasite axes for a host plot/获取宿主绘图的所有寄生轴
 *
 * This method returns a list of all parasite axes associated with the specified host plot.
 *
 * 此方法返回与指定宿主绘图关联的所有寄生轴的列表。
 *
 * @param hostPlot Pointer to the host QwtPlot/指向宿主QwtPlot的指针
 * @return List of parasite QwtPlot pointers/寄生QwtPlot指针列表
 * @retval Empty list if hostPlot is invalid or has no parasite axes/如果hostPlot无效或没有寄生轴则返回空列表
 *
 * @see createParasiteAxes()
 */
QList< QwtPlot* > QwtFigure::getParasiteAxes(QwtPlot* hostPlot) const
{
    if (!hostPlot) {
        return QList< QwtPlot* >();
    }
    return hostPlot->parasitePlots();
}

/**
 * @brief Save the figure to a QPixmap with specified DPI/使用指定DPI将图形保存为QPixmap
 *
 * This method renders the figure to a QPixmap with the specified DPI.
 * If DPI is -1, the current screen DPI is used.
 *
 * 此方法将图形渲染为具有指定DPI的QPixmap。
 * 如果DPI为-1，则使用当前屏幕DPI。
 *
 * @param dpi Dots per inch for the saved image (-1 to use screen DPI) / 保存图像的DPI（-1表示使用屏幕DPI）
 * @return QPixmap containing the rendered figure / 包含渲染图形的QPixmap
 *
 * @code
 * // Save the figure with screen DPI
 * // 使用屏幕DPI保存图形
 * QPixmap pixmap1 = figure.saveFig();
 *
 * // Save the figure with 300 DPI
 * // 使用300 DPI保存图形
 * QPixmap pixmap2 = figure.saveFig(300);
 * @endcode
 */
QPixmap QwtFigure::saveFig(int dpi) const
{
    // Calculate the target size based on DPI
    QSize targetSize;
    int targetDpi;

    if (dpi <= 0) {
        QScreen* screen = QGuiApplication::primaryScreen();
        targetDpi       = screen ? screen->logicalDotsPerInch() : 96;
        targetSize      = size();
    } else {
        targetDpi                 = dpi;
        QSizeF physicalSizeInches = getSizeInches();
        targetSize                = QSize(static_cast< int >(physicalSizeInches.width() * dpi),
                           static_cast< int >(physicalSizeInches.height() * dpi));
    }

    // Use const_cast to call non-const methods
    QwtFigure* nonConstThis = const_cast< QwtFigure* >(this);

    if (dpi <= 0) {
        // No scaling needed, just grab the current state
        return nonConstThis->grab();
    }
    // Create pixmap with target size
    QPixmap pixmap(targetSize);

    // Use QPainter for high-quality scaling
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Calculate scaling factors
    qreal scaleX = static_cast< qreal >(targetSize.width()) / width();
    qreal scaleY = static_cast< qreal >(targetSize.height()) / height();
    painter.scale(scaleX, scaleY);

    // Render the figure with scaling
    nonConstThis->render(&painter);
    painter.end();

    // Set DPI information if needed
    // 当你在图像文件中设置DPI信息时，图像处理软件（如Photoshop、GIMP等）和打印机会知道如何正确解释图像的物理尺寸。
    // 如果没有DPI信息，软件通常会使用默认的DPI（通常是72或96），这会导致物理尺寸计算错误。
    // 不同的设备和软件可能有不同的默认DPI设置。明确设置DPI可以确保图像在所有平台上显示一致的物理尺寸。
    QImage image = pixmap.toImage();
    // Convert DPI to dots per meter (1 inch = 2.54 cm, so 1 meter = 100/2.54 inches)
    // 将DPI转换为每米的点数（1英寸=2.54厘米，所以1米=100/2.54英寸）
    image.setDotsPerMeterX(targetDpi * 100 / 2.54);
    image.setDotsPerMeterY(targetDpi * 100 / 2.54);
    return QPixmap::fromImage(image);
}

/**
 * @brief Save the figure to a QPixmap with specified size in inches/使用指定英寸尺寸将图形保存为QPixmap
 *
 * This method renders the figure to a QPixmap with the specified physical size in inches.
 * The current DPI setting of the figure is used to calculate the pixel size.
 *
 * 此方法将图形渲染为具有指定物理尺寸（英寸）的QPixmap。
 * 使用图形当前的DPI设置来计算像素尺寸。
 *
 * @param inchesSize Physical size in inches / 物理尺寸（英寸）
 * @return QPixmap containing the rendered figure / 包含渲染图形的QPixmap
 *
 * @code
 * // Save the figure as a 6x4 inch image
 * // 将图形保存为6x4英寸的图像
 * QPixmap pixmap = figure.saveFig(QSizeF(6.0, 4.0));
 * @endcode
 */
QPixmap QwtFigure::saveFig(QSizeF& inchesSize) const
{
    // Use current DPI to calculate target pixel size
    // 使用当前DPI计算目标像素尺寸
    QScreen* screen = QGuiApplication::primaryScreen();
    int currentDpi  = screen ? screen->logicalDotsPerInch() : 96;
    QSize targetSize(static_cast< int >(inchesSize.width() * currentDpi),
                     static_cast< int >(inchesSize.height() * currentDpi));

    // Use const_cast to call non-const methods
    // 使用const_cast调用非const方法
    QwtFigure* nonConstThis = const_cast< QwtFigure* >(this);

    // Create pixmap with target size
    // 创建目标尺寸的pixmap
    QPixmap pixmap(targetSize);

    // Use QPainter for high-quality scaling
    // 使用QPainter进行高质量缩放
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Calculate scaling factors
    // 计算缩放因子
    qreal scaleX = static_cast< qreal >(targetSize.width()) / width();
    qreal scaleY = static_cast< qreal >(targetSize.height()) / height();
    painter.scale(scaleX, scaleY);

    // Render the figure with scaling
    // 渲染图形并应用缩放
    nonConstThis->render(&painter);
    painter.end();

    // Set DPI information
    // 设置DPI信息
    QImage image = pixmap.toImage();
    image.setDotsPerMeterX(currentDpi * 100 / 2.54);
    image.setDotsPerMeterY(currentDpi * 100 / 2.54);
    return QPixmap::fromImage(image);
}

/**
 * @brief Save the figure to a file with specified DPI/使用指定DPI将图形保存到文件
 *
 * This method saves the figure to an image file with the specified DPI.
 *
 * 此方法将图形保存为具有指定DPI的图像文件。
 *
 * @param filename Name of the file to save / 要保存的文件名
 * @param dpi Dots per inch for the saved image (-1 to use screen DPI) / 保存图像的DPI（-1表示使用屏幕DPI）
 * @return true if saved successfully, false otherwise / 成功保存返回true，否则返回false
 *
 * @code
 * // Save the figure with screen DPI
 * // 使用屏幕DPI保存图形
 * figure.saveFig("figure.png");
 *
 * // Save the figure with 300 DPI
 * // 使用300 DPI保存图形
 * figure.saveFig("high_res_figure.png", 300);
 * @endcode
 */
bool QwtFigure::saveFig(const QString& filename, int dpi) const
{
    QPixmap pixmap = saveFig(dpi);
    return pixmap.save(filename, nullptr, -1);
}

/**
 * @brief Set the current axes (plot)/设置当前坐标轴（绘图）
 *
 * This method sets the specified QwtPlot as the current active axes in the figure.
 *
 * 此方法将指定的QwtPlot设置为图形中当前活动的坐标轴。
 *
 * @param plot QwtPlot to set as current / 要设置为当前的QwtPlot
 *
 * @code
 * // Set a specific plot as current axes
 * // 将特定绘图设置为当前坐标轴
 * QList<QwtPlot*> plots = figure.getAllAxes();
 * if (!plots.isEmpty()) {
 *     figure.setCurrentAxes(plots.first());  // Set first plot as current
 * }
 * @endcode
 */
void QwtFigure::setCurrentAxes(QwtPlot* plot)
{
    // 允许设置为 nullptr，或仅当 plot 属于本 figure 管理时才设置
    if (plot == nullptr || hasAxes(plot)) {
        m_data->currentAxes = plot;
        Q_EMIT currentAxesChanged(plot);
    }
}

/**
 * @brief Set the current axes (plot)/设置当前坐标轴（绘图）
 * @param plot QwtPlot to set as current / 要设置为当前的QwtPlot
 * @sa setCurrentAxes
 */
void QwtFigure::sca(QwtPlot* plot)
{
    setCurrentAxes(plot);
}

/**
 * @brief Get the current axes (plot)/获取当前坐标轴（绘图）
 *
 * This method returns the current active QwtPlot in the figure.
 * The current axes is typically the last axes that was added, modified, or plotted on.
 *
 * 此方法返回图形中当前活动的QwtPlot。
 * 当前坐标轴通常是最后添加、修改或绘图的坐标轴。
 *
 * @return Pointer to the current QwtPlot, or nullptr if no axes exist / 指向当前QwtPlot的指针，如果没有坐标轴则返回nullptr
 *
 * @code
 * // Get the current axes and plot some data
 * // 获取当前坐标轴并绘制一些数据
 * QwtPlot* currentPlot = figure.currentAxes();
 * if (currentPlot) {
 *     // Add curve to the current plot
 *     // 在当前绘图中添加曲线
 *     QwtPlotCurve* curve = new QwtPlotCurve;
 *     curve->attach(currentPlot);
 * }
 * @endcode
 * @sa gca
 */
QwtPlot* QwtFigure::currentAxes() const
{
    return m_data->currentAxes;
}

/**
 * @brief Get the current axes (plot)/获取当前坐标轴（绘图）
 * @return Pointer to the current QwtPlot, or nullptr if no axes exist / 指向当前QwtPlot的指针，如果没有坐标轴则返回nullptr
 * @sa currentAxes
 */
QwtPlot* QwtFigure::gca() const
{
    return currentAxes();
}

/**
 * @brief Get the normalized rectangle for a axes/获取坐标系的归一化矩形
 *
 * This method returns the normalized coordinates [0,1] for the specified axes
 * in the figure. If the axes is not found in the figure, an invalid QRectF is returned.
 *
 * 此方法返回布局中指定坐标系的归一化坐标[0,1]。如果在绘图中未找到该坐标系，则返回无效的QRectF。
 *
 * @param widget Widget to query / 要查询的坐标系
 * @return Normalized coordinates [left, top, width, height] in range [0,1], or invalid QRectF if not found
 *         归一化坐标 [左, 上, 宽, 高]，范围 [0,1]，如果未找到则返回无效QRectF
 *
 * @code
 * // Get the normalized position of a widget
 * // 获取窗口部件的归一化位置
 * QRectF normRect = figure->axesNormRect(plot);
 * if (normRect.isValid()) {
 *     qDebug() << "axes position:" << normRect;
 * } else {
 *     qDebug() << "axes not found in figure";
 * }
 * @endcode
 */
QRectF QwtFigure::axesNormRect(QwtPlot* plot) const
{
    QWTFIGURE_SAFEGET_LAY_RET(lay, QRect())
    return lay->widgetNormRect(plot);
}

/**
 * @brief Get the normalized rectangle for a widget/获取窗口的归一化矩形
 *
 * This method returns the normalized coordinates [0,1] for the specified axes
 * in the figure. If the widget is not found in the figure, an invalid QRectF is returned.
 *
 * 此方法返回布局中指定坐标系的归一化坐标[0,1]。如果在绘图中未找到该窗口，则返回无效的QRectF。
 *
 * @param widget Widget to query / 要查询的窗口
 * @return Normalized coordinates [left, top, width, height] in range [0,1], or invalid QRectF if not found
 *         归一化坐标 [左, 上, 宽, 高]，范围 [0,1]，如果未找到则返回无效QRectF
 */
QRectF QwtFigure::widgetNormRect(QWidget* w) const
{
    QWTFIGURE_SAFEGET_LAY_RET(lay, QRect())
    return lay->widgetNormRect(w);
}

/**
 * @brief 获取在此坐标下的绘图，如果此坐标下没有，则返回nullptr，存在寄生轴情况只返回宿主轴
 * @note 隐藏的窗口不会获取到
 * @param pos 坐标
 * @return 如果此坐标下没有，则返回nullptr
 */
QwtPlot* QwtFigure::plotUnderPos(const QPoint& pos) const
{
    const QList< QwtPlot* > result = findChildren< QwtPlot* >(QString(), Qt::FindDirectChildrenOnly);
    if (result.empty()) {
        return nullptr;
    }
    for (QwtPlot* plot : result) {
        if (!(plot->isVisibleTo(this))) {
            continue;
        }
        // 判断子窗口的区域是否包含转换后的点
        if (plot->geometry().contains(pos)) {
            return plot;
        }
    }
    return nullptr;
}

/**
 * @brief 通过真实的窗口坐标计算归一化坐标
 * @param geoRect 真实窗口坐标，就是子窗口的geometry()
 * @return
 */
QRectF QwtFigure::calcNormRect(const QRect& geoRect) const
{
    return QwtFigureLayout::calcNormRect(rect(), geoRect);
}

/**
 * @brief 通过正则矩形计算真实矩形
 * @param normRect
 * @return
 */
QRect QwtFigure::calcActualRect(const QRectF& normRect)
{
    QWTFIGURE_SAFEGET_LAY_RET(lay, QRect())
    return lay->calcActualRect(rect(), normRect);
}

void QwtFigure::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background
    painter.fillRect(rect(), m_data->faceBrush);

    // Draw border
    if (m_data->edgeLineWidth > 0) {
        QPen pen(m_data->edgeColor);
        pen.setWidth(m_data->edgeLineWidth);
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -1, -1));
    }

    QFrame::paintEvent(event);
}
