/******************************************************************************
 * Retained grid layout demo, entry point.
 *
 * See GridWindow.cpp for the description of the example and for the hooks used
 * by the screenshot variants.
 *****************************************************************************/

#include <QApplication>

#include "GridWindow.h"
#include "../../common/qwt_example_screenshot.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("figureGrid"));

    GridWindow window;
    window.setWindowTitle(QStringLiteral("QwtFigure retained grid layout example"));
    window.resize(1280, 820);

    // Variants of the documentation screenshots ( see examples/screenshots-manifest.json )
    QwtExampleScreenshot::addVariant(QStringLiteral("drag"), [ &window ]() {
        // Two real drag sessions through the splitter: the column divider between
        // column 0 and 1, then the row divider between row 0 and 1
        window.dragDivider(0, 120);
        window.dragDivider(2, 26);
    });
    QwtExampleScreenshot::addVariant(QStringLiteral("spacing"), [ &window ]() {
        // Wide gaps make the space between the tracks visible; the tracks stay
        // equal so every cell keeps enough room for its titles
        window.applyPreset(QStringLiteral("equal"));
        window.setGridSpacing(0.018, 0.018);
    });
    QwtExampleScreenshot::addVariant(QStringLiteral("reenter"), [ &window ]() {
        // Exercises disableGrid() and the re-registration: the members freeze at
        // their geometry while the grid is gone, and a ratio change after the grid
        // was enabled again only moves them when their cells are registered again
        window.leaveGrid();
        window.reenterGrid();
        window.applyPreset(QStringLiteral("golden"));
        window.dragDivider(0, -60);
    });

    window.show();

    QwtExampleScreenshot::start(&window);
    return app.exec();
}
