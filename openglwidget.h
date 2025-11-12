#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <vector>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    ~OpenGLWidget();

    void setHeightMapData(const std::vector<std::vector<unsigned char>>& data);
    void loadTexture(const QString &path);

    // PÚBLICO: Variables y funciones de agua (movidas desde private)
    bool showWater = true;
    float waterLevel = 50.0f;
    void setWaterLevel(float level);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void generateMesh();
    void generateWaterMesh();

    // Datos del heightmap
    std::vector<std::vector<unsigned char>> heightMapData;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int mapWidth = 0;
    int mapHeight = 0;

    // Parámetros de cámara
    float rotationX = -60.0f;
    float rotationY = 45.0f;
    float zoom = 4.0f;
    float cameraX = 0.0f;
    float cameraZ = 0.0f;
    float moveSpeed = 5.0f;
    float cameraY = 0.0f;  // NUEVA: Posición vertical de la cámara

    QPoint lastMousePos;

    // Sistema de texturas
    QOpenGLTexture *terrainTexture = nullptr;
    bool useTexture = false;

    // Sistema de agua
    QVector3D waterColor = QVector3D(0.2f, 0.4f, 0.8f);
    float waterAlpha = 0.6f;
    std::vector<float> waterVertices;
    std::vector<unsigned int> waterIndices;

    // Proyección
    QMatrix4x4 projection;
};

#endif // OPENGLWIDGET_H
