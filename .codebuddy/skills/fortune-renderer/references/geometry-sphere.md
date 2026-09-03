# 模块：几何求交 Sphere / Disk / Intersection / Ray 变换

文件：`source/Sphere.h`、`source/Sphere.cpp`、`source/Disk.h`、`source/Disk.cpp`、`source/Ray.h`

## Intersection（Ray.h 内结构体，供所有图元共用）

| 字段 | 含义 |
|------|------|
| `position` | 交点位置（世界空间） |
| `normal` | 交点法线（世界空间，已归一化） |
| `t` | 射线参数，交点到射线原点的距离 |

## Sphere

### 成员
- `mRadius`：球半径；球体建模在对象空间原点。
- 变换矩阵由所属 `SceneObject` 提供（`m_pSceneObject->Get*()`），自身不存矩阵。

### 构造 `Sphere(SceneObject*, R)`
球心 = SceneObject 的 position；矩阵的持有与管理见 references/geometry-primitive.md。

### `bool Intersect(Ray ray, Intersection& isect) const`
1. **空间变换**：`Ray r = m_pSceneObject->GetWorldToObject() * ray`，把射线转到球体局部空间（球心在原点，方程最简）。
2. **二次方程**：`A = dot(d,d)`，`B = 2·dot(d,o)`，`C = dot(o,o) − R²`；判别式 `delta = B² − 4AC < 0` 无交。
3. **取根**：`t1 ≤ t2`；`t2 < mint` 或 `t1 > maxt` 整段无效；否则优先取 `t1`，`t1 < mint` 时取 `t2`（从球内部射出的情形），`t2 > maxt` 也无效。
4. **填充结果**：局部交点 `p = o + t·d`，局部法线 `n = normalize(p)`（球心在原点所以法线=位置归一化）；位置用 `w=1`、法线用 `w=0` 变换回世界空间，法线再归一化。

### Ray 的矩阵变换（Ray.h 中 `operator*(const Matrix4x4&, const Ray&)`）
- 起点 `o` 按点变换（w=1），方向 `d` 按向量变换（w=0），mint/maxt 原样保留。
- 注意：变换后的 `d` 不再归一化（长度被矩阵缩放），此时 `t` 的语义变为"对象空间的距离"；本项目球体无非均匀缩放，t 在两个空间中一致。

## 设计说明

- 在局部空间求交是通用模式：任意形状只需在最简坐标系下写方程，复杂变换交给矩阵。
- `Intersect` 按值传 `Ray`（不修改调用方射线），结果通过引用 `isect` 输出。
