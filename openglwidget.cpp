#include "openglwidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>
#include <cmath>

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
    rotationX = -60.0f;
    rotationY = 45.0f;
    zoom = 4.0f;
    cameraX = 0.0f;
    cameraY = 0.0f;
    cameraZ = 0.0f;
    moveSpeed = 5.0f;

    setFocusPolicy(Qt::StrongFocus);

    qDebug() << "OpenGLWidget constructor called";
}

OpenGLWidget::~OpenGLWidget()
{
    makeCurrent();
    if (terrainTexture) {
        delete terrainTexture;
        terrainTexture = nullptr;
    }
    doneCurrent();
}
void OpenGLWidget::setHeightMapData(const std::vector<std::vector<unsigned char>>& data)
{
    qDebug() << "setHeightMapData called";

    if (data.empty() || data[0].empty()) {
        qDebug() << "WARNING: Empty heightmap data!";
        return;
    }

    heightMapData = data;
    mapHeight = data.size();
    mapWidth = data[0].size();

    qDebug() << "Map dimensions:" << mapWidth << "x" << mapHeight;

    generateMesh();
    generateWaterMesh();
    update();
}

void OpenGLWidget::generateMesh()
{
    qDebug() << "generateMesh called";

    vertices.clear();
    indices.clear();

    if (heightMapData.empty() || mapWidth == 0 || mapHeight == 0) {
        qDebug() << "ERROR: Cannot generate mesh - no heightmap data";
        return;
    }

    // Generar vértices
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            float height = heightMapData[y][x] / 255.0f * 100.0f;

            // Posición (X, Y, Z)
            vertices.push_back(static_cast<float>(x));
            vertices.push_back(height);
            vertices.push_back(static_cast<float>(y));

            // Color basado en altura
            float r, g, b;
            if (height < 20.0f) {
                r = 0.2f; g = 0.4f; b = 0.8f; // Agua
            } else if (height < 40.0f) {
                r = 0.76f; g = 0.7f; b = 0.5f; // Arena
            } else if (height < 60.0f) {
                r = 0.2f; g = 0.6f; b = 0.2f; // Hierba
            } else if (height < 80.0f) {
                r = 0.5f; g = 0.5f; b = 0.5f; // Roca
            } else {
                r = 1.0f; g = 1.0f; b = 1.0f; // Nieve
            }
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);

            // Coordenadas de textura (U, V)
            vertices.push_back(static_cast<float>(x) / mapWidth);
            vertices.push_back(static_cast<float>(y) / mapHeight);
        }
    }

    // Generar índices (dos triángulos por quad)
    for (int y = 0; y < mapHeight - 1; ++y) {
        for (int x = 0; x < mapWidth - 1; ++x) {
            unsigned int topLeft = y * mapWidth + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (y + 1) * mapWidth + x;
            unsigned int bottomRight = bottomLeft + 1;

            // Primer triángulo (antihorario)
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Segundo triángulo (antihorario)
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    qDebug() << "Mesh generated:" << vertices.size() / 8 << "vertices,"
             << indices.size() / 3 << "triangles";
}

void OpenGLWidget::generateWaterMesh()
{
    waterVertices.clear();
    waterIndices.clear();

    if (mapWidth <= 0 || mapHeight <= 0) {
        qDebug() << "ERROR: Invalid map dimensions for water mesh";
        return;
    }

    float waterHeight = waterLevel;

    // Crear un plano rectangular simple
    // Vértice 0: esquina inferior izquierda
    waterVertices.push_back(0.0f);
    waterVertices.push_back(waterHeight);
    waterVertices.push_back(0.0f);
    waterVertices.push_back(waterColor.x());
    waterVertices.push_back(waterColor.y());
    waterVertices.push_back(waterColor.z());

    // Vértice 1: esquina inferior derecha
    waterVertices.push_back(static_cast<float>(mapWidth - 1));
    waterVertices.push_back(waterHeight);
    waterVertices.push_back(0.0f);
    waterVertices.push_back(waterColor.x());
    waterVertices.push_back(waterColor.y());
    waterVertices.push_back(waterColor.z());

    // Vértice 2: esquina superior derecha
    waterVertices.push_back(static_cast<float>(mapWidth - 1));
    waterVertices.push_back(waterHeight);
    waterVertices.push_back(static_cast<float>(mapHeight - 1));
    waterVertices.push_back(waterColor.x());
    waterVertices.push_back(waterColor.y());
    waterVertices.push_back(waterColor.z());

    // Vértice 3: esquina superior izquierda
    waterVertices.push_back(0.0f);
    waterVertices.push_back(waterHeight);
    waterVertices.push_back(static_cast<float>(mapHeight - 1));
    waterVertices.push_back(waterColor.x());
    waterVertices.push_back(waterColor.y());
    waterVertices.push_back(waterColor.z());

    // Dos triángulos para formar el plano (antihorario)
    waterIndices.push_back(0);
    waterIndices.push_back(1);
    waterIndices.push_back(2);

    waterIndices.push_back(0);
    waterIndices.push_back(2);
    waterIndices.push_back(3);

    qDebug() << "Water mesh generated at level:" << waterLevel;
}

void OpenGLWidget::setWaterLevel(float level)
{
    waterLevel = level;
    generateWaterMesh();
    update();

    qDebug() << "Water level set to:" << waterLevel;
}
void OpenGLWidget::loadTexture(const QString &path)
{
    qDebug() << "Loading texture from:" << path;

    if (terrainTexture) {
        delete terrainTexture;
        terrainTexture = nullptr;
    }

    QImage image(path);
    if (image.isNull()) {
        qDebug() << "ERROR: Failed to load texture";
        useTexture = false;
        return;
    }

    terrainTexture = new QOpenGLTexture(image.flipped(Qt::Vertical));
    terrainTexture->setMinificationFilter(QOpenGLTexture::Linear);
    terrainTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    terrainTexture->setWrapMode(QOpenGLTexture::Repeat);

    useTexture = true;
    update();

    qDebug() << "Texture loaded successfully";
}
void OpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    qDebug() << "OpenGL initialized successfully";
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    projection.setToIdentity();
    projection.perspective(45.0f, static_cast<float>(w) / h, 0.1f, 10000.0f);
}
void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.constData());

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(0.0f, -50.0f + cameraY, -zoom);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);

    glTranslatef(-cameraX, 0.0f, -cameraZ);

    glTranslatef(-mapWidth / 2.0f, 0.0f, -mapHeight / 2.0f);

    // RENDERIZAR TERRENO
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 8 * sizeof(float), vertices.data());
    glTexCoordPointer(2, GL_FLOAT, 8 * sizeof(float), vertices.data() + 6);

    if (useTexture && terrainTexture) {
        glDisableClientState(GL_COLOR_ARRAY);
        glEnable(GL_TEXTURE_2D);
        terrainTexture->bind();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        glEnableClientState(GL_COLOR_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glColorPointer(3, GL_FLOAT, 8 * sizeof(float), vertices.data() + 3);
    }

    glDepthMask(GL_TRUE);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    if (useTexture && terrainTexture) {
        glDisable(GL_TEXTURE_2D);
        terrainTexture->release();
    }

    // RENDERIZAR AGUA
    if (showWater && !waterVertices.empty() && !waterIndices.empty()) {
        glDisable(GL_TEXTURE_2D);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glVertexPointer(3, GL_FLOAT, 6 * sizeof(float), waterVertices.data());
        glColorPointer(3, GL_FLOAT, 6 * sizeof(float), waterVertices.data() + 3);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(waterColor.x(), waterColor.y(), waterColor.z(), waterAlpha);

        glDepthMask(GL_FALSE);
        glDrawElements(GL_TRIANGLES, waterIndices.size(), GL_UNSIGNED_INT, waterIndices.data());
        glDepthMask(GL_TRUE);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
    }
}
void OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
}

void OpenGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->pos().x() - lastMousePos.x();
    int dy = event->pos().y() - lastMousePos.y();

    if (event->buttons() & Qt::LeftButton) {
        rotationX += dy * 0.5f;
        rotationY += dx * 0.5f;

        if (rotationX > 89.0f) rotationX = 89.0f;
        if (rotationX < -89.0f) rotationX = -89.0f;

        update();
    }

    lastMousePos = event->pos();
}

void OpenGLWidget::wheelEvent(QWheelEvent *event)
{
    zoom -= event->angleDelta().y() / 120.0f;

    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 20.0f) zoom = 20.0f;

    update();
}

void OpenGLWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        cameraZ += moveSpeed;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        cameraZ -= moveSpeed;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        cameraX -= moveSpeed;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        cameraX += moveSpeed;
        break;
    case Qt::Key_E:
        cameraY += moveSpeed;  // Subir cámara
        break;
    case Qt::Key_Q:
        cameraY -= moveSpeed;  // Bajar cámara
        break;
    case Qt::Key_R:
        cameraX = 0.0f;
        cameraY = 0.0f;
        cameraZ = 0.0f;
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
        return;
    }

    update();
}
