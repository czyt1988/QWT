请先阅读 `README.md` 以了解全局规范。

# plan00 — 渲染基础设施与公共组件

## 目标

为 3D 模块创建现代 OpenGL 渲染基础设施层和公共抽象组件。本计划包含三大部分：

1. **GPU 资源管理**：VBO/VAO RAII 封装、着色器程序编译/链接、渲染上下文检测
2. **公共数据抽象**：`Field3D` 基类（标量场/向量场的统一接口）、`TriangleMesh` 通用三角网格
3. **公共几何工具**：`TubeBuilder`（管状几何构建器，供线图和流线复用）

所有新绘图类型（plan02-12）将基于此基础设施实现。

## 前置依赖

无。本计划是所有现代 OpenGL 绘图和标量场/向量场可视化的基础前提。

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_glbuffer.h` | VBO/VAO 管理 RAII 封装 |
| `src/plot3d/qwt3d_glbuffer.cpp` | VBO/VAO 管理实现 |
| `src/plot3d/qwt3d_shader.h` | 着色器程序管理封装 |
| `src/plot3d/qwt3d_shader.cpp` | 着色器程序实现 |
| `src/plot3d/qwt3d_shaders.h` | 内置着色器源码（内联字符串，含 Core Profile 和兼容回退） |
| `src/plot3d/qwt3d_rendercontext.h` | 渲染上下文检测（Core Profile vs 兼容模式） |
| `src/plot3d/qwt3d_rendercontext.cpp` | 渲染上下文实现 |
| `src/plot3d/qwt3d_field3d.h` | `Field3D` 基类 + `ScalarField`/`VectorField` 抽象接口 |
| `src/plot3d/qwt3d_field3d.cpp` | `GridScalarField`/`GridVectorField`/`FunctionScalarField`/`FunctionVectorField` 实现 |
| `src/plot3d/qwt3d_tubebuilder.h` | 管状几何构建器 |
| `src/plot3d/qwt3d_tubebuilder.cpp` | TubeBuilder 实现 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 将新 .h/.cpp 文件加入对应列表 |

---

## 技术方案

### 1. QOpenGLExtraFunctions 使用策略

**核心原则**：所有 OpenGL 函数调用必须通过 `QOpenGLExtraFunctions` 获取，不直接链接系统 GL 库。

每个 GL 资源管理类内部持有 `QOpenGLExtraFunctions*`，在构造时获取：

```cpp
#include <QOpenGLExtraFunctions>

class GLBuffer::PrivateData {
    QWT_DECLARE_PUBLIC(GLBuffer)
public:
    PrivateData(GLBuffer* q, unsigned int target, int usage);
    ~PrivateData();

    void release();  ///< Release GL resources (safe to call when context is gone)

    QOpenGLExtraFunctions* m_gl = nullptr;
    unsigned int m_bufferId = 0;   ///< OpenGL buffer name (not GLuint in public API)
    unsigned int m_target = 0;     ///< GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    int m_usage = 0;               ///< GL_STATIC_DRAW / GL_DYNAMIC_DRAW / GL_STREAM_DRAW
    size_t m_size = 0;
    bool m_released = false;       ///< True if GL resources have been released
};
```

**关键约束**：
- `QOpenGLExtraFunctions` 通过 `QOpenGLContext::currentContext()` 获取，存储在 PrivateData 中
- 如果 `currentContext()` 返回 `nullptr`（GL 上下文已销毁），所有操作变为 no-op
- 不使用 Qt 的 `QOpenGLBuffer` / `QOpenGLShaderProgram` 等高级封装，手动封装提供更细粒度控制

### 2. GLBuffer — VBO/VAO 管理

#### 设计要点

- RAII 封装：构造时创建 GL 资源，析构时安全释放
- **上下文安全析构**：析构函数检查 `QOpenGLContext::currentContext()` 是否有效；若上下文已销毁，跳过 `glDeleteBuffers`（GL 驱动会在上下文销毁时自动回收资源）
- 支持顶点缓冲（`GL_ARRAY_BUFFER`）和索引缓冲（`GL_ELEMENT_ARRAY_BUFFER`）
- 支持动态更新（`update()` 方法，使用 `glBufferSubData`）
- **公共 API 不暴露 GL 类型**：使用 `unsigned int` 代替 `GLuint`，`int` 代替 `GLenum`/`GLint`

#### API 设计

```cpp
namespace Qwt3D {

/// @brief Buffer usage hint
enum class BufferUsage {
    Static,   ///< GL_STATIC_DRAW — data set once, used many times
    Dynamic,  ///< GL_DYNAMIC_DRAW — data changed frequently
    Stream    ///< GL_STREAM_DRAW — data changed every frame
};

/// @brief Buffer target
enum class BufferTarget {
    Vertex,   ///< GL_ARRAY_BUFFER
    Index     ///< GL_ELEMENT_ARRAY_BUFFER
};

/// @brief RAII wrapper for OpenGL Vertex Buffer Object (VBO)
class QWT3D_EXPORT GLBuffer {
public:
    explicit GLBuffer(BufferTarget target, BufferUsage usage = BufferUsage::Static);
    ~GLBuffer();

    GLBuffer(const GLBuffer&) = delete;
    GLBuffer& operator=(const GLBuffer&) = delete;
    GLBuffer(GLBuffer&& other) noexcept;
    GLBuffer& operator=(GLBuffer&& other) noexcept;

    /// @brief Upload data to the buffer
    /// @param data  Pointer to the data
    /// @param sizeBytes  Data size in bytes
    void upload(const void* data, size_t sizeBytes);

    /// @brief Update a portion of the buffer (requires prior upload)
    void update(size_t offsetBytes, const void* data, size_t sizeBytes);

    /// @brief Bind the buffer to its target
    void bind();

    /// @brief Unbind the buffer
    void unbind();

    /// @brief Returns the OpenGL buffer name (ID)
    unsigned int id() const;

    /// @brief Returns the buffer size in bytes
    size_t size() const;

    /// @brief Returns true if the buffer was successfully created
    bool isValid() const;

    /// @brief Explicitly release GL resources (call before context destruction if needed)
    void release();

private:
    QWT_DECLARE_PRIVATE(GLBuffer)
};

/// @brief RAII wrapper for OpenGL Vertex Array Object (VAO)
class QWT3D_EXPORT GLVertexArray {
public:
    GLVertexArray();
    ~GLVertexArray();

    GLVertexArray(const GLVertexArray&) = delete;
    GLVertexArray& operator=(const GLVertexArray&) = delete;
    GLVertexArray(GLVertexArray&& other) noexcept;
    GLVertexArray& operator=(GLVertexArray&& other) noexcept;

    void bind();
    void unbind();
    unsigned int id() const;
    bool isValid() const;
    void release();

private:
    QWT_DECLARE_PRIVATE(GLVertexArray)
};

}  // namespace Qwt3D
```

#### 析构安全实现

```cpp
GLBuffer::PrivateData::~PrivateData()
{
    release();
}

void GLBuffer::PrivateData::release()
{
    if (m_released || m_bufferId == 0)
        return;

    // Check if GL context is still alive
    auto* ctx = QOpenGLContext::currentContext();
    if (ctx && m_gl) {
        m_gl->glDeleteBuffers(1, &m_bufferId);
    }
    // If context is gone, GL driver will reclaim resources automatically

    m_bufferId = 0;
    m_released = true;
}
```

### 3. ShaderProgram — 着色器管理

#### 设计要点

- 封装顶点着色器 + 片段着色器的编译、链接
- 统一变量（uniform）缓存：首次获取后缓存 location，避免重复查询
- 着色器源码以内嵌字符串形式提供（`qwt3d_shaders.h`），不依赖外部文件
- 编译/链接失败时输出错误日志到 `qWarning()` 并返回 false
- 提供 `#version 330 core` 和 `#version 120` 两套着色器，运行时根据 GLSL 版本选择
- **公共 API 不暴露 GL 类型**

#### API 设计

```cpp
namespace Qwt3D {

/// @brief OpenGL shader program wrapper with uniform caching
class QWT3D_EXPORT ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    /// @brief Compile and link from source strings
    /// @return true on success, false on failure (error logged via qWarning)
    bool create(const char* vertexSource, const char* fragmentSource);

    /// @brief Create using built-in shaders, auto-selecting GLSL version
    /// @param shaderType Built-in shader identifier
    bool createFromBuiltin(int shaderType);

    void bind();
    void release();

    /// @brief Set uniform values (cached location lookup)
    void setUniform(const char* name, int value);
    void setUniform(const char* name, float value);
    void setUniform(const char* name, double value);
    void setUniform(const char* name, float x, float y, float z);
    void setUniform(const char* name, const float matrix[16]);

    /// @brief Set uniform from a QMatrix4x4 (convenience)
    void setUniform(const char* name, const QMatrix4x4& matrix);

    int attributeLocation(const char* name) const;
    unsigned int id() const;
    bool isValid() const;

    /// @brief Explicitly release GL resources
    void release();

private:
    QWT_DECLARE_PRIVATE(ShaderProgram)
};

/// @brief Built-in shader type identifiers
enum class BuiltinShader {
    Surface,    ///< Lit surface rendering (position, normal, color)
    Point,      ///< Point sprite rendering (gl_PointSize, circle discard)
    Line,       ///< Simple line rendering (color passthrough)
    Instanced,  ///< Instanced rendering with per-instance transform
    Volume      ///< Volume ray casting (see plan10)
};

}  // namespace Qwt3D
```

### 4. 内置着色器（qwt3d_shaders.h）

每种内置着色器提供两套源码：`Core`（`#version 330 core`）和 `Compat`（`#version 120`）。

`ShaderProgram::createFromBuiltin()` 根据 `RenderContext::glslVersion()` 自动选择。

#### 4.1 表面着色器（BuiltinShader::Surface）

```glsl
// === Core Profile (#version 330 core) ===
// kSurfaceVertexShaderCore
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;
out vec4 vColor;
out vec3 vEyePos;

void main() {
    vec4 eyePos = uModelView * vec4(aPosition, 1.0);
    vEyePos = eyePos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    gl_Position = uProjection * eyePos;
}

// kSurfaceFragmentShaderCore
#version 330 core
in vec3 vNormal;
in vec4 vColor;
in vec3 vEyePos;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uShininess;

out vec4 fragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vEyePos);
    vec3 V = normalize(-vEyePos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), uShininess);

    vec3 color = uAmbient * vColor.rgb + diff * vColor.rgb * uLightColor
               + spec * uLightColor;
    fragColor = vec4(color, vColor.a);
}
```

```glsl
// === Compatibility Profile (#version 120) ===
// kSurfaceVertexShaderCompat
#version 120
attribute vec3 aPosition;
attribute vec3 aNormal;
attribute vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

varying vec3 vNormal;
varying vec4 vColor;
varying vec3 vEyePos;

void main() {
    vec4 eyePos = uModelView * vec4(aPosition, 1.0);
    vEyePos = eyePos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vColor = aColor;
    gl_Position = uProjection * eyePos;
}

// kSurfaceFragmentShaderCompat
#version 120
varying vec3 vNormal;
varying vec4 vColor;
varying vec3 vEyePos;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform float uShininess;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vEyePos);
    vec3 V = normalize(-vEyePos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), uShininess);

    vec3 color = uAmbient * vColor.rgb + diff * vColor.rgb * uLightColor
               + spec * uLightColor;
    gl_FragColor = vec4(color, vColor.a);
}
```

#### 4.2 点着色器（BuiltinShader::Point）

Core Profile 和 Compat 版本结构类似，关键差异：
- Core：`gl_PointSize` 输出，`gl_PointCoord` 圆形裁剪，`out vec4 fragColor`
- Compat：`gl_PointSize` 输出，`gl_PointCoord` 圆形裁剪，`gl_FragColor`

#### 4.3 线着色器（BuiltinShader::Line）

最简单的颜色直通着色器，Core/Compat 差异仅 in/out vs varying 和 gl_FragColor。

#### 4.4 实例化着色器（BuiltinShader::Instanced）

用于散点图球体/立方体 glyph、向量场箭头的实例化渲染。
- 顶点属性：glyph 顶点 (location 0) + glyph 法线 (location 1)
- 实例属性：位置 (location 2) + 方向 (location 3) + 颜色 (location 4) + 缩放 (location 5)
- 在顶点着色器中构建旋转矩阵，对 glyph 几何体进行变换

**注意**：实例化渲染需要 `glVertexAttribDivisor`（OpenGL 3.3 core 或 `GL_ARB_instanced_arrays` 扩展）。`RenderContext` 负责检测支持情况。Compat 回退方案为逐实例绘制（不使用 instancing，多次 bind+draw）。

#### 4.5 体积光线投射着色器（BuiltinShader::Volume）

详见 plan10。仅 Core Profile 版本（体积渲染无法用 `#version 120` 实现）。

### 5. RenderContext — 渲染上下文检测

#### 设计要点

- 在 `Plot3D::initializeGL()` 中调用 `detect()` 进行一次检测
- **支持重复检测**：如果 GL 上下文发生变化（如 MultiPlot 场景），`detect()` 会重新检测
- 检测项：Core Profile / VBO / VAO / 着色器 / GLSL 版本 / 实例化渲染支持 / 3D 纹理
- 新绘图类型根据 `canUseModernPipeline()` 选择渲染路径

#### API 设计

```cpp
namespace Qwt3D {

/// @brief OpenGL rendering context capability detector
class QWT3D_EXPORT RenderContext {
public:
    /// @brief Get the singleton instance
    static RenderContext& instance();

    /// @brief Detect capabilities from the current GL context (safe to call multiple times)
    void detect();

    /// @brief Returns true if Core Profile is active
    bool isCoreProfile() const;

    /// @brief Returns true if VBO/VAO are available
    bool hasVBO() const;

    /// @brief Returns true if shader programs are available
    bool hasShaders() const;

    /// @brief Returns the OpenGL major version
    int glVersionMajor() const;

    /// @brief Returns the GLSL version as a float (e.g., 3.30, 1.20)
    float glslVersion() const;

    /// @brief Returns true if instanced rendering (glVertexAttribDivisor) is available
    bool hasInstancing() const;

    /// @brief Returns true if 3D textures (GL_TEXTURE_3D) are available
    bool has3DTexture() const;

    /// @brief Returns true if modern rendering pipeline (VBO+shader) can be used
    bool canUseModernPipeline() const;

    /// @brief Returns true if the context has changed since last detect() call
    /// @details Used by MultiPlot to re-detect when switching contexts
    bool hasChanged() const;

private:
    RenderContext();
    QWT_DECLARE_PRIVATE(RenderContext)
};

}  // namespace Qwt3D
```

### 6. Field3D — 标量场/向量场统一基类

**设计动机**：plan07（流线）、plan08（等高线/切片）、plan09（等值面）、plan10（体积渲染）都需要从 3D 空间采样场数据。提取公共基类避免接口重复，并提供统一的生命周期管理。

```cpp
namespace Qwt3D {

/// @brief Abstract base for 3D field data (scalar or vector)
/// @details Provides domain bounds and containment check.
///          Use std::shared_ptr<Field3D> for safe lifetime management.
class QWT3D_EXPORT Field3D
{
public:
    virtual ~Field3D() = default;

    /// @brief Returns the domain bounding box
    virtual ParallelEpiped domain() const = 0;

    /// @brief Returns true if position is inside the domain
    virtual bool contains(Triple const& pos) const;

    /// @brief Returns the value range [min, max]
    virtual QwtInterval range() const = 0;
};

/// @brief Abstract interface for 3D scalar field sampling
class QWT3D_EXPORT ScalarField : public Field3D
{
public:
    /// @brief Sample scalar value at position (x, y, z)
    /// @return Scalar value; returns 0 outside domain
    virtual double sample(double x, double y, double z) const = 0;
};

/// @brief Abstract interface for 3D vector field sampling
class QWT3D_EXPORT VectorField : public Field3D
{
public:
    /// @brief Sample vector at position (x, y, z)
    /// @return Vector (vx, vy, vz); returns zero vector outside domain
    virtual Triple sample(double x, double y, double z) const = 0;
};

/// @brief Scalar field from a regular 3D grid with trilinear interpolation
class QWT3D_EXPORT GridScalarField : public ScalarField
{
public:
    /// @param origin  Grid origin (min x, y, z)
    /// @param spacing Grid spacing (dx, dy, dz)
    /// @param nx, ny, nz Grid dimensions
    /// @param data    Scalar values, size = nx*ny*nz, x varies fastest
    GridScalarField(Triple origin, Triple spacing,
                    unsigned nx, unsigned ny, unsigned nz,
                    std::vector<double> const& data);

    double sample(double x, double y, double z) const override;
    ParallelEpiped domain() const override;
    QwtInterval range() const override;

    /// @brief Direct access to raw grid data
    std::vector<double> const& rawData() const;
    unsigned dimX() const;
    unsigned dimY() const;
    unsigned dimZ() const;
    Triple origin() const;
    Triple spacing() const;

private:
    QWT_DECLARE_PRIVATE(GridScalarField)
};

/// @brief Scalar field from an analytical function
class QWT3D_EXPORT FunctionScalarField : public ScalarField
{
public:
    using ScalarFunc = std::function<double(double x, double y, double z)>;

    FunctionScalarField(ScalarFunc func, ParallelEpiped domain, QwtInterval range);
    double sample(double x, double y, double z) const override;
    ParallelEpiped domain() const override;
    QwtInterval range() const override;

private:
    QWT_DECLARE_PRIVATE(FunctionScalarField)
};

/// @brief Vector field from a regular 3D grid with trilinear interpolation
class QWT3D_EXPORT GridVectorField : public VectorField
{
public:
    GridVectorField(Triple origin, Triple spacing,
                    unsigned nx, unsigned ny, unsigned nz,
                    TripleField const& data);

    Triple sample(double x, double y, double z) const override;
    ParallelEpiped domain() const override;
    QwtInterval range() const override;

private:
    QWT_DECLARE_PRIVATE(GridVectorField)
};

/// @brief Vector field from an analytical function
class QWT3D_EXPORT FunctionVectorField : public VectorField
{
public:
    using VectorFunc = std::function<Triple(double x, double y, double z)>;

    FunctionVectorField(VectorFunc func, ParallelEpiped domain);
    Triple sample(double x, double y, double z) const override;
    ParallelEpiped domain() const override;
    QwtInterval range() const override;

private:
    QWT_DECLARE_PRIVATE(FunctionVectorField)
};

}  // namespace Qwt3D
```

**生命周期管理约定**：所有使用 `ScalarField`/`VectorField` 的绘图类型（plan07-10）应通过 `std::shared_ptr<Field3D>` 持有引用，确保场数据在 `createData()` 执行期间有效：

```cpp
// In StreamlinePlot, ContourSlicePlot, IsosurfacePlot, VolumePlot:
void setScalarField(std::shared_ptr<ScalarField> field);
void setVectorField(std::shared_ptr<VectorField> field);
```

### 7. TriangleMesh — 通用三角网格

**设计动机**：Marching Cubes（plan09）、Delaunay（plan11）、管状几何体（plan05/07）都产生三角网格。提取为通用结构体，避免循环依赖（如 `IsosurfacePlot::IsosurfaceMesh` 被 `MarchingCubes` 引用）。

```cpp
namespace Qwt3D {

/// @brief Generic triangle mesh (positions, normals, optional colors, indexed)
struct QWT3D_EXPORT TriangleMesh {
    TripleField vertices;                     ///< Vertex positions
    TripleField normals;                      ///< Per-vertex normals (may be empty)
    std::vector<RGBA> colors;                 ///< Per-vertex colors (may be empty)
    std::vector<unsigned int> indices;        ///< Triangle indices (size % 3 == 0)

    /// @brief Returns number of triangles
    size_t triangleCount() const { return indices.size() / 3; }

    /// @brief Returns number of vertices
    size_t vertexCount() const { return vertices.size(); }

    /// @brief Returns true if the mesh has no data
    bool empty() const { return vertices.empty(); }

    /// @brief Clear all data
    void clear();

    /// @brief Compute flat normals (one normal per triangle, duplicated to vertices)
    void computeFlatNormals();

    /// @brief Compute smooth normals (area-weighted average of adjacent face normals)
    void computeSmoothNormals();
};

}  // namespace Qwt3D
```

### 8. TubeBuilder — 管状几何构建器

**设计动机**：plan05（线图 Tube 模式）和 plan07（流线 Tube 模式）都需要沿折线构建圆柱带几何体。算法完全相同，提取为公共工具类。

```cpp
namespace Qwt3D {

/// @brief Builds cylindrical tube geometry along a polyline path
class QWT3D_EXPORT TubeBuilder {
public:
    /// @brief Configuration for tube geometry
    struct Config {
        double radius = 0.05;       ///< Tube radius
        unsigned segments = 12;     ///< Circumference segments (quality)
        bool capEnds = true;        ///< Generate end caps
    };

    /// @brief Build tube geometry along a polyline
    /// @param path     Ordered vertices of the polyline
    /// @param config   Tube configuration
    /// @param mesh     [out] Output triangle mesh
    /// @param perVertexColor Optional color function evaluated at each path vertex
    static void build(TripleField const& path,
                      Config const& config,
                      TriangleMesh& mesh,
                      std::function<RGBA(Triple const&)> perVertexColor = nullptr);

private:
    /// @brief Compute parallel transport frame at a path vertex
    /// @details Uses the parallel transport frame (not Frenet-Serret) to avoid
    ///          discontinuities at inflection points
    static void computeFrame(Triple const& tangent,
                             Triple const& prevNormal,
                             Triple& normal,
                             Triple& binormal);
};

}  // namespace Qwt3D
```

**算法说明**：
- 使用 **parallel transport frame**（而非 Frenet-Serret 标架），避免在拐点处产生法线跳变
- 在每个路径顶点处，沿圆周生成 `segments` 个顶点
- 相邻顶点环之间用三角形带连接
- 端点法线由相邻两点的差分近似
- 如果 `perVertexColor` 非空，每个环上的顶点颜色由该函数决定；否则使用默认色

**使用示例（plan05 中）**：

```cpp
void LinePlot3D::buildTubeGeometry()
{
    TubeBuilder::Config config;
    config.radius = d->m_tubeRadius;
    config.segments = d->m_tubeQuality;

    for (auto const& seg : d->m_series) {
        TriangleMesh tube;
        auto colorFunc = [this](Triple const& v) -> RGBA {
            return (*dataColor())(v);
        };
        TubeBuilder::build(seg, config, tube, colorFunc);

        // Merge tube mesh into GraphData VBO
        appendMeshToGraphData(tube);
    }
}
```

### 9. 集成到 Plot3D

在 `Plot3D::initializeGL()` 中调用 `RenderContext::instance().detect()` 进行一次检测。

新绘图类型在 `createData()` 中根据 `RenderContext::instance().canUseModernPipeline()` 选择渲染路径：

```cpp
void ScatterPlot::createData()
{
    if (RenderContext::instance().canUseModernPipeline()) {
        createDataModern();  // VBO + shader
    } else {
        createDataLegacy();  // immediate mode fallback
    }
}
```

### 10. CMake 集成

```cmake
set(QWTPLOT3D_HEADER_PLOT_3D
    # ... existing headers ...
    qwt3d_glbuffer.h
    qwt3d_shader.h
    qwt3d_shaders.h
    qwt3d_rendercontext.h
    qwt3d_field3d.h
    qwt3d_tubebuilder.h
)

set(QWTPLOT3D_SOURCE_PLOT_3D
    # ... existing sources ...
    qwt3d_glbuffer.cpp
    qwt3d_shader.cpp
    qwt3d_rendercontext.cpp
    qwt3d_field3d.cpp
    qwt3d_tubebuilder.cpp
)
```

---

## 验证方式

1. **编译验证**：`.\build.ps1 build` 成功通过，无编译错误
2. **单元测试**：编写一个最小化测试用例，验证 GLBuffer 和 ShaderProgram 能正确创建、上传数据、编译链接
3. **上下文安全验证**：在 GL 上下文销毁后析构 GLBuffer/GLVertexArray/ShaderProgram，验证不崩溃（`release()` 被正确跳过）
4. **着色器版本验证**：在 Core Profile 和兼容上下文下分别验证 `createFromBuiltin()` 选择正确的着色器版本
5. **Field3D 验证**：编写测试用例验证 `GridScalarField` 和 `FunctionScalarField` 的采样正确性（包括边界外返回 0、三线性插值精度）
6. **TubeBuilder 验证**：构建一条直线路径的管状几何体，验证截面为正圆、法线朝外
7. **TriangleMesh 验证**：验证 `computeSmoothNormals()` 在球体网格上产生正确的法线方向
8. **回退验证**：在兼容上下文（非 Core Profile）下运行，验证自动回退到固定管线

---

## 风险与注意事项

1. **OpenGL 函数指针**：必须通过 `QOpenGLExtraFunctions` 获取，不能直接调用 `glGenBuffers` 等。`QOpenGLExtraFunctions` 在 Qt 5.8+ / Qt 6 中可用，需确认项目的最低 Qt 版本要求。

2. **VBO 生命周期**：GL 资源必须在拥有当前 GL 上下文的线程中创建和销毁。析构安全已通过 `QOpenGLContext::currentContext()` 检查解决。如果需要在非 GL 线程销毁对象，应先调用 `release()` 在 GL 线程中释放资源，再让对象在非 GL 线程中被析构。

3. **着色器版本兼容**：`#version 120` 对应 OpenGL 2.1 / GLSL 1.20，不支持 `layout(location)` 语法。Compat 着色器使用 `attribute`/`varying` 关键字，且属性位置需要通过 `glBindAttribLocation()` 在链接前绑定，或使用 `glGetAttribLocation()` 在链接后查询。

4. **与 display list 共存**：现有代码使用 `glGenLists`/`glCallList`。新基础设施不应干扰现有显示列表机制。新绘图类型可以完全不用显示列表，直接在 `createData()` 中构建 VBO 并在 `paintGL()` 中绘制。

5. **实例化渲染兼容性**：`glVertexAttribDivisor` 在 OpenGL 3.3 core 中可用，在 3.1/3.2 中需要 `GL_ARB_instanced_arrays` 扩展。`RenderContext::hasInstancing()` 应检查两者。不支持时的回退方案：在 CPU 端对每个实例执行变换后合并到统一的 VBO 中（性能差但兼容）。

6. **性能基准**：建立简单基准测试（如渲染 100K 点的散点图），对比 immediate mode vs VBO 渲染时间，确保性能提升符合预期。参考硬件：Intel UHD 630（集成 GPU）或 NVIDIA GTX 1060（独立 GPU）。

7. **`Field3D::range()` 的计算成本**：对于 `FunctionScalarField`，`range()` 需要采样整个域来确定范围，可能很昂贵。建议在构造时预计算并缓存，或使用用户提供的估计范围。
