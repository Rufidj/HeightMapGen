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

    // Limpiar shaders
    if (terrainShader) {
        delete terrainShader;
        terrainShader = nullptr;
    }
    if (waterShader) {
        delete waterShader;
        waterShader = nullptr;
    }

    // Limpiar buffers de terreno
    if (terrainVAO) {
        terrainVAO->destroy();
        delete terrainVAO;
    }
    if (terrainVBO) {
        terrainVBO->destroy();
        delete terrainVBO;
    }
    if (terrainEBO) {
        terrainEBO->destroy();
        delete terrainEBO;
    }

    // Limpiar buffers de agua
    if (waterVAO) {
        waterVAO->destroy();
        delete waterVAO;
    }
    if (waterVBO) {
        waterVBO->destroy();
        delete waterVBO;
    }
    if (waterEBO) {
        waterEBO->destroy();
        delete waterEBO;
    }

    if (terrainTexture) {
        delete terrainTexture;
        terrainTexture = nullptr;
    }
    // AGREGAR limpieza de textura del agua
    if (waterTexture) {
        delete waterTexture;
        waterTexture = nullptr;
    }

    doneCurrent();
}

void OpenGLWidget::setupShaders()
{
    // Shader para terreno
    terrainShader = new QOpenGLShaderProgram(this);
    if (!terrainShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/terrain.vert")) {
        qDebug() << "ERROR: Failed to compile terrain vertex shader:" << terrainShader->log();
        return;
    }
    if (!terrainShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/terrain.frag")) {
        qDebug() << "ERROR: Failed to compile terrain fragment shader:" << terrainShader->log();
        return;
    }
    if (!terrainShader->link()) {
        qDebug() << "ERROR: Failed to link terrain shader:" << terrainShader->log();
        return;
    }

    // Shader para agua
    waterShader = new QOpenGLShaderProgram(this);
    if (!waterShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/water.vert")) {
        qDebug() << "ERROR: Failed to compile water vertex shader:" << waterShader->log();
        return;
    }
    if (!waterShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/water.frag")) {
        qDebug() << "ERROR: Failed to compile water fragment shader:" << waterShader->log();
        return;
    }
    if (!waterShader->link()) {
        qDebug() << "ERROR: Failed to link water shader:" << waterShader->log();
        return;
    }

    qDebug() << "Shaders compiled and linked successfully";
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

    qDebug() << "=== INITIALIZING SHADERS ===";

    // Verificar que setupShaders() existe y se llama
    if (terrainShader || waterShader) {
        qDebug() << "ERROR: Shaders already exist before setup!";
    }

    setupShaders();

    // VERIFICACIÓN CRÍTICA
    if (!terrainShader || !waterShader) {
        qDebug() << "CRITICAL ERROR: Shaders failed to initialize!";
        qDebug() << "terrainShader:" << (terrainShader != nullptr);
        qDebug() << "waterShader:" << (waterShader != nullptr);
        qDebug() << "FALLING BACK TO FIXED PIPELINE - WILL CRASH WITH LARGE MAPS";
    } else {
        qDebug() << "SUCCESS: Shaders initialized correctly";
        qDebug() << "terrainShader valid:" << terrainShader->isLinked();
        qDebug() << "waterShader valid:" << waterShader->isLinked();
    }

    // Crear VAOs y VBOs solo si los shaders funcionan
    if (terrainShader && waterShader) {
        terrainVAO = new QOpenGLVertexArrayObject(this);
        terrainVAO->create();

        terrainVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        terrainVBO->create();

        terrainEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        terrainEBO->create();

        waterVAO = new QOpenGLVertexArrayObject(this);
        waterVAO->create();

        waterVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        waterVBO->create();

        waterEBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
        waterEBO->create();

        qDebug() << "VAOs and VBOs created successfully";
    }

    qDebug() << "OpenGL initialized successfully";

    // Generar mallas diferidas
    if (!heightMapData.empty() && mapWidth > 0 && mapHeight > 0) {
        qDebug() << "Generating deferred meshes...";
        generateMesh();
        generateWaterMesh();
    }
}
void OpenGLWidget::setupTerrainBuffers()
{
    if (vertices.empty() || indices.empty()) {
        qDebug() << "ERROR: No terrain data to upload";
        return;
    }

    terrainVAO->bind();

    // VBO para vértices
    terrainVBO->bind();
    terrainVBO->allocate(vertices.data(), vertices.size() * sizeof(float));

    // Layout: position (3), color (3), texCoord (2) = 8 floats por vértice
    terrainShader->bind();

    // Atributo 0: position
    terrainShader->enableAttributeArray(0);
    terrainShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));

    // Atributo 1: color
    terrainShader->enableAttributeArray(1);
    terrainShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 8 * sizeof(float));

    // Atributo 2: texCoord
    terrainShader->enableAttributeArray(2);
    terrainShader->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, 8 * sizeof(float));

    // EBO para índices
    terrainEBO->bind();
    terrainEBO->allocate(indices.data(), indices.size() * sizeof(unsigned int));

    terrainVAO->release();
    terrainShader->release();

    qDebug() << "Terrain buffers configured";
}

void OpenGLWidget::setupWaterBuffers()
{
    if (waterVertices.empty() || waterIndices.empty()) {
        qDebug() << "No water data to upload";
        return;
    }

    waterVAO->bind();

    // VBO para vértices de agua
    waterVBO->bind();
    waterVBO->allocate(waterVertices.data(), waterVertices.size() * sizeof(float));

    // MODIFICAR: Layout ahora es position (3), color (3), texCoord (2) = 8 floats por vértice
    waterShader->bind();

    // Atributo 0: position
    waterShader->enableAttributeArray(0);
    waterShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 8 * sizeof(float));  // CAMBIO: 8 en lugar de 6

    // Atributo 1: color
    waterShader->enableAttributeArray(1);
    waterShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 8 * sizeof(float));  // CAMBIO: 8 en lugar de 6

    // AGREGAR: Atributo 2: texCoord
    waterShader->enableAttributeArray(2);
    waterShader->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, 8 * sizeof(float));

    // EBO para índices
    waterEBO->bind();
    waterEBO->allocate(waterIndices.data(), waterIndices.size() * sizeof(unsigned int));

    waterVAO->release();
    waterShader->release();

    qDebug() << "Water buffers configured with texture coordinates";
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

            vertices.push_back(static_cast<float>(x));
            vertices.push_back(height);
            vertices.push_back(static_cast<float>(y));

            float r, g, b;
            if (height < 20.0f) {
                r = 0.2f; g = 0.4f; b = 0.8f;
            } else if (height < 40.0f) {
                r = 0.76f; g = 0.7f; b = 0.5f;
            } else if (height < 60.0f) {
                r = 0.2f; g = 0.6f; b = 0.2f;
            } else if (height < 80.0f) {
                r = 0.5f; g = 0.5f; b = 0.5f;
            } else {
                r = 1.0f; g = 1.0f; b = 1.0f;
            }
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);

            vertices.push_back(static_cast<float>(x) / mapWidth);
            vertices.push_back(static_cast<float>(y) / mapHeight);
        }
    }

    // Generar índices
    for (int y = 0; y < mapHeight - 1; ++y) {
        for (int x = 0; x < mapWidth - 1; ++x) {
            unsigned int topLeft = y * mapWidth + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (y + 1) * mapWidth + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    qDebug() << "Mesh generated:" << vertices.size() / 8 << "vertices,"
             << indices.size() / 3 << "triangles";

    setupTerrainBuffers();
}

void OpenGLWidget::generateWaterMesh()
{
    waterVertices.clear();
    waterIndices.clear();

    if (mapWidth <= 0 || mapHeight <= 0 || heightMapData.empty()) {
        qDebug() << "ERROR: Invalid map dimensions for water mesh";
        return;
    }

    float waterHeight = waterLevel;
    unsigned int vertexIndex = 0;

    // Generar agua solo en zonas bajas del terreno
    for (int y = 0; y < mapHeight - 1; ++y) {
        for (int x = 0; x < mapWidth - 1; ++x) {
            // Obtener las alturas de las 4 esquinas de la celda
            float h1 = heightMapData[y][x] / 255.0f * 100.0f;
            float h2 = heightMapData[y][x + 1] / 255.0f * 100.0f;
            float h3 = heightMapData[y + 1][x + 1] / 255.0f * 100.0f;
            float h4 = heightMapData[y + 1][x] / 255.0f * 100.0f;

            // Solo generar agua si AL MENOS UNA esquina está bajo el nivel del agua
            if (h1 < waterHeight || h2 < waterHeight ||
                h3 < waterHeight || h4 < waterHeight) {

                // Vértice 0: esquina superior izquierda (X, Y, Z, R, G, B, U, V)
                waterVertices.push_back(static_cast<float>(x));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x) / mapWidth);
                waterVertices.push_back(static_cast<float>(y) / mapHeight);

                // Vértice 1: esquina superior derecha
                waterVertices.push_back(static_cast<float>(x + 1));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x + 1) / mapWidth);
                waterVertices.push_back(static_cast<float>(y) / mapHeight);

                // Vértice 2: esquina inferior derecha
                waterVertices.push_back(static_cast<float>(x + 1));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y + 1));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x + 1) / mapWidth);
                waterVertices.push_back(static_cast<float>(y + 1) / mapHeight);

                // Vértice 3: esquina inferior izquierda
                waterVertices.push_back(static_cast<float>(x));
                waterVertices.push_back(waterHeight);
                waterVertices.push_back(static_cast<float>(y + 1));
                waterVertices.push_back(waterColor.x());
                waterVertices.push_back(waterColor.y());
                waterVertices.push_back(waterColor.z());
                waterVertices.push_back(static_cast<float>(x) / mapWidth);
                waterVertices.push_back(static_cast<float>(y + 1) / mapHeight);

                // Dos triángulos para formar el quad
                waterIndices.push_back(vertexIndex);
                waterIndices.push_back(vertexIndex + 1);
                waterIndices.push_back(vertexIndex + 2);

                waterIndices.push_back(vertexIndex);
                waterIndices.push_back(vertexIndex + 2);
                waterIndices.push_back(vertexIndex + 3);

                vertexIndex += 4;
            }
        }
    }

    qDebug() << "Water mesh generated:" << vertexIndex / 4 << "quads at level:" << waterLevel;

    // Configurar buffers después de generar geometría
    setupWaterBuffers();
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

void OpenGLWidget::loadWaterTexture(const QString &path)
{
    qDebug() << "Loading water texture from:" << path;

    if (waterTexture) {
        delete waterTexture;
        waterTexture = nullptr;
    }

    QImage image(path);
    if (image.isNull()) {
        qDebug() << "ERROR: Failed to load water texture";
        useWaterTexture = false;
        return;
    }

    waterTexture = new QOpenGLTexture(image.flipped(Qt::Vertical));
    waterTexture->setMinificationFilter(QOpenGLTexture::Linear);
    waterTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    waterTexture->setWrapMode(QOpenGLTexture::Repeat);

    useWaterTexture = true;
    update();

    qDebug() << "Water texture loaded successfully";
}

void OpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    projection.setToIdentity();
    // Cambiar 0.1f a 1.0f o 2.0f
    projection.perspective(45.0f, static_cast<float>(w) / h, 1.0f, 10000.0f);
}

void OpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (vertices.empty() || indices.empty()) {
        return;
    }

    // Configurar matrices de transformación
    view.setToIdentity();
    view.translate(0.0f, -50.0f + cameraY, -zoom);
    view.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    view.rotate(rotationY, 0.0f, 1.0f, 0.0f);
    view.translate(-cameraX, 0.0f, -cameraZ);

    model.setToIdentity();
    model.translate(-mapWidth / 2.0f, 0.0f, -mapHeight / 2.0f);

    QMatrix4x4 mvp = projection * view * model;

    // RENDERIZAR TERRENO CON SHADER
    terrainShader->bind();
    terrainShader->setUniformValue("mvpMatrix", mvp);
    terrainShader->setUniformValue("useTexture", useTexture);

    if (useTexture && terrainTexture) {
        terrainTexture->bind(0);
        terrainShader->setUniformValue("textureSampler", 0);
    }

    terrainVAO->bind();
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    terrainVAO->release();

    if (useTexture && terrainTexture) {
        terrainTexture->release();
    }
    terrainShader->release();
    // RENDERIZAR AGUA CON SHADER
    if (showWater && !waterVertices.empty() && !waterIndices.empty()) {
        glDisable(GL_CULL_FACE);

        waterShader->bind();
        waterShader->setUniformValue("mvpMatrix", mvp);
        waterShader->setUniformValue("waterAlpha", waterAlpha);

        // AGREGAR: Configurar textura del agua si existe
        if (useWaterTexture && waterTexture) {
            waterShader->setUniformValue("useWaterTexture", true);
            waterShader->setUniformValue("waterTextureSampler", 0);
            glActiveTexture(GL_TEXTURE0);
            waterTexture->bind();
        } else {
            waterShader->setUniformValue("useWaterTexture", false);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        waterVAO->bind();
        glDrawElements(GL_TRIANGLES, waterIndices.size(), GL_UNSIGNED_INT, 0);
        waterVAO->release();

        glDepthMask(GL_TRUE);

        // Liberar textura si se usó
        if (useWaterTexture && waterTexture) {
            waterTexture->release();
        }

        waterShader->release();
        glEnable(GL_CULL_FACE);
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
        cameraY += moveSpeed;
        break;
    case Qt::Key_Q:
        cameraY -= moveSpeed;
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

    // Solo generar mallas si OpenGL ya está inicializado
    if (context() && context()->isValid()) {
        generateMesh();
        generateWaterMesh();
        update();
    } else {
        qDebug() << "OpenGL not ready yet, deferring mesh generation";
    }
}
