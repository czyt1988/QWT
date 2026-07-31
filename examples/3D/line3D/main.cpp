/******************************************************************************
 * Qwt 3D line plot example
 *
 * Renders a 3D helix (x = cos t, y = sin t, z = t) as a lit tube swept along
 * the polyline. Demonstrates Qwt3DLine with Tube / Lines / Dots styles,
 * per-vertex colormap coloring (by height), optional point markers, theme
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

#include "qwt3d_color.h"
#include "qwt3d_colormap_color.h"
#include "qwt3d_coordsys.h"
#include "qwt3d_line3d.h"
#include "qwt3d_plot.h"
#include "qwt3d_theme.h"
#include "qwt3d_types.h"
#include "qwt_point_3d.h"

class LineWindow : public QMainWindow
{
public:
    explicit LineWindow(QWidget* parent = nullptr)
        : QMainWindow(parent)
    {
        setWindowTitle("Qwt3D Line Plot (3D helix)");
        resize(960, 720);

        m_plot = new Qwt3DPlot(this);
        setCentralWidget(m_plot);

        m_line = new Qwt3DLine();
        m_line->attach(m_plot);

        m_plot->setTitle("3D helix: (cos t, sin t, t)");
        m_plot->setTitleFont("Arial", 12, QFont::Bold);
        m_plot->setTitleColor(RGBA(0.1, 0.1, 0.1, 1.0));

        setupAxes();

        // Helix samples
        const int n = 240;
        const double tMin = 0.0;
        const double tMax = 4.0 * 2 * std::acos(-1.0); // 4 * pi
        QVector<QwtPoint3D> samples;
        samples.reserve(n);
        for (int i = 0; i < n; ++i) {
            const double t = tMin + (tMax - tMin) * i / (n - 1);
            samples.append(QwtPoint3D(std::cos(t), std::sin(t), t));
        }
        m_line->setSamples(samples);

        m_line->setLineStyle(Qwt3DLine::Tube);
        m_line->setTubeRadius(0.05);
        m_line->setTubeSegments(10);
        m_line->setDataColor(new Qwt3DColorMapColor("plasma"));
        m_line->setPointSize(10.0);
        m_line->setPointVisible(false);

        // Lighting
        m_plot->enableLighting(true);
        m_plot->illuminate(0);
        m_plot->setLightComponent(GL_DIFFUSE, 0.7, 0);
        m_plot->setLightComponent(GL_SPECULAR, 0.3, 0);
        m_plot->setShininess(32.0);
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
        m_plot->setRotation(40, 0, 20);
        m_plot->setViewportShift(0.0, -0.03);
        m_plot->setScale(1, 1, 1);
        m_plot->setZoom(0.85);
    }

    void createToolbar()
    {
        auto* tb = addToolBar("Controls");
        tb->setMovable(false);

        // Line style group
        auto* styleGroup = new QActionGroup(this);
        styleGroup->setExclusive(true);
        auto addStyle = [&](const QString& name, int v, bool checked = false) {
            auto* a = styleGroup->addAction(name);
            a->setCheckable(true);
            a->setChecked(checked);
            a->setData(v);
        };
        addStyle("Lines", int(Qwt3DLine::Lines));
        addStyle("Tube", int(Qwt3DLine::Tube), true);
        addStyle("Dots", int(Qwt3DLine::Dots));
        auto* styleAction = tb->addAction("Line Style");
        styleAction->setMenu(new QMenu(tb));
        for (auto* a : styleGroup->actions())
            styleAction->menu()->addAction(a);
        connect(styleGroup, &QActionGroup::triggered, this, [this](QAction* a) {
            m_line->setLineStyle(static_cast<Qwt3DLine::LineStyle>(a->data().toInt()));
            m_plot->update();
        });

        tb->addSeparator();

        auto* markersAction = tb->addAction("Point Markers");
        markersAction->setCheckable(true);
        connect(markersAction, &QAction::toggled, this, [this](bool on) {
            m_line->setPointVisible(on);
            m_plot->update();
        });

        tb->addSeparator();

        tb->addWidget(new QLabel("Theme:"));
        m_themeCombo = new QComboBox();
        m_themeCombo->addItems(Qwt3DTheme::availablePresets());
        tb->addWidget(m_themeCombo);
        connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx) {
            m_plot->applyTheme(m_themeCombo->itemText(idx));
            m_line->setTubeRadius(0.05);
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
    Qwt3DLine* m_line = nullptr;
    QComboBox* m_themeCombo = nullptr;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    LineWindow w;
    w.show();
    return app.exec();
}
