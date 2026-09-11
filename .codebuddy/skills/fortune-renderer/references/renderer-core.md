# 模块：渲染器框架 Renderer

文件：`source/Renderer.h`、`source/Renderer.cpp`；入口 `source/main.cpp`

## 类结构 `Renderer`

### 成员变量

| 成员 | 类型 | 含义 |
|------|------|------|
| `mViewportWidth` / `mViewportHeight` | `int`（就地默认 800/600） | 视口宽高，构造时传入 |
| `mBuffer` | `uint32_t*` | 像素缓冲，行优先，下标 `y * mViewportWidth + x`，打包 `0x00RRGGBB` |
| `mCurrentPixelIndex` | `std::atomic<int>` | 渲染队列：原子像素下标，多线程用 `fetch_add(1)` 抢占下一个待渲染像素 |
| `SamplePerPixel` | `int`（就地默认 100，可由构造参数指定） | 屏幕上每个像素的采样次数（SPP，SSAA / 路径追踪共用配置） |
| `mMaxDepth` | `int`（就地默认 10，可由构造参数指定） | 路径追踪的最大递归深度（光线弹射上限） |
| `mScene` | `Scene*` | 场景：持有相机与所有场景对象，`RenderSubPixel` 通过它取相机生成光线并求交 |

### 公开接口

- `Renderer(int w, int h, int maxDepth, int samplePerPixel, const char* filepath)`：构造，初始化列表设置宽高、`mMaxDepth(maxDepth)`、`SamplePerPixel(samplePerPixel)`；函数体中：`mCurrentPixelIndex.store(0)`；`mScene = Scene::LoadSceneFromXML(filepath, w, h)` 加载场景（相机与物体都来自 XML）。
- `void Run()`：主循环。
  1. `mfb_open_ex("Fortune Renderer", W, H, MFB_WF_RESIZABLE)` 开窗口（失败直接返回）。
  2. `malloc` 分配 `mBuffer`（W×H×4 字节）。
  3. 重置 `mCurrentPixelIndex = 0`。
  4. 启动多线程渲染：`numThreads = std::thread::hardware_concurrency()`，创建线程数组，每个线程执行 `RunRenderThread` 后 `detach()`。
  5. Present 循环：`mfb_update_ex` 提交 buffer → `mfb_wait_sync` 等垂直同步，`MFB_STATE_OK` 以外退出。
  6. 结束后 `free(mBuffer)`。
- `virtual Color RenderPixel(int x, int y)`：像素着色入口，子类重写此函数实现不同画面。当前实现为 **SSAA 超采样抗锯齿**：`const int N = SamplePerPixel` 次循环，每次在 `(x,y)-(x+1,y+1)` 像素方格内用 `glm::linearRand(0,1)` 随机取亚像素点 `(px, py)`，调用 `RenderSubPixel(px, py)` 得颜色，`resultColor += color / N`，最终直接返回累加结果（即平均值）。需 include `<glm/gtc/random.hpp>`。
- `virtual Color RenderSubPixel(float x, float y)`：单个亚像素采样点的着色：`mScene->GetCamera().GetRay(x, y)` 生成世界空间光线 → 交给 `GetIrradiance(ray)` 求着色。采样策略与 SSAA 平均仍解耦在 RenderPixel。
- `Color GetRadiance(const Ray& ray, int depth)`：路径追踪——给定世界空间光线，调用 `mScene->Intersect(ray, isect)` 求最近交点（未命中返回黑色）。命中后：
  - 取命中 SceneObject 的材质 `Material* pMaterial`；
  - 以命中点法线为 z 轴构建局部坐标系：`localToWorld = MakeCoordinateSystem(isect.normal)`、`worldToLocal = transpose(localToWorld)`；
  - 出射方向 `wo = worldToLocal * (-ray.d)`（局部坐标）；
  - **直接光照**：遍历 `mScene->GetLights()`，对每盏灯先投 shadow ray（`mint=1e-3` 避免自相交，`maxt=length(sourcePos - isect.position)`），被遮挡则 `continue`；未遮挡时 `wi = worldToLocal * shadowRay.d`、`cosθ = dot(isect.normal, shadowRay.d)`，按 `Lo += pMaterial->BRDF(wo, wi) * L * max(cosθ, 0)` 累加；
  - **间接光照（蒙特卡洛，N=1 即"路径追踪"）**：`const int N = 1`——每层只随机采 1 个方向，这就是**路径追踪（Path Tracing）**的定义性写法：每条相机光线形成一条随机路径，噪声靠外层 SSAA 的 `SamplePerPixel` 次采样消除。在交点法线所在的上半球采 `wi = GetSphericalCoordinate(theta, phi)`（`theta ∈ [0, π/2]`，`phi ∈ [0, 2π)`），递归调 `GetRadiance(r, depth+1)` 得 `Li`，直接把 `Lo += BRDF * Li * cosθ * sinθ * π²` 累加（`π²` 是所用采样方案下的 1/pdf 因子，`sinθ` 是球面参数化 Jacobian）。**注意**：若把 N 改成 >1，此写法会漏除 N（整体偏亮 N 倍），需改成 `π²/N` 才是正确的均值估计；
  - **早退**：`depth > mMaxDepth` 时直接返回 `Color(0)`，防止无限递归。
  - 返回 `Lo`（出射辐射，线性 RGB）。
- `Color GetIrradiance(const Ray& ray)`：**入射辐照度版**——同样求交 + shadow ray，但**不乘 BRDF**，按 `E = Σ L * max(cosθ, 0)` 累加，返回"到达该点的光"。与 GetRadiance 相对（后者是"反射出去的光"）。当前 `RenderSubPixel` 只调 GetRadiance；GetIrradiance 保留作调试/对比用途。
- 调用方：`RenderSubPixel` 调 `GetRadiance(ray, 0)` 拿到出射辐射。
- `void RunRenderThread()`：渲染线程入口（消费者循环）。
  - `while (true)` 中 `int pixelIndex = mCurrentPixelIndex.fetch_add(1)` 原子认领像素；
  - `pixelIndex >= W*H` 时 break（一帧全部认领完毕）；
  - 一维下标反算二维坐标：`x = pixelIndex % W`，`y = pixelIndex / W`；
  - 调 `RenderPixel(x, y)` 得线性颜色，各通道 `round(c*255)` 后 `clamp` 到 `[0,255]`，打包 `(r<<16)|(g<<8)|b` 写入 `mBuffer`。

### 线程模型

"原子像素队列"模式：渲染队列 `mCurrentPixelIndex` 是唯一同步点，无需互斥锁；每个渲染线程处理完一帧的所有像素后自动退出。主线程只负责 Present。

已知取舍：线程 `detach()` 且 Present 直接读 `mBuffer`，与渲染线程写之间无同步（单缓冲撕裂风险，当前场景可接受）。

## 程序入口 main.cpp

`Renderer renderer(800, 600, 10, 2, "../scenes/scene04.xml"); renderer.Run();`（参数依次为：宽、高、路径追踪最大深度、每像素采样数 SPP、场景 XML 路径）

## 依赖

- minifb：窗口 + 软件呈现（`mfb_open_ex` / `mfb_update_ex` / `mfb_wait_sync`）
- GLM：`glm::clamp`
- 标准库：`<thread>`、`<atomic>`、`<vector>`、`<cmath>`（round）、`<chrono>`（sleep 模拟耗时）
