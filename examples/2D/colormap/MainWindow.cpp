#include "MainWindow.h"

#include <qnamespace.h>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QHash>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLinearGradient>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QSize>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

#include <memory>

#include "Plot.h"

#include <qwt_colormap.h>
#include <qwt_colormap_preset.h>

namespace
{
    /**
        @brief A gradient strip of one preset, used as the icon of the preset list
     */
    QIcon presetIcon(const QString& name)
    {
        const int width = 128;
        const int height = 18;

        QPixmap pixmap(width, height);
        pixmap.fill(Qt::transparent);

        const std::unique_ptr< QwtColorMap > colorMap = QwtColorMapPreset::create(name);

        QPainter painter(&pixmap);
        for (int i = 0; i < width; i++)
        {
            const double v = double(i) / (width - 1);
            painter.setPen(QColor::fromRgba(colorMap->rgb(0.0, 1.0, v)));
            painter.drawLine(i, 0, i, height);
        }

        return QIcon(pixmap);
    }

    /**
        @brief Short description, where each preset is typically used
     */
    QString presetDescription(const QString& name)
    {
        static const QHash< QString, QString > descriptions = {
            { QStringLiteral("viridis"), QObject::tr("Perceptually uniform, colorblind safe; matplotlib default") },
            { QStringLiteral("plasma"), QObject::tr("Perceptually uniform, purple to bright yellow") },
            { QStringLiteral("inferno"), QObject::tr("Perceptually uniform, black to bright yellow") },
            { QStringLiteral("magma"), QObject::tr("Perceptually uniform, black to pinkish white") },
            { QStringLiteral("cividis"), QObject::tr("Blue to yellow, readable when printed grayscale") },
            { QStringLiteral("jet"), QObject::tr("Classic MATLAB rainbow, non uniform with visible banding") },
            { QStringLiteral("hot"), QObject::tr("Thermal scale, black red yellow white") },
            { QStringLiteral("cool"), QObject::tr("Cyan to magenta") },
            { QStringLiteral("spring"), QObject::tr("Magenta to yellow") },
            { QStringLiteral("summer"), QObject::tr("Greens") },
            { QStringLiteral("autumn"), QObject::tr("Red to yellow") },
            { QStringLiteral("winter"), QObject::tr("Blue to green") },
            { QStringLiteral("gray"), QObject::tr("Black to white grayscale") },
            { QStringLiteral("bone"), QObject::tr("Grayscale with a blue tint") },
            { QStringLiteral("copper"), QObject::tr("Black to copper brown") },
            { QStringLiteral("rainbow"), QObject::tr("Full hue cycle, red to purple") },
            { QStringLiteral("hsv"), QObject::tr("Full HSV hue cycle") },
            { QStringLiteral("turbo"), QObject::tr("Google's improved rainbow, more uniform than jet") },
            { QStringLiteral("coolwarm"), QObject::tr("Diverging blue white red for signed values") },
            { QStringLiteral("rdylbu"), QObject::tr("Diverging red yellow blue (ColorBrewer)") },
            { QStringLiteral("rdylgn"), QObject::tr("Diverging red yellow green (ColorBrewer)") },
            { QStringLiteral("spectral"), QObject::tr("Diverging multi hue red to blue (ColorBrewer)") },
        };

        return descriptions.value(name);
    }

    const QString kBackdropLight(QObject::tr("Light"));
    const QString kBackdropDark(QObject::tr("Dark"));
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_presetList = new QListWidget();
    m_presetList->setMinimumWidth(200);
    m_presetList->setMaximumWidth(280);

    for (const QString& name : QwtColorMapPreset::availablePresets())
    {
        QListWidgetItem* item = new QListWidgetItem(presetIcon(name), name);
        item->setToolTip(presetDescription(name));
        item->setSizeHint(QSize(150, 34));
        m_presetList->addItem(item);
    }

    m_plot = new Plot();

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_presetList);
    splitter->addWidget(m_plot);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    m_backdropBox = new QComboBox();
    m_backdropBox->addItem(kBackdropLight);
    m_backdropBox->addItem(kBackdropDark);

    m_contourBox = new QCheckBox(tr("Contours"));

    m_colorTableBox = new QComboBox();
    m_colorTableBox->addItem(tr("Smooth"), 0);
    m_colorTableBox->addItem(QStringLiteral("256"), 256);
    m_colorTableBox->addItem(QStringLiteral("1024"), 1024);
    m_colorTableBox->addItem(QStringLiteral("16384"), 16384);

    QToolButton* screenshotButton = new QToolButton();
    screenshotButton->setText(tr("Save Screenshots..."));
    screenshotButton->setToolButtonStyle(Qt::ToolButtonTextOnly);

    QToolBar* toolBar = new QToolBar();
    toolBar->setMovable(false);
    toolBar->addWidget(new QLabel(tr("Backdrop ")));
    toolBar->addWidget(m_backdropBox);
    toolBar->addSeparator();
    toolBar->addWidget(new QLabel(tr("Color Table ")));
    toolBar->addWidget(m_colorTableBox);
    toolBar->addSeparator();
    toolBar->addWidget(m_contourBox);
    toolBar->addSeparator();
    toolBar->addWidget(screenshotButton);
    addToolBar(toolBar);

    statusBar()->showMessage(tr("Select one of the 22 scientific colormap presets"));

    connect(m_presetList, &QListWidget::currentRowChanged,
        this, &MainWindow::onPresetSelectionChanged);
    connect(m_backdropBox, QOverload< int >::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onBackdropChanged);
    connect(m_contourBox, &QCheckBox::toggled,
        this, &MainWindow::onContoursToggled);
    connect(m_colorTableBox, QOverload< int >::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onColorTableChanged);
    connect(screenshotButton, &QToolButton::clicked,
        this, &MainWindow::chooseScreenshotDirectory);

    m_presetList->setCurrentRow(0);
}

bool MainWindow::exportScreenshots(const QString& directory)
{
    if (directory.isEmpty())
        return false;

    if (!QDir().mkpath(directory))
    {
        statusBar()->showMessage(tr("Cannot create directory: %1").arg(directory));
        return false;
    }

    const QString previousPreset = m_plot->colorMapPreset();

    // One image per preset. The resolution is the same as the resolution of
    // the plot on screen, so that the images look like what the user sees.
    const int width = 1280;
    const int height = 800;
    const int dotsPerMeter = qRound(96.0 * 1000.0 / 25.4);

    for (const QString& name : QwtColorMapPreset::availablePresets())
    {
        m_plot->setColorMapPreset(name);

        QImage image(width, height, QImage::Format_ARGB32);
        image.setDotsPerMeterX(dotsPerMeter);
        image.setDotsPerMeterY(dotsPerMeter);
        image.fill(Qt::white);

        QPainter painter(&image);
        m_renderer.render(m_plot, &painter, QRectF(0, 0, width, height));
        painter.end();

        const QString filePath = QDir(directory).filePath(name + ".png");
        if (!image.save(filePath))
        {
            statusBar()->showMessage(tr("Failed to save %1").arg(QDir::toNativeSeparators(filePath)));
            m_plot->setColorMapPreset(previousPreset);
            return false;
        }

        statusBar()->showMessage(tr("Saved %1").arg(QDir::toNativeSeparators(filePath)));
    }

    m_plot->setColorMapPreset(previousPreset);

    saveOverviewImage(QDir(directory).filePath(QStringLiteral("overview.png")));
    saveWindowImage(QDir(directory).filePath(QStringLiteral("colormap.png")));

    return true;
}

void MainWindow::saveOverviewImage(const QString& filePath)
{
    const QStringList presets = QwtColorMapPreset::availablePresets();

    // Two columns of gradient strips, ordered like the preset list
    const int columns = 2;
    const int rows = (presets.count() + columns - 1) / columns;

    const int margin = 20;
    const int gutter = 40;
    const int stripWidth = 560;
    const int stripHeight = 36;
    const int labelHeight = 22;
    const int cellHeight = labelHeight + stripHeight;
    const int rowSpacing = 10;

    const QSize imageSize(
        margin * 2 + columns * stripWidth + gutter,
        margin * 2 + rows * cellHeight + (rows - 1) * rowSpacing);

    QImage image(imageSize, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QFont font = painter.font();
    font.setPointSize(font.pointSize() + 4);
    painter.setFont(font);

    for (int i = 0; i < presets.count(); i++)
    {
        const int column = i / rows;
        const int row = i % rows;

        const QPoint origin(margin + column * (stripWidth + gutter),
            margin + row * (cellHeight + rowSpacing));

        painter.setPen(Qt::black);
        painter.drawText(QRect(origin, QSize(stripWidth, labelHeight)),
            Qt::AlignLeft | Qt::AlignVCenter, presets.at(i));

        const std::unique_ptr< QwtColorMap > colorMap = QwtColorMapPreset::create(presets.at(i));

        QLinearGradient gradient(origin.x(), 0, origin.x() + stripWidth, 0);
        for (int step = 0; step <= 64; step++)
        {
            const double v = double(step) / 64;
            gradient.setColorAt(v, QColor::fromRgba(colorMap->rgb(0.0, 1.0, v)));
        }

        painter.fillRect(QRect(origin + QPoint(0, labelHeight), QSize(stripWidth, stripHeight)),
            QBrush(gradient));
    }

    painter.end();

    if (!image.save(filePath))
        statusBar()->showMessage(tr("Failed to save %1").arg(QDir::toNativeSeparators(filePath)));
}

void MainWindow::saveWindowImage(const QString& filePath)
{
    QPixmap pixmap = grab();
    if (pixmap.isNull())
    {
        pixmap = QPixmap(size() * devicePixelRatio());
        pixmap.setDevicePixelRatio(devicePixelRatio());
        render(&pixmap);
    }

    if (!pixmap.save(filePath))
        statusBar()->showMessage(tr("Failed to save %1").arg(QDir::toNativeSeparators(filePath)));
}

void MainWindow::onPresetSelectionChanged()
{
    const QListWidgetItem* item = m_presetList->currentItem();
    if (item == nullptr)
        return;

    const QString name = item->text();
    m_plot->setColorMapPreset(name);
    statusBar()->showMessage(QStringLiteral("%1 - %2").arg(name, presetDescription(name)));
}

void MainWindow::onBackdropChanged(int index)
{
    m_plot->setBackdropDark(index == 1);
}

void MainWindow::onContoursToggled(bool on)
{
    m_plot->setContoursVisible(on);
}

void MainWindow::onColorTableChanged(int index)
{
    m_plot->setColorTableSize(m_colorTableBox->itemData(index).toInt());
}

void MainWindow::chooseScreenshotDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Save Screenshots To"));
    if (directory.isEmpty())
        return;

    exportScreenshots(directory);
}

#include "moc_MainWindow.cpp"
