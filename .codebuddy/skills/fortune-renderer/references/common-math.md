# 模块：数学库 Common.h

文件：`source/Common.h`（全内联头文件，无 cpp）

## 类型别名

| 别名 | GLM 类型 | 用途 |
|------|----------|------|
| `Vector2f` / `Vector3f` / `Vector4f` | `glm::vec2/3/4` | 浮点向量（点、方向、颜色分量等） |
| `Vector2i` / `Vector3i` / `Vector4i` | `glm::ivec2/3/4` | 整数向量 |
| `Matrix3x3` | `glm::mat3` | 3x3 矩阵（坐标系变换等） |
| `Matrix4x4` | `glm::mat4` | 4x4 矩阵（仿射变换） |
| `Color` | `glm::vec3` | 线性 RGB 颜色，分量范围 0.0~1.0，用 `.r/.g/.b` 访问 |

包含头：`glm/glm.hpp`、`glm/gtc/matrix_transform.hpp`、`glm/gtc/constants.hpp`。

## 矩阵构造函数（全部 inline）

### `MakeTranslation(const Vector3f& t) -> Matrix4x4`
构造平移矩阵。GLM 列主序：构造参数每 4 个构成一列，平移量 `(t.x, t.y, t.z, 1)` 位于第 3 列。

### `MakeRotation(const Vector3f& euler) -> Matrix4x4`
构造欧拉角旋转矩阵（弧度）。先绕 X、再绕 Y、最后绕 Z（作用顺序 R = Rz·Ry·Rx，矩阵从右往左作用于列向量）。

### `MakeScale(const Vector3f& s) -> Matrix4x4`
构造缩放矩阵，对角线为 `(s.x, s.y, s.z, 1)`。

### `MakeWorld(position, euler, scale) -> Matrix4x4`
构造**对象空间 → 世界空间**的世界变换矩阵 M = T·R·S：作用于点时先缩放、再旋转、最后平移。
- `position`：对象在世界坐标系中的平移位置（世界空间）。
- `euler`：对象相对于世界坐标轴的欧拉角（弧度，世界空间下的朝向）。
- `scale`：对象在自身对象空间各轴上的缩放比例（对象空间）。

### `MakeWorldTransform(position, euler, scale) -> Matrix4x4`
`MakeWorld` 的便捷重载，`scale` 为 `float` 统一缩放因子，内部展开为 `Vector3f(scale)`。

### 路径追踪用工具函数
- `float Random01()`：线程安全的 `[0, 1)` 均匀随机数（`thread_local std::mt19937` + `uniform_real_distribution`）。
- `float Random(float a, float b)`：`[a, b)` 内的均匀随机数。
- `Vector3f GetSphericalCoordinate(theta, phi)`：球面坐标 → 单位向量，`z = cosθ`，适合蒙特卡洛半球采样。

### `MakeCoordinateSystem(const Vector3f& w) -> Matrix3x3`
给定单位向量 w（新坐标系 Z 轴），用叉积构造与其正交的 u、v，返回 3x3 矩阵（三列即新坐标系的 X/Y/Z 轴基向量）。该矩阵把新坐标系下的局部坐标变换到世界坐标。w 若与 X 轴 `(1,0,0)`近平行（dot > 0.99）则改用 Y 轴辅助，避免退化。

## 设计说明

- 全部函数 `inline`，可直接放头文件被多个 cpp 包含。
- 列主序约定：GLM 与 OpenGL 一致，`M[col][row]`，矩阵乘列向量实现变换。
