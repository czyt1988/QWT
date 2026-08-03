# 08-plan: CoordinateSystem / Axis / Label / ColorLegend 现代化

> **前置阅读**：执行本任务前，必须完整阅读 `plan3D/ARCHITECTURE-PRINCIPLES.md`，特别是第五章「OpenGL 技术规范」（5.2 禁止 Legacy API、5.3 必须 Modern API、5.4 渲染管线）和第九章「不可触碰的约束」。

## 目标（Objective）

将 `Qwt3DDrawable`、`Qwt3DAxis`、`Qwt3DLabel`、`Qwt3DCoordinateSystem`、`Qwt3DColorLegend` 中的所有 legacy OpenGL 调用（`glBegin/glEnd`、`glPushMatrix/glPopMatrix`、`glBlendFunc`、`glLineSmooth` 等）替换为 VBO/VAO + GLSL shader 渲染。同时删除 `Qwt3DDrawable::saveGLState()` / `restoreGLState()`。

## 涉及文件

| 文件 | 操作 |
|------|------|
| `qwt3d_drawable.h` / `.cpp` | 修改：删除 saveGLState/restoreGLState，简化 draw() |
| `qwt3d_axis.h` / `.cpp` | 修改：替换 legacy GL 为 VBO/VAO |
| `qwt3d_label.h` / `.cpp` | 修改：替换 legacy GL |
| `qwt3d_coordsys.h` / `.cpp` | 修改：替换 legacy GL |
| `qwt3d_colorlegend.h` / `.cpp` | 修改：替换 legacy GL |
| `qwt3d_enrichment_std.h` / `.cpp` | 修改：替换 legacy GL（CrossHair/Dot/Cone/Arrow） |
| `qwt3d_openglhelper.h` | **删除**：最终删除此文件（Plan 01 已去命名空间，本任务中随各调用方一起重构后删除） |
| `qwt3d_gridplot.cpp` | 修改：移除 GLStateBewarer 使用 |
| `qwt3d_meshplot.cpp` | 修改：移除 GLStateBewarer 使用 |
| `qwt3d_plot.cpp` | 修改：移除 GLStateBewarer 使用 |
| `qwt3d_types.h` | 修改：移除 `#include "qwt3d_openglhelper.h"` |
| `qwt3d_lighting.cpp` | 修改：确认 Plan 05 已处理（移除 glLightfv/glMaterialfv/glRotatef/glPushMatrix/glPopMatrix 等 legacy GL 调用，仅保留参数存储） |
| `qwt3d_io_gl2ps.h` / `qwt3d_io_gl2ps.cpp` | 修改：setDeviceLineWidth/setDevicePointSize/drawDevicePixels/drawDeviceText/setDevicePolygonOffset 函数的处理 |
| `src/plot3d/shaders/` | **新建**：通用 shader 文件 |
| `CMakeLists.txt` | 修改：添加 shader 资源 |

## 前提条件（Prerequisites）

- Task 01–07 已完成
- `Qwt3DPlot` 已使用 `QMatrix4x4` CPU 端矩阵
- 构建通过

## 执行步骤（Steps）

**步骤依赖顺序：**
1. 先创建 shader 文件（步骤 1）
2. 提供 ViewPort2World/World2ViewPort 替代方案（步骤 1.5）
3. 重写 Drawable（步骤 2）— 删除旧方法前先提供替代
4. 逐个重写组件（步骤 3-7）
5. 更新 Qwt3DPlot paintGL（步骤 8）
6. 清理 GLStateBewarer 使用（步骤 8.5）
7. 删除 qwt3d_openglhelper.h（最后执行）

### 1. 创建通用 Shader

在 `src/plot3d/shaders/` 目录下创建：

**`line.vert`** — 通用线条/轴线顶点着色器：
```glsl
#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
uniform mat4 uModelView;
uniform mat4 uProjection;
out vec4 vColor;
void main() {
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
    vColor = aColor;
}
```

**`line.frag`** — 通用线条/轴线片元着色器：
```glsl
#version 330
in vec4 vColor;
out vec4 fragColor;
uniform float uLineWidth;
void main() {
    fragColor = vColor;
}
```

**`point.vert`** — 点渲染顶点着色器：
```glsl
#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
uniform mat4 uModelView;
uniform mat4 uProjection;
uniform float uPointSize;
out vec4 vColor;
void main() {
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
    gl_PointSize = uPointSize;
    vColor = aColor;
}
```

**`point.frag`** — 点渲染片元着色器：
```glsl
#version 330
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
```

**`polygon.vert`** — 多边形填充顶点着色器：
```glsl
#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
uniform mat4 uModelView;
uniform mat4 uProjection;
out vec4 vColor;
void main() {
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
    vColor = aColor;
}
```

**`polygon.frag`** — 多边形填充片元着色器：
```glsl
#version 330
in vec4 vColor;
out vec4 fragColor;
uniform float uAlpha;
void main() {
    fragColor = vec4(vColor.rgb, vColor.a * uAlpha);
}
```

**`text.vert`** — 文本渲染顶点着色器：
```glsl
#version 330
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uModelView;
uniform mat4 uProjection;
out vec2 vTexCoord;
void main() {
    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
    vTexCoord = aTexCoord;
}
```

**`text.frag`** — 文本渲染片元着色器：
```glsl
#version 330
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D uTextTexture;
uniform vec4 uTextColor;
void main() {
    float alpha = texture(uTextTexture, vTexCoord).a;
    fragColor = vec4(uTextColor.rgb, uTextColor.a * alpha);
}
```

### 1.5. 提供坐标转换替代方案

`ViewPort2World()` / `World2ViewPort()` 在 `Axis::drawLabel()`、`Label::convert2Screen()`、`Label::setRelPosition()`、`CoordinateSystem::chooseAxes()`、`CoordinateSystem::autoDecorateExposedAxis()`、`ColorLegend::setGeometryInternal()` 中被大量调用。

**替代方案：**
- 在 `Qwt3DPlot` 中提供 `QPointF worldToScreen(const Triple& world) const` 和 `Triple screenToWorld(const QPointF& screen) const` 方法
- 使用 `QMatrix4x4::map()` / `QMatrix4x4::inverted().map()` 实现
- 在 `Qwt3DDrawable` 基类中添加 `Qwt3DPlot* plot() const` 访问器（或通过现有机制让 Drawable 子类获取变换矩阵）
- 列出所有调用点及其替换方式：
  - `ViewPort2World(x, y, z)` → `plot()->screenToWorld(QPointF(x, y))`（注意参数语义可能反转）
  - `World2ViewPort(x, y, z)` → `plot()->worldToScreen(Triple(x, y, z))`

### 2. 重写 Qwt3DDrawable

- **删除** `saveGLState()` 和 `restoreGLState()` 及其所有 GL 状态保存/恢复逻辑
- **删除** `modelMatrix[16]`、`projMatrix[16]`、`viewport[4]` 成员
- **删除** `ViewPort2World()` / `World2ViewPort()` 方法（legacy gluUnProject/gluProject）
- `draw()` 简化为：遍历子 Drawable 调用 `draw()`，无 GL 状态保存
- `attach()` / `detach()` / `detachAll()` / `setColor()` 保持不变
- 删除 `qwt3d_openglhelper.h` 文件（Plan 01 中已去命名空间但保留内容，本任务中所有调用方已完成现代化，文件可以删除）。从 CMakeLists.txt 和所有 #include 中移除。

### 3. 重写 Qwt3DAxis

原 `draw()` 中使用 `glBegin(GL_LINES)` 逐线段绘制轴线和刻度。改为：
- 收集所有线段顶点（轴主线 + 主刻度 + 次刻度）到 `QVector<float>`
- 上传到 VBO
- 使用 `line.vert` / `line.frag` shader
- `glDrawArrays(GL_LINES, 0, lineVertexCount)`

数字标签和轴标签继续使用 `Qwt3DLabel::draw()`（下一步现代化）。

### 4. 重写 Qwt3DLabel

原 `draw()` 中使用 `glRasterPos3d` + `glDrawPixels`（CPU→GPU 逐帧传输像素数据）绘制文本。改为：
- 文本渲染到 `QImage` → `QOpenGLTexture`（一次性上传，缓存复用）
- 使用 `text.vert` / `text.frag` shader
- 用 VBO 定义四边形顶点 + 纹理坐标
- `glDrawArrays(GL_TRIANGLE_STRIP, 0, 4)` 绘制纹理四边形

### 5. 重写 Qwt3DCoordinateSystem

原 `draw()` 中使用 `glBegin(GL_LINES)` 绘制网格线。改为：
- 收集所有网格线段顶点
- 上传到 VBO
- 使用 line shader 绘制
- 遍历 12 个 `Qwt3DAxis` 调用各自的 `draw()`

### 6. 重写 Qwt3DColorLegend

原 `draw()` 中使用 `glBegin(GL_POLYGON)` 逐色块绘制。改为：
- 收集所有色块四边形顶点 + 颜色
- 上传到 VBO
- 使用 polygon shader 绘制
- 刻度线使用 line shader

### 7. 重写 Enrichment 标准实现

`Qwt3DCrossHair`、`Qwt3DDot`：
- `glBegin(GL_LINES)` → VBO + line shader
- `glBegin(GL_POINTS)` → VBO + point shader

`Qwt3DCone`、`Qwt3DArrow`：
- 原 `gluCylinder` / `gluDisk`（GLU quadric 对象）→ 需 CPU 端生成圆锥/圆柱三角网格
- 圆锥几何生成算法：以底面圆心为原点，绕 Y 轴生成 n 个三角形扇
  ```
  for (int i = 0; i < segments; i++) {
      float angle1 = 2 * PI * i / segments;
      float angle2 = 2 * PI * (i+1) / segments;
      // 三角形: apex, base[i], base[i+1]
  }
  ```
- 圆柱几何生成：上下两个圆环 + 侧面三角带
- Arrow 的 `glPushMatrix/glTranslatef/glRotatef` → 通过 `QMatrix4x4` 计算 model matrix，作为 uniform 传入

### 8. 更新 Qwt3DPlot::paintGL

确保 `paintGL()` 中：
- 在调用 `coordinates()->draw()` / `legend()->draw()` 之前设置好当前 shader 的 uniform（modelView, projection）
- 或让每个 Drawable 自己管理 shader bind/uniform set

**推荐方案**：`Qwt3DPlot` 持有共享的通用 shader（line/point/polygon/text），通过 `plot()->lineShader()` 等接口提供给 item 和 drawable 使用。

**共享 shader API：**
在 `Qwt3DPlot` 中定义：
```cpp
public:
    QOpenGLShaderProgram* lineShader() const;
    QOpenGLShaderProgram* pointShader() const;
    QOpenGLShaderProgram* polygonShader() const;
    QOpenGLShaderProgram* textShader() const;
private:
    std::unique_ptr<QOpenGLShaderProgram> m_lineShader;
    std::unique_ptr<QOpenGLShaderProgram> m_pointShader;
    std::unique_ptr<QOpenGLShaderProgram> m_polygonShader;
    std::unique_ptr<QOpenGLShaderProgram> m_textShader;
```
在 `initializeGL()` 中编译并缓存 shader。
Drawable 子类通过 `plot()->lineShader()` 等接口获取共享 shader。
需在 `Qwt3DDrawable` 基类中添加 `Qwt3DPlot* plot() const` 方法。

**qwt3d_io_gl2ps.h/.cpp 中的 setDevice* 函数处理：**
- `setDeviceLineWidth()` / `setDevicePointSize()`：Core Profile 中 glLineWidth > 1.0 不被保证支持。暂时移除 GL 调用，存储为成员变量，通过 shader uniform 传入（TODO）
- `drawDevicePixels()`：替换为纹理四边形方案
- `drawDeviceText()`：替换为 Qwt3DLabel 的纹理渲染方案
- `setDevicePolygonOffset()`：Core Profile 中可用 `glPolygonOffset`，保留
- 这些函数被 qwt3d_drawable.cpp、qwt3d_axis.cpp、qwt3d_coordsys.cpp、qwt3d_enrichment_std.cpp、qwt3d_label.cpp 调用，现代化后调用方应直接使用 shader uniform

**CMake 和资源文件：**
- 创建 `src/plot3d/shaders/shaders.qrc`：
```xml
<RCC>
  <qresource prefix="/shaders">
    <file>line.vert</file>
    <file>line.frag</file>
    <file>point.vert</file>
    <file>point.frag</file>
    <file>polygon.vert</file>
    <file>polygon.frag</file>
    <file>text.vert</file>
    <file>text.frag</file>
  </qresource>
</RCC>
```
- 在 CMakeLists.txt 中添加 .qrc 文件到 target sources
- 从 CMakeLists.txt header 列表中移除 qwt3d_openglhelper.h

**线宽处理策略：**
Core Profile 中 `glLineWidth` 仅保证支持 1.0。轴线和网格线的宽度差异需要替代方案：
- 方案 A：使用 geometry shader 生成屏幕空间四边形条带（复杂但效果好）
- 方案 B：暂时使用 `glLineWidth`（大部分驱动仍支持，但不保证）
- 推荐：暂时使用方案 B，标注 TODO 后续优化

### 8.5. 清理 GLStateBewarer 使用

在删除 `qwt3d_openglhelper.h` 之前，需要先清理以下文件中对 `GLStateBewarer` 的使用：

- `qwt3d_gridplot.cpp`（3 处）：`GLStateBewarer sb(GL_LINE_SMOOTH, true)` 等 — 移除这些使用，相关 GL 状态设置移到 shader 或 Qwt3DPlot 的 paintGL 中
- `qwt3d_meshplot.cpp`（2 处）：同上
- `qwt3d_plot.cpp`（2 处）：同上

注意：如果 Plan 05/06 已将这些文件中的相关代码注释掉或删除（因为 SurfacePlot 变为空壳），则这些 GLStateBewarer 引用可能已经不存在。执行前先验证这些文件是否仍包含 GLStateBewarer 调用。

注意：`qwt3d_lighting.cpp` 中的 legacy GL 调用已由 Plan 05 步骤 6 处理（enableLighting/setMaterialComponent/setLightComponent/setShininess 中移除 GL 调用）。本步骤需确认 Plan 05 已完成此处理。

同时检查 `qwt3d_io_gl2ps.cpp` 中的 `#include "qwt3d_openglhelper.h"`（第 6 行）——如果该文件仍在使用 openglhelper 中的函数（如 World2ViewPort 等），需在条件编译块内处理或移除 include。

同时修改 `qwt3d_types.h`：移除 `#include "qwt3d_openglhelper.h"` 行（Plan 01 中已处理 GL 类型依赖，将 GLdouble 替换为 double）。

## 构建通过

```powershell
.\build.ps1 rebuild -Examples OFF -Playground OFF
```

构建必须通过。3D 渲染窗口应能正确显示坐标轴、网格线、图例、标签。

## git 提交

```powershell
git add -A
git commit -m "refactor(plot3d): modernize CoordinateSystem/Axis/Label/ColorLegend/Enrichment to VBO/VAO + shaders

- Replace all glBegin/glEnd with VBO + glDrawArrays/glDrawElements
- Replace glPushMatrix/glRotatef/glTranslatef with QMatrix4x4 uniforms
- Replace glLightfv/glMaterialfv with shader uniforms
- Delete Drawable::saveGLState/restoreGLState (legacy GL state management)
- Delete Drawable::ViewPort2World/World2ViewPort (legacy gluProject/gluUnProject)
- Add generic GLSL shaders (line, point, polygon, text) in src/plot3d/shaders/
- Modernize CrossHair/Dot/Cone/Arrow enrichments"
```
