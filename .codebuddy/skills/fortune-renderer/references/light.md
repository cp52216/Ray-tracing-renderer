# 模块：光源 Light

文件：`source/Light.h`、`source/Light.cpp`（方法在头内联实现）

## 抽象基类 Light

```cpp
class Light
{
public:
    virtual Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const = 0;
};
```

- 输入：被着色点 `p`（世界空间）
- 输出：入射到 `p` 的辐射（Color），以及光源在世界空间中的位置 `sourcePos`（供阴影光线追踪使用）
- 所有光源的位置/方向等参数都属于**世界坐标系**；Radiance/Intensity 为线性 RGB 颜色

## 派生类

### DirectionalLight（平行光）
构造：`DirectionalLight(direction, radiance)`，`direction` 在构造时被 `glm::normalize`。
- `sourcePos = p - mDirection * 100000.0f`（模拟无穷远光源在光线反方向上的点）
- 返回 `mRadiance`

### PointLight（点光源）
构造：`PointLight(position, intensity, attenuations)`，`attenuations = (A, B, C)`。
- 衰减：`k = 1 / (C + B*R + A*R²)`，`R = length(p - mPosition)`（加了 `1e-4f` 下界防 0）
- 返回 `mIntensity * k`

### SpotLight（聚光灯）
构造：`SpotLight(position, direction, intensity, innerAngle, outerAngle, attenuations)`，角度用**弧度**，构造时预存 `cos` 值。
- 距离衰减 `k1` 与点光相同
- 角度衰减 `k2 = (cosTheta - mCosOuterAngle) / (mCosInnerAngle - mCosOuterAngle)`，`cosTheta = dot(normalize(p - mPosition), mDirection)`，`clamp(k2, 0, 1)`
- 返回 `mIntensity * k1 * k2`

## 在 Scene 中的使用

- `Scene::CreateLight<T>(args...)`：工厂模板，自动 `new` + 推入 `mLights`，返回 `T*`。
- `Scene::GetLights()`：返回 `const std::vector<Light*>&`，渲染端用它做直接光照 / 阴影追踪。
- 所有权：Scene 拥有所有 Light，析构时统一 `delete`。

## XML 格式（`scenes/*.xml` 中 `<Lights>` 节点）

| 标签 | 子节点 |
|------|--------|
| `<DirectionalLight>` | `<Direction>`（"x, y, z"）、`<Radiance>`（"r, g, b"） |
| `<PointLight>` | `<Position>`、`<Intensity>`、`<Attenuations>`（"A, B, C"） |
| `<SpotLight>` | `<Position>`、`<Direction>`、`<Intensity>`、`<InnerAngle>`（**度**）、`<OuterAngle>`（**度**）、`<Attenuations>` |

`LoadSceneFromXML` 自动把 InnerAngle/OuterAngle 从度转弧度再传入构造函数。

示例（`scenes/scene03.xml`）：
```xml
<Lights>
    <DirectionalLight>
        <Direction>0, -1, 0</Direction>
        <Radiance>1, 1, 1</Radiance>
    </DirectionalLight>
    <PointLight>
        <Position>0, 1.4, 0</Position>
        <Intensity>1, 1, 1</Intensity>
        <Attenuations>1, 0, 0</Attenuations>
    </PointLight>
    <SpotLight>
        <Position>0, 1.4, 0</Position>
        <Direction>0, -1, 0</Direction>
        <Intensity>1, 1, 1</Intensity>
        <InnerAngle>15</InnerAngle>
        <OuterAngle>30</OuterAngle>
        <Attenuations>1, 0, 0</Attenuations>
    </SpotLight>
</Lights>
```
