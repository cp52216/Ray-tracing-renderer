# 模块：几何求交 Triangle（三角形，Möller–Trumbore）

文件：`source/Triangle.h`、`source/Triangle.cpp`

## Triangle

### 成员
- `mVertices[3]`：三角形三个顶点，存储在**对象空间**（局部坐标）。
- 变换矩阵由所属 `SceneObject` 提供（`m_pSceneObject->GetObjectToWorld()`），自身不存矩阵和法线。

### 构造 `Triangle(SceneObject*, v0, v1, v2)`
顶点原样保存到对象空间；世界空间位置由 SceneObject 的 TRS 变换决定。

### `bool Intersect(Ray ray, Intersection& isect) const`
Möller–Trumbore 算法，世界空间计算（每次求交时把顶点用 `GetObjectToWorld()`（w=1）变换到世界空间，SceneObject 变化后仍正确）：

1. 预计算：`p0/p1/p2 = ObjectToWorld · (v, 1)`；`e1 = p1-p0`、`e2 = p2-p0`、`s = ray.o - p0`、`s1 = cross(d, e2)`、`s2 = cross(s, e1)`。
2. 行列式：`det = dot(s1, e1)`；`|det| < 1e-6` 说明射线与三角形所在平面平行，无交。
3. 求解：`invDet = 1/det`；`b1 = dot(s1, s)·invDet`（对应 e1 方向的重心分量）、`b2 = dot(s2, d)·invDet`（对应 e2 方向）、`t = dot(s2, e2)·invDet`。
4. 范围检查：`t < mint || t > maxt` 无效。
5. 内部检查：`b0 = 1 - b1 - b2`；任一重心分量为负 ⇒ 交点在三角形外。
6. 填充结果：`position = ray.o + t·ray.d`；法线 = 世界空间边向量叉积归一化（每次现算，方向依赖顶点绕序）；`isect.t = t`。

### 与 Sphere/Disk 的差异
| | Sphere/Disk | Triangle |
|---|---|---|
| 求交空间 | 对象局部空间 | 世界空间（顶点求交时变换） |
| 法线 | 局部解析式（位置归一化 / 固定 +Z） | 世界空间边向量叉积现算 |

## 设计说明

- Möller–Trumbore 通过 Cramer 法则联立求解射线方程与三角形所在平面，比"先解平面 t 再做重心坐标内测试"的写法少一次方程求解。
- 三角形是网格模型的基本单元，该实现是后续加载 obj 模型、构建 BVH 加速结构的基础。
- 教程中 `cross/dot` 为全局调用（依赖 ADL），实现中统一写 `glm::cross/glm::dot`。
