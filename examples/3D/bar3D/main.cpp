/******************************************************************************
 * Qwt 3D bar chart example
 *
 * Renders a Gaussian peak sampled on a regular x/y grid as a 3D bar chart
 * (3D histogram / bar3 style). Demonstrates Qwt3DBar with the 2D-grid data
 * entry, per-bar colormap coloring, FilledMesh / Wireframe styles, theme
 * integration, and lighting.
 *****************************************************************************/

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QActionGroup>
#include <QToolBar>
#include <QVector>

#include <cmath>
#include <vector>

#include "qwt3d_bar.h"
#include "qwt3d_color.h"
#include "qwt3d_colormap_color.h"
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

        // Gaussian peak on a 9x9 grid over [-2, 2] x [-2, 2]
        const int cols = 9, rows = 9;
        const double minX = -2.0, maxX = 2.0, minY = -2.0, maxY = 2.0;
        QVector<QVector<double>> z(cols, QVector<double>(rows));
        for (int i = 0; i < cols; ++i) {
            const double x = minX + (maxX - minX) * i / (cols - 1);
            for (int j = 0; j < rows; ++j) {
                const double y = minY + (maxY - minY) * j / (rows - 1);
                z[i][j] = std::exp(-(x * x + y * y) / 1.5);
            }
        }
        std::vector<double*> ptrs(cols);
        for (int i = 0; i < cols; ++i)
            ptrs[i] = z[i].data();

        m_bar->setSamples(ptrs.data(), cols, rows, minX, maxX, minY, maxY);
        m_bar->setBarStyle(Qwt3DBar::FilledMesh);
        m_bar->setDataColor(new Qwt3DColorMapColor("viridis"));
        m_bar->setMeshColor(RGBA(0.1, 0.1, 0.1, 0.4));
        m_bar->setMeshLineWidth(1.0);
        m_bar->setBaseline(0.0);

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

        // Bar style group
        auto* styleGroup = new QActionGroup(this);
        styleGroup->setExclusive(true);
        auto addStyle = [&](const QString& name, int v, bool checked = false) {
            auto* a = styleGroup->addAction(name);
            a->setCheckable(true);
            a->setChecked(checked);
            a->setData(v);
        };
        addStyle("Filled", int(Qwt3DBar::Filled));
        addStyle("FilledMesh", int(Qwt3DBar::FilledMesh), true);
        addStyle("Wireframe", int(Qwt3DBar::Wireframe));
        auto* styleAction = tb->addAction("Bar Style");
        styleAction->setMenu(new QMenu(tb));
        for (auto* a : styleGroup->actions())
            styleAction->menu()->addAction(a);
        connect(styleGroup, &QActionGroup::triggered, this, [this](QAction* a) {
            m_bar->setBarStyle(static_cast<Qwt3DBar::BarStyle>(a->data().toInt()));
            m_plot->update();
        });

        tb->addSeparator();

        // Theme selector
        tb->addWidget(new QLabel("Theme:"));
        m_themeCombo = new QComboBox();
        m_themeCombo->addItems(Qwt3DTheme::availablePresets());
        tb->addWidget(m_themeCombo);
        connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) {
            m_plot->applyTheme(m_themeCombo->itemText(idx));
            // Re-apply bar-specific mesh transparency lost after theme dispatch
            m_bar->setMeshColor(RGBA(0.1, 0.1, 0.1, 0.4));
            m_plot->update();
        });

        tb->addSeparator();

        auto* lightAction = tb->addAction("Lighting");
        lightAction->setCheckable(true);
        lightAction->setChecked(true);
        connect(lightAction, &QAction::toggled, this, [this](bool on) {
            m_plot->enableLighting(on);
            m_plot->update();
        });

        auto* legendAction = tb->addAction("Color Legend");
        legendAction->setCheckable(true);
        legendAction->setChecked(true);
        connect(legendAction, &QAction::toggled, this, [this](bool on) {
            m_plot->showColorLegend(on);
            m_plot->update();
        });

        tb->addSeparator();

        tb->addAction("Reset View", this, [this] { resetView(); });
    }

    Qwt3DPlot* m_plot = nullptr;
    Qwt3DBar* m_bar = nullptr;
    QComboBox* m_themeCombo = nullptr;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    BarWindow w;
    w.show();
    return app.exec();
}
