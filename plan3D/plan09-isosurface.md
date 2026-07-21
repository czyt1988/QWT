请先阅读 `README.md` 以了解全局规范。

# plan09 — 3D 等值面 (IsosurfacePlot)

## 目标

实现 3D 等值面可视化，从 3D 标量场中提取等值面（isosurface）并渲染为三角网格。使用 Marching Cubes 算法进行面提取，支持多等值面级别、每面颜色、透明度。等值面是医学影像（CT/MRI）、地质勘探、CFD 分析等领域的重要可视化手段。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- 复用 plan08 的 `ScalarField`/`GridScalarField` 抽象接口

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_isosurfaceplot.h` | IsosurfacePlot 类声明 |
| `src/plot3d/qwt3d_isosurfaceplot.cpp` | IsosurfacePlot 类实现 |
| `src/plot3d/qwt3d_isosurfaceplot_p.h` | PIMPL 私有数据 |
| `src/plot3d/qwt3d_marchingcubes.h` | Marching Cubes 算法封装 |
| `src/plot3d/qwt3d_marchingcubes.cpp` | Marching Cubes 实现（查找表 + 三角形提取） |
| `examples/3D/isosurface/isosurface.cpp` | 示例程序 |
| `examples/3D/isosurface/CMakeLists.txt` | 示例构建配置 |

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

/// @brief 3D isosurface plot widget
class QWT3D_EXPORT IsosurfacePlot : public Plot3D
{
    Q_OBJECT

public:
    explicit IsosurfacePlot(QWidget* parent = nullptr);
    ~IsosurfacePlot() override;

    /// @brief Set the scalar field for isosurface extraction
    /// @note IsosurfacePlot holds a shared_ptr to ensure field lifetime
    void setScalarField(std::shared_ptr<ScalarField> field);

    /// @brief Add an isosurface at the given value
    /// @param value   Iso value (the surface where field == value)
    /// @param color   Surface color (optional, defaults to data color)
    /// @param opacity Surface opacity (1.0 = opaque, 0.0 = fully transparent)
    void addIsosurface(double value, RGBA color = RGBA(), double opacity = 1.0);

    /// @brief Remove an isosurface by value
    void removeIsosurface(double value);

    /// @brief Clear all isosurfaces
    void clearIsosurfaces();

    /// @brief Set grid resolution for Marching Cubes sampling
    /// @param nx, ny, nz Grid dimensions
    void setGridResolution(unsigned nx, unsigned ny, unsigned nz);

    /// @brief Enable/disable smooth normals (vertex normal averaging)
    void setSmoothNormals(bool enabled);
    bool smoothNormals() const;

    /// @brief Set isosurface color mode
    /// @param dataDriven If true, color from data color functor; if false, use per-surface color
    void setDataDrivenColor(bool dataDriven);

protected:
    QWT_DECLARE_PRIVATE(IsosurfacePlot)

    void calculateHull() override;
    void createData() override;

    /// @brief Extract isosurface mesh from scalar field
    /// @param value Iso value
    /// @return Triangle mesh (vertices, normals, indices) — uses TriangleMesh from plan00
    TriangleMesh extractIsosurface(double value) const;

    /// @brief Render using VBO
    void renderVBO();

    /// @brief Render using immediate mode
    void renderImmediate();
};

}  // namespace Qwt3D
```

### 2. Marching Cubes 算法

Marching Cubes 是 3D 等值面提取的经典算法。对体素网格的每个立方体单元，根据 8 个顶点值与等值面的关系（高于/低于），查表确定三角形拓扑。

#### 2.1 算法封装

```cpp
namespace Qwt3D {

/// @brief Marching Cubes isosurface extraction algorithm
class QWT3D_EXPORT MarchingCubes
{
public:
    /// @brief Extract isosurface mesh from a 3D scalar grid
    /// @param grid    Scalar values, size = nx*ny*nz
    /// @param nx, ny, nz Grid dimensions
    /// @param origin  Grid origin in world coordinates
    /// @param spacing Grid spacing
    /// @param isovalue Iso value for surface extraction
    /// @param smoothNormals If true, compute smooth vertex normals by averaging
    /// @return Triangle mesh (uses TriangleMesh from qwt3d_tubebuilder.h / plan00)
    static TriangleMesh extract(
        std::vector<double> const& grid,
        unsigned nx, unsigned ny, unsigned nz,
        Triple origin, Triple spacing,
        double isovalue,
        bool smoothNormals = true);

private:
    /// @brief 256-entry case table (maps 8-bit vertex sign to edge list)
    static const int s_edgeTable[256];

    /// @brief 256×16 triangle table (maps case to triangle edge indices)
    static const int s_triTable[256][16];

    /// @brief Edge-to-vertex offset table (12 edges of a cube)
    static const Triple s_edgeOffsets[12];

    /// @brief Compute interpolated vertex position on an edge
    static Triple interpolateEdge(
        Triple const& p1, double v1,
        Triple const& p2, double v2,
        double isovalue);
};

}  // namespace Qwt3D
```

#### 2.2 算法步骤

```
1. 采样标量场到规则网格 grid[nx][ny][nz]
   - 对于 GridScalarField: 直接访问内部网格数据
   - 对于 FunctionScalarField: 在网格点调用 sample()

2. 遍历每个体素单元 (ix, iy, iz), ix∈[0,nx-2], iy∈[0,ny-2], iz∈[0,nz-2]
   a. 获取 8 个顶点的标量值
   b. 计算 8 位 case index (0-255)
   c. 查 s_edgeTable[caseIdx] 确定哪些边有交点
   d. 对每条有交点的边，线性插值计算交点 3D 位置
   e. 查 s_triTable[caseIdx] 确定三角形拓扑
   f. 生成三角形顶点和索引

3. (可选) 计算法线:
   - FLAT: 每三角形面法线 (normalizedcross)
   - SMOOTH: 顶点法线 (共享顶点的面法线平均)
```

#### 2.3 顶点去重

Marching Cubes 原始算法会产生重复顶点（相邻体素共享边交点）。使用哈希表去重：

```cpp
// Edge key: (cellIndex, edgeIndex) → unique vertex index
struct EdgeKey {
    unsigned cellIdx;
    unsigned edgeIdx;
    bool operator==(EdgeKey const& o) const {
        return cellIdx == o.cellIdx && edgeIdx == o.edgeIdx;
    }
};
struct EdgeKeyHash {
    size_t operator()(EdgeKey const& k) const {
        return std::hash<unsigned>()(k.cellIdx) ^ (std::hash<unsigned>()(k.edgeIdx) << 1);
    }
};

std::unordered_map<EdgeKey, unsigned, EdgeKeyHash> vertexMap;
// When processing an edge, check if it's already in the map
// If yes, reuse the vertex index; if no, create new vertex and add to map
```

### 3. 多等值面

支持同时显示多个等值面：

```cpp
struct IsosurfaceInfo {
    double value;
    RGBA color;
    double opacity;
    TriangleMesh mesh;  ///< Uses TriangleMesh from plan00
};

// In PrivateData:
std::vector<IsosurfaceInfo> m_isosurfaces;
```

每个等值面独立提取网格，独立渲染。半透明等值面需从后向前排序。

### 4. 渲染

#### VBO 模式

每个等值面一个 VBO，使用表面着色器（`kSurfaceVertexShader`/`kSurfaceFragmentShader`）：

```cpp
void IsosurfacePlot::renderVBO()
{
    for (auto const& iso : d->m_isosurfaces) {
        // Upload mesh to VBO (or use cached VBO)
        // Set uniform color/opacity
        // Draw triangles
    }
}
```

#### Immediate Mode 回退

```cpp
void IsosurfacePlot::renderImmediate()
{
    for (auto const& iso : d->m_isosurfaces) {
        glColor4d(iso.color.r, iso.color.g, iso.color.b, iso.opacity);

        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < iso.mesh.indices.size(); i += 3) {
            for (int j = 0; j < 3; ++j) {
                unsigned idx = iso.mesh.indices[i + j];
                Triple const& n = iso.mesh.normals[idx];
                Triple const& v = iso.mesh.vertices[idx];
                glNormal3d(n.x, n.y, n.z);
                glVertex3d(v.x, v.y, v.z);
            }
        }
        glEnd();
    }
}
```

### 5. 法线计算

#### FLAT 法线（面法线）

每个三角形独立计算法线，同一顶点在不同三角形中有不同法线。渲染效果为平面着色（faceted look）。

#### SMOOTH 法线（顶点法线）

共享顶点的所有面法线取平均，产生平滑着色。实现：

```cpp
void computeSmoothNormals(TripleField& normals,
                          TripleField const& vertices,
                          std::vector<unsigned int> const& indices)
{
    normals.assign(vertices.size(), Triple(0, 0, 0));

    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned a = indices[i], b = indices[i+1], c = indices[i+2];
        Triple const& va = vertices[a];
        Triple const& vb = vertices[b];
        Triple const& vc = vertices[c];
        Triple faceNormal = normalizedcross(vb - va, vc - va);
        normals[a] += faceNormal;
        normals[b] += faceNormal;
        normals[c] += faceNormal;
    }

    for (auto& n : normals)
        n.normalize();
}
```

### 6. 半透明排序

当多个半透明等值面同时显示时，需从后向前渲染：

```cpp
// Sort isosurfaces by distance to camera (back to front)
std::sort(d->m_isosurfaces.begin(), d->m_isosurfaces.end(),
    [](const IsosurfaceInfo& a, const IsosurfaceInfo& b) {
        // Compare by distance to camera origin
        return distanceToCamera(a) > distanceToCamera(b);
    });
```

对于单个等值面内部的半透明，需要深度排序三角形（开销大，通常不建议）。

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/isosurface`，展示：
   - 球体等值面（标量场 = x²+y²+z²，iso=1.0）
   - 圆环面（torus）等值面
   - 双等值面（如 iso=0.5 和 iso=1.0 同时显示，不同颜色/透明度）
   - 切换 FLAT/SMOOTH 法线
3. **正确性验证**：已知解析解的标量场（如球体），验证提取的等值面顶点数、三角形数和几何形状正确
4. **性能验证**：64³ 网格（约 26 万体素）的等值面提取时间 < 100ms
5. **光照验证**：启用光照，验证法线方向正确（等值面外侧为正面法线）
6. **透明度验证**：设置半透明等值面，验证能看到内部结构

---

## 风险与注意事项

1. **Marching Cubes 歧义**：经典 Marching Cubes 在某些 case 有拓扑歧义（面补丁方式不唯一），可能导致等值面上出现"孔洞"。需要使用修正的查找表（如 Chernyaev 或 Lopes-Brodlie 修正）来消除歧义。

2. **网格分辨率 vs 精度**：Marching Cubes 的精度取决于采样网格分辨率。过低会导致等值面粗糙，过高会增加计算量。64³ 是常见平衡点，128³ 用于高质量渲染。

3. **内存使用**：128³ 网格 = 2M double = 16MB。等值面可能产生数万三角形，每个三角形 3 顶点 × 24 bytes = 2MB+ 顶点数据 + 索引。可控。

4. **实时更新**：如果标量场动态变化（如时间序列数据），每次更新都需要重新运行 Marching Cubes。考虑使用增量更新或 GPU 着色器实现（但复杂度高，不建议初始版本实现）。

5. **法线方向**：Marching Cubes 生成的三角形顶点顺序决定法线方向。如果法线指向内部（反面），光照会不正确。需要确保一致的面法线方向（通常向外）。

6. **与 VolumePlot 的关系**：IsosurfacePlot 提取并渲染等值面（三角网格），VolumePlot（plan10）直接渲染体积数据（光线投射）。两者是互补的等值面可视化方式。IsosurfacePlot 更适合清晰的面边界展示，VolumePlot 更适合内部结构展示。

7. **顶点去重的重要性**：不去重会导致顶点数膨胀（每条边交点在相邻体素中重复生成），法线计算错误（重复顶点无法正确平均面法线）。去重使用全局边键：`edgeKey = globalCellIdx * 12 + localEdgeIdx`（每个体素有 12 条边，全局体素索引 = `iz * (nx-1) * (ny-1) + iy * (nx-1) + ix`），映射为 `std::unordered_map<uint64_t, unsigned>`。这比结构体键的哈希冲突率更低。

8. **查找表大小**：s_edgeTable 有 256 项，s_triTable 有 256×16 项。这些表是静态常量，放在 `qwt3d_marchingcubes.cpp` 中作为 `static const` 数组。查找表来源：Paul Bourke 的经典 Marching Cubes 实现（公有领域）。
