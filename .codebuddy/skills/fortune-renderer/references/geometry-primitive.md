# 模块：图元抽象基类 Primitive / 场景对象 SceneObject

文件：`source/Primitive.h`、`source/SceneObject.h`

## Primitive

所有可求交几何体的公共抽象接口：

```cpp
class Primitive
{
public:
    Primitive(SceneObject* pSceneObject) : m_pSceneObject(pSceneObject) {}
    virtual ~Primitive() {}
    virtual bool Intersect(Ray ray, Intersection& isect) const = 0;

protected:
    SceneObject* m_pSceneObject = nullptr;
};
```

- `Intersect` 为纯虚函数：签名统一为"按值传 Ray、引用输出 Intersection、const 成员函数"。
- **变换矩阵的所有权上移**：图元不再自己存 ObjectToWorld/WorldToObject，而是持有指向所属 `SceneObject` 的指针（protected，派生类可直接用 `m_pSceneObject->Get*()`）。
- 头文件用 `class SceneObject;` 前向声明避免与 SceneObject.h 循环包含。

## SceneObject（SceneObject.h / SceneObject.cpp）

场景对象 = 一组图元共享的摆放变换：

- 构造 `SceneObject(position, euler, scale)`：
  - `Vector3f scale` 版本：`mObjectToWorld = MakeWorld(position, euler, scale)`，`mWorldToObject = glm::inverse(...)`；
  - `float scale` 版本（统一缩放）：内部走 `MakeWorldTransform(position, euler, scale)`。
  - `position/euler/scale` 都是物体相对于世界坐标系的属性。
  - `position`：物体中心在世界坐标系中的平移位置（世界空间）。
  - `euler`：物体相对于世界坐标轴的欧拉角（弧度，世界空间下的朝向）。
  - `scale`：物体在自身对象空间各轴上的缩放比例（对象空间）。
- `void AddPrimitive(Primitive* primitive)`：把**已创建好**的图元指针加入 `mPrimitives`。
- `template<typename T, typename... Args> T* CreatePrimitive(Args&&... args)`：**推荐用法**——内部 `new T(this, std::forward<Args>(args)...)`（自动把 `this` 作为 `SceneObject*` 传入图元构造），push 进 `mPrimitives` 并返回 `T*`。调用方只需写几何参数，不再手动 `new` + `AddPrimitive`。需要 `#include <utility>`（`std::forward`）。
- `bool Intersect(Ray ray, Intersection& isect) const`：在世界空间下遍历自身所有图元，命中后用 `isect.t` 收缩 `ray.maxt`，最终返回是否命中，并令 `isect` 保存**最近交点**。
- `~SceneObject()`：析构时遍历 `mPrimitives` 并 `delete` 每个图元，**SceneObject 拥有其下挂接的图元**。
- 访问器 `GetObjectToWorld()` / `GetWorldToObject()`（const 返回矩阵副本）。
- 私有成员：两个矩阵 + `std::vector<Primitive*> mPrimitives`。
- 一个 SceneObject 可挂多个图元（例如一辆车 = 多个 mesh 图元共享同一变换）。

## 派生类

| 类 | 文件 | 构造参数 | 求交策略 |
|----|------|----------|----------|
| `Sphere` | Sphere.h/cpp | `(SceneObject*, R)` | 局部空间二次方程 |
| `Disk` | Disk.h/cpp | `(SceneObject*, radius)` | 局部空间 z=0 平面 + 半径裁剪 |
| `Triangle` | Triangle.h/cpp | `(SceneObject*, v0, v1, v2)` | Möller–Trumbore，顶点存对象空间、求交时经 SceneObject 变换到世界空间 |

三者均已迁移到 SceneObject 模式：不再自持变换矩阵，求交时通过 `m_pSceneObject->Get*()` 取矩阵。

## 设计意义

- 变换与几何解耦：同一 SceneObject 下多个图元共享摆放；图元类只写最简几何方程。
- SceneObject 拥有其下挂接的图元，析构时自动释放；Renderer 只需管理 `SceneObject` 生命周期，不再直接持有 `Primitive` 列表。
- 新增图元只需继承 Primitive、实现 Intersect，再通过 `SceneObject::CreatePrimitive<T>(args...)` 创建并挂接，无需改动求交调用方。
- 推荐 `CreatePrimitive` 而非手动 `new + AddPrimitive`：所有权关系（SceneObject 拥有图元）在创建处就一目了然。
