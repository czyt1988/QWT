请先阅读 `README.md` 以了解全局规范。

# plan02 — GraphPlot 基类实现

## 目标

实现 `GraphPlot` 类（当前仅有头文件声明，构造函数无 .cpp 定义，实例化会导致链接错误）。GraphPlot 作为 `Plot3D` 和具体图基绘图类型（散点图、柱状图、线图、向量场、流线）之间的中间基类，提供图元数据的存储、管理和 VBO 渲染基础设施集成。

## 前置依赖

- **plan00**（渲染基础设施）：GraphPlot 使用 `GLBuffer`、`GLVertexArray`、`ShaderProgram` 进行 VBO 渲染

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_graphplot.cpp` | GraphPlot 类实现 |
| `src/plot3d/qwt3d_graphplot_p.h` | GraphPlot PIMPL 私有数据头文件 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/qwt3d_graphplot.h` | 扩展 public/protected API 声明，添加 `GraphData` 定义 |
| `src/plot3d/CMakeLists.txt` | 将 `qwt3d_graphplot.cpp` 加入 `QWTPLOT3D_SOURCE_PLOT_3D` |

---

## 技术方案

### 1. 设计定位

GraphPlot 是图基绘图的中间基类，处理"离散图元"（点、线、柱、箭头）而非"连续表面"（SurfacePlot 处理的网格曲面）。与 SurfacePlot 的区别：

| 特性 | SurfacePlot | GraphPlot |
|------|-------------|-----------|
| 数据拓扑 | 规则网格 / 多边形网格 | 离散图元列表 |
| 数据类型 | GridData / CellData | GraphData（新） |
| 渲染方式 | GL_TRIANGLE_STRIP / GL_POLYGON | GL_POINTS / GL_LINES / GL_TRIANGLES |
| 图元间关系 | 相邻顶点构成面 | 图元相互独立 |
| 典型子类 | Function, ParametricSurface | ScatterPlot, Bar3D, LinePlot3D, ... |

### 2. 数据模型 — GraphData

GraphData 定义在 `qwt3d_graphplot.h` 中（而非 `qwt3d_types.h`，避免 types 膨胀）。

```cpp
namespace Qwt3D {

/// @brief Graph primitive type
enum class PrimitiveType {
    Points,   ///< GL_POINTS — scatter plot
    Lines,    ///< GL_LINES / GL_LINE_STRIP — line plot
    Bars,     ///< GL_TRIANGLES — bar chart (box geometry)
    Arrows    ///< GL_TRIANGLES — vector field (arrow geometry)
};

/// @brief Data storage for graph-based plots
class QWT3D_EXPORT GraphData : public Data
{
public:
    GraphData();
    ~GraphData() override;

    void clear() override;
    bool empty() const override;

    /// @brief Vertex positions (x,y,z for each vertex)
    TripleField vertices;

    /// @brief Per-vertex colors (optional; if empty, data color functor is used)
    std::vector<RGBA> colors;

    /// @brief Per-vertex normals (optional, for lighting on Bars/Arrows)
    TripleField normals;

    /// @brief Index array (optional, for indexed drawing)
    std::vector<unsigned int> indices;

    /// @brief Primitive type for rendering
    PrimitiveType primitiveType = PrimitiveType::Points;

    /// @brief Per-vertex auxiliary scalar data (e.g., for color/size mapping)
    std::vector<double> scalars;

    /// @brief Compute the axis-aligned bounding box of vertices
    /// @return Bounding box; returns unit box if vertices is empty
    ParallelEpiped computeBounds() const;
};

}  // namespace Qwt3D
```

### 3. GraphPlot 类设计

#### 头文件扩展（`qwt3d_graphplot.h`）

```cpp
namespace Qwt3D {

class GraphData;
class ShaderProgram;
class GLBuffer;
class GLVertexArray;
class TriangleMesh;

class QWT3D_EXPORT GraphPlot : public Plot3D
{
    Q_OBJECT

public:
    explicit GraphPlot(QWidget* parent = nullptr);
    ~GraphPlot() override;

    /// @brief Load point data (scatter plot)
    /// @param positions Vertex positions
    /// @param scalars Optional scalar values for color mapping
    void setData(TripleField const& positions,
                 std::vector<double> const& scalars = {});

    /// @brief Load line data (multiple polyline segments)
    /// @param segments Vector of polylines
    void setData(std::vector<TripleField> const& segments);

    /// @brief Load indexed geometry data
    /// @param vertices Vertex positions
    /// @param indices  Index array defining primitives
    /// @param type     Primitive type
    void setData(TripleField const& vertices,
                 std::vector<unsigned int> const& indices,
                 PrimitiveType type);

    /// @brief Returns the current primitive type
    PrimitiveType primitiveType() const;

    /// @brief Set the primitive type
    void setPrimitiveType(PrimitiveType type);

    /// @brief Returns number of vertices
    size_t vertexCount() const;

    /// @brief Returns true if data has been loaded
    bool hasData() const;

Q_SIGNALS:
    /// @brief Emitted when data changes (used by MultiPlot for update propagation)
    void dataChanged();

protected:
    QWT_DECLARE_PRIVATE(GraphPlot)

    void calculateHull() override;
    // createData() remains pure virtual — subclasses must implement

    /// @brief Access the GraphData (for subclasses)
    GraphData& graphData();
    const GraphData& graphData() const;

    /// @brief Rebuild VBO from GraphData (called by subclasses in createData())
    void rebuildVBO();

    /// @brief Render using VBO + shader (called by subclasses in paintGL)
    virtual void renderVBO();

    /// @brief Render using immediate mode (fallback for non-Core Profile)
    virtual void renderImmediate();

    /// @brief Append a TriangleMesh into GraphData (for Bar3D, VectorFieldPlot etc.)
    void appendMesh(TriangleMesh const& mesh);

    /// @brief Select and compile the appropriate built-in shader for current PrimitiveType
    void ensureShader();
};

}  // namespace Qwt3D
```

**关键设计决策**：
- `renderVBO()` 和 `renderImmediate()` 声明为 `virtual`，子类可 override
- `graphData()` 返回引用而非裸指针，避免调用者误删
- 新增 `dataChanged()` 信号，供 plan12（MultiPlot）监听子绘图数据更新
- 新增 `appendMesh()` 方法，供 Bar3D/VectorFieldPlot 等将 `TriangleMesh` 合并到 GraphData

#### PIMPL 私有数据（`qwt3d_graphplot_p.h`）

```cpp
class GraphPlot::PrivateData
{
    QWT_DECLARE_PUBLIC(GraphPlot)

public:
    PrivateData(GraphPlot* q);

    GraphData m_graphData;

    // VBO infrastructure (lazily initialized on first GL call)
    std::unique_ptr<GLBuffer> m_vertexBuffer;
    std::unique_ptr<GLBuffer> m_colorBuffer;
    std::unique_ptr<GLBuffer> m_normalBuffer;
    std::unique_ptr<GLBuffer> m_indexBuffer;
    std::unique_ptr<GLVertexArray> m_vao;
    std::unique_ptr<ShaderProgram> m_shader;

    bool m_vboDirty = true;
    bool m_shaderReady = false;
};
```

### 4. 核心方法实现

#### 4.1 构造函数

```cpp
GraphPlot::GraphPlot(QWidget* parent)
    : Plot3D(parent)
    , QWT_PIMPL_CONSTRUCT
{
    setActualData(new GraphData());
}
```

#### 4.2 `calculateHull()` — 安全包围盒计算

**关键修正**：原方案使用 `(&maxV.x)[i]` 指针算术访问 Triple 成员，这是 C++ 未定义行为。改为逐成员访问。

```cpp
void GraphPlot::calculateHull()
{
    QWT_D(d);
    ParallelEpiped hull = d->m_graphData.computeBounds();
    setHull(hull);
}
```

`GraphData::computeBounds()` 实现：

```cpp
ParallelEpiped GraphData::computeBounds() const
{
    if (vertices.empty())
        return ParallelEpiped(Triple(0, 0, 0), Triple(1, 1, 1));

    Triple minV = vertices[0];
    Triple maxV = vertices[0];

    for (auto const& v : qwt_as_const(vertices)) {
        minV.x = std::min(minV.x, v.x);
        minV.y = std::min(minV.y, v.y);
        minV.z = std::min(minV.z, v.z);
        maxV.x = std::max(maxV.x, v.x);
        maxV.y = std::max(maxV.y, v.y);
        maxV.z = std::max(maxV.z, v.z);
    }

    // Avoid zero-size hull (逐成员安全访问，无 UB)
    constexpr double eps = 1e-10;
    if (maxV.x - minV.x < eps) maxV.x += 1.0;
    if (maxV.y - minV.y < eps) maxV.y += 1.0;
    if (maxV.z - minV.z < eps) maxV.z += 1.0;

    return ParallelEpiped(minV, maxV);
}
```

#### 4.3 `setData(TripleField const&, std::vector<double> const&)`

点数据加载（散点图使用）：

```cpp
void GraphPlot::setData(TripleField const& positions,
                         std::vector<double> const& scalars)
{
    QWT_D(d);
    d->m_graphData.clear();
    d->m_graphData.vertices = positions;
    d->m_graphData.scalars = scalars;
    d->m_graphData.primitiveType = PrimitiveType::Points;
    d->m_vboDirty = true;

    calculateHull();
    createCoordinateSystem();
    updateData();
    Q_EMIT dataChanged();
}
```

#### 4.4 `setData(std::vector<TripleField> const&)`

线数据加载（线图使用）：

```cpp
void GraphPlot::setData(std::vector<TripleField> const& segments)
{
    QWT_D(d);
    d->m_graphData.clear();
    d->m_graphData.primitiveType = PrimitiveType::Lines;

    for (auto const& seg : segments) {
        auto baseIndex = static_cast<unsigned int>(d->m_graphData.vertices.size());
        for (auto const& v : seg)
            d->m_graphData.vertices.push_back(v);

        // Build line indices: consecutive pairs for GL_LINES
        for (size_t i = 1; i < seg.size(); ++i) {
            d->m_graphData.indices.push_back(baseIndex + static_cast<unsigned int>(i) - 1);
            d->m_graphData.indices.push_back(baseIndex + static_cast<unsigned int>(i));
        }
    }

    d->m_vboDirty = true;
    calculateHull();
    createCoordinateSystem();
    updateData();
    Q_EMIT dataChanged();
}
```

#### 4.5 `rebuildVBO()`

将 GraphData 上传到 VBO：

```cpp
void GraphPlot::rebuildVBO()
{
    QWT_D(d);
    if (!RenderContext::instance().canUseModernPipeline())
        return;

    ensureShader();

    auto const& data = d->m_graphData;

    // Lazy init VBO objects
    if (!d->m_vertexBuffer) {
        d->m_vertexBuffer = qwt_make_unique<GLBuffer>(BufferTarget::Vertex, BufferUsage::Dynamic);
        d->m_colorBuffer = qwt_make_unique<GLBuffer>(BufferTarget::Vertex, BufferUsage::Dynamic);
        d->m_vao = qwt_make_unique<GLVertexArray>();
    }

    // Upload vertex positions
    d->m_vertexBuffer->bind();
    d->m_vertexBuffer->upload(data.vertices.data(),
                               data.vertices.size() * sizeof(Triple));

    // Compute and upload colors
    std::vector<RGBA> colors;
    if (data.colors.empty()) {
        const Color* col = dataColor();
        colors.reserve(data.vertices.size());
        if (!data.scalars.empty()) {
            // Scalar-based color mapping
            double sMin = *std::min_element(data.scalars.begin(), data.scalars.end());
            double sMax = *std::max_element(data.scalars.begin(), data.scalars.end());
            for (size_t i = 0; i < data.vertices.size(); ++i) {
                double normalized = (sMax > sMin) ? (data.scalars[i] - sMin) / (sMax - sMin) : 0.5;
                colors.push_back((*col)(normalized, 0, 0));
            }
        } else {
            for (auto const& v : data.vertices)
                colors.push_back((*col)(v));
        }
    } else {
        colors = data.colors;
    }
    d->m_colorBuffer->bind();
    d->m_colorBuffer->upload(colors.data(), colors.size() * sizeof(RGBA));

    // Upload normals if present
    if (!data.normals.empty()) {
        if (!d->m_normalBuffer)
            d->m_normalBuffer = qwt_make_unique<GLBuffer>(BufferTarget::Vertex, BufferUsage::Dynamic);
        d->m_normalBuffer->bind();
        d->m_normalBuffer->upload(data.normals.data(),
                                    data.normals.size() * sizeof(Triple));
    }

    // Setup VAO
    d->m_vao->bind();
    d->m_vertexBuffer->bind();
    glEnableVertexAttribArray(0);  // position
    glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, sizeof(Triple), nullptr);
    d->m_colorBuffer->bind();
    glEnableVertexAttribArray(2);  // color
    glVertexAttribPointer(2, 4, GL_DOUBLE, GL_FALSE, sizeof(RGBA), nullptr);
    if (d->m_normalBuffer && !data.normals.empty()) {
        d->m_normalBuffer->bind();
        glEnableVertexAttribArray(1);  // normal
        glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, sizeof(Triple), nullptr);
    }

    // Upload index buffer if present
    if (!data.indices.empty()) {
        if (!d->m_indexBuffer)
            d->m_indexBuffer = qwt_make_unique<GLBuffer>(BufferTarget::Index, BufferUsage::Dynamic);
        d->m_indexBuffer->bind();
        d->m_indexBuffer->upload(data.indices.data(),
                                   data.indices.size() * sizeof(unsigned int));
    }

    d->m_vao->unbind();
    d->m_vboDirty = false;
}
```

#### 4.6 `renderVBO()`

```cpp
void GraphPlot::renderVBO()
{
    QWT_D(d);
    if (d->m_vboDirty)
        rebuildVBO();

    if (!d->m_shader || !d->m_shader->isValid())
        return;

    d->m_shader->bind();

    // Set model-view-projection matrices
    GLdouble modelMatrix[16], projMatrix[16];
    GLint viewport[4];
    getMatrices(modelMatrix, projMatrix, viewport);

    float modelView[16], projection[16];
    for (int i = 0; i < 16; ++i) {
        modelView[i] = static_cast<float>(modelMatrix[i]);
        projection[i] = static_cast<float>(projMatrix[i]);
    }
    d->m_shader->setUniform("uModelView", modelView);
    d->m_shader->setUniform("uProjection", projection);

    d->m_vao->bind();

    auto const& data = d->m_graphData;
    unsigned int mode = GL_POINTS;
    switch (data.primitiveType) {
        case PrimitiveType::Points: mode = GL_POINTS; break;
        case PrimitiveType::Lines:  mode = GL_LINES; break;
        case PrimitiveType::Bars:   mode = GL_TRIANGLES; break;
        case PrimitiveType::Arrows: mode = GL_TRIANGLES; break;
    }

    if (!data.indices.empty()) {
        glDrawElements(mode, static_cast<int>(data.indices.size()),
                       GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(mode, 0, static_cast<int>(data.vertices.size()));
    }

    d->m_vao->unbind();
    d->m_shader->release();
}
```

#### 4.7 `renderImmediate()`

固定管线回退渲染。注意多系列线数据使用 `GL_LINES`（非 `GL_LINE_STRIP`），避免系列间连线。

```cpp
void GraphPlot::renderImmediate()
{
    QWT_DC(d);
    auto const& data = d->m_graphData;
    const Color* col = dataColor();

    unsigned int mode = GL_POINTS;
    switch (data.primitiveType) {
        case PrimitiveType::Points: mode = GL_POINTS; break;
        case PrimitiveType::Lines:  mode = GL_LINES; break;
        case PrimitiveType::Bars:   mode = GL_TRIANGLES; break;
        case PrimitiveType::Arrows: mode = GL_TRIANGLES; break;
    }

    glBegin(mode);
    if (!data.indices.empty()) {
        for (auto idx : data.indices) {
            Triple const& v = data.vertices[idx];
            RGBA c = data.colors.empty() ? (*col)(v) : data.colors[idx];
            if (idx < data.normals.size()) {
                Triple const& n = data.normals[idx];
                glNormal3d(n.x, n.y, n.z);
            }
            glColor4d(c.r, c.g, c.b, c.a);
            glVertex3d(v.x, v.y, v.z);
        }
    } else {
        for (size_t i = 0; i < data.vertices.size(); ++i) {
            Triple const& v = data.vertices[i];
            RGBA c = data.colors.empty() ? (*col)(v) : data.colors[i];
            if (i < data.normals.size()) {
                Triple const& n = data.normals[i];
                glNormal3d(n.x, n.y, n.z);
            }
            glColor4d(c.r, c.g, c.b, c.a);
            glVertex3d(v.x, v.y, v.z);
        }
    }
    glEnd();
}
```

#### 4.8 `appendMesh()`

将 `TriangleMesh` 合并到 GraphData，供 Bar3D/VectorFieldPlot 等使用：

```cpp
void GraphPlot::appendMesh(TriangleMesh const& mesh)
{
    QWT_D(d);
    auto baseIdx = static_cast<unsigned int>(d->m_graphData.vertices.size());

    d->m_graphData.vertices.insert(d->m_graphData.vertices.end(),
                                     mesh.vertices.begin(), mesh.vertices.end());
    if (!mesh.normals.empty())
        d->m_graphData.normals.insert(d->m_graphData.normals.end(),
                                        mesh.normals.begin(), mesh.normals.end());
    if (!mesh.colors.empty())
        d->m_graphData.colors.insert(d->m_graphData.colors.end(),
                                       mesh.colors.begin(), mesh.colors.end());
    for (auto idx : mesh.indices)
        d->m_graphData.indices.push_back(baseIdx + idx);

    d->m_vboDirty = true;
}
```

#### 4.9 `ensureShader()`

根据当前 PrimitiveType 选择合适的内置着色器：

```cpp
void GraphPlot::ensureShader()
{
    QWT_D(d);
    if (d->m_shaderReady)
        return;

    d->m_shader = qwt_make_unique<ShaderProgram>();

    BuiltinShader type;
    switch (d->m_graphData.primitiveType) {
        case PrimitiveType::Points: type = BuiltinShader::Point; break;
        case PrimitiveType::Lines:  type = BuiltinShader::Line; break;
        default:                    type = BuiltinShader::Surface; break;
    }

    if (!d->m_shader->createFromBuiltin(static_cast<int>(type))) {
        qWarning("GraphPlot: failed to compile built-in shader");
    }

    d->m_shaderReady = true;
}
```

### 5. 子类集成方式

具体绘图类型继承 GraphPlot 后，主要在 `createData()` 中决定渲染策略：

```cpp
// ScatterPlot example
void ScatterPlot::createData()
{
    if (RenderContext::instance().canUseModernPipeline()) {
        rebuildVBO();
        renderVBO();
    } else {
        renderImmediate();
    }
}
```

子类可以 override `renderImmediate()` 和 `renderVBO()` 以添加自定义渲染逻辑（如散点图的点大小 uniform、向量场的实例化渲染等）。

### 6. CMake 集成

```cmake
set(QWTPLOT3D_SOURCE_PLOT_3D
    # ... existing ...
    qwt3d_graphplot.cpp
)
```

---

## 验证方式

1. **编译验证**：`.\build.ps1 build` 成功通过
2. **链接验证**：确认 `GraphPlot` 构造函数有定义，不再产生链接错误
3. **实例化验证**：创建一个最小的 GraphPlot 子类，验证能正确实例化、加载数据、渲染
4. **VBO 验证**：在 Core Profile 上下文下验证 VBO 渲染正常
5. **回退验证**：在兼容上下文下验证 immediate mode 回退渲染正常
6. **颜色映射验证**：验证 `Color` functor 和标量映射正确应用到每个顶点
7. **信号验证**：验证 `dataChanged()` 信号在每次 `setData()` 后被发射

---

## 风险与注意事项

1. **GraphData 定义位置**：放在 `qwt3d_graphplot.h` 中以减少 `qwt3d_types.h` 的膨胀。`qwt3d_graphplot.h` 已经 include 了 `qwt3d_plot.h`（间接 include 了 `qwt3d_types.h`），可以访问 `Data` 基类。

2. **VBO 生命周期**：`GLBuffer` 等对象在 PIMPL 中使用 `unique_ptr`，析构安全性由 plan00 的上下文检查保证。

3. **数据更新性能**：每次调用 `setData()` 都会触发 `updateData()` → `createData()` → `rebuildVBO()`。对于频繁更新的场景（如动画），可在 `setData()` 中增加 `bool updateOnlyVBO = false` 参数，为 true 时只调用 `rebuildVBO()` 而不重建坐标系。

4. **与主题系统兼容**：GraphPlot 的 `rebuildVBO()` 调用 `dataColor()` 获取 `Color` functor 来生成顶点颜色，主题系统设置的颜色预设会自动生效。

5. **`createData()` 仍为纯虚**：GraphPlot 不实现 `createData()`，保持纯虚，强制子类提供具体实现。

6. **不与 SurfacePlot 混用**：GraphPlot 和 SurfacePlot 是平行的继承分支，不要让一个类同时继承两者。
