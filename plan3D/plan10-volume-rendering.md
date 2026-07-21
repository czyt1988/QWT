请先阅读 `README.md` 以了解全局规范。

# plan10 — 体积渲染 (VolumePlot)

## 目标

实现 `VolumePlot` 类（当前为 stub，头文件标注 "TODO: not yet fully implemented"）。体积渲染通过 GPU 光线投射（ray casting）着色器直接渲染 3D 标量场，支持传输函数（标量→颜色+不透明度）、多种渲染模式（最大密度投影、前后合成、等值面模式）。这是医学影像和科学体数据可视化的核心技术。

## 前置依赖

- **plan00**（渲染基础设施）：**强依赖**——体积渲染必须使用 GPU 着色器，固定管线无法实现
- 复用 plan08 的 `ScalarField`/`GridScalarField` 抽象接口

## 涉及文件

### 新建文件

| 文件 | 说明 |
|------|------|
| `src/plot3d/qwt3d_volumeplot.cpp` | VolumePlot 类实现（当前仅有 .h stub） |
| `src/plot3d/qwt3d_volumeplot_p.h` | PIMPL 私有数据 |
| `src/plot3d/qwt3d_transferfunction.h` | 传输函数（标量→颜色+不透明度） |
| `src/plot3d/qwt3d_transferfunction.cpp` | 传输函数实现 |
| `examples/3D/volumeplot/volumeplot.cpp` | 示例程序 |
| `examples/3D/volumeplot/CMakeLists.txt` | 示例构建配置 |

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/qwt3d_volumeplot.h` | 扩展 public/protected API 声明（当前为空 stub） |
| `src/plot3d/CMakeLists.txt` | 添加新源文件 |

---

## 技术方案

### 1. 渲染原理 — GPU 光线投射

体积渲染使用片段着色器在 GPU 上执行光线投射算法：

```
1. 渲染包围盒的前面和后面到两个 FBO（或使用单 pass 着色器计算射线方向）
2. 对于每个像素，从前面到后面发射一条光线穿过体数据
3. 沿光线方向等步长采样 3D 纹理
4. 对每个采样点应用传输函数（标量→颜色+不透明度）
5. 根据渲染模式合成颜色:
   - 最大密度投影 (MIP): 取最大标量值
   - 前后合成: 从前向后累积颜色
   - 等值面: 仅在标量值接近 iso 值时着色
6. 输出最终像素颜色
```

### 2. 类设计

```cpp
namespace Qwt3D {

/// @brief Volume rendering mode
enum class VolumeRenderMode {
    MaximumIntensity,  ///< Maximum Intensity Projection (MIP)
    Composite,         ///< Front-to-back compositing
    Isosurface         ///< Isosurface rendering (semi-transparent)
};

/// @brief Transfer function node
struct TransferNode {
    double scalarValue;  ///< Scalar field value
    RGBA color;          ///< Color and opacity at this value
};

/// @brief 3D volume rendering widget
class QWT3D_EXPORT VolumePlot : public Plot3D
{
    Q_OBJECT

public:
    explicit VolumePlot(QWidget* parent = nullptr);
    ~VolumePlot() override;

    /// @brief Set volume data from a 3D grid
    /// @param origin  Volume origin (min x, y, z)
    /// @param spacing Voxel spacing (dx, dy, dz)
    /// @param dims    Grid dimensions (nx, ny, nz)
    /// @param data    Scalar values, size = nx*ny*nz
    void setData(Triple origin, Triple spacing, Triple dims,
                 std::vector<double> const& data);

    /// @brief Set volume data from a ScalarField (will be sampled)
    /// @note VolumePlot holds a shared_ptr to ensure field lifetime during sampling
    void setData(std::shared_ptr<ScalarField> field, unsigned resolution);

    /// @brief Set render mode
    void setRenderMode(VolumeRenderMode mode);
    VolumeRenderMode renderMode() const;

    /// @brief Set transfer function (color + opacity mapping)
    void setTransferFunction(std::vector<TransferNode> const& nodes);
    std::vector<TransferNode> transferFunction() const;

    /// @brief Set sampling step size (ray marching step)
    void setSamplingStep(double step);
    double samplingStep() const;

    /// @brief Set isovalue for Isosurface mode
    void setIsovalue(double value);
    double isovalue() const;

    /// @brief Set overall density scale (multiplier for opacity)
    void setDensityScale(double scale);
    double densityScale() const;

protected:
    QWT_DECLARE_PRIVATE(VolumePlot)

    void calculateHull() override;
    void createData() override;

private:
    /// @brief Upload volume data to 3D texture
    void uploadVolumeTexture();

    /// @brief Build and compile the ray casting shader
    bool createRayCastShader();

    /// @brief Render the volume bounding box geometry
    void renderVolume();
};

}  // namespace Qwt3D
```

### 3. 3D 纹理管理

体积数据存储在 OpenGL 3D 纹理中：

```cpp
void VolumePlot::uploadVolumeTexture()
{
    QWT_D(d);

    // Store original data range for transfer function mapping
    d->m_dataMin = *std::min_element(d->m_data.begin(), d->m_data.end());
    d->m_dataMax = *std::max_element(d->m_data.begin(), d->m_data.end());
    double range = d->m_dataMax - d->m_dataMin;

    // Upload raw data as GL_R32F (32-bit float) to preserve precision
    // The transfer function handles the value mapping in the shader
    std::vector<GLfloat> floatData;
    floatData.reserve(d->m_data.size());
    for (double v : d->m_data) {
        floatData.push_back(static_cast<GLfloat>(v));
    }

    // Create 3D texture
    glGenTextures(1, &d->m_volumeTexture);
    glBindTexture(GL_TEXTURE_3D, d->m_volumeTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F,
                 d->m_nx, d->m_ny, d->m_nz, 0,
                 GL_RED, GL_FLOAT, floatData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Pass data range to shader as uniforms
    // uDataMin and uDataMax allow the shader to normalize sampled values
    // to [0,1] for transfer function lookup
}
```

### 4. 传输函数

传输函数将标量值映射到颜色和不透明度。在着色器中实现为 1D 查找纹理或解析函数：

```cpp
void VolumePlot::setTransferFunction(std::vector<TransferNode> const& nodes)
{
    QWT_D(d);
    d->m_transferNodes = nodes;

    // Build 1D lookup texture (256 entries)
    std::vector<GLfloat> lut(256 * 4);  // RGBA
    for (size_t i = 0; i < 256; ++i) {
        double value = static_cast<double>(i) / 255.0;
        // Interpolate between transfer function nodes
        RGBA color = interpolateTransfer(nodes, value);
        lut[i*4+0] = static_cast<GLfloat>(color.r);
        lut[i*4+1] = static_cast<GLfloat>(color.g);
        lut[i*4+2] = static_cast<GLfloat>(color.b);
        lut[i*4+3] = static_cast<GLfloat>(color.a);
    }

    // Upload to 1D texture
    glGenTextures(1, &d->m_transferTexture);
    glBindTexture(GL_TEXTURE_1D, d->m_transferTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, 256, 0, GL_RGBA, GL_FLOAT, lut.data());
    glBindTexture(GL_TEXTURE_1D, 0);
}
```

### 5. 光线投射着色器

#### 顶点着色器

渲染包围盒的 12 个三角形（立方体 6 面），在顶点着色器中传递世界坐标和眼坐标：

```glsl
#version 330 core
layout(location = 0) in vec3 aPosition;  // Bounding box vertex

uniform mat4 uModelView;
uniform mat4 uProjection;

out vec3 vWorldPos;     // Position in volume texture space
out vec3 vEyePos;       // Position in eye space

void main() {
    vWorldPos = aPosition;  // Assume vertices are in [0,1] texture space
    vec4 eyePos = uModelView * vec4(aPosition, 1.0);
    vEyePos = eyePos.xyz;
    gl_Position = uProjection * eyePos;
}
```

#### 片段着色器（核心 — 光线投射）

```glsl
#version 330 core
in vec3 vWorldPos;
in vec3 vEyePos;

uniform sampler3D uVolumeTex;    // 3D scalar field texture
uniform sampler1D uTransferTex;  // 1D transfer function texture
uniform vec3 uVolumeOrigin;      // Volume origin in world space
uniform vec3 uVolumeScale;        // Volume scale (size)
uniform float uSamplingStep;      // Ray marching step
uniform int uRenderMode;          // 0=MIP, 1=Composite, 2=Isosurface
uniform float uIsovalue;          // For Isosurface mode
uniform float uDensityScale;      // Opacity multiplier
uniform float uDataMin;           // Original data minimum (for transfer function)
uniform float uDataMax;           // Original data maximum (for transfer function)
uniform mat4 uModelView;
uniform mat4 uInverseModelView;   // Precomputed inverse (avoid per-fragment inverse)           // For computing ray direction

out vec4 fragColor;

void main() {
    // Convert world position to texture coordinates [0,1]
    vec3 texCoord = (vWorldPos - uVolumeOrigin) / uVolumeScale;

    // Compute ray direction (from eye to fragment)
    // uInverseModelView is precomputed on CPU to avoid per-fragment matrix inverse
    vec3 eyePos = (uInverseModelView * vec4(0,0,0,1)).xyz;
    vec3 rayDir = normalize(vWorldPos - eyePos);

    // Compute entry and exit points (ray-box intersection)
    // For front-facing fragments: entry = texCoord
    // For back-facing fragments: exit = texCoord
    // Simplified: march from front to back

    vec4 finalColor = vec4(0.0);
    float maxIntensity = 0.0;

    // March along the ray
    for (float t = 0.0; t < 1.0; t += uSamplingStep) {
        vec3 samplePos = texCoord + t * rayDir;

        // Check bounds
        if (any(lessThan(samplePos, vec3(0.0))) ||
            any(greaterThan(samplePos, vec3(1.0))))
            break;

        // Sample scalar field and normalize to [0,1] for transfer function
        float rawScalar = texture(uVolumeTex, samplePos).r;
        float scalar = (uDataMax > uDataMin)
            ? (rawScalar - uDataMin) / (uDataMax - uDataMin)
            : 0.5;

        if (uRenderMode == 0) {
            // Maximum Intensity Projection
            maxIntensity = max(maxIntensity, scalar);
        }
        else if (uRenderMode == 1) {
            // Front-to-back compositing
            vec4 tfColor = texture(uTransferTex, scalar);
            tfColor.a *= uDensityScale * uSamplingStep;
            // Under operator: C_dst += (1 - A_dst) * C_src
            finalColor.rgb += (1.0 - finalColor.a) * tfColor.a * tfColor.rgb;
            finalColor.a += (1.0 - finalColor.a) * tfColor.a;
            if (finalColor.a > 0.99) break;  // Early ray termination
        }
        else if (uRenderMode == 2) {
            // Isosurface mode
            if (abs(scalar - uIsovalue) < 0.01) {
                vec4 tfColor = texture(uTransferTex, scalar);
                finalColor = tfColor;
                break;
            }
        }
    }

    if (uRenderMode == 0) {
        // MIP output
        vec4 color = texture(uTransferTex, maxIntensity);
        fragColor = vec4(color.rgb, 1.0);
    } else {
        fragColor = finalColor;
    }
}
```

### 6. 渲染流程

```
paintGL():
  1. 清除帧缓冲
  2. 应用视角变换（旋转/平移/缩放）
  3. 绑定光线投射着色器
  4. 设置 uniform:
     - uModelView, uProjection: 当前变换矩阵
     - uVolumeTex: 3D 纹理 (unit 0)
     - uTransferTex: 1D 传输函数纹理 (unit 1)
     - uSamplingStep: 0.01 (100 步)
     - uRenderMode: 当前模式
  5. 渲染包围盒立方体（12 个三角形）
     - 禁用背面剔除（需要从两侧看到包围盒内部）
     - 启用深度测试
  6. 绘制坐标系、标题等
```

### 7. 与固定管线的兼容问题

**体积渲染无法用固定管线实现**。光线投射是 GPU 着色器技术，依赖 `texture3D` 采样和循环逻辑，这些在固定管线中不存在。

因此 VolumePlot **要求** Core Profile 或至少 OpenGL 3.3+。在不支持着色器的环境（如旧硬件或软件渲染）下，VolumePlot 应：
1. 显示错误消息
2. 回退为等值面渲染（委托给 IsosurfacePlot，plan09）

```cpp
void VolumePlot::createData()
{
    auto& rc = RenderContext::instance();
    if (!rc.hasShaders() || !rc.canUseModernPipeline()) {
        qWarning("VolumePlot: GPU shaders not available, falling back to isosurface rendering");
        renderFallback();  // Use Marching Cubes to extract a visible surface
        return;
    }
    uploadVolumeTexture();
    renderVolume();
}
```

### 8. 传输函数预设

提供常见预设：

| 预设 | 说明 | 适用场景 |
|------|------|---------|
| Grayscale | 灰度线性映射 | 通用 |
| Heat | 蓝→绿→黄→红 | 温度场 |
| Bone | 白色不透明骨骼 | CT 骨骼 |
| MRI | 软组织色 | MRI |
| Rainbow | 彩虹色 | 通用标量 |

---

## 验证方式

1. **编译验证**：`.\build.ps1 build`
2. **GPU 验证**：确认运行环境支持 OpenGL 3.3+ 和 3D 纹理
3. **示例验证**：运行 `examples/3D/volumeplot`，展示：
   - 合成体数据（球体 + 噪声）
   - 切换 MIP/Composite/Isosurface 模式
   - 调整传输函数（预设切换）
   - 调整采样步长（0.005-0.05）
   - 调整密度缩放
4. **性能验证**：128³ 体数据，Composite 模式帧率 > 20fps（取决于 GPU）
5. **正确性验证**：MIP 模式下球体体积应显示为最大值球面
6. **回退验证**：在不支持着色器的环境下验证回退到等值面渲染

---

## 风险与注意事项

1. **GPU 依赖**：VolumePlot 是唯一**无法回退到固定管线**的绘图类型。在不支持现代 OpenGL 的环境中，只能回退到 IsosurfacePlot（plan09）的等值面提取。这是架构上的根本限制。

2. **3D 纹理支持**：3D 纹理 (`GL_TEXTURE_3D`) 在 OpenGL 1.2+ 可用，但在某些移动 GPU 或软件渲染器中可能不支持。需在 `RenderContext` 中检测。

3. **光线投射性能**：片段着色器中的光线投射循环是性能瓶颈。100 步 × 全屏像素数 = 大量纹理采样。优化措施：
   - 早终止（`if (finalColor.a > 0.99) break`）
   - 降低采样步长（牺牲质量换性能）
   - 降低渲染分辨率（离屏渲染到低分辨率 FBO，再放大）
   - 空空间跳过（empty space skipping，检查光线是否经过空区域）

4. **包围盒光线入口/出口计算**：必须正确计算光线与体数据包围盒的交点（ray-AABB intersection）。推荐使用 **slab method**：

    ```glsl
    // Ray-box intersection (slab method) — compute tNear and tFar
    vec3 invDir = 1.0 / rayDir;
    vec3 t0 = (vec3(0.0) - texCoord) * invDir;
    vec3 t1 = (vec3(1.0) - texCoord) * invDir;
    vec3 tMin = min(t0, t1);
    vec3 tMax = max(t0, t1);
    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar  = min(min(tMax.x, tMax.y), tMax.z);
    if (tNear > tFar) discard;  // Ray misses the box
    tNear = max(tNear, 0.0);    // Clamp to near plane

    // March from tNear to tFar
    for (float t = tNear; t < tFar; t += uSamplingStep) { ... }
    ```

    初始版本可使用简化实现（仅渲染包围盒前面，从 texCoord 开始步进），但 slab method 在旋转视角下渲染正确性更好。

5. **纹理内存**：128³ × 4 bytes (GLfloat) = 8MB，256³ = 64MB。需检查 GPU 纹理内存限制。

6. **传输函数交互**：未来可考虑提供交互式传输函数编辑器（拖动控制点调整颜色和不透明度曲线），但不在本计划范围内。

7. **与 IsosurfacePlot 的关系**：VolumePlot 可以视为 IsosurfacePlot 的"体渲染"版本。两者都处理 3D 标量场，但：
   - IsosurfacePlot：提取等值面三角网格 → 表面渲染
   - VolumePlot：直接在 GPU 上光线投射 → 体渲染
   VolumePlot 的 Isosurface 模式与 IsosurfacePlot 的结果类似，但实现方式完全不同。

8. **多通道渲染**：高级实现使用双 pass（先渲染后面到纹理，再从前面发射光线使用后面的退出点）。初始版本可使用单 pass 简化实现（在片段着色器中计算光线与包围盒的交点）。

9. **Core Profile 下不可用 glDisable(GL_LIGHTING) 等**：VolumePlot 不使用固定管线光照，而是通过着色器自行计算颜色。因此不需要调用 `glEnable(GL_LIGHTING)` 等。
