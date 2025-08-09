#include "qwt_stylesheet_recorder.h"
#include "qwt_math.h"

QwtStyleSheetRecorder::QwtStyleSheetRecorder(const QSize& size) : QwtNullPaintDevice(), m_size(size)
{
}
void QwtStyleSheetRecorder::updateState(const QPaintEngineState& state)
{
    if (state.state() & QPaintEngine::DirtyPen) {
        m_pen = state.pen();
    }
    if (state.state() & QPaintEngine::DirtyBrush) {
        m_brush = state.brush();
    }
    if (state.state() & QPaintEngine::DirtyBrushOrigin) {
        m_origin = state.brushOrigin();
    }
}

void QwtStyleSheetRecorder::drawRects(const QRectF* rects, int count)
{
    for (int i = 0; i < count; i++)
        border.rectList += rects[ i ];
}

void QwtStyleSheetRecorder::drawRects(const QRect* rects, int count)
{
    for (int i = 0; i < count; i++) {
        this->border.rectList += rects[ i ];
    }
}

void QwtStyleSheetRecorder::drawPath(const QPainterPath& path)
{
    const QRectF rect(QPointF(0.0, 0.0), m_size);
    if (path.controlPointRect().contains(rect.center())) {
        setCornerRects(path);
        alignCornerRects(rect);

        background.path   = path;
        background.brush  = m_brush;
        background.origin = m_origin;
    } else {
        border.pathList += path;
    }
}

void QwtStyleSheetRecorder::setCornerRects(const QPainterPath& path)
{
    QPointF pos(0.0, 0.0);

    for (int i = 0; i < path.elementCount(); i++) {
        QPainterPath::Element el = path.elementAt(i);
        switch (el.type) {
        case QPainterPath::MoveToElement:
        case QPainterPath::LineToElement: {
            pos.setX(el.x);
            pos.setY(el.y);
            break;
        }
        case QPainterPath::CurveToElement: {
            QRectF r(pos, QPointF(el.x, el.y));
            clipRects += r.normalized();

            pos.setX(el.x);
            pos.setY(el.y);

            break;
        }
        case QPainterPath::CurveToDataElement: {
            if (clipRects.size() > 0) {
                QRectF r = clipRects.last();
                r.setCoords(qwtMinF(r.left(), el.x),
                            qwtMinF(r.top(), el.y),
                            qwtMaxF(r.right(), el.x),
                            qwtMaxF(r.bottom(), el.y));
                clipRects.last() = r.normalized();
            }
            break;
        }
        }
    }
}

QSize QwtStyleSheetRecorder::sizeMetrics() const
{
    return m_size;
}

void QwtStyleSheetRecorder::alignCornerRects(const QRectF& rect)
{
    for (int i = 0; i < clipRects.size(); i++) {
        QRectF& r = clipRects[ i ];
        if (r.center().x() < rect.center().x()) {
            r.setLeft(rect.left());
        } else {
            r.setRight(rect.right());
        }

        if (r.center().y() < rect.center().y()) {
            r.setTop(rect.top());
        } else {
            r.setBottom(rect.bottom());
        }
    }
}
