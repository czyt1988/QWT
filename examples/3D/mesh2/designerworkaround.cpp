#include "designerworkaround.h"

namespace {
QString tr(const char *val)
{
    return QObject::tr(val);
}

void setCheckable(QActionGroup *ag)
{
    QList<QAction *> tmplist = ag->actions();
    for (QList<QAction *>::iterator it = tmplist.begin(); it != tmplist.end(); ++it)
        (*it)->setCheckable(true);
}

}

void DesignerWorkaround::setupWorkaround(QMainWindow *mw)
{
    // actions

    openFile = new QAction(QIcon(":/images/fileopen.png"), tr("&Open File"), mw);
    openFile->setShortcut(QKeySequence(tr("CTRL+O")));
    openMeshFile = new QAction(QIcon(":/images/filecell.png"), tr("Open FEM File"), mw);

    Exit = new QAction(tr("&Exit"), mw);
    Exit->setShortcut(QKeySequence(tr("CTRL+Q")));

    animation = new QAction(QIcon(":/images/movie.png"), tr("Animation"), mw);
    animation->setCheckable(true);
    dump = new QAction(QIcon(":/images/savecontent.png"), "", mw);

    coord = new QActionGroup(mw);
    Box = new QAction(QIcon(":/images/box.png"), "", coord);
    Frame = new QAction(QIcon(":/images/frame.png"), "", coord);
    None = new QAction(QIcon(":/images/grid.png"), "", coord);
    setCheckable(coord);

    grids = new QActionGroup(mw);
    front = new QAction(QIcon(":/images/gridfr.png"), "", grids);
    back = new QAction(QIcon(":/images/gridb.png"), "", grids);
    right = new QAction(QIcon(":/images/gridr.png"), "", grids);
    left = new QAction(QIcon(":/images/gridl.png"), "", grids);
    ceil = new QAction(QIcon(":/images/gridc.png"), "", grids);
    floor = new QAction(QIcon(":/images/gridf.png"), "", grids);
    grids->setExclusive(false);
    setCheckable(grids);

    plotstyle = new QActionGroup(mw);
    pointstyle = new QAction(QIcon(":/images/scattered.png"), "", plotstyle);
    wireframe = new QAction(QIcon(":/images/wireframe.png"), "", plotstyle);
    hiddenline = new QAction(QIcon(":/images/hiddenline.png"), "", plotstyle);
    polygon = new QAction(QIcon(":/images/polygon.png"), "", plotstyle);
    filledmesh = new QAction(QIcon(":/images/filledmesh.png"), "", plotstyle);
    nodata = new QAction(QIcon(":/images/nodata.png"), "", plotstyle);
    setCheckable(plotstyle);

    floorstyle = new QActionGroup(mw);
    floordata = new QAction(QIcon(":/images/floordata.png"), "", floorstyle);
    flooriso = new QAction(QIcon(":/images/flooriso.png"), "", floorstyle);
    floornone = new QAction(QIcon(":/images/floorempty.png"), "", floorstyle);
    setCheckable(floorstyle);

    normals = new QAction(QIcon(":/images/normals.png"), "", mw);
    normals->setCheckable(true);

    color = new QActionGroup(mw);
    axescolor = new QAction(tr("&Axes"), color);
    backgroundcolor = new QAction(tr("&Background"), color);
    meshcolor = new QAction(tr("&Mesh"), color);
    numbercolor = new QAction(tr("&Numbers"), color);
    labelcolor = new QAction(tr("&Label"), color);
    titlecolor = new QAction(tr("Caption"), color);
    datacolor = new QAction(tr("Data color"), color);
    resetcolor = new QAction(tr("&Reset"), color);

    font = new QActionGroup(mw);
    numberfont = new QAction(tr("&Scale numbering"), font);
    labelfont = new QAction(tr("&Axes label"), font);
    titlefont = new QAction(tr("&Caption"), font);
    resetfont = new QAction(tr("&Reset"), font);

    // toolbars
    mainToolbar = new QToolBar(QString(""), mw);
    mainToolbar->setMovable(false);

    mainToolbar->addAction(openFile);
    mainToolbar->addAction(openMeshFile);
    mainToolbar->addAction(dump);

    filetypeCB = new QComboBox;
    functionCB = new QComboBox;
    psurfaceCB = new QComboBox;

    mainToolbar->addWidget(filetypeCB);
    mainToolbar->addWidget(functionCB);
    mainToolbar->addWidget(psurfaceCB);

    mainToolbar->addAction(animation);

    csToolbar = new QToolBar(QString(""), mw);
    csToolbar->setMovable(false);

    csToolbar->addActions(coord->actions());
    csToolbar->addActions(grids->actions());
    csToolbar->addActions(plotstyle->actions());
    csToolbar->addActions(floorstyle->actions());
    csToolbar->addAction(normals);

    // menubar
    menubar = mw->menuBar(); // new QMenuBar();
    filemenu = menubar->addMenu("&File");
    filemenu->addAction(openFile);
    filemenu->addAction(openMeshFile);
    filemenu->addAction(animation);
    filemenu->addAction(Exit);
    colormenu = menubar->addMenu(tr("&Color"));
    colormenu->addActions(color->actions());
    fontmenu = menubar->addMenu(tr("&Fonts"));
    fontmenu->addActions(font->actions());

    mw->addToolBar(mainToolbar);
    mw->addToolBar(csToolbar);

    functionCB->clear();
    functionCB->addItem(tr("---"));
    functionCB->addItem(tr("Hat"));
    functionCB->addItem(tr("Rosenbrock"));
    functionCB->addItem(tr("Saddle"));
    functionCB->addItem(tr("Sombrero"));
    functionCB->addItem(tr("Ripple"));
    functionCB->setToolTip(tr("Display function"));
    psurfaceCB->clear();
    psurfaceCB->addItem(tr("---"));
    psurfaceCB->addItem(tr("Torus"));
    psurfaceCB->addItem(tr("Seashell"));
    psurfaceCB->addItem(tr("Boy"));
    psurfaceCB->addItem(tr("Dini"));
    psurfaceCB->addItem(tr("Cone"));
    psurfaceCB->setToolTip(tr("Display parametric surface. Turn Lighting on for better quality."));

    QObject::connect(Exit, &QAction::triggered, mw, &QMainWindow::close);
}
