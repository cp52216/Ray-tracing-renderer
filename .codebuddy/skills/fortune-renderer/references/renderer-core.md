# 模块：渲染器框架 Renderer

文件：`source/Renderer.h`、`source/Renderer.cpp`；入口 `source/main.cpp`

## 类结构 `Renderer`

### 成员变量

| 成员 | 类型 | 含义 |
|------|------|------|
| `mViewportWidth` / `mViewportHeight` | `int`（就地默认 800/600） | 视口宽高，构造时传入 |
| `mBuffer` | `uint32_t*` | 像素缓冲，行优先，下标 `y * mViewportWidth + x`，打包 `0x00RRGGBB` |
| `mCurrentPixelIndex` | `std::atomic<int>` | 渲染队列：原子像素下标，多线程用 `fetch_add(1)` 抢占下一个待渲染像素 |
| `mCamera` | `Camera` | 相机对象；构造函数中 `Initialize` 完成初始化，渲染线程可用 `mCamera.GetRay(x, y)` 生成光线 |
| `SamplePerPixel` | `int`（就地默认 100，可由构造参数指定） | 屏幕上每个像素的采样次数（SPP，SSAA / 路径追踪共用配置） |
| `mTestSceneObject` | `SceneObject*` | 当前测试场景对象（包含多个基本图元），`RenderSubPixel` 直接对其求交。 |

### 公开接口

- `Renderer(int w = 800, int h = 600, int samplePerPixel = 100)`：构造，初始化列表设置宽高；函数体中初始化相机：位置 `(0,0,0)`、目标 `(0,0,1)`、上向量 `(0,1,0)`、FOV 60°（弧度）、近裁剪 0.1、远裁剪 1000、视口 w×h；函数体开头 `SamplePerPixel = samplePerPixel`（每像素采样数 SPP 由外部配置）；随后创建一个测试 `SceneObject`（`position/euler/scale` 均为相对世界坐标系的属性），并通过 `CreatePrimitive<Triangle>(...)` 挂接两个三角形拼成矩形：矩形中心 `(0,0,5)`、无旋转、统一缩放 `2`，顶点在对象空间 `z=0` 平面上覆盖 `[-1,1]x[-1,1]`。
- `void Run()`：主循环。
  1. `mfb_open_ex("Fortune Renderer", W, H, MFB_WF_RESIZABLE)` 开窗口（失败直接返回）。
  2. `malloc` 分配 `mBuffer`（W×H×4 字节）。
  3. 重置 `mCurrentPixelIndex = 0`。
  4. 启动多线程渲染：`numThreads = std::thread::hardware_concurrency()`，创建线程数组，每个线程执行 `RunRenderThread` 后 `detach()`。
  5. Present 循环：`mfb_update_ex` 提交 buffer → `mfb_wait_sync` 等垂直同步，`MFB_STATE_OK` 以外退出。
  6. 结束后 `free(mBuffer)`。
- `virtual Color RenderPixel(int x, int y)`：像素着色入口，子类重写此函数实现不同画面。当前实现为 **SSAA 超采样抗锯齿**：`const int N = SamplePerPixel` 次循环，每次在 `(x,y)-(x+1,y+1)` 像素方格内用 `glm::linearRand(0,1)` 随机取亚像素点 `(px, py)`，调用 `RenderSubPixel(px, py)` 得颜色，`resultColor += color / N`，最终直接返回累加结果（即平均值）。需 include `<glm/gtc/random.hpp>`。
- `virtual Color RenderSubPixel(float x, float y)`：单个亚像素采样点的着色：`GetRay(x, y)` 生成光线 → 对 `mTestSceneObject` 调用 `Intersect(ray, isect)` 求交；`SceneObject::Intersect` 内部已用 `isect.t` 收缩 `ray.maxt`，最终 `isect` 即**最近交点**；`bHit` 为 true 时返回法线可视化颜色 `isect.normal * 0.5f + 0.5f`，否则黑色。采样策略与 SSAA 平均仍解耦在 RenderPixel。
- `void RunRenderThread()`：渲染线程入口（消费者循环）。
  - `while (true)` 中 `int pixelIndex = mCurrentPixelIndex.fetch_add(1)` 原子认领像素；
  - `pixelIndex >= W*H` 时 break（一帧全部认领完毕）；
  - 一维下标反算二维坐标：`x = pixelIndex % W`，`y = pixelIndex / W`；
  - 调 `RenderPixel(x, y)` 得线性颜色，各通道 `round(c*255)` 后 `clamp` 到 `[0,255]`，打包 `(r<<16)|(g<<8)|b` 写入 `mBuffer`。

### 线程模型

"原子像素队列"模式：渲染队列 `mCurrentPixelIndex` 是唯一同步点，无需互斥锁；每个渲染线程处理完一帧的所有像素后自动退出。主线程只负责 Present。

已知取舍：线程 `detach()` 且 Present 直接读 `mBuffer`，与渲染线程写之间无同步（单缓冲撕裂风险，当前场景可接受）。

## 程序入口 main.cpp

`Renderer renderer(800, 600, 100); renderer.Run();`（第三参数为每像素采样数 SPP）

## 依赖

- minifb：窗口 + 软件呈现（`mfb_open_ex` / `mfb_update_ex` / `mfb_wait_sync`）
- GLM：`glm::clamp`
- 标准库：`<thread>`、`<atomic>`、`<vector>`、`<cmath>`（round）、`<chrono>`（sleep 模拟耗时）
