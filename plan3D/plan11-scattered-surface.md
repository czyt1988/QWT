请先阅读 `README.md` 以了解全局规范。

# plan11 — 散乱点插值曲面 (ScatteredSurfacePlot)

## 目标

实现散乱点（scattered point）插值曲面可视化。从非规则的散乱 (x,y,z) 数据点集出发，通过 Delaunay 三角化或薄板样条插值，生成三角网格曲面并渲染。这解决了实际测量数据（如地形测绘、传感器网络）无法用规则网格表达的问题。

## 前置依赖

- 无强依赖（不继承 GraphPlot，而是扩展 SurfacePlot 以复用其 CellData 渲染管线）
- 复用 SurfacePlot 的 `CellData` 和 `loadFromData(TripleField, CellField)` 接口

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_scatteredsurfaceplot.h` | ScatteredSurfacePlot 类声明 |
| `src/plot3d/qwt3d_scatteredsurfaceplot.cpp` | ScatteredSurfacePlot 类实现 |
| `src/plot3d/qwt3d_scatteredsurfaceplot_p.h` | PIMPL 私有数据 |
| `src/plot3d/qwt3d_delaunay.h` | Delaunay 三角化算法封装 |
| `src/plot3d/qwt3d_delaunay.cpp` | Delaunay 实现 |
| `examples/3D/scatteredsurface/scatteredsurface.cpp` | 示例程序 |
| `examples/3D/scatteredsurface/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 添加新源文件 |
| `examples/CMakeLists.txt` | 注册新示例 |

---

## 技术方案

### 1. 设计定位

ScatteredSurfacePlot 继承自 SurfacePlot（而非 GraphPlot），因为：
- 最终输出是三角网格曲面（CellData），与 SurfacePlot 的 CellData 渲染管线完全一致
- 复用 SurfacePlot 的 `loadFromData(TripleField, CellField)`、`createDataC()`、`createFloorDataC()` 等方法
- 只需添加数据预处理（Delaunay 三角化或插值）步骤

与 SurfacePlot 的区别：
- SurfacePlot 需要规则网格数据或预定义的多边形网格
- ScatteredSurfacePlot 接受散乱点集，自动生成三角网格

### 2. 类设计

```cpp
namespace Qwt3D {

/// @brief Interpolation method for scattered data
enum class InterpolationMethod {
    Delaunay,     ///< Delaunay triangulation (exact interpolation, no smoothing)
    ThinPlateSpline,  ///< Thin plate spline (smooth interpolation)
    Nearest       ///< Nearest neighbor (fastest, blocky appearance)
};

/// @brief Scattered point surface plot widget
class QWT3D_EXPORT ScatteredSurfacePlot : public SurfacePlot
{
    Q_OBJECT

public:
    explicit ScatteredSurfacePlot(QWidget* parent = nullptr);
    ~ScatteredSurfacePlot() override;

    /// @brief Set scattered point data
    /// @param points (x, y, z) scattered data points
    /// @param method Interpolation method
    void setData(TripleField const& points,
                 InterpolationMethod method = InterpolationMethod::Delaunay);

    /// @brief Set scattered data with Delaunay triangulation
    /// @param points Scattered (x,y,z) points
    /// @param triangulation Pre-computed triangulation (optional, if empty auto-compute)
    void setData(TripleField const& points, CellField const& triangulation);

    /// @brief Set scattered data with grid interpolation
    /// @param points Scattered (x,y,z) points
    /// @param gridX, gridY Output grid dimensions
    /// @param method Interpolation method for grid generation
    void setData(TripleField const& points,
                 unsigned gridX, unsigned gridY,
                 InterpolationMethod method);

    /// @brief Set interpolation method
    void setInterpolationMethod(InterpolationMethod method);
    InterpolationMethod interpolationMethod() const;

    /// @brief Set output grid resolution (for grid interpolation mode)
    void setGridResolution(unsigned gridX, unsigned gridY);

    /// @brief Set interpolation smoothing parameter (for ThinPlateSpline)
    void setSmoothingFactor(double factor);
    double smoothingFactor() const;

protected:
    QWT_DECLARE_PRIVATE(ScatteredSurfacePlot)

    /// @brief Compute Delaunay triangulation of 2D (x,y) projection
    CellField computeDelaunay(TripleField const& points) const;

    /// @brief Interpolate scattered data to regular grid
    /// @return Grid z-values (gridX * gridY matrix)
    std::vector<std::vector<double>> interpolateToGrid(
        TripleField const& points,
        unsigned gridX, unsigned gridY,
        InterpolationMethod method) const;

private:
    /// @brief Process data and load into SurfacePlot
    void processData(TripleField const& points, InterpolationMethod method);
};

}  // namespace Qwt3D
```

### 3. Delaunay 三角化

#### 3.1 算法选择

2D Delaunay 三角化（将散乱点投影到 x-y 平面后三角化）有多种实现：

| 算法 | 复杂度 | 实现难度 | 适用规模 |
|------|--------|---------|---------|
| Bowyer-Watson | O(n log n) | 中 | < 10K 点 |
| 分治法 | O(n log n) | 高 | < 100K 点 |
| 增量法 | O(n²) | 低 | < 1K 点 |

建议实现 **Bowyer-Watson** 算法，平衡复杂度和实现难度。

#### 3.2 Bowyer-Watson 算法

```cpp
CellField ScatteredSurfacePlot::computeDelaunay(TripleField const& points) const
{
    // 1. Create super-triangle containing all points
    // 2. For each point:
    //    a. Find all triangles whose circumcircle contains the point ("bad triangles")
    //    b. Remove bad triangles, forming a polygonal hole
    //    c. Re-triangulate the hole by connecting the point to each boundary edge
    // 3. Remove triangles sharing vertices with the super-triangle

    // Extract 2D coordinates
    std::vector<Tuple> points2D;
    for (auto const& p : points)
        points2D.push_back(Tuple(p.x, p.y));

    // Bowyer-Watson implementation
    // ...

    // Result: CellField where each Cell is a triangle (3 vertex indices)
    return triangulation;
}
```

#### 3.3 Delaunay 封装

```cpp
namespace Qwt3D {

/// @brief 2D Delaunay triangulation
class QWT3D_EXPORT Delaunay2D
{
public:
    /// @brief Compute Delaunay triangulation of 2D points
    /// @param points 2D points (x, y)
    /// @return Triangle indices (each triangle = 3 indices into points)
    static CellField triangulate(std::vector<Tuple> const& points);

    /// @brief Check if point d is inside the circumcircle of triangle (a, b, c)
    static bool inCircumcircle(Tuple const& d, Tuple const& a,
                               Tuple const& b, Tuple const& c);
};

}  // namespace Qwt3D
```

### 4. 薄板样条插值 (Thin Plate Spline)

对于需要平滑曲面的场景，使用薄板样条插值：

```cpp
std::vector<std::vector<double>> interpolateToGrid(
    TripleField const& points,
    unsigned gridX, unsigned gridY,
    InterpolationMethod method) const
{
    // Compute grid bounds
    double minX, maxX, minY, maxY;
    // ... compute from points ...

    std::vector<std::vector<double>> grid(gridX, std::vector<double>(gridY));

    if (method == InterpolationMethod::ThinPlateSpline) {
        // 1. Build TPS system: solve for weights
        //    Matrix: [[K P], [P^T 0]] * [w; a] = [z; 0]
        //    K[i][j] = r_ij^2 * log(r_ij) where r_ij = distance(pi, pj)
        //    P[i] = [1, xi, yi]
        //    w = weights, a = [a0, ax, ay] (affine part)
        // 2. Evaluate at grid points:
        //    z(x,y) = a0 + ax*x + ay*y + sum(wi * K(point_i, (x,y)))

        // Build and solve the linear system (e.g., using Eigen or manual LU)
        // Note: Project does NOT use Eigen. Use manual matrix operations or
        // Qt's QGenericMatrix (limited) or implement a simple linear solver.
    }

    // ... fill grid ...

    return grid;
}
```

**重要约束**：项目不依赖 Eigen 或其他线性代数库。TPS 需要求解 (n+3)×(n+3) 线性方程组。

**线性求解器选择**：实现**带部分主元选取的高斯消元**（partial pivoting Gaussian elimination），保证数值稳定性：

```cpp
/// @brief Solve Ax = b using Gaussian elimination with partial pivoting
/// @param A Coefficient matrix (n×n), modified in-place
/// @param b Right-hand side vector (n), modified in-place to solution
/// @return true on success, false if matrix is singular or near-singular
bool solveLinearSystem(std::vector<std::vector<double>>& A,
                       std::vector<double>& b)
{
    int n = static_cast<int>(A.size());
    constexpr double eps = 1e-12;  // Singularity threshold

    for (int col = 0; col < n; ++col) {
        // Partial pivoting: find row with largest absolute value in column
        int maxRow = col;
        double maxVal = std::abs(A[col][col]);
        for (int row = col + 1; row < n; ++row) {
            if (std::abs(A[row][col]) > maxVal) {
                maxVal = std::abs(A[row][col]);
                maxRow = row;
            }
        }

        if (maxVal < eps) {
            qWarning("TPS: matrix is singular or near-singular (column %d)", col);
            return false;
        }

        // Swap rows
        if (maxRow != col) {
            std::swap(A[col], A[maxRow]);
            std::swap(b[col], b[maxRow]);
        }

        // Eliminate below
        for (int row = col + 1; row < n; ++row) {
            double factor = A[row][col] / A[col][col];
            for (int j = col; j < n; ++j)
                A[row][j] -= factor * A[col][j];
            b[row] -= factor * b[col];
        }
    }

    // Back substitution
    for (int row = n - 1; row >= 0; --row) {
        for (int j = row + 1; j < n; ++j)
            b[row] -= A[row][j] * b[j];
        b[row] /= A[row][row];
    }
    return true;
}
```

**TPS 核函数 r²log(r) 在 r=0 处的处理**：
```cpp
// TPS kernel: U(r) = r² * log(r), with U(0) = 0
double tpsKernel(double r)
{
    if (r < 1e-15)
        return 0.0;
    return r * r * std::log(r);
}
```

**性能约束**：(n+3)×(n+3) 高斯消元复杂度 O(n³)。n=500 时约 125M 次浮点运算（~100ms），n=1000 时约 1G 次（~1s）。建议限制 TPS 最大点数为 500，超出时自动降级为 Delaunay 或最近邻模式并警告用户。

### 5. 数据处理流程

#### 5.1 Delaunay 模式

```
setData(points, Delaunay)
    ↓
computeDelaunay(points) → CellField (triangulation)
    ↓
SurfacePlot::loadFromData(points, triangulation)
    ↓
SurfacePlot 的 CellData 渲染管线接管 (createDataC, createNormalsC, etc.)
```

#### 5.2 Grid 插值模式

```
setData(points, gridX, gridY, method)
    ↓
interpolateToGrid(points, gridX, gridY, method) → z[gridX][gridY]
    ↓
构建 double** 矩阵
    ↓
SurfacePlot::loadFromData(double** data, gridX, gridY, minX, maxX, minY, maxY)
    ↓
SurfacePlot 的 GridData 渲染管线接管 (createDataG, calcNormals, etc.)
```

### 6. 与 SurfacePlot 的集成

ScatteredSurfacePlot 的 `createData()` 不需要 override，因为数据处理完成后调用的是 SurfacePlot 的 `loadFromData()`，后者会设置 `actualData` 并触发 `updateData()`，最终调用 SurfacePlot 的 `createData()`。

```cpp
void ScatteredSurfacePlot::processData(TripleField const& points,
                                        InterpolationMethod method)
{
    if (method == InterpolationMethod::Delaunay) {
        CellField triangulation = computeDelaunay(points);
        loadFromData(points, triangulation);  // SurfacePlot's method
    } else {
        unsigned gridX = d->m_gridX;
        unsigned gridY = d->m_gridY;
        auto grid = interpolateToGrid(points, gridX, gridY, method);
        // Convert to double** and call SurfacePlot::loadFromData
        // ...
    }
}
```

### 7. 最近邻插值

最简单但最粗糙的方法：

```cpp
// For each grid point, find the nearest scattered point and use its z value
double nearestZ(double gx, double gy, TripleField const& points)
{
    double minDist = std::numeric_limits<double>::max();
    double nearestZ = 0;
    for (auto const& p : points) {
        double dist = (p.x - gx) * (p.x - gx) + (p.y - gy) * (p.y - gy);
        if (dist < minDist) {
            minDist = dist;
            nearestZ = p.z;
        }
    }
    return nearestZ;
}
```

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/scatteredsurface`，展示：
   - 随机散乱点（100-1000 个）的地形曲面
   - Delaunay 模式（精确三角化，无平滑）
   - 薄板样条模式（平滑曲面）
   - 最近邻模式（块状外观）
   - 规则网格插值模式
3. **正确性验证**：使用已知函数（如 z = sin(x) * cos(y)）生成散乱点，验证插值结果与原函数接近
4. **Delaunay 正确性**：验证三角网格满足 Delaunay 性质（空圆性质：每个三角形的外接圆内不包含其他点）
5. **性能验证**：1000 点的 Delaunay 三角化 < 100ms；100 点的 TPS 插值 < 500ms
6. **渲染验证**：Delaunay 模式生成的三角网格正确显示为 CellData 曲面，法线正确
7. **地板投影**：验证 FLOORISO/FLOORDATA 地板投影在散乱点曲面上正常工作

---

## 风险与注意事项

1. **Delaunay 实现复杂度**：Bowyer-Watson 算法需要正确处理：
   - 超级三角形的创建（必须包含所有点）
   - 坏三角形检测（点在外接圆内）
   - 边界提取（坏三角形集合的外边界）
   - 超级三角形相关三角形的清理
   - 共线点处理（3 点共线时三角形退化）

2. **TPS 线性求解器**：项目不依赖 Eigen。需要实现简单的 LU 分解或高斯消元求解器。对于 n 个散乱点，TPS 需要 (n+3)×(n+3) 的矩阵求解，n > 500 时计算量显著。建议限制 TPS 最大点数或提供近似方法（如选取 k 个最近邻作为 RBF 中心）。

3. **点投影到 2D 的重叠处理**：Delaunay 三角化在 x-y 平面上进行。如果散乱点的 x-y 投影有重叠（同一 (x,y) 有不同 z 值），三角化会产生退化三角形。**处理策略**：
   - **检测**：在 `computeDelaunay()` 入口检查 x-y 投影中是否存在距离小于容差（如 `1e-10 * maxRange`）的点对
   - **警告**：如果检测到重叠，通过 `qWarning()` 通知用户
   - **预处理选项**：提供 `setOverlapHandling()` 方法，支持三种策略：
     - `Warn`（默认）：仅警告，仍尝试三角化
     - `AverageZ`：合并重叠点，z 值取平均
     - `PerturbXY`：对重叠点施加微小随机扰动（量级 `1e-8 * maxRange`），使投影不再重叠

4. **边界处理**：散乱点数据的边界可能不规则。Delaunay 三角化会在凸包边界产生三角形，可能导致大三角形拉伸。可考虑 alpha-shape 裁剪边界或用户指定边界多边形。

5. **大数据集**：10K+ 点的 Delaunay 三角化可能需要秒级计算。应支持后台线程计算（类似 plan07 的多线程方案）。

6. **内存管理**：`loadFromData(points, triangulation)` 将数据复制到 CellData 内部存储。原 TripleField 和 CellField 可以在调用后释放。

7. **与 SurfacePlot 的 API 兼容**：ScatteredSurfacePlot 继承 SurfacePlot 的所有 public API（`setPlotStyle`, `setFloorStyle`, `setResolution` 等）。`setResolution()` 在 Delaunay 模式下不起作用（因为不是网格数据），应 override 并记录 `qWarning("setResolution() has no effect in Delaunay mode")`。在 Grid 插值模式下，`setResolution()` 控制输出网格分辨率，功能正常。

8. **Delanuay 库选择**：如果不想手动实现，可以考虑使用项目允许的轻量级 Delaunay 库（如 `del-triangulation` 头文件库）。但项目原则是"不引入新的第三方库"（见 README.md §3.3），因此建议手动实现 Bowyer-Watson 算法。
