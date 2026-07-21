请先阅读 `README.md` 以了解全局规范。

# plan03 — 3D 散点图 (ScatterPlot)

## 目标

实现 3D 散点图，在三维空间中绘制离散数据点。支持每点颜色映射、大小映射、多种点形状（点/球体/立方体/自定义 glyph）。这是最基础的 3D 数据可视化类型，用于展示三维数据的分布。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- **plan02**（GraphPlot 基类）：继承 GraphPlot，使用 GraphData 数据模型

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_scatterplot.h` | ScatterPlot 类声明 |
| `src/plot3d/qwt3d_scatterplot.cpp` | ScatterPlot 类实现 |
| `src/plot3d/qwt3d_scatterplot_p.h` | PIMPL 私有数据 |
| `examples/3D/scatterplot/scatterplot.cpp` | 示例程序 |
| `examples/3D/scatterplot/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 添加新源文件 |
| `examples/CMakeLists.txt` | 注册新示例 |

---

## 技术方案

### 1. 类设计

```cpp
namespace Qwt3D {

/// @brief Point shape for scatter plot glyphs
enum class PointShape {
    Point,    ///< GL_POINTS — simple points (fastest)
    Sphere,   ///< 3D sphere geometry (instanced)
    Cube,     ///< 3D cube geometry (instanced)
    Custom    ///< User-defined glyph (via Enrichment)
};

/// @brief 3D scatter plot widget
class QWT3D_EXPORT ScatterPlot : public GraphPlot
{
    Q_OBJECT

public:
    explicit ScatterPlot(QWidget* parent = nullptr);
    ~ScatterPlot() override;

    /// @brief Set point data with optional scalar values for color mapping
    /// @param positions (x,y,z) for each point
    /// @param scalars Optional scalar per point (mapped to color via Color functor)
    void setData(TripleField const& positions,
                 std::vector<double> const& scalars = {});

    /// @brief Set point shape
    void setPointShape(PointShape shape);
    PointShape pointShape() const;

    /// @brief Set uniform point size (for Point shape, or glyph scale for others)
    void setPointSize(double size);
    double pointSize() const;

    /// @brief Enable per-point size mapping from scalar values
    /// @param minSize Minimum point size
    /// @param maxSize Maximum point size
    void setSizeMapping(double minSize, double maxSize);
    void disableSizeMapping();
    bool sizeMappingEnabled() const;

protected:
    QWT_DECLARE_PRIVATE(ScatterPlot)

    void createData() override;

    /// @brief Render points using VBO (modern pipeline)
    void renderVBO() override;

    /// @brief Render points using immediate mode (fallback)
    void renderImmediate() override;

private:
    /// @brief Build instanced geometry for sphere/cube glyphs
    void buildGlyphGeometry();
};

}  // namespace Qwt3D
```

### 2. 数据模型

ScatterPlot 使用 GraphPlot 的 `GraphData`，设置 `PrimitiveType::Points`。

```
用户调用 setData(positions, scalars)
    ↓
GraphData.vertices = positions
GraphData.scalars = scalars
GraphData.primitiveType = PrimitiveType::Points
    ↓
calculateHull() — 计算包围盒
    ↓
createData() → rebuildVBO() + renderVBO()
```

### 3. 渲染管线

#### 3.1 Point 形状（GL_POINTS）

最简单快速的模式。使用 plan00 的点着色器（`kPointVertexShader`/`kPointFragmentShader`）：

```glsl
// 顶点着色器
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec4 aColor;

uniform mat4 uModelView;
uniform mat4 uProjection;
uniform float uPointSize;

out vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
}
```

```glsl
// 片段着色器 — 圆形点
#version 330 core
in vec4 vColor;
out vec4 fragColor;

void main() {
    // Discard fragments outside circle radius
    vec2 coord = gl_PointCoord - vec2(0.5);
    if (dot(coord, coord) > 0.25)
        discard;
    fragColor = vColor;
}
```

#### 3.2 Sphere/Cube 形状（实例化渲染）

对于球体和立方体，使用 OpenGL 实例化渲染（`glDrawArraysInstanced`）以避免每点单独绘制：

1. 预构建单位球体/立方体的顶点几何体（存储在单独的 VBO 中）
2. 每个实例的 position/color 作为实例属性（通过 `glVertexAttribDivisor`）
3. 使用实例化着色器，在顶点着色器中对单位几何体进行缩放和平移

```glsl
// 球体实例化顶点着色器
#version 330 core
layout(location = 0) in vec3 aGlyphVertex;  // unit sphere vertex
layout(location = 1) in vec3 aInstancePos;   // per-instance position
layout(location = 2) in vec4 aInstanceColor; // per-instance color
layout(location = 3) in float aInstanceSize; // per-instance size

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec4 vColor;
out vec3 vNormal;

void main() {
    vec3 pos = aGlyphVertex * aInstanceSize + aInstancePos;
    vNormal = normalize(aGlyphVertex);
    vColor = aInstanceColor;
    gl_Position = uProjection * uModelView * vec4(pos, 1.0);
}
```

#### 3.3 Immediate Mode 回退

```cpp
void ScatterPlot::renderImmediate()
{
    QWT_DC(d);
    auto const& data = graphData();
    const Color* col = dataColor();

    glPointSize(static_cast<GLfloat>(d->m_pointSize));
    glEnable(GL_POINT_SMOOTH);

    glBegin(GL_POINTS);
    for (size_t i = 0; i < data->vertices.size(); ++i) {
        Triple const& v = data->vertices[i];
        RGBA c = (*col)(v);
        glColor4d(c.r, c.g, c.b, c.a);
        glVertex3d(v.x, v.y, v.z);
    }
    glEnd();

    glDisable(GL_POINT_SMOOTH);
}
```

### 4. 颜色映射

颜色通过 `Plot3D::dataColor()` 返回的 `Color` functor 生成。`StandardColor` 基于 z 值映射，`ColorMapColor` 桥接 core 模块的 colormap。

如果 `GraphData::scalars` 非空，则颜色基于标量值（归一化到 [0,1] 后作为 Color functor 的 x 参数输入）：

```cpp
// In rebuildVBO(), when computing colors:
if (!data.scalars.empty()) {
    double sMin = *std::min_element(data.scalars.begin(), data.scalars.end());
    double sMax = *std::max_element(data.scalars.begin(), data.scalars.end());
    for (size_t i = 0; i < data.vertices.size(); ++i) {
        // Normalize scalar to [0,1] — works with any Color functor
        double normalized = (sMax > sMin) ? (data.scalars[i] - sMin) / (sMax - sMin) : 0.5;
        colors.push_back((*col)(normalized, 0, 0));
    }
}
```

**注意**：标量颜色映射对任何 `Color` 子类都有效（`StandardColor`、`ColorMapColor`、自定义），因为归一化后的值作为 x 参数传入 `operator()(x, y, z)`。

### 5. 大小映射

当 `setSizeMapping()` 启用时，每点的渲染大小基于标量值线性映射：

```
size_i = minSize + (scalar_i - sMin) / (sMax - sMin) * (maxSize - minSize)
```

在 VBO 模式下，大小作为实例属性传入着色器；在 immediate mode 下，每点单独 `glPointSize()`。

### 6. Custom 形状

通过 Enrichment 系统实现自定义 glyph：

```cpp
void ScatterPlot::setCustomGlyph(VertexEnrichment const& glyph)
{
    // Clone and store, use in createData() to draw per-point
}
```

在 `createData()` 中，如果 shape 为 Custom，则遍历所有顶点调用 `glyph.draw(vertex)`。

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/scatterplot`，展示：
   - 随机生成的 1000 个 3D 点
   - 切换 Point/Sphere/Cube 形状
   - 切换颜色映射（viridis/plasma/jet）
   - 切换大小映射开/关
3. **性能验证**：渲染 100K 点，VBO 模式帧率应 > 30fps
4. **回退验证**：在兼容上下文下验证 immediate mode 渲染正常
5. **交互验证**：鼠标旋转/缩放/平移正常工作

---

## 风险与注意事项

1. **实例化渲染支持**：`glDrawArraysInstanced` + `glVertexAttribDivisor` 需要 OpenGL 3.3 core 或 `GL_ARB_instanced_arrays` 扩展（OpenGL 3.1/3.2 可用）。`RenderContext::hasInstancing()` 负责检测。不支持时回退到 GL_POINTS 模式（不使用 glyph 几何体）。

2. **球体几何质量**：单位球体的细分程度影响渲染质量和性能。建议默认 16 段（经度）x 8 段（纬度），可通过 `setGlyphQuality()` 调整。

3. **深度排序**：对于半透明点，需要正确的深度排序。GL_POINTS 模式下深度缓冲区处理透明度有限，建议在启用 alpha < 1.0 时禁用深度写入或使用 additive blending。

4. **数据更新频率**：对于实时数据流（如传感器数据），每次 `setData()` 都会重建 VBO。对于高频更新，应支持只更新 VBO 内容（`glBufferSubData`）而不重建 VAO。

5. **与 ColorLegend 集成**：当颜色映射启用时，应自动显示 ColorLegend。复用 `Plot3D::showColorLegend(true)`。

6. **主题兼容**：ScatterPlot 应响应主题系统。主题设置的颜色预设应自动应用到散点颜色映射。
