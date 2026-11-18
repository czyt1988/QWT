#include "test_figure_grid.h"

#include <QSignalSpy>
#include <QTest>
#include <QVector>
#include <QWidget>

#include "qwt_figure_grid.h"

namespace
{
bool fuzzyEqual(qreal a, qreal b)
{
    return qAbs(a - b) <= 1e-9;
}

bool fuzzyRectEqual(const QRectF& actual, const QRectF& expected)
{
    return fuzzyEqual(actual.left(), expected.left()) && fuzzyEqual(actual.top(), expected.top())
           && fuzzyEqual(actual.width(), expected.width()) && fuzzyEqual(actual.height(), expected.height());
}

qreal sumOf(const QVector< qreal >& sizes)
{
    qreal sum = 0.0;
    for (qreal size : sizes) {
        sum += size;
    }
    return sum;
}
}  // namespace

void TestFigureGrid::testSetupEqualTracks()
{
    QwtFigureGrid grid;
    QVERIFY(!grid.isEnabled());
    QCOMPARE(grid.rowCount(), 0);
    QCOMPARE(grid.columnCount(), 0);

    grid.setup(2, 3);
    QVERIFY(grid.isEnabled());
    QCOMPARE(grid.rowCount(), 2);
    QCOMPARE(grid.columnCount(), 3);

    const QVector< qreal > rows = grid.rowSizes();
    QCOMPARE(rows.size(), 2);
    QVERIFY(fuzzyEqual(rows.at(0), 0.5));
    QVERIFY(fuzzyEqual(rows.at(1), 0.5));
    QVERIFY(fuzzyEqual(sumOf(rows), 1.0));

    const QVector< qreal > columns = grid.columnSizes();
    QCOMPARE(columns.size(), 3);
    for (int i = 0; i < columns.size(); ++i) {
        QVERIFY(fuzzyEqual(columns.at(i), 1.0 / 3.0));
    }
    QVERIFY(fuzzyEqual(sumOf(columns), 1.0));
}

void TestFigureGrid::testSetupInvalidCounts()
{
    QwtFigureGrid grid;
    QSignalSpy spy(&grid, &QwtFigureGrid::sizesChanged);

    grid.setup(0, 2);
    QVERIFY(!grid.isEnabled());
    QCOMPARE(spy.count(), 0);

    grid.setup(2, -1);
    QVERIFY(!grid.isEnabled());
    QCOMPARE(spy.count(), 0);

    grid.setup(2, 3);
    QVERIFY(grid.isEnabled());
    QCOMPARE(spy.count(), 1);

    // An invalid configuration must not reset an enabled grid
    grid.setup(-2, 2);
    QVERIFY(grid.isEnabled());
    QCOMPARE(grid.rowCount(), 2);
    QCOMPARE(grid.columnCount(), 3);
    QCOMPARE(spy.count(), 1);

    // Re-running setup re-initializes the tracks and drops registrations
    QWidget w;
    QVERIFY(grid.addItem(&w, 0, 0));
    QCOMPARE(grid.items().size(), 1);

    grid.setup(2, 2);
    QCOMPARE(grid.items().size(), 0);
    QCOMPARE(grid.rowCount(), 2);
    QCOMPARE(grid.columnCount(), 2);
    QCOMPARE(grid.columnSizes().size(), 2);
    QVERIFY(fuzzyEqual(grid.columnSizes().at(0), 0.5));
    QCOMPARE(spy.count(), 2);
}

void TestFigureGrid::testMinimumTrackPixels()
{
    QwtFigureGrid grid;
    QCOMPARE(grid.minimumTrackPixels(), 40);

    grid.setMinimumTrackPixels(12);
    QCOMPARE(grid.minimumTrackPixels(), 12);

    grid.setMinimumTrackPixels(-1);
    QCOMPARE(grid.minimumTrackPixels(), 12);

    grid.setMinimumTrackPixels(0);
    QCOMPARE(grid.minimumTrackPixels(), 0);
}

void TestFigureGrid::testCustomRatiosAreNormalized()
{
    QwtFigureGrid grid;
    grid.setup(2, 2);

    grid.setColumnSizes({ 1.0, 3.0 });
    const QVector< qreal > columns = grid.columnSizes();
    QCOMPARE(columns.size(), 2);
    QVERIFY(fuzzyEqual(columns.at(0), 0.25));
    QVERIFY(fuzzyEqual(columns.at(1), 0.75));
    QVERIFY(fuzzyEqual(sumOf(columns), 1.0));

    grid.setRowSizes({ 2.0, 1.0 });
    const QVector< qreal > rows = grid.rowSizes();
    QCOMPARE(rows.size(), 2);
    QVERIFY(fuzzyEqual(rows.at(0), 2.0 / 3.0));
    QVERIFY(fuzzyEqual(rows.at(1), 1.0 / 3.0));
    QVERIFY(fuzzyEqual(sumOf(rows), 1.0));
}

void TestFigureGrid::testInvalidSizesRejected()
{
    QwtFigureGrid grid;

    // A disabled grid ignores size updates
    grid.setColumnSizes({ 1.0, 1.0 });
    QVERIFY(grid.columnSizes().isEmpty());

    grid.setup(1, 2);
    const QVector< qreal > expectedColumns = grid.columnSizes();

    grid.setColumnSizes({ 1.0 });            // too short
    grid.setColumnSizes({ 1.0, 1.0, 1.0 });  // too long
    grid.setColumnSizes({ 1.0, 0.0 });       // not positive
    grid.setColumnSizes({ 0.0, 1.0 });       // not positive
    grid.setColumnSizes({ 1.0, -2.0 });      // negative
    grid.setColumnSizes({ 1.0, qQNaN() });   // not finite
    grid.setColumnSizes({ 1.0, qInf() });    // not finite

    QCOMPARE(grid.columnSizes().size(), 2);
    QVERIFY(fuzzyEqual(grid.columnSizes().at(0), expectedColumns.at(0)));
    QVERIFY(fuzzyEqual(grid.columnSizes().at(1), expectedColumns.at(1)));

    grid.setRowSizes({ 1.0, 2.0 });  // one row track only
    QCOMPARE(grid.rowSizes().size(), 1);
    QVERIFY(fuzzyEqual(grid.rowSizes().at(0), 1.0));

    grid.setRowSizes({ 2.0 });  // valid: normalized to a single full track
    QCOMPARE(grid.rowSizes().size(), 1);
    QVERIFY(fuzzyEqual(grid.rowSizes().at(0), 1.0));
}

void TestFigureGrid::testNormRectSingleCells()
{
    QwtFigureGrid grid;
    grid.setup(2, 2);

    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0), QRectF(0.0, 0.0, 0.5, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 1), QRectF(0.5, 0.0, 0.5, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 0), QRectF(0.0, 0.5, 0.5, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 1), QRectF(0.5, 0.5, 0.5, 0.5)));
}

void TestFigureGrid::testNormRectSpans()
{
    QwtFigureGrid grid;
    grid.setup(2, 3);

    // Column span
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0, 1, 2), QRectF(0.0, 0.0, 2.0 / 3.0, 0.5)));
    // Row span
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0, 2, 1), QRectF(0.0, 0.0, 1.0 / 3.0, 1.0)));
    // Full span
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0, 2, 3), QRectF(0.0, 0.0, 1.0, 1.0)));
    // Both spans, offset from the origin
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 1, 1, 2), QRectF(1.0 / 3.0, 0.5, 2.0 / 3.0, 0.5)));
}

void TestFigureGrid::testNormRectSpacing()
{
    QwtFigureGrid grid;
    grid.setup(2, 2);
    grid.setSpacing(0.1, 0.0);

    // The first column is untouched by wspace
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0), QRectF(0.0, 0.0, 0.5, 0.5)));
    // The second column starts after the gap and is clamped to the right edge
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 1), QRectF(0.6, 0.0, 0.4, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 0), QRectF(0.0, 0.5, 0.5, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 1), QRectF(0.6, 0.5, 0.4, 0.5)));

    // The same applies to hspace along Y
    grid.setSpacing(0.0, 0.1);
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0), QRectF(0.0, 0.0, 0.5, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 0), QRectF(0.0, 0.6, 0.5, 0.4)));

    // Spacing combines with custom track ratios
    grid.setSpacing(0.1, 0.1);
    grid.setColumnSizes({ 0.25, 0.75 });
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 0), QRectF(0.0, 0.0, 0.25, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 1), QRectF(0.35, 0.0, 0.65, 0.5)));

    // Invalid spacing is rejected and keeps the previous value
    grid.setSpacing(-0.1, 0.0);
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 1), QRectF(0.35, 0.0, 0.65, 0.5)));
    grid.setSpacing(0.1, qQNaN());
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 1), QRectF(0.35, 0.0, 0.65, 0.5)));
}

/**
 * @brief The spacing getters report the retained gap values
 */
void TestFigureGrid::testSpacingGetters()
{
    QwtFigureGrid grid;

    // No gap by default, on a disabled and on an enabled grid
    QVERIFY(fuzzyEqual(grid.wspace(), 0.0));
    QVERIFY(fuzzyEqual(grid.hspace(), 0.0));
    grid.setup(2, 2);
    QVERIFY(fuzzyEqual(grid.wspace(), 0.0));
    QVERIFY(fuzzyEqual(grid.hspace(), 0.0));

    // Values set through setSpacing() come back unchanged
    grid.setSpacing(0.125, 0.25);
    QVERIFY(fuzzyEqual(grid.wspace(), 0.125));
    QVERIFY(fuzzyEqual(grid.hspace(), 0.25));

    // The getters report the state the geometry is computed from
    QVERIFY(fuzzyRectEqual(grid.normRectFor(0, 1), QRectF(0.625, 0.0, 0.375, 0.5)));
    QVERIFY(fuzzyRectEqual(grid.normRectFor(1, 0), QRectF(0.0, 0.75, 0.5, 0.25)));

    // A rejected request leaves the getters on the previous values
    grid.setSpacing(-0.1, 0.5);
    QVERIFY(fuzzyEqual(grid.wspace(), 0.125));
    QVERIFY(fuzzyEqual(grid.hspace(), 0.25));
    grid.setSpacing(0.5, qInf());
    QVERIFY(fuzzyEqual(grid.wspace(), 0.125));
    QVERIFY(fuzzyEqual(grid.hspace(), 0.25));
}

void TestFigureGrid::testNormRectInvalid()
{
    QwtFigureGrid grid;

    // A disabled grid has no valid geometry
    QVERIFY(!grid.normRectFor(0, 0).isValid());

    grid.setup(2, 2);

    QVERIFY(!grid.normRectFor(-1, 0).isValid());
    QVERIFY(!grid.normRectFor(0, -1).isValid());
    QVERIFY(!grid.normRectFor(2, 0).isValid());
    QVERIFY(!grid.normRectFor(0, 2).isValid());
    QVERIFY(!grid.normRectFor(0, 0, 0, 1).isValid());
    QVERIFY(!grid.normRectFor(0, 0, 1, 0).isValid());
    QVERIFY(!grid.normRectFor(0, 0, -1, 1).isValid());
    QVERIFY(!grid.normRectFor(1, 1, 2, 1).isValid());
    QVERIFY(!grid.normRectFor(0, 1, 1, 2).isValid());
}

void TestFigureGrid::testAddRemoveItems()
{
    QwtFigureGrid grid;
    QWidget a;
    QWidget b;
    QWidget c;

    // A disabled grid rejects registrations
    QVERIFY(!grid.addItem(&a, 0, 0));

    grid.setup(2, 2);

    QVERIFY(!grid.addItem(nullptr, 0, 0));
    QVERIFY(!grid.addItem(&a, -1, 0));
    QVERIFY(!grid.addItem(&a, 0, -1));
    QVERIFY(!grid.addItem(&a, 0, 0, 0, 1));
    QVERIFY(!grid.addItem(&a, 0, 0, 1, 0));
    QVERIFY(!grid.addItem(&a, 0, 2));
    QVERIFY(!grid.addItem(&a, 2, 0));
    QVERIFY(!grid.addItem(&a, 1, 1, 2, 1));

    QVERIFY(grid.addItem(&a, 0, 0, 1, 2));
    QVERIFY(grid.addItem(&b, 1, 1));
    QVERIFY(!grid.addItem(&a, 1, 0));  // duplicate registration

    QCOMPARE(grid.items().size(), 2);
    QVERIFY(grid.items().contains(&a));
    QVERIFY(grid.items().contains(&b));

    int row     = -1;
    int col     = -1;
    int rowSpan = -1;
    int colSpan = -1;
    QVERIFY(grid.gridPosition(&a, &row, &col, &rowSpan, &colSpan));
    QCOMPARE(row, 0);
    QCOMPARE(col, 0);
    QCOMPARE(rowSpan, 1);
    QCOMPARE(colSpan, 2);

    QVERIFY(grid.gridPosition(&b, &row, &col, &rowSpan, &colSpan));
    QCOMPARE(row, 1);
    QCOMPARE(col, 1);
    QCOMPARE(rowSpan, 1);
    QCOMPARE(colSpan, 1);

    // Only non-null output pointers are written
    int onlyColSpan = -1;
    QVERIFY(grid.gridPosition(&a, nullptr, nullptr, nullptr, &onlyColSpan));
    QCOMPARE(onlyColSpan, 2);

    // Unregistered and null widgets are not reported
    row = -1;
    QVERIFY(!grid.gridPosition(&c, &row, nullptr, nullptr, nullptr));
    QCOMPARE(row, -1);
    QVERIFY(!grid.gridPosition(nullptr, nullptr, nullptr, nullptr, nullptr));

    QVERIFY(!grid.removeItem(nullptr));
    QVERIFY(!grid.removeItem(&c));

    QVERIFY(grid.removeItem(&a));
    QVERIFY(!grid.removeItem(&a));
    QCOMPARE(grid.items().size(), 1);
    QVERIFY(grid.items().contains(&b));

    // A widget can be registered again after it was removed
    QVERIFY(grid.addItem(&a, 1, 0));
    QVERIFY(grid.gridPosition(&a, &row, &col, &rowSpan, &colSpan));
    QCOMPARE(row, 1);
    QCOMPARE(col, 0);
}

void TestFigureGrid::testItemDestroyed()
{
    QwtFigureGrid grid;
    grid.setup(2, 2);

    QWidget* w = new QWidget;
    QVERIFY(grid.addItem(w, 0, 1));
    QCOMPARE(grid.items().size(), 1);

    // The registration is dropped automatically when the widget is destroyed
    delete w;
    QCOMPARE(grid.items().size(), 0);

    // Removing a widget before destroying it must not leave a stale connection
    QWidget* w2 = new QWidget;
    QVERIFY(grid.addItem(w2, 1, 1));
    QVERIFY(grid.removeItem(w2));
    delete w2;
    QCOMPARE(grid.items().size(), 0);
}

void TestFigureGrid::testDividers()
{
    QwtFigureGrid grid;
    grid.setup(2, 3);

    const QList< QwtFigureGrid::Divider > dividers = grid.dividers();
    QCOMPARE(dividers.size(), 3);  // 2 column dividers + 1 row divider

    QCOMPARE(dividers.at(0).orientation, Qt::Horizontal);
    QCOMPARE(dividers.at(0).index, 0);
    QVERIFY(fuzzyEqual(dividers.at(0).position, 1.0 / 3.0));

    QCOMPARE(dividers.at(1).orientation, Qt::Horizontal);
    QCOMPARE(dividers.at(1).index, 1);
    QVERIFY(fuzzyEqual(dividers.at(1).position, 2.0 / 3.0));

    QCOMPARE(dividers.at(2).orientation, Qt::Vertical);
    QCOMPARE(dividers.at(2).index, 0);
    QVERIFY(fuzzyEqual(dividers.at(2).position, 0.5));

    // A single track in one direction only yields dividers for the other one
    grid.setup(1, 3);
    QCOMPARE(grid.dividers().size(), 2);

    grid.setup(3, 1);
    QCOMPARE(grid.dividers().size(), 2);
}

void TestFigureGrid::testDividersWithSpacing()
{
    QwtFigureGrid grid;
    grid.setup(2, 2);
    grid.setSpacing(0.1, 0.2);
    grid.setColumnSizes({ 1.0, 3.0 });  // -> 0.25 / 0.75
    grid.setRowSizes({ 3.0, 2.0 });     // -> 0.6 / 0.4

    const QList< QwtFigureGrid::Divider > dividers = grid.dividers();
    QCOMPARE(dividers.size(), 2);

    QCOMPARE(dividers.at(0).orientation, Qt::Horizontal);
    QCOMPARE(dividers.at(0).index, 0);
    QVERIFY(fuzzyEqual(dividers.at(0).position, 0.25 + 0.1 / 2.0));

    QCOMPARE(dividers.at(1).orientation, Qt::Vertical);
    QCOMPARE(dividers.at(1).index, 0);
    QVERIFY(fuzzyEqual(dividers.at(1).position, 0.6 + 0.2 / 2.0));
}

void TestFigureGrid::testDividersDisabled()
{
    QwtFigureGrid grid;
    QVERIFY(grid.dividers().isEmpty());

    grid.setup(1, 1);
    QVERIFY(grid.dividers().isEmpty());

    grid.setup(2, 2);
    QCOMPARE(grid.dividers().size(), 2);
}

void TestFigureGrid::testSizesChangedSignal()
{
    QwtFigureGrid grid;
    QSignalSpy spy(&grid, &QwtFigureGrid::sizesChanged);

    grid.setup(2, 2);
    QCOMPARE(spy.count(), 1);

    grid.setColumnSizes({ 1.0, 3.0 });
    QCOMPARE(spy.count(), 2);

    // The same normalized ratios do not emit again
    grid.setColumnSizes({ 2.0, 6.0 });
    QCOMPARE(spy.count(), 2);

    grid.setRowSizes({ 1.0, 1.0 });  // unchanged (equal tracks)
    QCOMPARE(spy.count(), 2);

    grid.setColumnSizes({ 1.0, 0.0 });  // rejected
    QCOMPARE(spy.count(), 2);

    grid.setSpacing(0.1, 0.1);
    QCOMPARE(spy.count(), 3);

    grid.setSpacing(0.1, 0.1);  // unchanged
    QCOMPARE(spy.count(), 3);

    grid.setSpacing(-1.0, 0.0);  // rejected
    QCOMPARE(spy.count(), 3);

    // Re-running setup always announces the new state
    grid.setup(2, 2);
    QCOMPARE(spy.count(), 4);
}

void TestFigureGrid::testFinishChange()
{
    QwtFigureGrid grid;
    QSignalSpy spy(&grid, &QwtFigureGrid::sizesChangeFinished);

    grid.finishChange();
    QCOMPARE(spy.count(), 1);

    grid.finishChange();
    QCOMPARE(spy.count(), 2);
}

QTEST_MAIN(TestFigureGrid)
