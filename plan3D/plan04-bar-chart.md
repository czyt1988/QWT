请先阅读 `README.md` 以了解全局规范。

# plan04 — 3D 柱状图 (Bar3D)

## 目标

实现 3D 柱状图，在三维空间中绘制柱体。支持分组柱状图、堆叠柱状图、每柱颜色映射、柱体几何渲染。参考 `examples/3D/enrichments` 中的自定义 Bar enrichment 设计，但将其提升为独立的绘图类型。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- **plan02**（GraphPlot 基类）：继承 GraphPlot

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_bar3d.h` | Bar3D 类声明 |
| `src/plot3d/qwt3d_bar3d.cpp` | Bar3D 类实现 |
| `src/plot3d/qwt3d_bar3d_p.h` | PIMPL 私有数据 |
| `examples/3D/bar3d/bar3d.cpp` | 示例程序 |
| `examples/3D/bar3d/CMakeLists.txt` | 示例构建配置 |

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

/// @brief Bar chart layout mode
enum class BarLayout {
    Grouped,   ///< Bars side by side within each group
    Stacked    ///< Bars stacked on top of each other
};

/// @brief 3D bar chart widget
class QWT3D_EXPORT Bar3D : public GraphPlot
{
    Q_OBJECT

public:
    explicit Bar3D(QWidget* parent = nullptr);
    ~Bar3D() override;

    /// @brief Set bar data (simple mode: one value per (x,y) position)
    /// @param positions (x,y) grid positions (z is ignored, base = 0)
    /// @param heights   Bar heights (z values)
    /// @param barWidth  Bar width in x direction
    /// @param barDepth  Bar depth in y direction
    void setData(std::vector<Tuple> const& positions,
                 std::vector<double> const& heights,
                 double barWidth = 0.8,
                 double barDepth = 0.8);

    /// @brief Set bar data (full mode: explicit base and top)
    /// @param positions (x,y) positions
    /// @param bases     Bar base z values
    /// @param tops     Bar top z values
    void setData(std::vector<Tuple> const& positions,
                 std::vector<double> const& bases,
                 std::vector<double> const& tops,
                 double barWidth = 0.8,
                 double barDepth = 0.8);

    /// @brief Set multi-series data for grouped/stacked bar charts
    /// @param series Vector of series, each series is a vector of (position, height) pairs
    /// @param layout Grouped or Stacked
    void setMultiSeriesData(std::vector<std::vector<std::pair<Tuple, double>>> const& series,
                            BarLayout layout = BarLayout::Grouped);

    /// @brief Set bar width (x dimension)
    void setBarWidth(double w);
    double barWidth() const;

    /// @brief Set bar depth (y dimension)
    void setBarDepth(double d);
    double barDepth() const;

    /// @brief Set bar color (uniform color for all bars)
    void setBarColor(RGBA color);

    /// @brief Enable height-based color mapping
    void setHeightColorMapping(bool enabled);
    bool heightColorMappingEnabled() const;

    /// @brief Set layout mode for multi-series
    void setLayout(BarLayout layout);
    BarLayout layout() const;

protected:
    QWT_DECLARE_PRIVATE(Bar3D)

    void createData() override;

    /// @brief Build bar box geometry (12 triangles per bar) and upload to VBO
    void buildBarGeometry();

    /// @brief Render bars using VBO
    void renderVBO() override;

    /// @brief Render bars using immediate mode
    void renderImmediate() override;
};

}  // namespace Qwt3D
```

### 2. 柱体几何

每个柱体是一个长方体（box），由 12 个三角形（6 面 × 2 三角形）组成：

```
      4--------5
     /|       /|
    7--------6 |
    | |      | |
    | 0------|1
    |/       |/
    3--------2

顶点: 0=(x-w/2, y-d/2, z_base), 1=(x+w/2, y-d/2, z_base),
      2=(x+w/2, y+d/2, z_base), 3=(x-w/2, y+d/2, z_base),
      4=(x-w/2, y-d/2, z_top),  5=(x+w/2, y-d/2, z_top),
      6=(x+w/2, y+d/2, z_top),  7=(x-w/2, y+d/2, z_top)

面: bottom(0,1,2,3), top(4,5,6,7),
    front(0,1,5,4), back(3,2,6,7),
    left(0,3,7,4), right(1,2,6,5)
```

每个面需要法线，用于光照计算。

### 3. 几何构建

每个柱体使用 `TriangleMesh` 构建（每面独立顶点以支持 per-face 法线），通过 `appendMesh()` 合并到 GraphData。

**关键实现**：使用预构建的单位长方体模板几何体（24 个顶点 = 6 面 × 4 顶点，36 个索引），每个实例通过缩放和平移放置。

```cpp
void Bar3D::buildBarGeometry()
{
    QWT_D(d);
    graphData().clear();
    graphData().setPrimitiveType(PrimitiveType::Bars);  // GL_TRIANGLES

    const Color* col = dataColor();
    double barW = d->m_barWidth;
    double barD = d->m_barDepth;

    for (size_t i = 0; i < d->m_positions.size(); ++i) {
        Tuple const& pos = d->m_positions[i];
        double zBase = d->m_bases.empty() ? 0.0 : d->m_bases[i];
        double zTop = d->m_bases.empty() ? d->m_heights[i] : d->m_tops[i];

        double x0 = pos.x - barW / 2, x1 = pos.x + barW / 2;
        double y0 = pos.y - barD / 2, y1 = pos.y + barD / 2;
        double z0 = zBase, z1 = zTop;

        TriangleMesh bar;

        // 24 vertices: 4 per face (to support per-face normals)
        // Bottom face (normal: 0,0,-1)
        bar.vertices.insert(bar.vertices.end(), {
            Triple(x0,y0,z0), Triple(x1,y0,z0), Triple(x1,y1,z0), Triple(x0,y1,z0)});
        for (int j = 0; j < 4; ++j) bar.normals.push_back(Triple(0, 0, -1));

        // Top face (normal: 0,0,1)
        bar.vertices.insert(bar.vertices.end(), {
            Triple(x0,y0,z1), Triple(x1,y0,z1), Triple(x1,y1,z1), Triple(x0,y1,z1)});
        for (int j = 0; j < 4; ++j) bar.normals.push_back(Triple(0, 0, 1));

        // Front face (y-, normal: 0,-1,0)
        bar.vertices.insert(bar.vertices.end(), {
            Triple(x0,y0,z0), Triple(x1,y0,z0), Triple(x1,y0,z1), Triple(x0,y0,z1)});
        for (int j = 0; j < 4; ++j) bar.normals.push_back(Triple(0, -1, 0));

        // Back face (y+, normal: 0,1,0)
        bar.vertices.insert(bar.vertices.end(), {
            Triple(x0,y1,z0), Triple(x1,y1,z0), Triple(x1,y1,z1), Triple(x0,y1,z1)});
        for (int j = 0; j < 4; ++j) bar.normals.push_back(Triple(0, 1, 0));

        // Left face (x-, normal: -1,0,0)
        bar.vertices.insert(bar.vertices.end(), {
            Triple(x0,y0,z0), Triple(x0,y1,z0), Triple(x0,y1,z1), Triple(x0,y0,z1)});
        for (int j = 0; j < 4; ++j) bar.normals.push_back(Triple(-1, 0, 0));

        // Right face (x+, normal: 1,0,0)
        bar.vertices.insert(bar.vertices.end(), {
            Triple(x1,y0,z0), Triple(x1,y1,z0), Triple(x1,y1,z1), Triple(x1,y0,z1)});
        for (int j = 0; j < 4; ++j) bar.normals.push_back(Triple(1, 0, 0));

        // Colors
        RGBA barColor;
        if (d->m_heightColorMapping)
            barColor = (*col)(0, 0, zTop);
        else if (d->m_uniformColor)
            barColor = d->m_barColor;
        else
            barColor = (*col)(pos.x, pos.y, zTop);
        bar.colors.assign(24, barColor);

        // Indices: 12 triangles (CCW winding viewed from outside for correct normals)
        for (int face = 0; face < 6; ++face) {
            unsigned b = static_cast<unsigned>(face * 4);
            bar.indices.insert(bar.indices.end(), {b, b+1, b+2, b, b+2, b+3});
        }

        appendMesh(bar);
    }

    d->m_vboDirty = true;
}
```

**法线说明**：每面使用 4 个独立顶点（而非共享 8 个角点），使得每个面的法线唯一且正确。这会产生 24 个顶点/柱体（而非 8 个），但保证光照效果正确（flat shading 外观）。

### 4. 多系列分组/堆叠

#### 分组模式

多个系列在同一 (x,y) 位置附近并排放置：

```
系列1 ████  系列2 ████  系列3 ████
     (x-δ)        (x)         (x+δ)
```

每个系列的柱子宽度缩小为 `barWidth / numSeries`，沿 x 轴偏移。

#### 堆叠模式

多个系列在同一 (x,y) 位置堆叠：

```
系列3 █████
系列2 █████  (z_base = 系列1 的 top)
系列1 █████  (z_base = 0)
```

每个系列的 base z 值为前一系列的 top z 值。

### 5. 渲染

使用 plan00 的表面着色器（`kSurfaceVertexShader`/`kSurfaceFragmentShader`），支持 FLAT/GOURAUD 着色和光照。

VBO 模式下，顶点属性为 position(0)、normal(1)、color(2)，通过索引数组绘制 `GL_TRIANGLES`。

Immediate mode 回退使用 `glBegin(GL_TRIANGLES)` + `glVertex3d` + `glNormal3d` + `glColor4d`。

### 6. 参考已有实现

`examples/3D/enrichments` 中的自定义 `Bar` enrichment 使用 `GL_QUADS` 和 `GL_LINES` 绘制柱体：

```cpp
// From enrichments example — Bar enrichment
void draw(Triple const& pos) override
{
    // Draw a box at the position
    double len = ...;
    glBegin(GL_QUADS);
    // 6 faces...
    glEnd();
}
```

Bar3D 的设计参考此实现，但使用三角形而非四边形（Core Profile 不支持 `GL_QUADS`），并使用 VBO 而非 immediate mode。

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/bar3d`，展示：
   - 规则网格上的柱状图（如 5x5 的柱体网格）
   - 分组模式（3 个系列并排）
   - 堆叠模式（3 个系列堆叠）
   - 高度颜色映射（低=蓝色，高=红色）
   - 均匀颜色模式
3. **光照验证**：启用光照，验证柱体有正确的光照效果（面法线正确）
4. **回退验证**：在兼容上下文下验证 immediate mode 渲染正常
5. **交互验证**：鼠标旋转/缩放/平移正常工作
6. **导出验证**：导出为 PDF/EPS，验证矢量输出正确

---

## 风险与注意事项

1. **法线计算**：每个面的法线必须指向外部，否则光照不正确。上面代码中的三角形顶点顺序必须保证逆时针（从外部看）以获得正确法线。

2. **深度冲突 (Z-fighting)**：当柱体底面与坐标轴地板在同一 z 值时可能产生深度冲突。使用 `glPolygonOffset` 或微小的 z 偏移避免。

3. **大数据量**：100x100 网格 = 10000 柱体 × 8 顶点 × 3 float = 240KB 顶点数据，VBO 可以轻松处理。但 36 个索引/柱体 = 360K 索引，需确保索引缓冲使用 `unsigned int` 而非 `unsigned short`（超过 65535 顶点时）。

4. **半透明柱体**：如果支持半透明柱体，需要正确的深度排序（从后向前渲染）。这在 VBO 模式下需要 CPU 排序索引数组。

5. **与 SurfacePlot 区别**：Bar3D 继承 GraphPlot 而非 SurfacePlot。柱体是独立的几何体，不是连续曲面。不要尝试用 SurfacePlot 的 GridData 来渲染柱状图。

6. **柱体标签**：未来可考虑在柱顶显示数值标签（通过 Enrichment 或 Label 系统），但不在本计划范围内。
