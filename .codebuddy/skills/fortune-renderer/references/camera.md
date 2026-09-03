# 模块：相机 Camera / 射线 Ray

文件：`source/Camera.h`、`source/Camera.cpp`、`source/Ray.h`

## Ray（`Ray.h`，纯结构体）

| 字段 | 含义 |
|------|------|
| `o` | 射线起点（相机位置） |
| `d` | 射线方向（单位向量） |
| `mint = 0.0f` / `maxt = FLT_MAX` | 有效参数区间，供求交测试使用 |

## Camera

### 成员

| 成员 | 含义 |
|------|------|
| `mPosition` | 相机世界位置 |
| `mCombinedMatrix` | viewport · projection · view（世界→像素） |
| `mInvCombinedMatrix` | 上述矩阵的逆（像素→世界），生成光线用 |

### `Initialize(p, target, up, fov, n, f, W, H)`

三个矩阵串联（左手坐标系 LH）：
1. **观察矩阵**：`glm::lookAtLH(p, target, up)`。注释中保留手写推导：l=观察方向、r=cross(up,l)、u=cross(l,r)，view = 转置基矩阵 · MakeTranslation(-p)。
2. **投影矩阵**：`glm::perspectiveFovLH_ZO(fov, (float)W, (float)H, n, f)`，fov 为弧度，ZO 表示深度范围 [0,1]。
3. **视口矩阵**（手写）：NDC [-1,1] → 像素 [0,W]×[0,H]，`-H/2` 使 Y 翻转（像素 y 轴朝下），平移 `(W/2, H/2)`。

合并：`mCombinedMatrix = viewportMatrix * projectionMatrix * viewMatrix`，并预计算逆矩阵。

需要包含 `<glm/ext/matrix_clip_space.hpp>`（perspectiveFovLH_ZO；本项目 GLM 为 0.9.9+，该函数不在 gtc 下而在 ext 下，include `<glm/gtc/matrix_clip_space.hpp>` 会报 C1083）。

### `Ray GetRay(int x, int y) const` / `Ray GetRay(float x, float y) const`

两个重载：整型版直接委托给浮点版（`GetRay((float)x, (float)y)`）。浮点版是实际实现，供 SSAA 在像素内随机采样亚像素坐标使用。

1. 起点 `ray.o = mPosition`；
2. 屏幕点 `p = (x, y, 0, 1)` 乘 `mInvCombinedMatrix`，除 w 得世界坐标；
3. `ray.d = normalize(worldPos - mPosition)`（穿过该（亚）像素位置的方向）。

## 设计说明

- 相机走"逆变换"路线：正向矩阵用于常规光栅化，光线追踪只依赖逆矩阵把像素反投影回世界空间。
- `Initialize` 显式传 W/H 而不是依赖 Renderer，相机与渲染器解耦。
