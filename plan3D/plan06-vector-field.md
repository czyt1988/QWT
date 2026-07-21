请先阅读 `README.md` 以了解全局规范。

# plan06 — 3D 向量场图 (VectorFieldPlot)

## 目标

实现 3D 向量场可视化，在三维空间中绘制箭头表示向量场。支持规则网格和散乱点的向量场、按向量大小颜色映射、向量归一化和缩放控制。复用已有 `Arrow` enrichment 的箭头几何和方向旋转算法。

## 前置依赖

- **plan00**（渲染基础设施）：使用 GLBuffer/GLVertexArray/ShaderProgram
- **plan02**（GraphPlot 基类）：继承 GraphPlot

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_vectorfieldplot.h` | VectorFieldPlot 类声明 |
| `src/plot3d/qwt3d_vectorfieldplot.cpp` | VectorFieldPlot 类实现 |
| `src/plot3d/qwt3d_vectorfieldplot_p.h` | PIMPL 私有数据 |
| `examples/3D/vectorfield/vectorfield.cpp` | 示例程序 |
| `examples/3D/vectorfield/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/CMakeLists.txt` | 添加新源文件 |
| `examples/CMakeLists.txt` | 注册新示例 |

---

## 技术方案

### 1. 已有基础设施复用

#### 1.1 FreeVector / FreeVectorField（`qwt3d_types.h`）

已有数据结构可直接使用：

```cpp
struct FreeVector {
    Triple base;  // 向量起点
    Triple top;   // 向量终点
};
using FreeVectorField = std::vector<FreeVector>;
```

#### 1.2 Arrow Enrichment（`qwt3d_enrichment_std.h`）

已有的 `Arrow` 类实现了：
- `calcRotation(Triple& axis, FreeVector const& vec)` — 计算旋转轴和角度，将 z 轴方向旋转到向量方向
- 使用 GLU 二次曲面（`gluCylinder`/`gluDisk`）构建箭头几何
- 可配置：段数、锥长/锥径/杆径比例、颜色

**重要**：`calcRotation` 当前是 `Arrow` 类的 **private** 方法。需要将其改为 `public static`：

```cpp
// 修改 qwt3d_enrichment_std.h 中 Arrow 类：
// 将:
//   double calcRotation(Qwt3D::Triple& axis, Qwt3D::FreeVector const& vec);
// 改为:
public:
    /// @brief Compute rotation axis and angle to align z-axis with a given vector
    /// @param[out] axis Rotation axis (unit vector)
    /// @param vec  Target direction vector (base → top)
    /// @return Rotation angle in radians
    static double calcRotation(Qwt3D::Triple& axis, Qwt3D::FreeVector const& vec);
```

此修改是一个小范围的可见性变更（private → public static），不影响现有 `Arrow` enrichment 的行为（内部调用改为 `Arrow::calcRotation(axis, vec)`）。

VectorFieldPlot 将复用此旋转算法（在 immediate mode 回退中使用），但在 VBO 模式下使用着色器内的矩阵运算构建旋转（详见 §4 实例化着色器）。

### 2. 类设计

```cpp
namespace Qwt3D {

/// @brief Vector field rendering mode
enum class VectorRenderMode {
    Arrows,      ///< Arrow glyphs (cone + cylinder)
    Lines,       ///< Simple line segments (fastest)
    Streamlines  ///< Streamlines (delegates to StreamlinePlot, see plan07)
};

/// @brief 3D vector field plot widget
class QWT3D_EXPORT VectorFieldPlot : public GraphPlot
{
    Q_OBJECT

public:
    explicit VectorFieldPlot(QWidget* parent = nullptr);
    ~VectorFieldPlot() override;

    /// @brief Set vector field data from FreeVectorField
    void setData(FreeVectorField const& field);

    /// @brief Set vector field on a regular grid
    /// @param origin Grid origin (min x, y, z)
    /// @param spacing Grid spacing (dx, dy, dz)
    /// @param dims   Grid dimensions (nx, ny, nz)
    /// @param vectors Vector at each grid point (nx*ny*nz Triples)
    void setData(Triple origin, Triple spacing, Tuple dims,
                 TripleField const& vectors);

    /// @brief Set vector field from positions and directions
    /// @param positions Vector start points
    /// @param directions Vector directions (will be scaled by length)
    void setData(TripleField const& positions, TripleField const& directions);

    /// @brief Set render mode
    void setRenderMode(VectorRenderMode mode);
    VectorRenderMode renderMode() const;

    /// @brief Set arrow quality (number of segments around cone/cylinder)
    void setArrowQuality(unsigned segments);
    unsigned arrowQuality() const;

    /// @brief Set arrow size ratios
    /// @param relConeLength Cone length relative to total vector length (0.2-0.5)
    /// @param relConeRadius  Cone radius relative to vector length
    /// @param relStemRadius  Stem radius relative to vector length
    void setArrowProportions(double relConeLength, double relConeRadius, double relStemRadius);

    /// @brief Enable/disable vector normalization (all vectors same length)
    void setNormalize(bool enabled);
    bool normalizeEnabled() const;

    /// @brief Set vector length scale factor
    void setLengthScale(double scale);
    double lengthScale() const;

    /// @brief Enable color mapping by vector magnitude
    void setMagnitudeColorMapping(bool enabled);
    bool magnitudeColorMappingEnabled() const;

protected:
    QWT_DECLARE_PRIVATE(VectorFieldPlot)

    void createData() override;

    /// @brief Build arrow geometry and upload to VBO
    void buildArrowGeometry();

    /// @brief Render using VBO (instanced)
    void renderVBO() override;

    /// @brief Render using immediate mode
    void renderImmediate() override;

private:
    /// @brief Build unit arrow geometry (cylinder + cone along z-axis)
    void buildUnitArrow();
};

}  // namespace Qwt3D
```

### 3. 单位箭头几何体

预构建一个沿 z 轴方向的单位箭头几何体（杆 + 锥），后续通过实例化渲染在每个向量位置进行缩放/旋转：

```
        ___
       /   \  ← Cone (tip)
       \___/
        |
        |    ← Cylinder (stem)
        |
        0    ← Origin (z=0)
```

```cpp
void VectorFieldPlot::buildUnitArrow()
{
    // Build a unit arrow pointing along +Z, total length = 1.0
    // Stored in m_unitArrowVertices / m_unitArrowIndices

    unsigned quality = d->m_arrowQuality;  // e.g., 16
    double coneLen = d->m_relConeLength;   // e.g., 0.3
    double coneRad = d->m_relConeRadius;   // e.g., 0.08
    double stemRad = d->m_relStemRadius;   // e.g., 0.04
    double stemLen = 1.0 - coneLen;

    // Cylinder (stem): from z=0 to z=stemLen
    // ... build ring vertices for top and bottom of cylinder ...

    // Cone (tip): from z=stemLen to z=1.0
    // ... build cone vertices ...

    // Build triangle indices for cylinder side, cone side, and caps
}
```

### 4. 实例化渲染

使用 OpenGL 实例化渲染（`glDrawArraysInstanced`）高效绘制大量箭头：

1. **单位箭头 VBO**：预构建的单位箭头几何体顶点
2. **实例属性 VBO**：每个向量的位置(base)、方向(direction)、颜色(color)、缩放(scale)

```glsl
// 实例化箭头顶点着色器
#version 330 core
layout(location = 0) in vec3 aGlyphVertex;     // unit arrow vertex
layout(location = 1) in vec3 aGlyphNormal;      // unit arrow normal

layout(location = 2) in vec3 aInstancePos;       // per-instance base position
layout(location = 3) in vec3 aInstanceDir;       // per-instance direction
layout(location = 4) in vec4 aInstanceColor;     // per-instance color
layout(location = 5) in float aInstanceScale;    // per-instance length scale

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec4 vColor;
out vec3 vNormal;

void main() {
    // Build rotation matrix from direction (z-axis → direction)
    vec3 dir = normalize(aInstanceDir);
    vec3 up = abs(dir.z) > 0.9 ? vec3(0,1,0) : vec3(0,0,1);
    vec3 right = normalize(cross(dir, up));
    up = cross(right, dir);

    // Rotate and scale the unit arrow
    vec3 scaled = aGlyphVertex * aInstanceScale;
    vec3 rotated = vec3(
        dot(scaled, vec3(right.x, up.x, dir.x)),
        dot(scaled, vec3(right.y, up.y, dir.y)),
        dot(scaled, vec3(right.z, up.z, dir.z))
    );

    vec3 finalPos = rotated + aInstancePos;
    vColor = aInstanceColor;
    vNormal = mat3(right, up, dir) * aGlyphNormal;
    gl_Position = uProjection * uModelView * vec4(finalPos, 1.0);
}
```

### 5. 颜色映射

按向量大小（magnitude）映射颜色：

```cpp
// In buildArrowGeometry(), compute per-instance colors:
for (auto const& vec : d->m_field) {
    Triple dir = vec.top - vec.base;
    double magnitude = dir.length();

    if (d->m_normalize) {
        dir.normalize();
    }
    dir *= d->m_lengthScale;

    RGBA color;
    if (d->m_magnitudeColorMapping) {
        color = (*dataColor())(magnitude, 0, 0);  // Map magnitude to color
    } else {
        color = (*dataColor())(vec.base);
    }

    d->m_instancePositions.push_back(vec.base);
    d->m_instanceDirections.push_back(dir);
    d->m_instanceColors.push_back(color);
    d->m_instanceScales.push_back(d->m_normalize ? d->m_lengthScale : magnitude * d->m_lengthScale);
}
```

### 6. Immediate Mode 回退

在 immediate mode 下，复用 `Arrow` enrichment 的 GLU 二次曲面方法：

```cpp
void VectorFieldPlot::renderImmediate()
{
    QWT_DC(d);
    const Color* col = dataColor();

    // Use GLU quadric for arrow geometry
    GLUquadricObj* quadric = gluNewQuadric();
    gluQuadricDrawStyle(quadric, GLU_FILL);
    gluQuadricNormals(quadric, GLU_SMOOTH);

    for (auto const& vec : d->m_field) {
        Triple dir = vec.top - vec.base;
        double length = dir.length();

        if (d->m_normalize) {
            dir.normalize();
            length = 1.0;
        }
        length *= d->m_lengthScale;

        // Calculate rotation (reuse Arrow::calcRotation algorithm)
        Triple axis;
        double angle = calcRotation(axis, FreeVector(vec.base, vec.base + dir));

        glPushMatrix();
        glTranslated(vec.base.x, vec.base.y, vec.base.z);
        if (angle > 0) glRotated(angle * 180.0 / PI, axis.x, axis.y, axis.z);

        RGBA c = d->m_magnitudeColorMapping
            ? (*col)(dir.length(), 0, 0)
            : (*col)(vec.base);
        glColor4d(c.r, c.g, c.b, c.a);

        // Draw stem (cylinder)
        gluCylinder(quadric, d->m_relStemRadius * length,
                    d->m_relStemRadius * length,
                    length * (1 - d->m_relConeLength),
                    d->m_arrowQuality, 1);
        // Draw cone
        glTranslated(0, 0, length * (1 - d->m_relConeLength));
        gluCylinder(quadric, d->m_relConeRadius * length, 0,
                    length * d->m_relConeLength,
                    d->m_arrowQuality, 1);
        glPopMatrix();
    }

    gluDeleteQuadric(quadric);
}
```

### 7. Simple Lines 模式

对于不需要箭头几何体的快速预览模式，使用 `GL_LINES` 绘制简单线段：

```cpp
// Each vector is a single line from base to base+direction
// Optionally with a point at the tip to indicate direction
```

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **示例验证**：运行 `examples/3D/vectorfield`，展示：
   - 电偶极子电场（解析公式）
   - 磁场（毕奥-萨伐尔定律）
   - 流体速度场（涡旋）
   - 切换 Arrows/Lines 渲染模式
   - 切换归一化开/关
   - 切换大小颜色映射开/关
3. **性能验证**：渲染 50x50x10 = 25000 个箭头，VBO 实例化模式帧率 > 30fps
4. **方向正确性验证**：验证箭头方向与向量方向一致（特别检查任意方向向量，不仅限于轴向）
5. **回退验证**：在兼容上下文下验证 GLU 二次曲面 immediate mode 渲染正常
6. **颜色映射验证**：验证向量大小正确映射到颜色

---

## 风险与注意事项

1. **旋转计算**：从 z 轴方向旋转到任意向量方向的旋转矩阵计算必须正确。已有 `Arrow::calcRotation()` 实现可参考，但在着色器中需要用矩阵而非 GLU。注意零向量和近零向量的处理。

2. **实例化渲染支持**：`glVertexAttribDivisor` 和 `glDrawArraysInstanced` 需要 OpenGL 3.3+。在 `RenderContext` 中检测支持情况，不支持时回退到逐箭头绘制。

3. **Immediate mode 回退不使用 GLU**：`gluCylinder`/`gluDisk` 在 Core Profile 下不可用，且在某些平台（如 Wayland/移动端）可能缺失 GLU 库。immediate mode 回退应使用**手动构建的箭头几何体**（与 VBO 模式共享 `buildUnitArrow()` 生成的顶点数据，仅渲染方式不同——VBO 模式用 `glDrawArraysInstanced`，immediate mode 用逐实例 `glBegin/glEnd`）。

4. **向量密度**：高密度向量场（如 100x100x100 = 1M 向量）会导致视觉混乱和性能问题。应支持降采样（类似 `SurfacePlot::setResolution()`）。

5. **箭头几何质量 vs 性能**：`arrowQuality`（圆周段数）越高渲染质量越好但顶点数越多。默认 16 段，10K 箭头 × 16 段 × 2(杆+锥) × 2(顶+底) = 640K 顶点，VBO 可处理但需注意内存。

6. **与 Arrow Enrichment 的关系**：Arrow 是 VertexEnrichment（在每个顶点调用 `draw()`），适用于在表面图上叠加向量。VectorFieldPlot 是独立的绘图类型，适用于纯向量场可视化。两者可以共存：在表面图上用 Arrow enrichment 标注梯度方向，同时用 VectorFieldPlot 展示独立的向量场。

7. **向量缩放**：向量长度可能远大于或远小于坐标范围。`lengthScale` 参数允许用户调整向量显示长度。默认值 1.0 表示按原始长度绘制，0.1 表示缩小 10 倍。

8. **Streamlines 模式委托**：`VectorRenderMode::Streamlines` 模式委托给 plan07 的 StreamlinePlot 实现。**初始版本不包含此枚举值**——在 plan07 实现完成后再添加，避免暴露未实现的功能。
