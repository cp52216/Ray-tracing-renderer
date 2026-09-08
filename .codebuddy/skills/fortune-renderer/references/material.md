# 模块：材质 Material

文件：`source/Material.h`（目前方法在头内联实现，无需 cpp）

## 抽象基类 Material

```cpp
class Material
{
public:
    virtual Color BRDF(const Vector3f& wo, const Vector3f& wi) const = 0;
};
```

- `wo` / `wi` 是世界空间下的出/入射方向；
- `BRDF` 返回一个线性 RGB `Color`，用于渲染方程 `E += BRDF * L_i * max(cosθ, 0)`。

## 派生类

### LambertMaterial
构造：`LambertMaterial(const Color& albedo)`；`BRDF = albedo * INV_PI`（`INV_PI` 在 `Common.h` 中定义）。Lambert 与方向无关，所以 `wo/wi` 实际取值无影响。

## 在 Scene 中的使用

- `Scene::CreateMaterial<T>(name, args...)`：工厂模板，内部 `new T(args...)` 后 `mMaterials[name] = ptr`。
- `Scene::GetMaterial(name)`：按名字查表（`std::map`），找不到返回 `nullptr`。
- `SceneObject::SetMaterial / GetMaterial`：场景对象持有一个材质指针（不拥有）。
- 所有权：Scene 拥有所有 Material，析构时统一 `delete`。
- 若 SceneObject 未设材质，`Renderer::GetIrradiance` 会用 `Color(INV_PI)` 作为 fallback BRDF。

## XML 格式（`scenes/*.xml` 中 `<Materials>` 节点）

| 标签 | 子节点 |
|------|--------|
| `<Material>` | `<Name>`（唯一名）、`<Type>`（目前支持 `Lambert`）、`<Type 相关参数>` |

`<Materials>` 必须在 `<SceneObjects>` 之前定义，因为后续 SceneObject 通过 `<Material>name</Material>` 引用。引用解析走 `Scene::GetMaterial` 查表。

示例（`scenes/scene04.xml`）：
```xml
<Materials>
    <Material>
        <Name>M_Lambert_Red</Name>
        <Type>Lambert</Type>
        <Albedo>0.6, 0.2, 0.2</Albedo>
    </Material>
</Materials>
<SceneObjects>
    <SceneObject>
        <Transform>...</Transform>
        <Material>M_Lambert_Red</Material>   <!-- 按名字引用 -->
        <Primitives>...</Primitives>
    </SceneObject>
</SceneObjects>
```

## 在 Renderer 中的应用

`Renderer::GetIrradiance` 中：

```cpp
SceneObject* pHitObject = mScene->Intersect(ray, isect);
Material* pMaterial = pHitObject->GetMaterial();
Color BRDF = pMaterial ? pMaterial->BRDF(ray.d, ray.d) : Color(INV_PI);
// E += BRDF * L * max(cosθ, 0);
```

`BRDF` 与光照贡献相乘，得到物体表面在该点的辐射。Lambert 与方向无关，所以这里把 `wo`/`wi` 都传 `ray.d` 即可。
