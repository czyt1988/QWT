请先阅读 `README.md` 以了解全局规范。

# plan08 — 3D 等高线 / 切片 (ContourSlicePlot)

## 目标

实现 3D 标量场的等高线和切片可视化。包括：任意平面切片上的等高线投影、等高线填充（彩色带）、多层等高线、任意方向切片。扩展现有 SurfacePlot 的 `FLOORISO` 功能为独立且更强大的绘图类型。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- 复用 core 模块的 `QwtRasterData`/`QwtGridRasterData` 作为标量场数据源

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_contoursliceplot.h` | ContourSlicePlot 类声明 |
| `src/plot3d/qwt3d_contoursliceplot.cpp` | ContourSlicePlot 类实现 |
| `src/plot3d/qwt3d_contoursliceplot_p.h` | PIMPL 私有数据 |
| `examples/3D/contourslice/contourslice.cpp` | 示例程序 |
| `examples/3D/contourslice/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 添加 `qwt3d_contoursliceplot.cpp` 和 `_p.h` |
| `examples/CMakeLists.txt` | 注册 `3D/contourslice` 示例 |

---

## 技术方案

### 1. ScalarField 抽象接口

`ScalarField`、`GridScalarField`、`FunctionScalarField` 已在 plan00 的 `qwt3d_field3d.h` 中定义（继承自 `Field3D` 基类）。本计划不再重复定义，直接 `#include "qwt3d_field3d.h"` 使用。

**生命周期管理**：`ContourSlicePlot` 通过 `std::shared_ptr<ScalarField>` 持有标量场引用：

```cpp
void ContourSlicePlot::setScalarField(std::shared_ptr<ScalarField> field);
```

### 2. 类设计

```cpp
namespace Qwt3D {

/// @brief Slice plane definition
struct SlicePlane {
    enum class Normal { X, Y, Z, Arbitrary };
    Normal normal;       ///< Slice normal direction
    double position;     ///< Position along normal axis (for X/Y/Z)
    Triple arbitraryPoint;  ///< Point on plane (for Arbitrary)
    Triple arbitraryNormal; ///< Plane normal (for Arbitrary)
};

/// @brief Contour rendering mode
enum class ContourMode {
    Lines,    ///< Contour lines only (isolines)
    Filled,   ///< Filled contour bands (colored regions between lines)
    Both      ///< Lines + filled
};

/// @brief 3D contour and slice plot widget
class QWT3D_EXPORT ContourSlicePlot : public Plot3D
{
    Q_OBJECT

public:
    explicit ContourSlicePlot(QWidget* parent = nullptr);
    ~ContourSlicePlot() override;

    /// @brief Set the scalar field to visualize
    /// @note ContourSlicePlot holds a shared_ptr to ensure field lifetime
    void setScalarField(std::shared_ptr<ScalarField> field);

    /// @brief Add a slice plane
    /// @param plane Slice plane definition
    void addSlice(SlicePlane const& plane);

    /// @brief Add X-axis slices at regular intervals
    /// @param count Number of slices
    void addXSlices(unsigned count);

    /// @brief Add Y-axis slices at regular intervals
    void addYSlices(unsigned count);

    /// @brief Add Z-axis slices at regular intervals
    void addZSlices(unsigned count);

    /// @brief Clear all slices
    void clearSlices();

    /// @brief Set number of contour levels
    void setContourLevels(unsigned levels);
    unsigned contourLevels() const;

    /// @brief Set explicit contour values
    void setContourValues(std::vector<double> const& values);
    std::vector<double> contourValues() const;

    /// @brief Set contour rendering mode
    void setContourMode(ContourMode mode);
    ContourMode contourMode() const;

    /// @brief Set slice resolution (grid samples per slice)
    void setSliceResolution(unsigned resolution);
    unsigned sliceResolution() const;

protected:
    QWT_DECLARE_PRIVATE(ContourSlicePlot)

    void calculateHull() override;
    void createData() override;

    /// @brief Generate contour data for a slice plane
    /// @return Vector of polylines (contour lines) and filled regions
    struct SliceData {
        TripleField vertices;            ///< All vertices on the slice
        CellField contourLines;          ///< Contour line segments
        CellField contourFills;          ///< Contour fill regions
        std::vector<RGBA> colors;        ///< Per-cell colors
    };
    SliceData generateSlice(SlicePlane const& plane) const;

    /// @brief Marching Squares algorithm for contour extraction on a 2D slice
    void marchingSquares(
        std::vector<double> const& grid,    ///< Scalar values on slice grid
        unsigned width, unsigned height,    ///< Grid dimensions
        Triple origin, Triple dxAxis, Triple dyAxis,  ///< 3D position mapping
        std::vector<double> const& levels,  ///< Contour levels
        SliceData& out                      ///< Output
    ) const;
};

}  // namespace Qwt3D
```

### 3. 切片生成

对于每个切片平面，在平面上建立 2D 采样网格，从 3D 标量场中采样：

```cpp
SliceData ContourSlicePlot::generateSlice(SlicePlane const& plane) const
{
    unsigned res = d->m_sliceResolution;  // e.g., 100

    // Determine two in-plane axes based on plane normal
    Triple origin, xAxis, yAxis;
    // For Normal::Z: origin = (minX, minY, position), xAxis = (1,0,0), yAxis = (0,1,0)
    // For Normal::X: origin = (position, minY, minZ), xAxis = (0,1,0), yAxis = (0,0,1)
    // For Normal::Y: origin = (minX, position, minZ), xAxis = (1,0,0), yAxis = (0,0,1)
    // For Arbitrary: compute two orthogonal in-plane vectors

    // Sample scalar field on the grid
    std::vector<double> grid(res * res);
    for (unsigned iy = 0; iy < res; ++iy) {
        for (unsigned ix = 0; ix < res; ++ix) {
            double u = static_cast<double>(ix) / (res - 1);
            double v = static_cast<double>(iy) / (res - 1);
            Triple pos = origin + u * xAxis + v * yAxis;
            grid[iy * res + ix] = m_field->sample(pos.x, pos.y, pos.z);
        }
    }

    // Run Marching Squares to extract contours
    SliceData data;
    marchingSquares(grid, res, res, origin, xAxis / (res-1), yAxis / (res-1),
                    d->m_contourValues, data);
    return data;
}
```

### 4. Marching Squares 算法

在 2D 网格上提取等高线的经典算法：

```cpp
void ContourSlicePlot::marchingSquares(
    std::vector<double> const& grid,
    unsigned width, unsigned height,
    Triple origin, Triple dxAxis, Triple dyAxis,
    std::vector<double> const& levels,
    SliceData& out) const
{
    for (double level : levels) {
        for (unsigned iy = 0; iy < height - 1; ++iy) {
            for (unsigned ix = 0; ix < width - 1; ++ix) {
                // Get four corner values
                double v00 = grid[iy * width + ix];
                double v10 = grid[iy * width + ix + 1];
                double v11 = grid[(iy+1) * width + ix + 1];
                double v01 = grid[(iy+1) * width + ix];

                // Compute case index (0-15) based on above/below level
                unsigned caseIdx = 0;
                if (v00 >= level) caseIdx |= 1;
                if (v10 >= level) caseIdx |= 2;
                if (v11 >= level) caseIdx |= 4;
                if (v01 >= level) caseIdx |= 8;

                if (caseIdx == 0 || caseIdx == 15)
                    continue;  // No contour in this cell

                // Compute interpolation factors
                auto lerp = [](double a, double b, double va, double vb) -> double {
                    return (vb == va) ? 0.5 : (a - va) / (vb - va);
                };

                // Compute edge intersection points (in grid space)
                double fx = ix, fy = iy;
                // Edge 0 (bottom): (ix, iy)-(ix+1, iy)
                // Edge 1 (right):  (ix+1, iy)-(ix+1, iy+1)
                // Edge 2 (top):    (ix+1, iy+1)-(ix, iy+1)
                // Edge 3 (left):   (ix, iy+1)-(ix, iy)

                // ... standard Marching Squares case table lookup ...
                // For each case, generate 1-2 line segments

                // Convert grid coordinates to 3D positions
                auto toWorld = [&](double gx, double gy) -> Triple {
                    return origin + gx * dxAxis + gy * dyAxis;
                };

                // Add contour line segments
                // (Implementation uses standard Marching Squares case table)
            }
        }
    }
}
```

### 5. 等高线填充

填充模式在相邻等高线之间填充颜色带：

1. 对每个网格单元，确定其值落在哪个等高线区间 `[levels[i], levels[i+1])` 内
2. 用对应的颜色（通过 `Color` functor）填充整个单元
3. 在填充上叠加等高线（如果 `ContourMode::Both`）

填充使用 `GL_QUADS` 或 `GL_TRIANGLES` 绘制每个网格单元的四边形。

### 6. 渲染

#### VBO 模式

- 等高线：`GL_LINES`，使用线着色器，每条线段颜色由等高线级别决定
- 填充：`GL_TRIANGLES`，使用表面着色器，每个四边形单元为一个填充区域

#### Immediate Mode 回退

```cpp
void ContourSlicePlot::renderImmediate()
{
    for (auto const& slice : d->m_slices) {
        if (d->m_contourMode == ContourMode::Filled || d->m_contourMode == ContourMode::Both) {
            // Draw filled triangles (not GL_QUADS — QUADS unavailable in Core Profile)
            glBegin(GL_TRIANGLES);
            for (auto const& cell : slice.contourFills) {
                // Each cell is a quad (4 vertices), split into 2 triangles
                if (cell.size() >= 4) {
                    RGBA const& c = slice.colors[&cell - &slice.contourFills[0]];
                    glColor4d(c.r, c.g, c.b, c.a);
                    // Triangle 1: vertices 0,1,2
                    Triple const& v0 = slice.vertices[cell[0]];
                    Triple const& v1 = slice.vertices[cell[1]];
                    Triple const& v2 = slice.vertices[cell[2]];
                    glVertex3d(v0.x, v0.y, v0.z);
                    glVertex3d(v1.x, v1.y, v1.z);
                    glVertex3d(v2.x, v2.y, v2.z);
                    // Triangle 2: vertices 0,2,3
                    Triple const& v3 = slice.vertices[cell[3]];
                    glVertex3d(v0.x, v0.y, v0.z);
                    glVertex3d(v2.x, v2.y, v2.z);
                    glVertex3d(v3.x, v3.y, v3.z);
                }
            }
            glEnd();
        }

        if (d->m_contourMode == ContourMode::Lines || d->m_contourMode == ContourMode::Both) {
            // Draw contour lines
            glBegin(GL_LINES);
            for (auto const& cell : slice.contourLines) {
                for (size_t i = 0; i < cell.size(); i += 2) {
                    Triple const& v1 = slice.vertices[cell[i]];
                    Triple const& v2 = slice.vertices[cell[i+1]];
                    glVertex3d(v1.x, v1.y, v1.z);
                    glVertex3d(v2.x, v2.y, v2.z);
                }
            }
            glEnd();
        }
    }
}
```

### 7. 与现有 FLOORISO 的关系

SurfacePlot 已有 `FLOORSTYLE::FLOORISO` 在 Z=0 地板上投影等高线。ContourSlicePlot 的区别：

| 特性 | SurfacePlot FLOORISO | ContourSlicePlot |
|------|---------------------|------------------|
| 数据源 | 表面数据 z=f(x,y) | 3D 标量场 v=f(x,y,z) |
| 切片位置 | 仅 Z=0 地板 | 任意平面（X/Y/Z/任意方向） |
| 等高线级别 | 固定数量(`setIsolines()`) | 可自定义值 |
| 填充 | 无 | 支持彩色填充 |
| 多切片 | 不支持 | 支持多切片同时显示 |

---

## 验证方式

### 单元测试

1. **编译验证**：`.\build.ps1 build`
2. **Marching Squares 正确性**：验证等高线连续不断裂，在极值点周围形成闭合环
3. **填充正确性**：验证填充颜色与等高线级别对应
4. **交互验证**：鼠标旋转/缩放/平移正常
5. **与 ColorLegend 集成**：颜色图例正确显示等高线级别对应的颜色

### 示例验证

运行 `examples/3D/contourslice`，展示：
- 3D 高斯函数标量场
- X/Y/Z 三个方向的切片同时显示
- 等高线模式 vs 填充模式 vs 组合模式
- 自定义等高线级别
- 任意方向切片（如 45° 对角切片）

### 截图验证

```powershell
# 线条模式截图
.\contourslice.exe -p "docs/assets/screenshots-3D/contourslice-lines.png"

# 填充模式截图
.\contourslice.exe -p "docs/assets/screenshots-3D/contourslice-filled.png"
```

截图列表：
| 截图文件名 | 渲染模式 | 说明 |
|-----------|---------|------|
| `contourslice-lines.png` | Lines only | 仅等高线 |
| `contourslice-filled.png` | Filled | 等高线+填充 |

---

## 风险与注意事项

1. **Marching Squares 歧义**：经典的 Marching Squares 在 `caseIdx == 6` 和 `caseIdx == 9` 时有拓扑歧义（鞍点情况）。采用**渐近线决定法（Asymptotic Decider）**消歧：计算单元中心值 `vCenter = (v00 + v10 + v11 + v01) / 4`，若 `vCenter >= level` 则选择连接方式 A，否则选择连接方式 B。这保证等高线在鞍点处不会断裂。

2. **采样分辨率**：`sliceResolution` 决定切片网格的精细程度。过低会导致等高线粗糙，过高会增加计算量。建议默认 100，范围 50-500。

3. **多切片性能**：每个切片需要 `res × res` 次标量场采样 + Marching Squares 计算。10 个切片 × 100×100 = 100K 采样 + 10K 单元计算，应 < 100ms。

4. **任意方向切片**：`SlicePlane::Normal::Arbitrary` 需要计算平面上的两个正交基向量。确保基向量与平面法线正交且右手系。

5. **填充的深度冲突**：多个切片的填充区域可能重叠，需要正确的深度排序或透明度混合。

6. **标量场生命周期**：`setScalarField()` 接受 `std::shared_ptr<ScalarField>`，持有共享引用，确保在 `createData()` 期间有效。

7. **与 core 模块集成**：`GridScalarField` 可以包装 core 模块的 `QwtGridRasterData`（3D 栅格数据），复用其数据加载和插值能力。检查 `QwtGridRasterData` 的 API 是否适合 3D 采样。

8. **等高线级别自动计算**：如果用户不指定等高线值，应自动根据标量场范围生成均匀分布的级别（类似 `QwtScaleEngine` 的刻度美化算法）。可复用 core 模块的 `QwtLinearScaleEngine`。
