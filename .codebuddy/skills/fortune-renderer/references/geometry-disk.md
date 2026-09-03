# 模块：几何求交 Disk（圆盘）

文件：`source/Disk.h`、`source/Disk.cpp`

## Disk

### 成员
- `mRadius`：圆盘半径；圆盘建模在对象空间 **z=0 平面**，圆心在原点，法线为 +Z。
- 变换矩阵由所属 `SceneObject` 提供（经 `m_pSceneObject->Get*()` 获取），自身不存矩阵。

### 构造 `Disk(SceneObject*, radius)`
摆放（位置/欧拉角/缩放）全部由 SceneObject 承担；朝向由 SceneObject 的欧拉角决定，可把圆盘摆成任意朝向。

### `bool Intersect(Ray ray, Intersection& isect) const`
1. **空间变换**：`Ray r = m_pSceneObject->GetWorldToObject() * ray`，转到局部空间（此时圆盘就是 z=0 平面）。
2. **平行剔除**：`|r.d.z| < 1e-6` 时射线与平面平行，无交。
3. **求 t**：平面方程 z=0 ⇒ `t = -r.o.z / r.d.z`；`t < mint || t > maxt` 无效。
4. **半径裁剪**：局部交点 `p = o + t·d`；`dot(p,p) > R²` 说明交点在圆盘外（用 dot 避免开方）。
5. **填充结果**：位置用 `w=1`、法线固定为局部 (0,0,1,0) 用 `w=0`，经 `GetObjectToWorld()` 变换回世界空间，法线归一化；`isect.t = t`。

### 与 Sphere 的差异
- 平面求交是线性方程（一次），不是二次方程，无取根分支。
- 法线对圆盘上所有点恒定（局部 +Z），不像球体随位置变化。

## 设计说明

- 沿用"局部空间求交"通用模式：对象空间里圆盘退化成最简形式（z=0 平面上的 |p|≤R 判定）。
- 旋转由 SceneObject 的欧拉角矩阵承担，`Intersect` 内无需任何朝向特判。
- 教程截图中末行 `return isect.t;` 为笔误（float 隐转 bool），实现中写为 `return true;`。
