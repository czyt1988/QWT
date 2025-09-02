#ifndef QWTSTYLESHEETRECORDER_H
#define QWTSTYLESHEETRECORDER_H
#include "qwt_null_paintdevice.h"

#include <QList>
#include <QVector>
#include <QRectF>
#include <QRectF>
#include <QPainterPath>
#include <QBrush>
#include <QPointF>

class QWT_EXPORT QwtStyleSheetRecorder QWT_FINAL : public QwtNullPaintDevice
{
public:
    explicit QwtStyleSheetRecorder(const QSize& size);
    virtual void updateState(const QPaintEngineState& state) QWT_OVERRIDE;
    virtual void drawRects(const QRectF* rects, int count) QWT_OVERRIDE;
    virtual void drawRects(const QRect* rects, int count) QWT_OVERRIDE;
    virtual void drawPath(const QPainterPath& path) QWT_OVERRIDE;
    void setCornerRects(const QPainterPath& path);

protected:
    virtual QSize sizeMetrics() const QWT_OVERRIDE;

private:
    void alignCornerRects(const QRectF& rect);

public:
    QVector< QRectF > clipRects;

    struct Border
    {
        QList< QPainterPath > pathList;
        QList< QRectF > rectList;
        QRegion clipRegion;
    } border;

    struct Background
    {
        QPainterPath path;
        QBrush brush;
        QPointF origin;
    } background;

private:
    const QSize m_size;

    QPen m_pen;
    QBrush m_brush;
    QPointF m_origin;
};

#endif  // QWTSTYLESHEETRECORDER_H
