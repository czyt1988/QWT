#ifndef QWT_FIGURE_LAYOUT_H
#define QWT_FIGURE_LAYOUT_H
// stl
#include <memory>

// Qt
#include <QLayout>

// qwt
#include "qwt_global.h"

/**
 * @class QwtFigureLayout
 * @brief Custom layout manager for QwtFigureWidget that handles both normalized coordinates and grid layouts
 *
 * 自定义布局管理器，用于QwtFigureWidget，支持归一化坐标和网格布局
 */
class QWT_EXPORT QwtFigureLayout : public QLayout
{
    Q_OBJECT
    QWT_DECLARE_PRIVATE(QwtFigureLayout)
public:
    QwtFigureLayout();
    explicit QwtFigureLayout(QWidget* parent);
    virtual ~QwtFigureLayout();

    virtual void addItem(QLayoutItem* item) override;
    virtual QLayoutItem* itemAt(int index) const override;
    virtual QLayoutItem* takeAt(int index) override;
    virtual int count() const override;
    virtual QSize sizeHint() const override;
    virtual QSize minimumSize() const override;
    virtual void setGeometry(const QRect& rect) override;

    // Add a widget with normalized coordinates/使用归一化坐标添加窗口部件
    void addAxes(QWidget* widget, const QRectF& rect);

    // Add a widget with normalized coordinates using separate parameters/使用分离参数和归一化坐标添加窗口部件
    void addAxes(QWidget* widget, qreal left, qreal top, qreal width, qreal height);

    // Add a widget by grid layout/添加窗口部件到网格布局
    void addAxes(QWidget* widget,
                 int rowCnt,
                 int colCnt,
                 int row,
                 int col,
                 int rowSpan  = 1,
                 int colSpan  = 1,
                 qreal wspace = 0.0,
                 qreal hspace = 0.0);

    // 改变已经添加的窗口的位置占比,如果窗口还没添加，此函数无效
    void setAxesNormPos(QWidget* widget, const QRectF& rect);

    // Update layout parameters/更新布局参数
    void adjustLayout(qreal left, qreal bottom, qreal right, qreal top);

    // Get the normalized rectangle for a widget/获取窗口部件的归一化矩形
    QRectF widgetNormRect(QWidget* widget) const;

    // 计算rect相对于parentRect的归一化坐标
    static QRectF calcNormRect(const QRect& parentRect, const QRect& rect);

protected:
    // calc the normalized rectangle for a grid cell/获取网格单元格的归一化矩形
    QRectF calcGridRect(int rowCnt,
                        int colCnt,
                        int row,
                        int col,
                        int rowSpan  = 1,
                        int colSpan  = 1,
                        qreal wspace = 0.0,
                        qreal hspace = 0.0) const;
};

#endif  // QWT_FIGURE_LAYOUT_H
