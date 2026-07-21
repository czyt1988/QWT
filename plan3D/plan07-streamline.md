请先阅读 `README.md` 以了解全局规范。

# plan07 — 3D 流线图 (StreamlinePlot)

## 目标

实现 3D 流线可视化，通过对 3D 向量场进行数值积分（RK4）追踪流线轨迹，并以折线或管状体渲染。流线是 CFD（计算流体力学）、电磁场分析、气象学等领域的重要可视化手段。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- **plan02**（GraphPlot 基类）：继承 GraphPlot
- 参考但非强依赖 **plan05**（LinePlot3D）：管状渲染算法可复用

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_streamlineplot.h` | StreamlinePlot 类声明 |
| `src/plot3d/qwt3d_streamlineplot.cpp` | StreamlinePlot 类实现 |
| `src/plot3d/qwt3d_streamlineplot_p.h` | PIMPL 私有数据 |
| `examples/3D/streamline/streamline.cpp` | 示例程序 |
| `examples/3D/streamline/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 添加 `qwt3d_streamlineplot.cpp` 和 `_p.h` |
| `examples/CMakeLists.txt` | 注册 `3D/streamline` 示例 |

---

## 技术方案

### 1. VectorField 抽象接口

`VectorField`、`GridVectorField`、`FunctionVectorField` 已在 plan00 的 `qwt3d_field3d.h` 中定义（继承自 `Field3D` 基类）。本计划不再重复定义，直接 `#include "qwt3d_field3d.h"` 使用。

**生命周期管理**：`StreamlinePlot` 通过 `std::shared_ptr<VectorField>` 持有向量场引用，确保在 `createData()` 执行期间向量场不会被销毁：

```cpp
void StreamlinePlot::setVectorField(std::shared_ptr<VectorField> field);
```

### 2. 类设计

```cpp
namespace Qwt3D {

/// @brief Streamline integration direction
enum class StreamDirection {
    Forward,   ///< Integrate forward (positive dt)
    Backward,  ///< Integrate backward (negative dt)
    Both       ///< Integrate both directions from seed
};

/// @brief 3D streamline plot widget
class QWT3D_EXPORT StreamlinePlot : public GraphPlot
{
    Q_OBJECT

public:
    explicit StreamlinePlot(QWidget* parent = nullptr);
    ~StreamlinePlot() override;

    /// @brief Set the vector field to trace streamlines through
    /// @note StreamlinePlot holds a shared_ptr to ensure field lifetime during createData()
    void setVectorField(std::shared_ptr<VectorField> field);

    /// @brief Set seed points for streamline tracing
    /// @param seeds Vector of seed positions
    void setSeeds(TripleField const& seeds);

    /// @brief Set regular grid of seed points
    /// @param origin Seed grid origin
    /// @param spacing Seed grid spacing
    /// @param dims   Seed grid dimensions
    void setRegularSeeds(Triple origin, Triple spacing, Tuple dims);

    /// @brief Set random seed points
    /// @param count Number of random seeds
    /// @param domain Seed placement domain
    void setRandomSeeds(unsigned count, ParallelEpiped domain);

    /// @brief Set integration direction
    void setDirection(StreamDirection dir);
    StreamDirection direction() const;

    /// @brief Set integration step size
    void setStepSize(double ds);
    double stepSize() const;

    /// @brief Set maximum number of integration steps per streamline
    void setMaxSteps(unsigned max);
    unsigned maxSteps() const;

    /// @brief Set termination threshold (velocity magnitude below which tracing stops)
    void setTerminationThreshold(double v);
    double terminationThreshold() const;

    /// @brief Set line width (Simple mode) or tube radius (Tube mode)
    void setLineWidth(double w);
    void setTubeRadius(double r);

    /// @brief Set render mode (simple lines or tubes)
    void setRenderMode(LineRenderMode mode);
    LineRenderMode renderMode() const;

    /// @brief Enable color mapping by velocity magnitude
    void setMagnitudeColorMapping(bool enabled);

protected:
    QWT_DECLARE_PRIVATE(StreamlinePlot)

    void createData() override;

    /// @brief Trace all streamlines from seed points
    void traceStreamlines();

    /// @brief Trace a single streamline from a seed point
    /// @return Ordered vertices of the streamline
    TripleField traceStreamline(Triple const& seed) const;

    /// @brief Render using VBO
    void renderVBO() override;

    /// @brief Render using immediate mode
    void renderImmediate() override;
};

}  // namespace Qwt3D
```

### 3. RK4 数值积分

使用四阶 Runge-Kutta 方法对向量场进行积分：

```cpp
TripleField StreamlinePlot::traceStreamline(Triple const& seed) const
{
    TripleField points;
    double ds = d->m_stepSize;
    unsigned maxSteps = d->m_maxSteps;
    double threshold = d->m_terminationThreshold;

    Triple current = seed;
    points.push_back(current);

    for (unsigned step = 0; step < maxSteps; ++step) {
        // RK4 integration step
        Triple k1 = d->m_field->sample(current.x, current.y, current.z);
        if (k1.length() < threshold)
            break;

        Triple k2 = d->m_field->sample(
            current.x + 0.5 * ds * k1.x,
            current.y + 0.5 * ds * k1.y,
            current.z + 0.5 * ds * k1.z);

        Triple k3 = d->m_field->sample(
            current.x + 0.5 * ds * k2.x,
            current.y + 0.5 * ds * k2.y,
            current.z + 0.5 * ds * k2.z);

        Triple k4 = d->m_field->sample(
            current.x + ds * k3.x,
            current.y + ds * k3.y,
            current.z + ds * k3.z);

        Triple next = current + (ds / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);

        // Check domain bounds
        if (!d->m_field->contains(next))
            break;

        points.push_back(next);
        current = next;
    }

    return points;
}
```

### 4. 流线追踪流程

```
1. setVectorField(field) — 存储向量场引用
2. setSeeds(seeds) — 设置种子点
3. createData() 被调用:
   a. traceStreamlines() — 对每个种子点:
      - Forward 方向: 从种子点沿正方向积分
      - Backward 方向: 从种子点沿负方向积分
      - Both: 两个方向积分后合并
   b. 将所有流线存入 m_streamlines (vector<TripleField>)
   c. 构建 VBO:
      - Simple: 构建 GL_LINE_STRIP 索引
      - Tube: 构建管状几何体 (复用 plan05 的算法)
```

### 5. 颜色映射

按速度大小（向量场的幅度）映射颜色：

```cpp
// During VBO construction, color each vertex based on local velocity:
for (auto const& point : streamline) {
    Triple vel = field->sample(point.x, point.y, point.z);
    double magnitude = vel.length();
    RGBA color = (*dataColor())(magnitude, 0, 0);
    data->colors.push_back(color);
}
```

### 6. 多线程计算

流线追踪是 CPU 密集型计算，可以在后台线程执行：

```cpp
void StreamlinePlot::traceStreamlines()
{
    // Option 1: Synchronous (simple, blocks UI)
    for (auto const& seed : d->m_seeds) {
        d->m_streamlines.push_back(traceStreamline(seed));
    }

    // Option 2: Background thread (non-blocking, recommended for large fields)
    // QtConcurrent::map(d->m_seeds, [this](Triple const& seed) {
    //     return traceStreamline(seed);
    // }).then([this]() {
    //     QMetaObject::invokeMethod(this, "updateData", Qt::QueuedConnection);
    // });
}
```

初始版本使用同步方式，后续可优化为多线程。

### 7. 种子点策略

#### 7.1 规则网格种子

```cpp
void StreamlinePlot::setRegularSeeds(Triple origin, Triple spacing, Tuple dims)
{
    TripleField seeds;
    for (unsigned ix = 0; ix < dims.x; ++ix)
        for (unsigned iy = 0; iy < dims.y; ++iy)
            for (unsigned iz = 0; iz < dims.z; ++iz) {
                seeds.push_back(Triple(
                    origin.x + ix * spacing.x,
                    origin.y + iy * spacing.y,
                    origin.z + iz * spacing.z));
            }
    setSeeds(seeds);
}
```

#### 7.2 随机种子

```cpp
void StreamlinePlot::setRandomSeeds(unsigned count, ParallelEpiped domain)
{
    // Use a deterministic random generator for reproducibility
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<> distX(domain.minVertex.x, domain.maxVertex.x);
    std::uniform_real_distribution<> distY(domain.minVertex.y, domain.maxVertex.y);
    std::uniform_real_distribution<> distZ(domain.minVertex.z, domain.maxVertex.z);

    TripleField seeds;
    for (unsigned i = 0; i < count; ++i) {
        seeds.push_back(Triple(distX(rng), distY(rng), distZ(rng)));
    }
    setSeeds(seeds);
}
```

### 8. 渲染

流线渲染复用 GraphPlot 的渲染基础设施和 plan00 的 TubeBuilder：

- **Simple 模式**：`GL_LINES`（使用索引对，非 `GL_LINE_STRIP`，避免不同流线间的连线），使用线着色器
- **Tube 模式**：使用 plan00 的 `TubeBuilder` 工具类构建管状几何体（parallel transport frame），通过 `appendMesh()` 合并到 GraphData，使用表面着色器（含光照和法线）

```cpp
// Tube mode example:
void StreamlinePlot::buildTubeGeometry()
{
    TubeBuilder::Config config;
    config.radius = d->m_tubeRadius;
    config.segments = d->m_tubeQuality;

    for (auto const& streamline : d->m_streamlines) {
        TriangleMesh tube;
        auto colorFunc = [this](Triple const& v) -> RGBA {
            Triple vel = d->m_field->sample(v.x, v.y, v.z);
            return (*dataColor())(vel.length(), 0, 0);
        };
        TubeBuilder::build(streamline, config, tube, colorFunc);
        appendMesh(tube);
    }
}
```

---

## 验证方式

### 单元测试

1. **编译验证**：`.\build.ps1 build`
2. **积分准确性验证**：使用已知解析解的向量场（如旋转流 v=(-y,x,0)），验证流线为圆形
3. **终止条件验证**：验证流线在域边界正确终止，在零速度处正确停止
4. **种子密度验证**：调整种子点数量，验证可视化效果
5. **性能验证**：100 条流线 × 1000 步，追踪时间 < 1s（同步模式）

### 示例验证

运行 `examples/3D/streamline`，展示：
- Lorenz 吸引子流线（混沌系统）
- 旋涡流场（解析公式：v = (-y, x, 0) + 噪声）
- ABC 流（Arnold-Beltrami-Childress 流）
- 切换 Simple/Tube 渲染模式
- 切换 Forward/Backward/Both 方向

### 截图验证

```powershell
# Simple 模式截图
.\streamline.exe -p "docs/assets/screenshots-3D/streamline-simple.png"

# Tube 模式截图（需在示例代码中切换模式后重新编译，或通过命令行参数控制）
.\streamline.exe -p "docs/assets/screenshots-3D/streamline-tube.png" -d 1000
```

截图列表：
| 截图文件名 | 渲染模式 | 说明 |
|-----------|---------|------|
| `streamline-simple.png` | Simple (GL_LINES) | 基本流线渲染 |
| `streamline-tube.png` | Tube (圆柱几何) | 管状流线渲染，需增大延迟 |

---

## 风险与注意事项

1. **积分稳定性**：RK4 方法在向量场变化剧烈的区域可能不稳定。步长 `stepSize` 需要根据场的特性调整。过大的步长会导致流线偏离真实轨迹，过小的步长会增加计算时间。可考虑自适应步长（基于曲率或速度变化率）。

2. **边界处理**：当流线离开向量场域时必须正确终止。`VectorField::contains()` 方法必须正确实现，避免在域外采样返回垃圾值。

3. **种子点放置**：种子点的位置和数量对流线可视化质量至关重要。过多的种子点会导致视觉混乱，过少则无法反映场的全貌。建议提供交互式种子点放置工具（不在本计划范围内）。

4. **多线程安全**：向量场采样在后台线程执行时，必须确保 `VectorField` 的 `sample()` 方法是线程安全的（只读操作，不修改状态）。`GridVectorField` 的采样是只读的，安全。

5. **内存使用**：100 条流线 × 2000 点 × 24 bytes（Triple）= 4.8MB，可以接受。Tube 模式 16 段 × 2000 点 × 2(顶点+法线) × 24 bytes = 1.5MB/流线，100 条 = 150MB，需注意内存。

6. **与 VectorFieldPlot 的关系**：VectorFieldPlot（plan06）用箭头展示向量场，StreamlinePlot（本计划）用流线展示。两者可以共存：在同一个 3D 场景中（通过 MultiPlot，plan12）同时显示箭头和流线，提供互补的向量场可视化。

7. **VectorField 生命周期**：`setVectorField()` 接受 `std::shared_ptr<VectorField>`，StreamlinePlot 持有共享引用，确保在 `createData()` 执行期间向量场不会被销毁。`GridVectorField` 的 `sample()` 方法是只读操作，线程安全。

8. **数值精度**：对于极端缩放的向量场（如速度量级 1e-3 或 1e6），浮点精度可能导致积分不稳定。建议在 `stepSize` 选择时考虑场的量级。
