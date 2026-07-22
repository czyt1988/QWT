#include <qmetaobject.h>

#include <qframe.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qaction.h>
#include <qmenubar.h>

#include <qtoolbar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qfiledialog.h>
#include <qstatusbar.h>
#include <qfileinfo.h>
#include <qslider.h>
#include <qtimer.h>
#include <qcombobox.h>
#include <qstring.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qfontdialog.h>

#include "mesh2mainwindow.h"

#include "functions.h"
#include "lightingdlg.h"
#include "femreader.h"
#include "qwt3d_io.h"
#include "qwt3d_io_gl2ps.h"
#include "qwt3d_io_reader.h"
#include "qwt_version_info.h"

using namespace std;

Mesh2MainWindow::~Mesh2MainWindow()
{
    delete dataWidget;
}

Mesh2MainWindow::Mesh2MainWindow(QWidget* parent) : DummyBase(parent)
{
    setupWorkaround(this);
    setupUi(this);
    QGridLayout* grid = new QGridLayout(frame);

    col_              = nullptr;
    legend_           = false;
    redrawWait        = 50;
    activeCoordSystem = None;

    dataWidget = new Qwt3DPlot(frame);
    surface = new Qwt3DSurface();
    surface->attach(dataWidget);
    grid->addWidget(dataWidget, 0, 0);

    connect(coord, &QActionGroup::triggered, this, &Mesh2MainWindow::pickCoordSystem);
    connect(plotstyle, &QActionGroup::triggered, this, &Mesh2MainWindow::pickPlotStyle);
    connect(axescolor, &QAction::triggered, this, &Mesh2MainWindow::pickAxesColor);
    connect(backgroundcolor, &QAction::triggered, this, &Mesh2MainWindow::pickBgColor);
    connect(floorstyle, &QActionGroup::triggered, this, &Mesh2MainWindow::pickFloorStyle);
    connect(meshcolor, &QAction::triggered, this, &Mesh2MainWindow::pickMeshColor);
    connect(numbercolor, &QAction::triggered, this, &Mesh2MainWindow::pickNumberColor);
    connect(labelcolor, &QAction::triggered, this, &Mesh2MainWindow::pickLabelColor);
    connect(titlecolor, &QAction::triggered, this, &Mesh2MainWindow::pickTitleColor);
    connect(datacolor, &QAction::triggered, this, &Mesh2MainWindow::pickDataColor);
    connect(lighting, &QPushButton::clicked, this, &Mesh2MainWindow::pickLighting);
    connect(resetcolor, &QAction::triggered, this, &Mesh2MainWindow::resetColors);
    connect(numberfont, &QAction::triggered, this, &Mesh2MainWindow::pickNumberFont);
    connect(labelfont, &QAction::triggered, this, &Mesh2MainWindow::pickLabelFont);
    connect(titlefont, &QAction::triggered, this, &Mesh2MainWindow::pickTitleFont);
    connect(resetfont, &QAction::triggered, this, &Mesh2MainWindow::resetFonts);
    connect(animation, &QAction::toggled, this, &Mesh2MainWindow::toggleAnimation);
    connect(dump, &QAction::triggered, this, &Mesh2MainWindow::dumpImage);
    connect(openFile, &QAction::triggered, this, &Mesh2MainWindow::open);
    connect(openMeshFile, &QAction::triggered, this, &Mesh2MainWindow::openMesh);

    connect(left, &QAction::toggled, this, &Mesh2MainWindow::setLeftGrid);
    connect(right, &QAction::toggled, this, &Mesh2MainWindow::setRightGrid);
    connect(ceil, &QAction::toggled, this, &Mesh2MainWindow::setCeilGrid);
    connect(floor, &QAction::toggled, this, &Mesh2MainWindow::setFloorGrid);
    connect(back, &QAction::toggled, this, &Mesh2MainWindow::setBackGrid);
    connect(front, &QAction::toggled, this, &Mesh2MainWindow::setFrontGrid);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Mesh2MainWindow::rotate);

    resSlider->setRange(1, 70);
    connect(resSlider, &QSlider::valueChanged, this, [this](int val) { surface->setResolution(val); });
    resSlider->setValue(1);

    connect(offsSlider, &QSlider::valueChanged, this, &Mesh2MainWindow::setPolygonOffset);

    connect(normButton, &QPushButton::clicked, this, &Mesh2MainWindow::setStandardView);

    QString qwtstr(" qwtplot3d ");
    qwtstr += QString::number(QWT_VERSION_MAJ) + ".";
    qwtstr += QString::number(QWT_VERSION_MIN) + ".";
    qwtstr += QString::number(QWT_VERSION_PAT) + " ";

    QLabel* info = new QLabel(qwtstr, statusBar());
    statusBar()->addWidget(info, 0);
    filenameWidget = new QLabel("                                  ", statusBar());
    statusBar()->addWidget(filenameWidget, 0);
    dimWidget = new QLabel("", statusBar());
    statusBar()->addWidget(dimWidget, 0);
    rotateLabel = new QLabel("", statusBar());
    statusBar()->addWidget(rotateLabel, 0);
    shiftLabel = new QLabel("", statusBar());
    statusBar()->addWidget(shiftLabel, 0);
    scaleLabel = new QLabel("", statusBar());
    statusBar()->addWidget(scaleLabel, 0);
    zoomLabel = new QLabel("", statusBar());
    statusBar()->addWidget(zoomLabel, 0);

    connect(dataWidget, &Qwt3DPlot::rotationChanged, this, &Mesh2MainWindow::showRotate);
    connect(dataWidget, &Qwt3DPlot::vieportShiftChanged, this, &Mesh2MainWindow::showShift);
    connect(dataWidget, &Qwt3DPlot::scaleChanged, this, &Mesh2MainWindow::showScale);
    connect(dataWidget, &Qwt3DPlot::zoomChanged, this, &Mesh2MainWindow::showZoom);

    connect(functionCB, &QComboBox::currentTextChanged, this, &Mesh2MainWindow::createFunction);
    connect(psurfaceCB, &QComboBox::currentTextChanged, this, &Mesh2MainWindow::createPSurface);
    connect(projection, &QCheckBox::toggled, this, &Mesh2MainWindow::toggleProjectionMode);
    connect(colorlegend, &QCheckBox::toggled, this, &Mesh2MainWindow::toggleColorLegend);
    connect(autoscale, &QCheckBox::toggled, this, &Mesh2MainWindow::toggleAutoScale);
    connect(shader, &QCheckBox::toggled, this, &Mesh2MainWindow::toggleShader);
    connect(mouseinput, &QCheckBox::toggled, dataWidget, &Qwt3DPlot::enableMouse);
    connect(lightingswitch, &QCheckBox::toggled, this, &Mesh2MainWindow::enableLighting);
    connect(normals, &QAction::toggled, this, &Mesh2MainWindow::showNormals);
    connect(normalsquality, &QSlider::valueChanged, this, &Mesh2MainWindow::setNormalQuality);
    connect(normalslength, &QSlider::valueChanged, this, &Mesh2MainWindow::setNormalLength);

    setStandardView();

    dataWidget->coordinates()->setLineSmooth(true);
    dataWidget->coordinates()->setGridLinesColor(RGBA(0.35, 0.35, 0.35, 1));
    dataWidget->enableMouse(true);
    dataWidget->setKeySpeed(15, 20, 20);

    lightingdlg_ = new LightingDlg(this);
    lightingdlg_->assign(dataWidget);

    connect(filetypeCB, &QComboBox::currentTextChanged, this, &Mesh2MainWindow::setFileType);

    filetypeCB->clear();

    QStringList list = Qwt3DIO::outputFormatList();
    filetypeCB->insertItems(0, list);

    filetype_ = filetypeCB->currentText();
    dataWidget->setTitleFont("Arial", 14, QFont::Normal);

    grids->setEnabled(false);

    auto* pmhandler = dynamic_cast<Qwt3DPixmapWriter*>(Qwt3DIO::outputHandler("JPEG"));
    if (!pmhandler)
        pmhandler = dynamic_cast<Qwt3DPixmapWriter*>(Qwt3DIO::outputHandler("jpeg"));
    if (pmhandler)
        pmhandler->setQuality(70);

#ifdef QWT3D_ENABLE_GL2PS
    auto* handler = dynamic_cast<Qwt3DVectorWriter*>(Qwt3DIO::outputHandler("PDF"));
    if (handler)
        handler->setTextMode(Qwt3DVectorWriter::TEX);
    handler = dynamic_cast<Qwt3DVectorWriter*>(Qwt3DIO::outputHandler("EPS"));
    if (handler)
        handler->setTextMode(Qwt3DVectorWriter::TEX);
    handler = dynamic_cast<Qwt3DVectorWriter*>(Qwt3DIO::outputHandler("EPS_GZ"));
    if (handler)
        handler->setTextMode(Qwt3DVectorWriter::TEX);
#endif
}

void Mesh2MainWindow::open()
{
    QString s = QFileDialog::getOpenFileName(this, "", "../../data", "GridData Files (*.mes *.MES)");

    if (s.isEmpty() || !dataWidget)
        return;

    QFileInfo fi(s);

    filenameWidget->setToolTip(s);
    QString ext = fi.suffix();
    filenameWidget->setText(fi.fileName());
    qApp->processEvents();

    if (Qwt3DIO::load(dataWidget, s, ext)) {
        double a = surface->facets().first;
        double b = surface->facets().second;

        dimWidget->setText(
            QString("Cells ") + QString::number(a * b) + " (" + QString::number(a) + "x" + QString::number(b) + ")"
        );

        surface->setResolution(3);
    }

    for (unsigned i = 0; i != dataWidget->coordinates()->axes.size(); ++i) {
        dataWidget->coordinates()->axes[ i ].setMajors(4);
        dataWidget->coordinates()->axes[ i ].setMinors(5);
        dataWidget->coordinates()->axes[ i ].setLabelString("");
    }

    updateColorLegend(4, 5);
    pickCoordSystem(activeCoordSystem);
    dataWidget->showColorLegend(legend_);
}

void Mesh2MainWindow::createFunction(QString const& name)
{
    dataWidget->legend()->setScale(LINEARSCALE);
    for (unsigned i = 0; i != dataWidget->coordinates()->axes.size(); ++i) {
        dataWidget->coordinates()->axes[ i ].setMajors(7);
        dataWidget->coordinates()->axes[ i ].setMinors(5);
    }

    if (name == QString("Rosenbrock")) {
        Rosenbrock rosenbrock;
        rosenbrock.setMesh(50, 51);
        rosenbrock.setDomain(-1.73, 1.55, -1.5, 1.95);
        rosenbrock.setMinZ(-100);
        rosenbrock.assign(*surface);
        rosenbrock.create();

        dataWidget->coordinates()->axes[ Z1 ].setScale(LOG10SCALE);
        dataWidget->coordinates()->axes[ Z2 ].setScale(LOG10SCALE);
        dataWidget->coordinates()->axes[ Z3 ].setScale(LOG10SCALE);
        dataWidget->coordinates()->axes[ Z4 ].setScale(LOG10SCALE);
        dataWidget->legend()->setScale(LOG10SCALE);
    } else if (name == QString("Hat")) {
        Hat hat;
        hat.setMesh(51, 72);
        hat.setDomain(-1.5, 1.5, -1.5, 1.5);
        hat.assign(*surface);
        hat.create();
    } else if (name == QString("Ripple")) {
        Ripple ripple;
        ripple.setMesh(120, 120);
        ripple.assign(*surface);
        ripple.create();
    } else if (name == QString("Saddle")) {
        Saddle saddle;
        saddle.setMesh(71, 71);
        double dom = 2.5;
        saddle.setDomain(-dom, dom, -dom, dom);
        saddle.assign(*surface);
        saddle.create();
    } else if (name == QString("Sombrero")) {
        Mex mex;
        mex.setMesh(91, 91);
        double dom = 15;
        mex.setDomain(-dom, dom, -dom, dom);
        mex.assign(*surface);
        mex.create();
    }

    double a = surface->facets().first;
    double b = surface->facets().second;

    dimWidget->setText(QString("Cells ") + QString::number(a * b) + " (" + QString::number(a) + "x" + QString::number(b) + ")");

    updateColorLegend(7, 5);

    dataWidget->coordinates()->axes[ X1 ].setLabelString(QString("X1"));
    dataWidget->coordinates()->axes[ X2 ].setLabelString(QString("X2"));
    dataWidget->coordinates()->axes[ X3 ].setLabelString(QString("X3"));
    dataWidget->coordinates()->axes[ X4 ].setLabelString(QString("X4"));

    dataWidget->coordinates()->axes[ Y1 ].setLabelString(QString("Y1"));
    dataWidget->coordinates()->axes[ Y2 ].setLabelString(QString("Y2"));
    dataWidget->coordinates()->axes[ Y3 ].setLabelString(QString("Y3"));
    dataWidget->coordinates()->axes[ Y4 ].setLabelString(QString("Y4"));

    dataWidget->coordinates()->axes[ Z1 ].setLabelString(QString("Z1"));
    dataWidget->coordinates()->axes[ Z2 ].setLabelString(QString("Z2"));
    dataWidget->coordinates()->axes[ Z3 ].setLabelString(QString("Z3"));
    dataWidget->coordinates()->axes[ Z4 ].setLabelString(QString("Z4"));

    pickCoordSystem(activeCoordSystem);
}

void Mesh2MainWindow::createPSurface(QString const& name)
{
    if (name == QString("Torus")) {
        Torus sf;
        sf.assign(*surface);
        sf.create();
    } else if (name == QString("Seashell")) {
        Seashell ss;
        ss.assign(*surface);
        ss.create();
    } else if (name == QString("Boy")) {
        Boy boy;
        boy.assign(*surface);
        boy.create();
    } else if (name == QString("Dini")) {
        Dini dini;
        dini.assign(*surface);
        dini.create();
    } else if (name == QString("Cone")) {
        TripleField conepos;
        CellField conecell;
        createCone(conepos, conecell);
        surface->loadFromData(conepos, conecell);
    }
    for (unsigned i = 0; i != dataWidget->coordinates()->axes.size(); ++i) {
        dataWidget->coordinates()->axes[ i ].setMajors(7);
        dataWidget->coordinates()->axes[ i ].setMinors(5);
    }

    double a = surface->facets().first;
    double b = surface->facets().second;

    dimWidget->setText(QString("Cells ") + QString::number(a * b) + " (" + QString::number(a) + "x" + QString::number(b) + ")");

    updateColorLegend(7, 5);

    dataWidget->coordinates()->axes[ X1 ].setLabelString(QString("X1"));
    dataWidget->coordinates()->axes[ X2 ].setLabelString(QString("X2"));
    dataWidget->coordinates()->axes[ X3 ].setLabelString(QString("X3"));
    dataWidget->coordinates()->axes[ X4 ].setLabelString(QString("X4"));

    dataWidget->coordinates()->axes[ Y1 ].setLabelString(QString("Y1"));
    dataWidget->coordinates()->axes[ Y2 ].setLabelString(QString("Y2"));
    dataWidget->coordinates()->axes[ Y3 ].setLabelString(QString("Y3"));
    dataWidget->coordinates()->axes[ Y4 ].setLabelString(QString("Y4"));

    dataWidget->coordinates()->axes[ Z1 ].setLabelString(QString("Z1"));
    dataWidget->coordinates()->axes[ Z2 ].setLabelString(QString("Z2"));
    dataWidget->coordinates()->axes[ Z3 ].setLabelString(QString("Z3"));
    dataWidget->coordinates()->axes[ Z4 ].setLabelString(QString("Z4"));

    pickCoordSystem(activeCoordSystem);
}

void Mesh2MainWindow::pickCoordSystem(QAction* action)
{
    if (!action || !dataWidget)
        return;

    activeCoordSystem = action;

    dataWidget->setTitle("QwtPlot3D (Use Ctrl-Alt-Shift-LeftBtn-Wheel or keyboard)");

    if (!dataWidget->hasItems()) {
        double l = 0.6;
        dataWidget->createCoordinateSystem(Triple(-l, -l, -l), Triple(l, l, l));
        for (unsigned i = 0; i != dataWidget->coordinates()->axes.size(); ++i) {
            dataWidget->coordinates()->axes[ i ].setMajors(4);
            dataWidget->coordinates()->axes[ i ].setMinors(5);
        }
    }

    if (action == Box || action == Frame) {
        if (action == Box)
            dataWidget->coordinates()->setStyle(BOX);
        if (action == Frame)
            dataWidget->coordinates()->setStyle(FRAME);
        grids->setEnabled(true);
    } else if (action == None) {
        dataWidget->setTitle("QwtPlot3D (Use Ctrl-Alt-Shift-LeftBtn-Wheel or keyboard)");
        dataWidget->coordinates()->setStyle(NOCOORD);
        grids->setEnabled(false);
    }
}

void Mesh2MainWindow::pickPlotStyle(QAction* action)
{
    if (!action || !dataWidget)
        return;

    if (action == polygon) {
        surface->setPlotStyle(FILLED);
    } else if (action == filledmesh) {
        surface->setPlotStyle(FILLEDMESH);
    } else if (action == wireframe) {
        surface->setPlotStyle(WIREFRAME);
    } else if (action == hiddenline) {
        surface->setPlotStyle(HIDDENLINE);
    } else if (action == pointstyle) {
        surface->setPlotStyle(QWT3D_POINTS);
    } else {
        surface->setPlotStyle(NOPLOT);
    }
    dataWidget->update();
}

void Mesh2MainWindow::pickFloorStyle(QAction* action)
{
    if (!action || !dataWidget)
        return;

    if (action == floordata) {
        surface->setFloorStyle(FLOORDATA);
    } else if (action == flooriso) {
        surface->setFloorStyle(FLOORISO);
    } else {
        surface->setFloorStyle(NOFLOOR);
    }

    dataWidget->update();
}

void Mesh2MainWindow::setLeftGrid(bool b)
{
    setGrid(LEFT, b);
}
void Mesh2MainWindow::setRightGrid(bool b)
{
    setGrid(RIGHT, b);
}
void Mesh2MainWindow::setCeilGrid(bool b)
{
    setGrid(CEIL, b);
}
void Mesh2MainWindow::setFloorGrid(bool b)
{
    setGrid(FLOOR, b);
}
void Mesh2MainWindow::setFrontGrid(bool b)
{
    setGrid(FRONT, b);
}
void Mesh2MainWindow::setBackGrid(bool b)
{
    setGrid(BACK, b);
}

void Mesh2MainWindow::setGrid(SIDE s, bool b)
{
    if (!dataWidget)
        return;

    int sum = dataWidget->coordinates()->grids();

    if (b)
        sum |= s;
    else
        sum &= ~s;

    dataWidget->coordinates()->setGridLines(sum != NOSIDEGRID, sum != NOSIDEGRID, sum);
    dataWidget->update();
}

void Mesh2MainWindow::resetColors()
{
    if (!dataWidget)
        return;

    const RGBA axc = RGBA(0, 0, 0, 1);
    const RGBA bgc = RGBA(1.0, 1.0, 1.0, 1.0);
    const RGBA msc = RGBA(0, 0, 0, 1);
    const RGBA nuc = RGBA(0, 0, 0, 1);
    const RGBA lbc = RGBA(0, 0, 0, 1);
    const RGBA tc  = RGBA(0, 0, 0, 1);

    dataWidget->coordinates()->setAxesColor(axc);
    dataWidget->setBackgroundColor(bgc);
    surface->setMeshColor(msc);
    dataWidget->coordinates()->setNumberColor(nuc);
    dataWidget->coordinates()->setLabelColor(lbc);
    dataWidget->setTitleColor(tc);

    col_ = new Qwt3DStandardColor(dataWidget);
    surface->setDataColor(col_);
    surface->updateNormals();
    dataWidget->update();
}

void Mesh2MainWindow::pickAxesColor()
{
    QColor c = QColorDialog::getColor(Qt::white, this);
    if (!c.isValid())
        return;
    RGBA rgb = Qt2GL(c);
    dataWidget->coordinates()->setAxesColor(rgb);
    dataWidget->update();
}

void Mesh2MainWindow::pickBgColor()
{

    QColor c = QColorDialog::getColor(Qt::white, this);
    if (!c.isValid())
        return;
    RGBA rgb = Qt2GL(c);
    dataWidget->setBackgroundColor(rgb);
    dataWidget->update();
}

void Mesh2MainWindow::pickMeshColor()
{

    QColor c = QColorDialog::getColor(Qt::white, this);
    if (!c.isValid())
        return;
    RGBA rgb = Qt2GL(c);
    surface->setMeshColor(rgb);
    dataWidget->update();
}

void Mesh2MainWindow::pickNumberColor()
{

    QColor c = QColorDialog::getColor(Qt::white, this);
    if (!c.isValid())
        return;
    RGBA rgb = Qt2GL(c);
    dataWidget->coordinates()->setNumberColor(rgb);
    dataWidget->update();
}

void Mesh2MainWindow::pickLabelColor()
{
    QColor c = QColorDialog::getColor(Qt::white, this);
    if (!c.isValid())
        return;
    RGBA rgb = Qt2GL(c);
    dataWidget->coordinates()->setLabelColor(rgb);
    dataWidget->update();
}
void Mesh2MainWindow::pickTitleColor()
{
    QColor c = QColorDialog::getColor(Qt::white, this);
    if (!c.isValid())
        return;
    RGBA rgb = Qt2GL(c);
    dataWidget->setTitleColor(rgb);
    dataWidget->update();
}

void Mesh2MainWindow::pickLighting()
{
    lightingdlg_->show();
}

void Mesh2MainWindow::pickDataColor()
{
    QString s = QFileDialog::getOpenFileName(this, "", "./../../data/colormaps", "Colormap files (*.map *.MAP)");
    adaptDataColors(s);
}

void Mesh2MainWindow::adaptDataColors(const QString& fileName)
{
    ColorVector cv;

    if (!openColorMap(cv, fileName))
        return;

    col_ = new Qwt3DStandardColor(dataWidget);
    col_->setColorVector(cv);

    surface->setDataColor(col_);
    surface->updateNormals();
    dataWidget->showColorLegend(legend_);
    dataWidget->update();
}

void Mesh2MainWindow::pickNumberFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (!ok) {
        return;
    }
    dataWidget->coordinates()->setNumberFont(font);
    dataWidget->update();
}
void Mesh2MainWindow::pickLabelFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (!ok) {
        return;
    }
    dataWidget->coordinates()->setLabelFont(font);
    dataWidget->update();
}
void Mesh2MainWindow::pickTitleFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, this);
    if (!ok) {
        return;
    }
    dataWidget->setTitleFont(font.family(), font.pointSize(), font.weight(), font.italic());
}

void Mesh2MainWindow::resetFonts()
{
    dataWidget->coordinates()->setNumberFont(QFont("Courier", 12));
    dataWidget->coordinates()->setLabelFont(QFont("Courier", 14, QFont::Bold));
    dataWidget->setTitleFont("Arial", 14, QFont::Normal);
    dataWidget->update();
}

void Mesh2MainWindow::setStandardView()
{
    dataWidget->setRotation(30, 0, 15);
    dataWidget->setViewportShift(0.05, 0);
    dataWidget->setScale(1, 1, 1);
    dataWidget->setZoom(0.95);
}

void Mesh2MainWindow::dumpImage()
{
    static int counter = 0;
    if (!dataWidget)
        return;
    QString name;

    name = QString("dump_") + QString::number(counter++) + ".";

    if (filetype_ == "PS_GZ")
        name += "ps.gz";
    else if (filetype_ == "EPS_GZ")
        name += "eps.gz";
    else
        name += filetype_;

#ifdef QWT3D_ENABLE_GL2PS
    auto* vw = dynamic_cast<Qwt3DVectorWriter*>(Qwt3DIO::outputHandler("PDF"));
    if (vw)
        vw->setSortMode(Qwt3DVectorWriter::BSPSORT);
#endif
    Qwt3DIO::save(dataWidget, name.toLower(), filetype_);
}

void Mesh2MainWindow::toggleAnimation(bool val)
{
    if (val) {
        timer->start(redrawWait);
    } else {
        timer->stop();
    }
}

void Mesh2MainWindow::rotate()
{
    if (!dataWidget)
        return;

    dataWidget->setRotation(
        int(dataWidget->xRotation() + 1) % 360, int(dataWidget->yRotation() + 1) % 360, int(dataWidget->zRotation() + 1) % 360
    );
}

void Mesh2MainWindow::toggleProjectionMode(bool val)
{
    dataWidget->setOrtho(val);
}

void Mesh2MainWindow::toggleColorLegend(bool val)
{
    legend_ = val;
    dataWidget->showColorLegend(val);
}

void Mesh2MainWindow::toggleAutoScale(bool val)
{
    dataWidget->coordinates()->setAutoScale(val);
    dataWidget->update();
}

void Mesh2MainWindow::toggleShader(bool val)
{
    if (val)
        surface->setShading(GOURAUD);
    else
        surface->setShading(FLAT);
}

void Mesh2MainWindow::setPolygonOffset(int val)
{
    surface->setPolygonOffset(val / 10.0);
    dataWidget->update();
}

void Mesh2MainWindow::showRotate(double x, double y, double z)
{
    rotateLabel->setText(
        " Angles (" + QString::number(x, 'g', 3) + " ," + QString::number(y, 'g', 3) + " ," + QString::number(z, 'g', 3) + ")"
    );
}
void Mesh2MainWindow::showShift(double x, double y)
{
    shiftLabel->setText(" Shifts (" + QString::number(x, 'g', 3) + " ," + QString::number(y, 'g', 3) + " )");
}
void Mesh2MainWindow::showScale(double x, double y, double z)
{
    scaleLabel->setText(
        " Scales (" + QString::number(x, 'g', 3) + " ," + QString::number(y, 'g', 3) + " ," + QString::number(z, 'g', 3) + ")"
    );
}
void Mesh2MainWindow::showZoom(double z)
{
    zoomLabel->setText(" Zoom " + QString::number(z, 'g', 3));
}

void Mesh2MainWindow::openMesh()
{
    QString data(QFileDialog::getOpenFileName(this, "", "../../data", "nodes (*.nod)"));
    QString edges(QFileDialog::getOpenFileName(this, "", "../../data", "connectivities (*.cel)"));

    if (data.isEmpty() || edges.isEmpty() || !dataWidget)
        return;

    TripleField vdata;
    CellField vpoly;

    readNodes(vdata, QWT3DLOCAL8BIT(data), NodeFilter());
    readConnections(vpoly, QWT3DLOCAL8BIT(edges), CellFilter());

    surface->loadFromData(vdata, vpoly);
    dimWidget->setText(QString("Cells ") + QString::number(surface->facets().first));

    for (unsigned i = 0; i != dataWidget->coordinates()->axes.size(); ++i) {
        dataWidget->coordinates()->axes[ i ].setMajors(4);
        dataWidget->coordinates()->axes[ i ].setMinors(5);
        dataWidget->coordinates()->axes[ i ].setLabelString(QString(""));
    }

    updateColorLegend(4, 5);
    pickCoordSystem(activeCoordSystem);
}

void Mesh2MainWindow::showNormals(bool val)
{
    surface->showNormals(val);
    surface->updateNormals();
    dataWidget->update();
}

void Mesh2MainWindow::setNormalLength(int val)
{
    surface->setNormalLength(val / 400.);
    surface->updateNormals();
    dataWidget->update();
}

void Mesh2MainWindow::setNormalQuality(int val)
{
    surface->setNormalQuality(val);
    surface->updateNormals();
    dataWidget->update();
}

bool Mesh2MainWindow::openColorMap(ColorVector& cv, QString fname)
{
    if (fname.isEmpty())
        return false;

    ifstream file(QWT3DLOCAL8BIT(fname));

    if (!file)
        return false;

    RGBA rgb;
    cv.clear();

    while (file) {
        file >> rgb.r >> rgb.g >> rgb.b;
        file.ignore(1000, '\n');
        if (!file.good())
            break;
        else {
            rgb.a = 1;
            rgb.r /= 255;
            rgb.g /= 255;
            rgb.b /= 255;
            cv.push_back(rgb);
        }
    }

    return true;
}

void Mesh2MainWindow::updateColorLegend(int majors, int minors)
{
    dataWidget->legend()->setMajors(majors);
    dataWidget->legend()->setMinors(minors);
    double start, stop;
    dataWidget->coordinates()->axes[ Z1 ].limits(start, stop);
    dataWidget->legend()->setLimits(start, stop);
}

void Mesh2MainWindow::setFileType(QString const& name)
{
    filetype_ = name;
}

void Mesh2MainWindow::enableLighting(bool val)
{
    dataWidget->enableLighting(val);
    dataWidget->illuminate(0);
    dataWidget->update();
}
