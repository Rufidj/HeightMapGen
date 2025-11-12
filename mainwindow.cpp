#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "openglwidget.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <cmath>
#include <algorithm>
#include <QEvent>
#include <QDebug>
#include <QLayout>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QWidget>
#include <QMenuBar>
#include <QScrollArea>
#include <QLabel>
#include <QSize>
#include <random>
#include <numeric>
#include <chrono>
#include <sstream>
#include <QCheckBox>  // AGREGAR ESTA LÍNEA
#include <QSlider>    // AGREGAR ESTA LÍNEA TAMBIÉN

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (ui->scrollAreaDisplay) {
        ui->scrollAreaDisplay->installEventFilter(this);
    }

    isPainting = false;
    mapWidth = 0;
    mapHeight = 0;
    brushHeight = 128;
    dynamicImageLabel = nullptr;

    ui->lineEditWidth->setText("512");
    ui->lineEditHeight->setText("512");

    if (ui->sliderBrushSize) {
        ui->sliderBrushSize->setRange(1, 100);
        ui->sliderBrushSize->setValue(10);
    }

    if (ui->sliderBrushIntensity) {
        ui->sliderBrushIntensity->setRange(1, 100);
        ui->sliderBrushIntensity->setValue(50);
    }

    if (ui->spinBoxOctaves) {
        ui->spinBoxOctaves->setRange(1, 10);
        ui->spinBoxOctaves->setValue(6);
    }
    if (ui->doubleSpinBoxPersistence) {
        ui->doubleSpinBoxPersistence->setRange(0.1, 0.9);
        ui->doubleSpinBoxPersistence->setSingleStep(0.05);
        ui->doubleSpinBoxPersistence->setValue(0.55);
    }
    if (ui->doubleSpinBoxFrequencyScale) {
        ui->doubleSpinBoxFrequencyScale->setRange(1.0, 50.0);
        ui->doubleSpinBoxFrequencyScale->setSingleStep(0.5);
        ui->doubleSpinBoxFrequencyScale->setValue(8.0);
        frequencyScale = 8.0;
    }
    if (ui->lineEditOffset) {
        ui->lineEditOffset->setText("Aleatorio");
    }

    // Conectar botones
    connect(ui->pushButtonUndo, &QPushButton::clicked, this, &MainWindow::undo);
    connect(ui->pushButtonRedo, &QPushButton::clicked, this, &MainWindow::redo);

}

MainWindow::~MainWindow()
{
    if (dynamicImageLabel) {
        delete dynamicImageLabel;
    }
    delete ui;
}


bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->scrollAreaDisplay && dynamicImageLabel)
    {
        if (event->type() == QEvent::MouseButtonPress) {
            this->mousePressEvent(static_cast<QMouseEvent*>(event));
            return true;
        } else if (event->type() == QEvent::MouseMove) {
            if (isPainting) {
                this->mouseMoveEvent(static_cast<QMouseEvent*>(event));
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            this->mouseReleaseEvent(static_cast<QMouseEvent*>(event));
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on_pushButtonCreate_clicked()
{
    int newMapWidth = ui->lineEditWidth->text().toInt();
    int newMapHeight = ui->lineEditHeight->text().toInt();

    if (newMapWidth < 16 || newMapHeight < 16 || newMapWidth > 4096 || newMapHeight > 4096) {
        newMapWidth = 512;
        newMapHeight = 512;
        QMessageBox::warning(this, "Advertencia de Tamaño", "El tamaño debe estar entre 16 y 4096. Usando 512x512.");
        ui->lineEditWidth->setText("512");
        ui->lineEditHeight->setText("512");
    }

    mapWidth = newMapWidth;
    mapHeight = newMapHeight;

    heightMapData.assign(mapHeight, std::vector<unsigned char>(mapWidth, 128));
    currentImage = QImage(mapWidth, mapHeight, QImage::Format_RGB32);

    if (dynamicImageLabel) {
        delete dynamicImageLabel;
        dynamicImageLabel = nullptr;
    }

    dynamicImageLabel = new QLabel(ui->scrollAreaDisplay);
    dynamicImageLabel->setFixedSize(mapWidth, mapHeight);
    ui->scrollAreaDisplay->setWidget(dynamicImageLabel);

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    const int CONTROL_PANEL_WIDTH = 173;
    const int HORIZONTAL_FRAME_MARGIN = 50;
    const int VERTICAL_FRAME_MARGIN = 150;

    int maxScrollWidth = screenGeometry.width() - CONTROL_PANEL_WIDTH - HORIZONTAL_FRAME_MARGIN;
    int maxScrollHeight = screenGeometry.height() - VERTICAL_FRAME_MARGIN;

    int scrollAreaWidth = std::min(mapWidth, maxScrollWidth);
    int scrollAreaHeight = std::min(mapHeight, maxScrollHeight);

    ui->scrollAreaDisplay->setGeometry(180, 20, scrollAreaWidth, scrollAreaHeight);

    int requiredWidth = 180 + scrollAreaWidth + 20;
    int requiredHeight = 20 + scrollAreaHeight + 50;

    if (requiredHeight < 300) requiredHeight = 300;

    QSize newSize(requiredWidth, requiredHeight);
    this->setFixedSize(newSize);

    // Limpiar historial undo/redo
    undoStack.clear();
    redoStack.clear();

    updateHeightmapDisplay();
}

void MainWindow::updateHeightmapDisplay()
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) return;

    for (int y = 0; y < mapHeight; ++y) {
        QRgb *pixel = reinterpret_cast<QRgb*>(currentImage.scanLine(y));

        for (int x = 0; x < mapWidth; ++x) {
            unsigned char value = heightMapData[y][x];
            *pixel = qRgb(value, value, value);
            pixel++;
        }
    }

    dynamicImageLabel->setPixmap(QPixmap::fromImage(currentImage));
}

void MainWindow::on_pushButtonSave_clicked()
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Guardar Heightmap", "", "PNG Files (*.png)");
    if (fileName.isEmpty()) return;

    if (currentImage.save(fileName, "PNG")) {
        QMessageBox::information(this, "Éxito", "Heightmap guardado.");
    } else {
        QMessageBox::critical(this, "Error", "No se pudo guardar el archivo.");
    }
}


QPoint MainWindow::mapToDataCoordinates(int screenX, int screenY)
{
    int dataX = screenX;
    int dataY = screenY;

    return QPoint(
        std::min(std::max(dataX, 0), mapWidth - 1),
        std::min(std::max(dataY, 0), mapHeight - 1)
        );
}

void MainWindow::applyBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    double intensityFactor = 0.3;
    if (ui->sliderBrushIntensity) {
        intensityFactor = ui->sliderBrushIntensity->value() / 100.0;
    }

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);

                int currentValue = heightMapData[y][x];
                int targetValue = static_cast<int>(currentValue + (brushHeight - currentValue) * intensity * intensityFactor);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

// =================================================================
// === PERLIN NOISE FUNCTIONS
// =================================================================

void MainWindow::initializePerlin()
{
    p.resize(256);
    std::iota(p.begin(), p.end(), 0);

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(p.begin(), p.end(), std::default_random_engine(seed));

    p.insert(p.end(), p.begin(), p.end());

    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<> distrib(100.0, 5000.0);
    frequencyOffset = distrib(gen);
}

double MainWindow::fade(double t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double MainWindow::lerp(double t, double a, double b)
{
    return a + t * (b - a);
}

double MainWindow::grad(int hash, double x, double y, double z)
{
    int h = hash & 15;
    double u = (h < 8) ? x : y;
    double v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double MainWindow::perlin(double x, double y)
{
    if (p.empty()) initializePerlin();

    int X = (int)std::floor(x);
    int Y = (int)std::floor(y);

    x -= std::floor(x);
    y -= std::floor(y);
    double z = 0.0;

    double u = fade(x);
    double v = fade(y);

    int A = p[(X & 255)] + (Y & 255);
    int B = p[(X + 1) & 255] + (Y & 255);

    int AA = p[A & 511] + 0;
    int AB = p[B & 511] + 0;
    int BA = p[A & 511] + 1;
    int BB = p[B & 511] + 1;

    return lerp(v, lerp(u, grad(p[AA], x, y, z),
                        grad(p[BA], x - 1, y, z)),
                lerp(u, grad(p[AB], x, y - 1, z),
                     grad(p[BB], x - 1, y - 1, z)));
}

double MainWindow::fbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += perlin(x * freq, y * freq) * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return total / maxVal;
}

// =================================================================
// === SIMPLEX NOISE IMPLEMENTATION
// =================================================================

double MainWindow::simplexNoise(double xin, double yin)
{
    if (p.empty()) initializePerlin();

    const double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
    const double G2 = (3.0 - std::sqrt(3.0)) / 6.0;

    double s = (xin + yin) * F2;
    int i = std::floor(xin + s);
    int j = std::floor(yin + s);

    double t = (i + j) * G2;
    double X0 = i - t;
    double Y0 = j - t;
    double x0 = xin - X0;
    double y0 = yin - Y0;

    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else { i1 = 0; j1 = 1; }

    double x1 = x0 - i1 + G2;
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2;
    double y2 = y0 - 1.0 + 2.0 * G2;

    int ii = i & 255;
    int jj = j & 255;
    int gi0 = p[ii + p[jj]] % 12;
    int gi1 = p[ii + i1 + p[jj + j1]] % 12;
    int gi2 = p[ii + 1 + p[jj + 1]] % 12;

    double n0 = 0.0, n1 = 0.0, n2 = 0.0;

    double t0 = 0.5 - x0*x0 - y0*y0;
    if (t0 > 0) {
        t0 *= t0;
        n0 = t0 * t0 * (grad3[gi0][0]*x0 + grad3[gi0][1]*y0);
    }

    double t1 = 0.5 - x1*x1 - y1*y1;
    if (t1 > 0) {
        t1 *= t1;
        n1 = t1 * t1 * (grad3[gi1][0]*x1 + grad3[gi1][1]*y1);
    }

    double t2 = 0.5 - x2*x2 - y2*y2;
    if (t2 > 0) {
        t2 *= t2;
        n2 = t2 * t2 * (grad3[gi2][0]*x2 + grad3[gi2][1]*y2);
    }

    return 70.0 * (n0 + n1 + n2);
}

double MainWindow::simplexFbm(double x, double y)
{
    double total = 0.0;
    double amplitude = 1.0;
    double freq = 1.0;
    double maxVal = 0.0;

    for (int i = 0; i < octaves; ++i) {
        total += simplexNoise(x * freq, y * freq) * amplitude;
        maxVal += amplitude;

        amplitude *= persistence;
        freq *= 2.0;
    }

    return total / maxVal;
}

// =================================================================
// === ADDITIONAL BRUSH MODES
// =================================================================

void MainWindow::applySmoothBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    // Crear copia temporal para evitar modificar mientras calculamos promedios
    std::vector<std::vector<unsigned char>> tempData = heightMapData;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                int sum = 0;
                int count = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < mapWidth && ny >= 0 && ny < mapHeight) {
                            sum += tempData[ny][nx];
                            count++;
                        }
                    }
                }
                int average = sum / count;

                double intensity = 1.0 - (distSq / brushRadiusSq);
                int currentValue = tempData[y][x];
                int newValue = static_cast<int>(currentValue + (average - currentValue) * intensity * 0.3);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(newValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

void MainWindow::applyFlattenBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);
                int currentValue = heightMapData[y][x];

                int targetValue = static_cast<int>(currentValue + (flattenHeight - currentValue) * intensity * 0.1);
                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

void MainWindow::applyNoiseBrush(int mapX, int mapY)
{
    if (!ui->sliderBrushSize) return;

    int brushRadius = ui->sliderBrushSize->value();
    if (brushRadius < 1) brushRadius = 1;
    double brushRadiusSq = static_cast<double>(brushRadius) * brushRadius;

    if (mapX < 0 || mapX >= mapWidth || mapY < 0 || mapY >= mapHeight) return;

    int minX = std::max(0, mapX - brushRadius);
    int maxX = std::min(mapWidth - 1, mapX + brushRadius);
    int minY = std::max(0, mapY - brushRadius);
    int maxY = std::min(mapHeight - 1, mapY + brushRadius);

    if (p.empty()) initializePerlin();

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            double distSq = std::pow(static_cast<double>(x - mapX), 2) +
                            std::pow(static_cast<double>(y - mapY), 2);

            if (distSq <= brushRadiusSq) {
                double intensity = 1.0 - (distSq / brushRadiusSq);

                // Generar ruido en esta posición
                double noiseValue = perlin(x * 0.1, y * 0.1);
                int noiseHeight = static_cast<int>((noiseValue + 1.0) * 127.5);

                int currentValue = heightMapData[y][x];
                int targetValue = static_cast<int>(currentValue + (noiseHeight - currentValue) * intensity * 0.15);

                heightMapData[y][x] = static_cast<unsigned char>(std::min(std::max(targetValue, 0), 255));
            }
        }
    }

    updateHeightmapDisplay();
}

// =================================================================
// === UNDO/REDO SYSTEM
// =================================================================

void MainWindow::saveStateToUndo()
{
    undoStack.push_back(heightMapData);

    if (undoStack.size() > maxUndoSteps) {
        undoStack.erase(undoStack.begin());
    }

    clearRedoStack();
}

void MainWindow::undo()
{
    if (undoStack.empty()) {
        QMessageBox::information(this, "Deshacer", "No hay acciones para deshacer.");
        return;
    }

    redoStack.push_back(heightMapData);
    heightMapData = undoStack.back();
    undoStack.pop_back();

    updateHeightmapDisplay();
}

void MainWindow::redo()
{
    if (redoStack.empty()) {
        QMessageBox::information(this, "Rehacer", "No hay acciones para rehacer.");
        return;
    }

    undoStack.push_back(heightMapData);
    heightMapData = redoStack.back();
    redoStack.pop_back();

    updateHeightmapDisplay();
}

void MainWindow::clearRedoStack()
{
    redoStack.clear();
}

// =================================================================
// === TERRAIN GENERATION
// =================================================================

void MainWindow::on_pushButtonGenerate_clicked()
{
    if (mapWidth == 0 || mapHeight == 0) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero (Botón 'Crear').");
        return;
    }

    // 1. OBTENER PARÁMETROS DE LA GUI
    octaves = ui->spinBoxOctaves->value();
    persistence = ui->doubleSpinBoxPersistence->value();
    frequencyScale = ui->doubleSpinBoxFrequencyScale->value();

    QString offsetText = ui->lineEditOffset->text();
    if (offsetText.toLower() == "aleatorio" || offsetText.isEmpty()) {
        initializePerlin();
    } else {
        bool ok;
        double customOffset = offsetText.toDouble(&ok);
        if (ok) {
            frequencyOffset = customOffset;
        } else {
            initializePerlin();
            QMessageBox::warning(this, "Advertencia", "Desplazamiento no válido. Usando valor aleatorio.");
            ui->lineEditOffset->setText(QString::number(frequencyOffset));
        }
        if (p.empty()) initializePerlin();
    }

    double scale = std::min(mapWidth, mapHeight);
    const double baseFrequency = 1.0 / (scale * frequencyScale);

    // NUEVO: Determinar qué algoritmo usar
    QString noiseType = ui->comboBoxNoiseType->currentText();
    bool useSimplex = (noiseType == "Simplex Noise");

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            double sampleX = (double)x * baseFrequency + frequencyOffset;
            double sampleY = (double)y * baseFrequency + frequencyOffset;

            double noiseValue;
            if (useSimplex) {
                noiseValue = simplexFbm(sampleX, sampleY);
            } else {
                noiseValue = fbm(sampleX, sampleY);
            }

            unsigned char height = (unsigned char)((noiseValue + 1.0) * 127.5);
            heightMapData[y][x] = height;
        }
    }

    updateHeightmapDisplay();
    QMessageBox::information(this, "Éxito",
                             QString("Terreno generado con %1.\nOctavas: %2, Persistencia: %3, Escala: %4")
                                 .arg(noiseType)
                                 .arg(octaves)
                                 .arg(persistence)
                                 .arg(frequencyScale));
}

// =================================================================
// === MOUSE EVENTS
// =================================================================

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (mapWidth == 0 || mapHeight == 0 || !dynamicImageLabel) return;

    QPoint globalPos = event->globalPosition().toPoint();
    QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);

    if (!dynamicImageLabel->rect().contains(localPos)) return;

    saveStateToUndo();
    isPainting = true;

    QString brushModeText = ui->comboBoxBrushMode->currentText();

    if (brushModeText == "Suavizar") {
        currentBrushMode = SMOOTH;
    } else if (brushModeText == "Aplanar") {
        currentBrushMode = FLATTEN;
        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());
        flattenHeight = heightMapData[dataPos.y()][dataPos.x()];
    } else if (brushModeText == "Ruido") {
        currentBrushMode = NOISE;
    } else {
        currentBrushMode = RAISE_LOWER;
        if (event->button() == Qt::LeftButton) {
            brushHeight = 240;
        } else if (event->button() == Qt::RightButton) {
            brushHeight = 20;
        }
    }

    QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());

    switch (currentBrushMode) {
    case RAISE_LOWER:
        applyBrush(dataPos.x(), dataPos.y());
        break;
    case SMOOTH:
        applySmoothBrush(dataPos.x(), dataPos.y());
        break;
    case FLATTEN:
        applyFlattenBrush(dataPos.x(), dataPos.y());
        break;
    case NOISE:
        applyNoiseBrush(dataPos.x(), dataPos.y());
        break;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (isPainting && dynamicImageLabel) {
        QPoint globalPos = event->globalPosition().toPoint();
        QPoint localPos = dynamicImageLabel->mapFromGlobal(globalPos);

        if (!dynamicImageLabel->rect().contains(localPos)) return;

        QPoint dataPos = mapToDataCoordinates(localPos.x(), localPos.y());

        switch (currentBrushMode) {
        case RAISE_LOWER:
            applyBrush(dataPos.x(), dataPos.y());
            break;
        case SMOOTH:
            applySmoothBrush(dataPos.x(), dataPos.y());
            break;
        case FLATTEN:
            applyFlattenBrush(dataPos.x(), dataPos.y());
            break;
        case NOISE:
            applyNoiseBrush(dataPos.x(), dataPos.y());
            break;
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    isPainting = false;
}

// =================================================================
// === 3D VIEW WINDOW
// =================================================================

void MainWindow::on_pushButtonView3D_clicked()
{
    if (mapWidth == 0 || mapHeight == 0) {
        QMessageBox::warning(this, "Error", "Cree un mapa primero.");
        return;
    }

    // Crear ventana nueva con OpenGL widget
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Vista 3D - HeightMap");
    dialog->resize(800, 600);

    // Layout principal vertical
    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

    // === CONTROLES DE AGUA (Layout horizontal) ===
    QHBoxLayout *waterControls = new QHBoxLayout();

    // Checkbox para mostrar/ocultar agua
    QCheckBox *checkShowWater = new QCheckBox("Mostrar Agua", dialog);
    checkShowWater->setChecked(true);

    // Label y slider para nivel de agua
    QLabel *labelWaterLevel = new QLabel("Nivel de Agua:", dialog);
    QSlider *sliderWaterLevel = new QSlider(Qt::Horizontal, dialog);
    sliderWaterLevel->setRange(0, 100);
    sliderWaterLevel->setValue(50);
    sliderWaterLevel->setMinimumWidth(150);

    waterControls->addWidget(checkShowWater);
    waterControls->addWidget(labelWaterLevel);
    waterControls->addWidget(sliderWaterLevel);
    waterControls->addStretch();

    // === BOTÓN DE CARGAR TEXTURA ===
    QPushButton *btnTexture = new QPushButton("Cargar Textura", dialog);
    waterControls->addWidget(btnTexture);

    // Agregar controles al layout principal
    mainLayout->addLayout(waterControls);

    // === WIDGET OPENGL ===
    OpenGLWidget *glWidget = new OpenGLWidget(dialog);
    glWidget->setHeightMapData(heightMapData);
    mainLayout->addWidget(glWidget);

    // === CONEXIONES DE SEÑALES ===

    // Conexión del botón de textura
    connect(btnTexture, &QPushButton::clicked, [glWidget]() {
        QString fileName = QFileDialog::getOpenFileName(
            nullptr,
            "Seleccionar Textura",
            "",
            "Imágenes (*.png *.jpg *.jpeg *.bmp)"
            );

        if (!fileName.isEmpty()) {
            glWidget->loadTexture(fileName);
        }
    });

    // Conexión del slider de nivel de agua
    connect(sliderWaterLevel, &QSlider::valueChanged, [glWidget](int value) {
        glWidget->setWaterLevel(static_cast<float>(value));
    });

    // Conexión del checkbox de mostrar agua
    connect(checkShowWater, &QCheckBox::toggled, [glWidget](bool checked) {
        glWidget->showWater = checked;
        glWidget->update();
    });

    dialog->setLayout(mainLayout);
    dialog->show();
}
