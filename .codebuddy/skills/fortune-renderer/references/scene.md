# 模块：场景 Scene

文件：`source/Scene.h`、`source/Scene.cpp`

## 职责

场景 = 相机 + 所有场景对象（SceneObject）的容器，是渲染端（Renderer）与几何内容的中间层：

- 持有 `Camera mCamera`（世界空间；`SetCamera`/`GetCamera` 读写）。
- 持有 `std::vector<SceneObject*> mSceneObjects`，**Scene 拥有这些对象**。

## 接口

| 接口 | 说明 |
|------|------|
| `static Scene* LoadSceneFromXML(const char* filepath, int W, int H)` | 从 XML 加载整个场景（相机+物体）；文件缺失/解析失败返回 `nullptr` |
| `void SetCamera(const Camera& camera)` | 设置相机（位置/目标/上向量均为世界空间） |
| `const Camera& GetCamera() const` | 取相机，渲染端用它生成世界空间光线 |
| `SceneObject* CreateSceneObject(position, euler, scale)` | `new SceneObject(...)` 后 push 进 `mSceneObjects` 并返回指针；`position/euler/scale` 相对世界坐标系 |
| `SceneObject* Intersect(Ray ray, Intersection& isect) const` | 世界空间下遍历所有对象求最近交点；命中后 `ray.maxt = isect.t` 收缩区间，**返回最近命中的 SceneObject**（供后续取材质着色），未命中返回 `nullptr` |
| `~Scene()` | 遍历 `delete` 所有 SceneObject（SceneObject 析构再释放其图元） |

## XML 场景加载（LoadSceneFromXML）

依赖第三方库 **tinyxml2**（`tinyxml2/` 目录，仅 `tinyxml2.h/.cpp` 两个文件，Zlib 许可），`tinyxml2.cpp` 通过 `target_sources` 直接编进 FortuneRenderer 主工程，不生成独立 lib。

解析流程（Scene.cpp）：
1. `doc.LoadFile(filepath)` 失败 → 返回 `nullptr`；取 `<Scene>` 根元素。
2. `<Camera>`：`Position/Target/Up`（"x, y, z" 文本）、`NearZ/FarZ/Fov`（Fov 为**角度**，代码内转弧度）→ `camera.Initialize(..., W, H)` 后 `SetCamera`。
3. `<SceneObjects>` 下遍历 `<SceneObject>`：
   - `<Transform>`：`Position/Rotation/Scale`（Rotation 为**角度**，代码内 `glm::radians` 转弧度）→ `CreateSceneObject`；
   - `<Primitives>`：按标签依次解析 `Sphere(Radius)`、`Disk(Radius)`、`Triangle(Vertex×3 顺序读取)` → `CreatePrimitive<T>`。

辅助函数（Scene.cpp 文件级 static）：`ParseVector3f("x, y, z")`、`GetChildText(elem, name)`、`GetChildFloat(elem, name, default)`。

场景文件：`scenes/scene01.xml`（矩形+球，与原硬编码场景一致）、`scenes/scene02.xml`（康奈尔盒：地面/顶棚/左右墙/后墙 + 两球）。运行时路径相对工作目录，VS 调试传 `../scenes/scene01.xml`。

## 所有权链

```
Renderer ──拥有──> Scene ──拥有──> SceneObject ──拥有──> Primitive
```

`delete mScene` 即可释放整棵树，Renderer 不直接管理图元或对象。

## 与 Renderer 的关系

- Renderer 构造签名 `Renderer(int w, int h, int samplePerPixel, const char* filepath)`，构造中 `mScene = Scene::LoadSceneFromXML(filepath, w, h)`；原硬编码场景（相机+矩形+球）以注释保留在 Renderer.cpp 备查。
- `RenderSubPixel` 中：`mScene->GetCamera().GetRay(x, y)` 生成光线，`mScene->Intersect(ray, isect)` 求最近交点，返回值（命中对象）为后续材质/着色预留。

## 编码规范（强制）

**调用 `CreateSceneObject` / `CreatePrimitive` 时，每个实参都必须加注释**，标明参数含义与所属坐标系（世界空间/对象空间）。这是项目强制的书写规则，为项目新增物体时必须遵守。

标准写法（示例取自 Renderer.cpp 的测试矩形）：

```cpp
SceneObject* pSceneObject = mScene->CreateSceneObject(
    Vector3f(0, 0, 5),   // position：矩形中心位置（世界空间）
    Vector3f(0, 0, 0),   // euler   ：无旋转（世界空间下的欧拉角，弧度）
    2.0f);               // scale   ：整体放大 2 倍（对象空间）

pSceneObject->CreatePrimitive<Triangle>(
    Vector3f(-1, -1, 0), // v0：顶点 0（对象空间）
    Vector3f(1, -1, 0),  // v1：顶点 1（对象空间）
    Vector3f(1, 1, 0));  // v2：顶点 2（对象空间）
```

要点：
- `CreateSceneObject` 的三个参数注释为 `position / euler / scale`，坐标系标注：位置与朝向是**世界空间**，缩放是**对象空间**。
- `CreatePrimitive<T>` 的参数注释标明几何含义（如 `v0/v1/v2` 顶点），坐标系一般为**对象空间**。
- 每个实参单独占一行、行尾注释，禁止把多个参数挤在一行不加注释。

## 设计意义

- 相机归场景所有后，渲染器不再关心"从哪看"，只负责"画"。
- `Intersect` 返回命中对象而非 bool：为材质系统铺路（颜色/材质挂在 SceneObject 上）。
