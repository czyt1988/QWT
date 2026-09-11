/******************************************************************************
 * Qwt 3D bar chart example
 *
 * Renders a Gaussian peak sampled on a regular x/y grid as a 3D bar chart
 * (3D histogram / bar3 style). Demonstrates Qwt3DBar with the 2D-grid data
 * entry, per-bar colormap coloring, FilledMesh / Wireframe styles, theme
 * integration, and lighting. A BarSettingsDock provides interactive controls
 * for all bar properties and data regeneration.
 *****************************************************************************/

#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>

#include "BarSettingsDock.h"
#include "qwt3d_bar.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_plot.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"

class BarWindow : public QMainWindow
{
public:
    explicit BarWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("Qwt3D Bar Chart (3D histogram)");
        resize(960, 720);

        m_plot = new Qwt3DPlot(this);
        setCentralWidget(m_plot);

        m_bar = new Qwt3DBar();
        m_bar->attach(m_plot);

        m_plot->setTitle("Gaussian peak as 3D bars");
        m_plot->setTitleFont("Arial", 12, QFont::Bold);
        m_plot->setTitleColor(RGBA(0.1, 0.1, 0.1, 1.0));

        setupAxes();

        // Lighting
        m_plot->enableLighting(true);
        m_plot->illuminate(0);
        m_plot->setLightComponent(GL_DIFFUSE, 0.7, 0);
        m_plot->setLightComponent(GL_SPECULAR, 0.3, 0);
        m_plot->setShininess(20.0);
        m_plot->setLightRotation(20.0, 30.0, 0.0, 0);
        m_plot->setBackgroundColor(RGBA(1.0, 1.0, 1.0, 1.0));

        m_plot->enableMouse(true);
        m_plot->enableKeyboard(true);
        m_plot->showColorLegend(true);

        // Settings dock — provides bar property controls and data regeneration
        m_dock = new BarSettingsDock(this);
        m_dock->setPlot(m_plot);
        m_dock->setBar(m_bar);
        addDockWidget(Qt::LeftDockWidgetArea, m_dock);

        // Apply bar properties and generate initial data from dock defaults
        m_dock->reapplyAll();

        resetView();
        createToolbar();
    }

private:
    void setupAxes()
    {
        auto* coords = m_plot->coordinates();
        for (unsigned i = 0; i < coords->axes.size(); ++i) {
            coords->axes[i].setMajors(5);
            coords->axes[i].setMinors(2);
        }
        coords->axes[X1].setLabelString("X");
        coords->axes[Y1].setLabelString("Y");
        coords->axes[Z1].setLabelString("Z");
        coords->setStyle(BOX);
    }

    void resetView()
    {
        m_plot->setRotation(35, 0, 25);
        m_plot->setViewportShift(0.0, -0.05);
        m_plot->setScale(1, 1, 1);
        m_plot->setZoom(0.95);
    }

    void createToolbar()
    {
        auto* tb = addToolBar("Controls");
        tb->setMovable(false);

        // Coord style menu (Box / Frame / None)
        auto* coordGroup = new QActionGroup(this);
        coordGroup->setExclusive(true);
        auto addCoord = [&](const QString& name, COORDSTYLE v, bool checked = false) {
            auto* a = coordGroup->addAction(name);
            a->setCheckable(true);
            a->setChecked(checked);
            a->setData(int(v));
        };
        addCoord(QStringLiteral("Box"), BOX, true);
        addCoord(QStringLiteral("Frame"), FRAME);
        addCoord(QStringLiteral("None"), NOCOORD);
        auto* coordAction = tb->addAction(QStringLiteral("Coord Style"));
        coordAction->setMenu(new QMenu(tb));
        for (auto* a : coordGroup->actions())
            coordAction->menu()->addAction(a);
        connect(coordGroup, &QActionGroup::triggered, this, [this](QAction* a) {
            m_plot->coordinates()->setStyle(static_cast<COORDSTYLE>(a->data().toInt()));
            m_plot->update();
        });

        tb->addSeparator();

        // Theme selector
        tb->addWidget(new QLabel(QStringLiteral("Theme:")));
        m_themeCombo = new QComboBox();
        m_themeCombo->addItems(Qwt3DTheme::availablePresets());
        tb->addWidget(m_themeCombo);
        connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) {
            m_plot->applyTheme(m_themeCombo->itemText(idx));
            m_dock->reapplyAll();
            m_plot->update();
        });

        tb->addSeparator();

        auto* lightAction = tb->addAction(QStringLiteral("Lighting"));
        lightAction->setCheckable(true);
        lightAction->setChecked(true);
        connect(lightAction, &QAction::toggled, this, [this](bool on) {
            m_plot->enableLighting(on);
            m_plot->update();
        });

        auto* legendAction = tb->addAction(QStringLiteral("Color Legend"));
        legendAction->setCheckable(true);
        legendAction->setChecked(true);
        connect(legendAction, &QAction::toggled, this, [this](bool on) {
            m_plot->showColorLegend(on);
            m_plot->update();
        });

        tb->addSeparator();

        tb->addAction(QStringLiteral("Reset View"), this, [this] { resetView(); });
    }

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DBar* m_bar = nullptr;
    BarSettingsDock* m_dock = nullptr;
    QComboBox* m_themeCombo = nullptr;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    BarWindow w;
    w.show();
    return app.exec();
}
