请先阅读 `README.md` 以了解全局规范。

# plan01 — 3个已确认Bug修复

## 目标

修复 3D 模块深度审阅中发现的 3 个 Bug。这些 Bug 影响光照系统正确性和 Qt5/Qt6 跨版本兼容性。

## 前置依赖

无。本计划完全独立，可随时执行。

## 涉及文件

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/plot3d/qwt3d_lighting.cpp` | 修复 Bug 1（blowout）和 Bug 2（setLightComponent 双重转换） |
| `src/plot3d/qwt3d_mousekeyboard.cpp` | 修复 Bug 3（鼠标/滚轮事件未使用 compat 层） |

---

## 技术方案

### Bug 1 — `blowout()` 无法关灯

#### 问题描述

文件：`src/plot3d/qwt3d_lighting.cpp`，约第 83-88 行

```cpp
void Plot3D::blowout(unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].unlit = false;  // BUG: 应为 true
}
```

`illuminate()` 设置 `unlit = false`（表示"未熄灭"= 点亮），`blowout()` 应设置 `unlit = true`（表示"已熄灭"= 关灯）。但当前代码写了 `false`，导致 `blowout()` 的实际效果与 `illuminate()` 相同——**无法关灯**。

#### 修复方案

将 `false` 改为 `true`：

```cpp
void Plot3D::blowout(unsigned light)
{
    QWT_D(d);
    if (light > 7)
        return;
    d->m_lights[ light ].unlit = true;  // FIX: mark as extinguished
}
```

#### 影响分析

- `blowout()` 被 `Plot3D::enableLighting(false)` 间接使用（通过主题系统）
- 当前主题系统只使用 light 0，且 `enableLighting(false)` 通过 `glDisable(GL_LIGHTING)` 全局关闭光照，所以此 Bug 在实际使用中影响有限
- 但如果用户手动调用 `blowout(1)` 试图关闭 1 号灯，将不起作用
- 修复后不影响任何现有行为（没有代码依赖 `blowout()` 的错误行为）

### Bug 2 — `setLightComponent(intensity, light)` 双重 lightEnum 转换

#### 问题描述

文件：`src/plot3d/qwt3d_lighting.cpp`，约第 130-140 行

存在两个 `setLightComponent` 重载：

```cpp
// 6 参数版（RGBA）— 接受 unsigned light，内部调用 lightEnum(light)
void Plot3D::setLightComponent(GLenum property, double r, double g, double b, double a, unsigned light)
{
    QWT_D(d);
    // ...
    glLightfv(lightEnum(light), property, data);
}

// 4 参数版（intensity）— BUG: 传入 lightEnum(light) 给 6 参数版
void Plot3D::setLightComponent(GLenum property, double intensity, unsigned light)
{
    setLightComponent(property, intensity, intensity, intensity, 1.0, lightEnum(light));
    //                                                                              ^^^^^^^^^^^^
    // 6 参数版内部会再次调用 lightEnum(0x4001+light)，因 0x4001 > 7，走 default 返回 GL_LIGHT0
}
```

`lightEnum()` 函数将 `unsigned` (0-7) 转换为 `GLenum` (GL_LIGHT0=0x4000 ... GL_LIGHT7=0x4007)。但 4 参数版传入的已经是 `GLenum` 值（如 GL_LIGHT1=0x4001），6 参数版内部再次调用 `lightEnum(0x4001)`，因 0x4001 > 7，走 default 分支返回 `GL_LIGHT0`。

**结果**：intensity 便捷重载对 light 1-7 全部错误地操作 GL_LIGHT0。

#### 影响面

主题系统 `Qwt3DTheme::apply()` 中的光照设置使用的是 intensity 重载（`qwt3d_theme.cpp` 约第 207-215 行）。但当前主题只用 light 0，所以暂未暴露。如果未来主题使用多光源，此 Bug 将导致 light 1-7 的光照参数全部错误设置到 light 0。

#### 修复方案

4 参数版直接传 `unsigned light`（不调用 `lightEnum`），让 6 参数版内部完成转换：

```cpp
void Plot3D::setLightComponent(GLenum property, double intensity, unsigned light)
{
    setLightComponent(property, intensity, intensity, intensity, 1.0, light);
    //                                                                              ^^^^
    // FIX: pass unsigned light directly, let 6-param overload handle lightEnum conversion
}
```

#### 验证

修复后，调用 `setLightComponent(GL_DIFFUSE, 0.5, 3)` 应正确设置 GL_LIGHT3 的漫反射强度为 0.5，而非错误地设置 GL_LIGHT0。

### Bug 3 — 鼠标/滚轮事件未使用 Qt5/Qt6 兼容层

#### 问题描述

文件：`src/plot3d/qwt3d_mousekeyboard.cpp`

项目中存在 Qt5/Qt6 兼容层（`qwt_qt5qt6_compat.hpp`，`qwt::compat::` 命名空间），但 3D 模块的鼠标/滚轮事件处理代码使用了旧式 Qt5 API，在 Qt6 下可能存在兼容性隐患（当前 Qt6 仍保留了这些旧 API 但标记为 deprecated，且行为可能有细微差异）。

需要修复的具体位置（需在修改前通过 grep 精确定位行号）：

**位置 1 — `mouseMoveEvent` 中的 `e->pos()`**：

```cpp
// 当前代码（Qt5 API）
QPoint diff = e->pos() - m_data->mousepos;
m_data->mousepos = e->pos();
```

应改为：

```cpp
// 修复后（兼容层）
QPoint diff = qwt::compat::eventPos(e) - m_data->mousepos;
m_data->mousepos = qwt::compat::eventPos(e);
```

**位置 2 — `mousePressEvent` 中的 `e->pos()`**：

```cpp
// 当前代码
m_data->mousepos = e->pos();
```

应改为：

```cpp
m_data->mousepos = qwt::compat::eventPos(e);
```

**位置 3 — `wheelEvent` 中的 `e->angleDelta().y()`**：

```cpp
// 当前代码
// 可能直接使用 e->angleDelta().y() 或类似写法
```

应改为使用 `qwt::compat::wheelEventDelta(e)`。

#### 修复步骤

1. 在 `qwt3d_mousekeyboard.cpp` 头部添加 include：
   ```cpp
   #include "qwt_qt5qt6_compat.hpp"
   ```

2. 搜索文件中所有 `e->pos()` 调用，替换为 `qwt::compat::eventPos(e)`

3. 搜索文件中所有 `e->angleDelta().y()` 或 `e->delta()` 调用，替换为 `qwt::compat::wheelEventDelta(e)`

4. 搜索 `mouseMoveEvent`、`mousePressEvent`、`mouseReleaseEvent`、`wheelEvent` 四个事件处理函数中的所有位置

#### 验证

- 在 Qt5 和 Qt6 下分别编译运行 `examples/3D/mesh2` 示例
- 测试鼠标拖动旋转、滚轮缩放是否正常工作
- 对比修复前后的行为，确保无回归

---

## 验证方式

### 编译验证

```powershell
.\build.ps1 build
```

### 功能验证

1. **Bug 1 验证**：
   - 运行 `examples/3D/mesh2`，打开光照对话框
   - 点亮 light 1，然后调用 `blowout(1)`，确认 light 1 正确熄灭
   - 修复前：`blowout(1)` 不起任何作用（light 1 仍然亮着）

2. **Bug 2 验证**：
   - 编写测试代码：`plot->setLightComponent(GL_DIFFUSE, 0.5, 3)`
   - 检查 GL_LIGHT3 的漫反射强度是否为 0.5（可通过 GL 状态查询或视觉对比）
   - 修复前：GL_LIGHT0 的漫反射被错误修改，GL_LIGHT3 不受影响

3. **Bug 3 验证**：
   - 在 Qt5.15 和 Qt6.8 下分别运行 `examples/3D/mesh2`
   - 测试鼠标旋转、缩放、平移交互
   - 确保两种 Qt 版本下行为一致

### 回归验证

- 运行所有 6 个 3D 示例（`simpleplot3D`、`autoswitch`、`axes`、`enrichments`、`figureSurface3D`、`mesh2`），确认无视觉差异或交互异常

---

## 风险与注意事项

1. **Bug 1 修复风险极低**：仅改一个布尔值，语义明确，不影响任何其他代码路径。

2. **Bug 2 修复风险低**：当前主题系统只用 light 0，修复不影响现有主题行为。但需确认没有其他代码依赖错误行为（grep `setLightComponent.*intensity` 检查所有调用点）。

3. **Bug 3 修复需全面搜索**：`qwt3d_mousekeyboard.cpp` 中可能有多处 `e->pos()` 调用，必须全部替换。遗漏任何一处都会导致交互不一致。建议使用 grep 搜索 `->pos()` 确保无遗漏。

4. **不要修改其他文件**：此计划仅修改 `qwt3d_lighting.cpp` 和 `qwt3d_mousekeyboard.cpp` 两个文件。不要在此次修复中引入其他改动。

5. **保持代码风格**：修改后的代码必须遵循项目的 `.clang-format` 格式（4空格缩进、WebKit 花括号风格、120列宽）。
