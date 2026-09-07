# 变更记录

> 每次为项目新增/修改功能后，在顶部按日期追加一条记录。

## 2026-09-04（scene03 光源调参 + shadow_isect 独立化）

- **scene03.xml 光源更新**（scenes/）：康奈尔盒几何不变，三种光源参数调整：
  - `DirectionalLight` Direction `0,-1,0` → `1, -2, 0.4`（斜射），**已注释屏蔽**
  - `PointLight` Position `0,1.4,0` → `0, 1, 0`（盒内中央稍上方），保留启用
  - `SpotLight` Position `0,1.4,0`/Direction `0,-1,0` → Position `(-0.8, 0.9, 0)`、Direction `(2, -1, 0)`（左上往右下斜射），Inner/Outer 改为 `10°/60°`，**已注释屏蔽**
  - 场景当前实际生效光源：仅 `PointLight`。
- **GetIrradiance 阴影射线独立化**（Renderer.cpp）：shadow ray 改用独立 `Intersection shadow_isect`（不再复用 `isect`），`mint` 从 `1e-4` 提到 `1e-3` 增强自相交鲁棒性。

## 2026-09-04（直接光照：Renderer::GetIrradiance）

- **Renderer 拆出 GetIrradiance**（Renderer.h/.cpp）：新增私有 `Color GetIrradiance(const Ray& ray)`，对场景求最近交点（未命中返回黑色），命中后遍历 `mScene->GetLights()` 累加 Lambertian 漫反射 `E += L * max(cosθ, 0)`；`RenderSubPixel` 简化为"生成世界空间光线 → 调 GetIrradiance"。Renderer.h 显式 `#include "Ray.h"`。**当前未做阴影光线追踪**，下一步可加 shadow ray。

## 2026-09-04（阴影光线追踪：GetIrradiance 加 shadowRay）

- **加 shadow ray**（Renderer.cpp GetIrradiance）：从交点 `isect.position` 朝 `sourcePos` 投一条阴影光线（`mint=1e-4` 防自相交，`maxt=length(sourcePos - isect.position)` 只检测光源与表面之间的遮挡），与场景相交则 `continue` 跳过该光源的贡献。
- **技能文档**：renderer-core.md GetIrradiance 描述更新，changelog 同步。

## 2026-09-04（Light 光源模块 + scene03 康奈尔盒带光）

- **新增 Light 模块**（Light.h/.cpp）：抽象基类 `Light`（`GetRadiance(p, sourcePos)`）；派生 `DirectionalLight`（direction+radiance）、`PointLight`（position+intensity+attenuations (A,B,C)）、`SpotLight`（+ direction + 内外锥角；角度用弧度，构造时存 cos 方便运行时用）。所有光源参数都在世界空间，attenuation = `1 / (C + B*R + A*R²)`，加了 `1e-4` 防 R=0。
- **Scene 加入光源管理**（Scene.h/.cpp）：`CreateLight<T>(args...)` 工厂模板 + `mLights` + `GetLights()`；`~Scene()` 释放光源与对象。`LoadSceneFromXML` 扩展解析 `<Lights>` 节点（DirectionalLight/PointLight/SpotLight），SpotLight 的 InnerAngle/OuterAngle 自动从度转弧度；CreateLight 调用处遵守逐参数注释规范。
- **场景文件**：`scenes/scene03.xml` = scene02（康奈尔盒）+ 三种光源（顶向下 DirectionalLight、中心 PointLight (0,1.4,0)、同位置 SpotLight 15°/30° 锥角），都加二次衰减 (1,0,0)。
- **技能文档**：新增 `references/light.md`；SKILL.md 模块地图、scene.md、`changelog.md` 同步更新。

## 2026-09-03（XML 场景加载：tinyxml2 + scene01/scene02）

- **接入 tinyxml2**（第三方库）：`tinyxml2/` 目录（tinyxml2.h/.cpp，v11.0，Zlib 许可），CMakeLists 中 `target_sources` 把 tinyxml2.cpp 直接编进 FortuneRenderer 主工程（不生成独立 lib，避免 VS 多配置下 lib 输出路径找不到导致的 LNK1104）。
- **Scene::LoadSceneFromXML**（Scene.h/cpp）：静态工厂，从 XML 加载相机（Position/Target/Up/NearZ/FarZ/Fov，Fov 角度制）+ SceneObjects（Transform 的 Position/Rotation 角度制/Scale + Primitives 的 Sphere/Disk/Triangle）；Rotation 在代码内转弧度；文件级 static 辅助 ParseVector3f/GetChildText/GetChildFloat。
- **场景文件**（scenes/）：`scene01.xml`（相机 0,0,0→0,0,1 Fov60 + 矩形(0,0,5,scale2) + 球(0,0,2,R0.5)，与原硬编码场景一致）；`scene02.xml`（康奈尔盒：地面 -90°、顶棚 90°、左墙 0,90,0、右墙 0,-90,0、后墙 180°，两球 ±0.4,0.3,∓0.3 R0.3，相机 0,0.6,-2.58 Fov45）。
- **Renderer/main 改造**：构造函数加 `filepath` 参数，`mScene = Scene::LoadSceneFromXML(filepath, w, h)`，旧硬编码场景注释保留；main.cpp 改为 `Renderer(1920, 1080, 100, "../scenes/scene01.xml")`。
- **技能文档**：scene.md 新增 XML 加载章节；changelog 记录。

## 2026-09-03（场景新增球体，验证多物体遮挡）

- **场景添加第二个物体**（Renderer.cpp）：`pSceneObject2 = CreateSceneObject((0,0,2), 0, 1.0f)` + `CreatePrimitive<Sphere>(0.5f)`，球心在世界空间 (0,0,2)、位于矩形 (z=5) 前方，用于验证 `Scene::Intersect` 的最近交点/遮挡关系；调用处遵守逐参数注释规范。Renderer.cpp 补 include "Sphere.h"。

## 2026-09-03（编码规范：场景调用必须逐参数注释）

- **新增强制规则**：调用 `Scene::CreateSceneObject` / `SceneObject::CreatePrimitive` 时，每个实参必须加行尾注释（含义 + 坐标系），已写入 references/scene.md 编码规范章节与 SKILL.md 关键设计约定。
- **代码落实**（Renderer.cpp）：测试矩形的 `CreateSceneObject` 与两个 `CreatePrimitive<Triangle>` 调用已改为逐参数注释（position/euler/scale 标注世界空间/对象空间，v0/v1/v2 标注对象空间）。

## 2026-09-03（新增 Scene 场景类）

- **新增 Scene**（Scene.h/cpp）：持有 `Camera` 与 `std::vector<SceneObject*> mSceneObjects`；`SetCamera/GetCamera` 读写相机；`CreateSceneObject(position, euler, float scale)` 工厂式创建并接管对象；`Intersect(ray, isect)` 遍历所有对象求**最近交点并返回命中的 SceneObject**（为后续按对象取材质铺路）；`~Scene()` 释放所有对象（所有权链：Renderer → Scene → SceneObject → Primitive）。
- **Renderer 改用 Scene**（Renderer.h/cpp）：移除 `mCamera`/`mTestSceneObject` 成员，改为 `Scene* mScene`；构造中 `new Scene()` → 初始化相机（参数全为世界空间）→ `SetCamera` → `CreateSceneObject((0,0,5),0,2)` + 两个 `CreatePrimitive<Triangle>` 拼测试矩形；析构只 `delete mScene`；`RenderSubPixel` 改为 `mScene->GetCamera().GetRay(...)` + `mScene->Intersect(...)`（旧的 mPrimitives 遍历写法以注释保留）。RenderPixel/RenderSubPixel/RunRenderThread 移入 private。
- **技能文档**：新增 references/scene.md；SKILL.md 模块地图、renderer-core.md 同步更新。

## 2026-09-02（CreatePrimitive 工厂模板 + MakeWorldTransform 恢复）

- **SceneObject::CreatePrimitive**（SceneObject.h）：`template<typename T, typename... Args> T* CreatePrimitive(Args&&... args)`——内部 `new T(this, std::forward<Args>(args)...)` 自动把 `this` 作为 `SceneObject*` 传入图元构造，push 进 `mPrimitives` 并返回 `T*`；新增 `#include <utility>`。推荐替代手动 `new` + `AddPrimitive`。
- **float 构造改走 MakeWorldTransform**（SceneObject.h / Common.h）：`SceneObject(position, euler, float scale)` 不再委托 Vector3f 版本，而是直接调用恢复的 `MakeWorldTransform`（float 统一缩放，内部展开为 `Vector3f(scale)` 调 `MakeWorld`）。
- **Renderer 改用工厂**（Renderer.cpp）：测试矩形恢复创建，两个三角形改用 `mTestSceneObject->CreatePrimitive<Triangle>(v0, v1, v2)` 挂接（截图原代码 `pTriangle2` 处缺少逗号的笔误已修正）。

## 2026-09-02（SceneObject 拥有图元 + 测试矩形）

- **SceneObject 新增析构函数**（SceneObject.h/cpp）：`~SceneObject()` 遍历 `mPrimitives` 并 `delete` 每个图元，明确“场景对象拥有其下挂接的图元”。
- **SceneObject 新增统一缩放重载**（SceneObject.h）：`SceneObject(position, euler, float scale)` 委托给 `Vector3f scale` 版本，方便测试代码直接传 `2.0f`。
- **Renderer 改为单测试对象**（Renderer.h/cpp）：移除 `mPrimitives`/`mSceneObjects` 两个 vector，改为 `SceneObject* mTestSceneObject`；构造中创建一个位于 `(0,0,5)`、统一缩放 `2` 的 `SceneObject`，用两个 `Triangle` 拼成覆盖 `[-1,1]x[-1,1]` 的矩形并 `AddPrimitive` 挂接；析构只 `delete` `mTestSceneObject`。
- **RenderSubPixel 改为单对象求交**（Renderer.cpp）：直接调用 `mTestSceneObject->Intersect(ray, isect)`，由 `SceneObject` 内部管理 `ray.maxt` 并返回最近交点。
- **技能文档同步**（renderer-core.md / geometry-primitive.md / changelog.md）：更新成员表、构造/求交流程、所有权说明。

## 2026-09-02（SceneObject 增加求交与图元管理 + 坐标系注释）

- **新增 SceneObject.cpp**：实现 `Intersect(Ray, Intersection&)`，在世界空间下遍历 `mPrimitives`，命中后用 `isect.t` 收缩 `ray.maxt`，返回最近交点。
- **改写 SceneObject.h**：保留 `Vector3f scale` 构造函数；新增 `AddPrimitive(Primitive*)` 与 `Intersect` 声明；为 position/euler/scale、变换矩阵、图元列表补充坐标系与职责注释。
- **全项目坐标系注释**（Common.h / Ray.h / Camera.h/.cpp / Primitive.h / Sphere.h / Disk.h / Triangle.h / Renderer.h/.cpp）：明确标注世界空间、对象空间、屏幕像素空间等概念，说明各参数/变量所属坐标系。
- **Renderer 迁移到场景对象求交**（Renderer.h/cpp）：构造中三个 `SceneObject` 改用 `Vector3f` 缩放，并用 `AddPrimitive` 把 `Sphere`/`Disk`/`Triangle` 挂到对应对象；`RenderSubPixel` 改为遍历 `mSceneObjects` 调用 `Intersect`，由 `SceneObject` 内部管理 `ray.maxt`。`mPrimitives` 仍保留用于析构释放图元。

## 2026-09-02（Disk/Triangle 迁移到 SceneObject）

- **Disk 迁移**（Disk.h/cpp）：构造改为 `(SceneObject*, radius)`，不再持有矩阵；Intersect 经 m_pSceneObject 取 WorldToObject/GetObjectToWorld。
- **Triangle 迁移**（Triangle.h/cpp）：构造改为 `(SceneObject*, v0, v1, v2)`，顶点存对象空间；Intersect 每次把顶点经 GetObjectToWorld 变换到世界空间后做 Möller–Trumbore，法线改为世界空间现算（不再构造时缓存）。
- **Renderer**（Renderer.h/cpp）：为圆盘/三角形各建 SceneObject（参数沿用之前的平移/旋转/缩放）并 push 进 mSceneObjects。三个图元至此全部统一到"SceneObject 管变换、Primitive 管几何"的架构。

## 2026-09-02（SceneObject 变换容器）

- **新增 SceneObject**（SceneObject.h）：持有 mObjectToWorld/mWorldToObject（构造传 position/euler/scale）+ GetObjectToWorld/GetWorldToObject 访问器 + 私有 mPrimitives 列表（图元容器，待后续 AddPrimitive 启用）。
- **Primitive 反向引用 SceneObject**（Primitive.h）：构造传 `SceneObject*`，protected 成员 m_pSceneObject，前向声明避免循环包含。
- **Sphere 迁移到 SceneObject 模式**（Sphere.h/cpp）：构造改为 `(SceneObject*, R)`，不再持有矩阵；Intersect 中 WorldToObject/	ObjectToWorld 均取自 m_pSceneObject。Renderer 相应为球体创建 SceneObject（位置即球心）并存入 mSceneObjects，析构释放。

## 2026-09-02（最近交点遍历）

- **首中即停 → 最近交点**（Renderer.cpp RenderSubPixel）：遍历 mPrimitives 时每次命中后 `ray.maxt = isect.t` 收缩区间，最终 isect 保存最近交点（bool bHit 标记命中）；物体遮挡关系自此正确。

## 2026-09-02（场景图元列表统一管理）

- **Renderer 场景容器化**（Renderer.h/cpp）：三个独立成员（mSphere/mDisk/mTriangle）合并为 `std::vector<Primitive*> mPrimitives`；构造中 `new` 后逐个 `push_back`（圆盘改回无旋转，三角形世界矩阵改为位置(0,0,5)、欧拉(0,45°,60°)、缩放 2）；析构遍历 delete；`RenderSubPixel` 改为遍历 mPrimitives 求交（首中即 break，暂不比较最近交点）。

## 2026-09-01（图元继承体系 Primitive）

- **新增抽象基类 Primitive**（Primitive.h）：纯虚 `Intersect(Ray, Intersection&) const` + 虚析构；Sphere/Disk/Triangle 全部改为 `public Primitive` 继承并给 Intersect 标记 `override`（头文件 include 从 Ray.h 换成 Primitive.h）；.cpp 实现不变。为场景统一持有 Primitive* / 求最近交点打基础。模块文档见 references/geometry-primitive.md。

## 2026-09-01（采样数可通过构造参数配置）

- **构造函数增加 samplePerPixel 参数**（Renderer.h/cpp、main.cpp）：`Renderer(w, h, samplePerPixel=100)`，函数体中 `SamplePerPixel = samplePerPixel`；成员名从 N 改回 `SamplePerPixel`（就地默认 100），RenderPixel 内恢复局部 `const int N = SamplePerPixel`；main.cpp 调用改为 `Renderer(800, 600, 100)`。

## 2026-09-01（采样数成员改名 N）

- **SamplePerPixel → N**（Renderer.h/cpp）：采样数成员改名为 `N`（去掉就地初始化），改在构造函数体中 `N = 100` 赋值；RenderPixel 删除同名局部常量，直接使用成员 `N`。

## 2026-09-01（RenderSubPixel 拆分）

- **采样与着色解耦**（Renderer.h/cpp）：新增 `virtual Color RenderSubPixel(float x, float y)`（生成光线→求交→着色，当前测试 mDisk）；`RenderPixel` 只负责 SSAA 循环采样与平均，采样数改用新增成员 `int SamplePerPixel = 100`（去掉 static const 局部常量，子类可运行时配置）。

## 2026-09-01（SSAA 采样数提升）

- **N 4 → 100**（Renderer.cpp）：RenderPixel 每像素采样数提升到 100，边缘更平滑、噪点更少；取平均移入循环内（`resultColor += color / N`），与整体除 N 数学等价，渲染耗时约为原来的 25 倍（多线程下仍在可接受范围）。

## 2026-09-01（SSAA 超采样抗锯齿）

- **Camera 新增浮点版 GetRay**（Camera.h/cpp）：`GetRay(float x, float y)` 为实际实现（屏幕点 p 直接用浮点坐标），整型版委托调用；供像素内随机采样使用。
- **RenderPixel 改为 SSAA**（Renderer.cpp）：`N = 4` 次循环，`glm::linearRand(0,1)` 在像素方格内随机取亚像素点生成光线，与 `mDisk` 求交（当前测试目标）累加法线颜色，最后除 N 取平均；新增 include `<glm/gtc/random.hpp>`。

## 2026-09-01（三角形图元 Triangle）

- **新增 Triangle 图元**（Triangle.h/cpp）：Möller–Trumbore 算法。构造时传入三顶点 + 世界矩阵，顶点预变换到世界空间、法线由边向量叉积算一次；`Intersect` 在世界空间直接求解（行列式平行剔除 → t/重心坐标 b1/b2 → 范围与内部检查），命中填 position/normal/t。模块文档见 references/geometry-triangle.md。
- **场景挂载三物体**（Renderer.h/cpp）：补回 `Sphere* mSphere`（(-1,-1,2) R=1），`mDisk` 改为 (0,-2,5) 绕 X 轴旋转 90°，新增 `Triangle* mTriangle`（三顶点经 `MakeWorld((0,0,5),0,1)` 摆放）；析构逐一释放；RenderPixel 当前改为与 mTriangle 求交验证（逐物体切换测试）。

## 2026-09-01（Renderer 集成圆盘）

- **场景物体 Sphere → Disk**（Renderer.h/cpp）：include 与成员改为 `Disk* mDisk`；构造中 `new Disk((0,0,5), euler=(0,0,0), R=1)`；析构 delete；RenderPixel 改为与 `mDisk` 求交（命中仍输出法线可视化颜色）。

## 2026-09-01（圆盘图元 Disk）

- **新增 Disk 图元**（Disk.h/cpp）：圆盘建模在对象空间 z=0 平面；构造传入 center/euler/radius，用 `MakeWorld` 支持任意朝向摆放；`Intersect` 在局部空间用一次方程求平面交点 `t = -o.z/d.z`，再做半径裁剪（`dot(p,p) > R²` 剔除），法线固定局部 +Z 变换回世界空间（修正教程截图中 `return isect.t;` 笔误为 `return true;`）。模块文档见 references/geometry-disk.md。
- **Intersection 移至 Ray.h**（Ray.h/Sphere.h）：求交结果结构体从 Sphere.h 上移到 Ray.h，供所有图元（Sphere/Disk/…）共用，Sphere.h 改为包含 Ray.h 即可。

## 2026-09-01（法线可视化）

- **RenderPixel 改用法线颜色**（Renderer.cpp）：球体命中时不再输出红色，而是返回 `isect.normal * 0.5f + 0.5f`，将法线方向映射为颜色以验证求交法线正确性；背景保持黑色。

## 2026-09-01（首个光线追踪画面）

- **Renderer 集成球体**（Renderer.h/cpp）：新增成员 `Sphere* mSphere`（构造 new，析构 delete）；相机改为原点朝 +Z 看（位置(0,0,0)、目标(0,0,1)）；RenderPixel 与球求交，命中红色、未命中黑色。

## 2026-09-01（球体求交模块）

- **新增 Sphere/Intersection**（Sphere.h/cpp、Ray.h）：`Intersection` 结构体（position/normal/t）；`Sphere::Intersect` 把射线变换到局部空间解二次方程求交（判别式→取有效根→回变换世界空间），成功返回 true（修正了图中末行 return false 的笔误）；`Ray.h` 新增 `operator*(Matrix4x4, Ray)`（o 用 w=1、d 用 w=0 变换）。模块文档见 references/geometry-sphere.md。

## 2026-08-31（编译修复）

- **修复 C1083**（Camera.h）：项目自带 GLM 为 0.9.9+，`perspectiveFovLH_ZO` 位于 `<glm/ext/matrix_clip_space.hpp>`，include 路径从 `glm/gtc/` 改为 `glm/ext/`。
- **修复 C4819**（CMakeLists.txt）：MSVC 加 `/utf-8` 编译选项，统一按 UTF-8 解析含中文注释的无 BOM 源文件。改 CMake 后需重新运行 CMake 配置（重新生成解决方案）再编译。

## 2026-08-31（方向映射颜色）

- **光线方向可视化**（Renderer.cpp）：RenderPixel 改为 `color = ray.d * 0.5f + 0.5f`，把射线方向分量从 [-1,1] 线性映射到 [0,1] 作为颜色输出；旧渐变实现注释保留。

## 2026-08-31（RenderPixel 接入光线）

- **RenderPixel 生成光线**（Renderer.cpp）：开头调用 `Ray ray = mCamera.GetRay(x, y)`，为后续光线追踪求交做准备；注释掉 sleep_for(1ms) 模拟耗时。

## 2026-08-31（相机模块）

- **Renderer 集成 Camera**（Renderer.h/cpp）：新增成员 `Camera mCamera`；构造函数中调用 `mCamera.Initialize`（位置(10,5,20)、目标(10,10,30)、上(0,1,0)、FOV 60°、近 0.1、远 1000、视口 w×h）。
- **新增 Camera/Ray 模块**（Camera.h/cpp、Ray.h）：`Ray` 结构体（o/d/mint/maxt）；`Camera::Initialize` 用 lookAtLH + perspectiveFovLH_ZO + 手写视口矩阵合成 combined 矩阵并预存逆矩阵；`Camera::GetRay(x,y)` 通过逆矩阵把像素坐标反投影到世界空间生成光线。模块文档见 references/camera.md。

## 2026-08-31（渲染器框架）

- **多线程渲染队列**（Renderer.h/cpp）：`mCurrentPixelIndex` 改为 `std::atomic<int>`；`Run()` 按 `hardware_concurrency()` 创建渲染线程并 detach；`RunRenderThread()` 改为原子抢占像素的消费者循环（fetch_add，越界退出）；`RenderPixel()` 加 1ms sleep 模拟渲染耗时。
- **宽高构造化**（Renderer.h/cpp/main.cpp）：视口宽高改为构造参数（默认 800×600），新增成员 `mViewportWidth/mViewportHeight`，替换所有硬编码。
- **RenderPixel 抽象**（Renderer.h/cpp）：像素颜色计算抽成虚函数 `Color RenderPixel(x, y)`，缓冲填充逻辑移入 `RunRenderThread()`；颜色改为线性 RGB + round/clamp 量化打包。
- **Common.h 建立**（Common.h）：从 GLMDemo 迁移矩阵构造函数（Translation/Rotation/Scale/World/CoordinateSystem）为 inline 函数；建立类型别名（Vector2/3/4 f+i、Matrix3x3/4x4、Color）。
