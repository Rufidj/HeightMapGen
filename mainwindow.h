#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <QImage>
#include <QEvent>
#include <QLabel>
#include <QMouseEvent>
#include <random>
#include <numeric>
#include <chrono>
#include "openglwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

using HeightMapData_t = std::vector<std::vector<unsigned char>>;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void on_pushButtonCreate_clicked();
    void on_pushButtonSave_clicked();
    void on_pushButtonGenerate_clicked();
    void on_pushButtonView3D_clicked();

private:
    Ui::MainWindow *ui;
    QLabel *dynamicImageLabel;

    HeightMapData_t heightMapData;
    QImage currentImage;
    int mapWidth = 0;
    int mapHeight = 0;
    bool isPainting = false;
    int brushHeight = 128;
    double brushIntensity = 0.3;

    // === BRUSH MODES ===
    enum BrushMode {
        RAISE_LOWER,
        SMOOTH,
        FLATTEN,
        NOISE
    };
    BrushMode currentBrushMode = RAISE_LOWER;
    int flattenHeight = 128;

    // === PERLIN NOISE VARIABLES ===
    std::vector<int> p;
    int octaves = 6;
    double persistence = 0.55;
    double frequencyOffset = 0.0;
    double frequencyScale = 8.0;

    // === SIMPLEX NOISE FUNCTIONS ===
    static constexpr int grad3[12][3] = {
        {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
        {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
        {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
    };

    // === UNDO/REDO SYSTEM ===
    std::vector<HeightMapData_t> undoStack;
    std::vector<HeightMapData_t> redoStack;
    int maxUndoSteps = 50;

    // === 3D VIEW ===
    OpenGLWidget *glWidget3D = nullptr;

    // === UTILITY FUNCTIONS ===
    void updateHeightmapDisplay();
    QPoint mapToDataCoordinates(int screenX, int screenY);
    void applyBrush(int mapX, int mapY);
    void applySmoothBrush(int mapX, int mapY);
    void applyFlattenBrush(int mapX, int mapY);
    void applyNoiseBrush(int mapX, int mapY);

    // === PERLIN NOISE FUNCTIONS ===
    void initializePerlin();
    double fade(double t);
    double lerp(double t, double a, double b);
    double grad(int hash, double x, double y, double z);
    double perlin(double x, double y);
    double fbm(double x, double y);

    // === SIMPLEX NOISE FUNCTIONS ===
    double simplexNoise(double x, double y);
    double simplexFbm(double x, double y);

    // === UNDO/REDO FUNCTIONS ===
    void saveStateToUndo();
    void undo();
    void redo();
    void clearRedoStack();
};

#endif // MAINWINDOW_H
