#include <QCoreApplication>
#include <QOpenGLTexture>
#include <QSurfaceFormat>

#include "qyuvopenglwidget.h"

// 存储顶点坐标和纹理坐标
// 存在一起缓存在vbo
// 使用glVertexAttribPointer指定访问方式即可
// 顶点坐标 + 纹理坐标 (优化内存布局)
static const GLfloat coordinate[] = {
        // 顶点坐标 (x, y, z)
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,

        // 纹理坐标 (u, v)
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};

// 顶点着色器
static const QString s_vertShader = R"(
    attribute vec3 vertexIn;    // xyz顶点坐标
    attribute vec2 textureIn;   // xy纹理坐标
    varying vec2 textureOut;    // 传递给片段着色器的纹理坐标
    void main(void)
    {
        gl_Position = vec4(vertexIn, 1.0);  // 1.0表示vertexIn是一个顶点位置
        textureOut = textureIn; // 纹理坐标直接传递给片段着色器
    }
)";

// 修复后的片段着色器（标准BT.709矩阵）
static QString s_fragShader = R"(
    varying vec2 textureOut;
    uniform sampler2D textureY;
    uniform sampler2D textureU;
    uniform sampler2D textureV;

    void main() {
        float y = texture2D(textureY, textureOut).r;
        float u = texture2D(textureU, textureOut).r - 0.5;
        float v = texture2D(textureV, textureOut).r - 0.5;

        // BT.709 标准转换矩阵
        float r = y + 1.5748 * v;
        float g = y - 0.1873 * u - 0.4681 * v;
        float b = y + 1.8556 * u;

        gl_FragColor = vec4(r, g, b, 1.0);
    }
)";

QYUVOpenGLWidget::QYUVOpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    /*
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    QSurfaceFormat::setDefaultFormat(format);
    */
}

QYUVOpenGLWidget::~QYUVOpenGLWidget()
{
    makeCurrent();
    m_vbo.destroy();
    deInitTextures();
    deInitPBOs();  // 添加PBO清理
    doneCurrent();
}

QSize QYUVOpenGLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize QYUVOpenGLWidget::sizeHint() const
{
    return size();
}

void QYUVOpenGLWidget::setFrameSize(const QSize &frameSize)
{
    if (m_frameSize != frameSize) {
        m_frameSize = frameSize;
        m_needUpdate = true;
        // inittexture immediately
        repaint();
    }
}

const QSize &QYUVOpenGLWidget::frameSize()
{
    return m_frameSize;
}

void QYUVOpenGLWidget::updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU, quint32 linesizeV)
{
    if (m_textureInited) {
        updateTexture(m_texture[0], 0, dataY, linesizeY);
        updateTexture(m_texture[1], 1, dataU, linesizeU);
        updateTexture(m_texture[2], 2, dataV, linesizeV);
        update();
    }
}

void QYUVOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // 顶点缓冲对象初始化
    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(coordinate, sizeof(coordinate));
    initShader();

    // 初始化PBO
    initPBOs();

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void QYUVOpenGLWidget::initPBOs()
{
    // 为Y、U、V三个分量分别创建两个PBO（双缓冲）
    glGenBuffers(3 * 2, &m_pbo[0][0]);

    for (int i = 0; i < 3; ++i) {
        QSize size = (i == 0) ? m_frameSize : m_frameSize / 2;
        int bufferSize = size.width() * size.height() * (i == 0 ? 1 : 1); // YUV都是单通道

        for (int j = 0; j < 2; j++) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo[i][j]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, bufferSize, nullptr, GL_STREAM_DRAW);
        }

        m_pboIndex[i] = 0;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void QYUVOpenGLWidget::deInitPBOs()
{
    if (QOpenGLFunctions_3_3_Core::isInitialized()) {
        glDeleteBuffers(3 * 2, &m_pbo[0][0]);
    }

    memset(m_pbo, 0, sizeof(m_pbo));
    memset(m_pboIndex, 0, sizeof(m_pboIndex));
}

void QYUVOpenGLWidget::paintGL()
{
    if (m_needUpdate) {
        deInitTextures();
        initTextures();
        m_needUpdate = false;
    }

    if (m_textureInited) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture[0]);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_texture[1]);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_texture[2]);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

void QYUVOpenGLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    repaint();
}

void QYUVOpenGLWidget::initShader()
{
    // opengles的float、int等要手动指定精度
    if (QCoreApplication::testAttribute(Qt::AA_UseOpenGLES)) {
        s_fragShader.prepend(R"(
                             precision mediump int;
                             precision mediump float;
                             )");
    }
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, s_vertShader);
    m_shaderProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, s_fragShader);
    m_shaderProgram.link();
    m_shaderProgram.bind();

    // 指定顶点坐标在vbo中的访问方式
    // 参数解释：顶点坐标在shader中的参数名称，顶点坐标为float，起始偏移为0，顶点坐标类型为vec3，步幅为3个float
    m_shaderProgram.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 3, 3 * sizeof(float));
    // 启用顶点属性
    m_shaderProgram.enableAttributeArray("vertexIn");

    // 指定纹理坐标在vbo中的访问方式
    // 参数解释：纹理坐标在shader中的参数名称，纹理坐标为float，起始偏移为12个float（跳过前面存储的12个顶点坐标），纹理坐标类型为vec2，步幅为2个float
    m_shaderProgram.setAttributeBuffer("textureIn", GL_FLOAT, 12 * sizeof(float), 2, 2 * sizeof(float));
    m_shaderProgram.enableAttributeArray("textureIn");

    // 关联片段着色器中的纹理单元和opengl中的纹理单元（opengl一般提供16个纹理单元）
    m_shaderProgram.setUniformValue("textureY", 0);
    m_shaderProgram.setUniformValue("textureU", 1);
    m_shaderProgram.setUniformValue("textureV", 2);
}

void QYUVOpenGLWidget::initTextures()
{
    // 创建纹理
    glGenTextures(1, &m_texture[0]);
    glBindTexture(GL_TEXTURE_2D, m_texture[0]);
    // 设置纹理缩放时的策略
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 设置st方向上纹理超出坐标时的显示策略
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_frameSize.width(), m_frameSize.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &m_texture[1]);
    glBindTexture(GL_TEXTURE_2D, m_texture[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_frameSize.width() / 2, m_frameSize.height() / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1, &m_texture[2]);
    glBindTexture(GL_TEXTURE_2D, m_texture[2]);
    // 设置纹理缩放时的策略
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 设置st方向上纹理超出坐标时的显示策略
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_frameSize.width() / 2, m_frameSize.height() / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

    m_textureInited = true;
}

void QYUVOpenGLWidget::deInitTextures()
{
    if (QOpenGLFunctions_3_3_Core::isInitialized()) {
        glDeleteTextures(3, m_texture);
    }

    memset(m_texture, 0, sizeof(m_texture));
    m_textureInited = false;
}

void QYUVOpenGLWidget::updateTexture(GLuint texture, quint32 textureType, quint8 *pixels, quint32 stride)
{
    if (!pixels || !m_textureInited)
        return;

    QSize size = (textureType == 0) ? m_frameSize : m_frameSize / 2;

    makeCurrent();

    // 绑定当前PBO（用于上传新数据）
    int pboIdx = m_pboIndex[textureType];
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbo[textureType][pboIdx]);

    // 映射PBO内存
    void* pboData = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (pboData) {
        // 计算实际需要的内存大小
        int dataSize = size.width() * size.height();

        // 检查是否有步幅差异（行填充）
        if (stride == static_cast<quint32>(size.width())) {
            // 无行填充，直接拷贝
            memcpy(pboData, pixels, dataSize);
        } else {
            // 有行填充，需要逐行拷贝
            quint8* dst = static_cast<quint8*>(pboData);
            for (int i = 0; i < size.height(); ++i) {
                memcpy(dst, pixels + i * stride, size.width());
                dst += size.width();
            }
        }

        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }

    // 绑定纹理
    glBindTexture(GL_TEXTURE_2D, texture);

    // 从PBO更新纹理数据
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width(), size.height(),
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

    // 解绑PBO
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // 切换PBO索引（乒乓操作）
    m_pboIndex[textureType] = (pboIdx + 1) % 2;

    doneCurrent();
}
