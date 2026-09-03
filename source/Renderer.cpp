#include "Renderer.h"
#include "Common.h"
#include "MiniFB.h"
#include "Triangle.h"
#include "Sphere.h"
#include <cmath>
#include <chrono>
#include <glm/gtc/random.hpp>

Renderer::Renderer(int w, int h, int samplePerPixel, const char* filepath)
    : mViewportWidth(w)
    , mViewportHeight(h)
    , SamplePerPixel(samplePerPixel)
{
    mCurrentPixelIndex.store(0);

    // 从 XML 文件加载场景（相机与物体都定义在 XML 里）
    mScene = Scene::LoadSceneFromXML(filepath, w, h);

    // ---- 硬编码场景（已被 XML 场景描述取代，保留备查）----
    //SamplePerPixel = samplePerPixel; // 屏幕上每个像素的采样次数 (SPP)

    //// 创建场景（相机与所有场景对象都由 Scene 管理）
    //mScene = new Scene();

    //// 相机参数都是世界空间的，初始化后交给场景保管
    //Camera camera;
    //camera.Initialize(
    //    Vector3f(0, 0, 0),            // 相机位置
    //    Vector3f(0, 0, 1),            // 目标位置
    //    Vector3f(0.0f, 1.0f, 0.0f),   // 上向量
    //    glm::radians(60.0f),          // FOV
    //    0.1f,                         // 近裁剪面
    //    1000.0f,                      // 远裁剪面
    //    w, h);                        // 视口宽高
    //mScene->SetCamera(camera);

    //// 给场景添加物体：
    ////   规则：CreateSceneObject / CreatePrimitive 的每个实参都必须加注释，标明含义与所属坐标系
    //SceneObject* pSceneObject = mScene->CreateSceneObject(
    //    Vector3f(0, 0, 5),   // position：矩形中心位置（世界空间）
    //    Vector3f(0, 0, 0),   // euler   ：无旋转（世界空间下的欧拉角，弧度）
    //    2.0f);               // scale   ：整体放大 2 倍（对象空间）

    //// 三角形 1（矩形右下半）：参数为三个顶点，均在对象空间（局部坐标，z=0 平面）
    //pSceneObject->CreatePrimitive<Triangle>(
    //    Vector3f(-1, -1, 0), // v0：顶点 0（对象空间）
    //    Vector3f(1, -1, 0),  // v1：顶点 1（对象空间）
    //    Vector3f(1, 1, 0));  // v2：顶点 2（对象空间）

    //// 三角形 2（矩形左上半）：与三角形 1 共用对角线，拼成完整矩形
    //pSceneObject->CreatePrimitive<Triangle>(
    //    Vector3f(-1, -1, 0), // v0：顶点 0（对象空间）
    //    Vector3f(1, 1, 0),   // v1：顶点 1（对象空间）
    //    Vector3f(-1, 1, 0)); // v2：顶点 2（对象空间）

    //// 球体：位于矩形前方的测试物体（验证多物体遮挡：球应挡住矩形中心）
    //SceneObject* pSceneObject2 = mScene->CreateSceneObject(
    //    Vector3f(0, 0, 2),   // position：球心位置（世界空间，在矩形 z=5 前方）
    //    Vector3f(0, 0, 0),   // euler   ：无旋转（球体各向同性，世界空间下的欧拉角，弧度）
    //    1.0f);               // scale   ：1（半径由 Sphere 构造参数表达，对象空间）
    //pSceneObject2->CreatePrimitive<Sphere>(
    //    0.5f);               // R：球体半径（对象空间）

    //auto pSphere = new Sphere(Vector3f(-1.0f, -1.0f, 10), 1.0f);
    //auto pDisk = new Disk(Vector3f(0, -2.0f, 5), Vector3f(glm::radians(0.0f), 0, 0), 1.0f);
    //auto pTriangle = new Triangle(Vector3f(-1, 0, 0), Vector3f(0, 1, 0), Vector3f(1, 0, 0));
    //MakeWorldTransform(Vector3f(0, 0, 5), Vector3f(0, glm::radians(45.0f), glm::radians(60.0f)), 2.0f);
}

Renderer::~Renderer()
{
    // Scene 析构时先释放所有 SceneObject，SceneObject 再释放其下挂接的图元
    if (mScene)
        delete mScene;

    if (mBuffer)
    {
        free(mBuffer);
        mBuffer = nullptr;
    }
}

void Renderer::Run()
{
    struct mfb_window* window = mfb_open_ex("Fortune Renderer", mViewportWidth, mViewportHeight, MFB_WF_RESIZABLE);
    if (window == NULL)
        return ;

    // 屏幕/窗口/视口上每个像素点的颜色，以32位整数表示，格式为0xAARRGGBB (Alpha, Red, Green, Blue)
    mBuffer = (uint32_t*)malloc(mViewportWidth * mViewportHeight * 4);

    // 重置渲染队列，准备开始新一帧
    mCurrentPixelIndex.store(0);

    // 创建渲染线程：
    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> renderThreads(numThreads);
    for (int i = 0; i < numThreads; i++)
    {
        renderThreads[i] = std::thread(&Renderer::RunRenderThread, this);
        renderThreads[i].detach();
    }

    // Present：
    mfb_update_state state;
    do {
        state = mfb_update_ex(window, mBuffer, mViewportWidth, mViewportHeight);

        if (state != MFB_STATE_OK)
            break;

    } while (mfb_wait_sync(window));

    free(mBuffer);
    mBuffer = nullptr;
    window = nullptr;

}

// 渲染线程的入口函数，负责执行渲染循环
void Renderer::RunRenderThread()
{
    // 读取当前屏幕的下一个像素
    while (true)
    {
        // 原子地抢占下一个像素下标：每个线程拿到的 pixelIndex 互不重复
        int pixelIndex = mCurrentPixelIndex.fetch_add(1);

        // 所有像素都已被认领，本线程退出
        if (pixelIndex >= mViewportWidth * mViewportHeight)
            break;

        // 一维下标反算回二维坐标（行优先）
        int x = pixelIndex % mViewportWidth;
        int y = pixelIndex / mViewportWidth;

        // 通过 RenderPixel 拿到当前像素的颜色（线性 RGB，分量 0.0~1.0）
        Color color = RenderPixel(x, y);

        // 把每个通道从 0.0~1.0 量化到 0~255：
        //   round(c * 255) 四舍五入；clamp 防止超出 [0, 255]
        uint32_t r = glm::clamp((uint32_t)std::round(color.r * 255.0f), 0u, 255u);
        uint32_t g = glm::clamp((uint32_t)std::round(color.g * 255.0f), 0u, 255u);
        uint32_t b = glm::clamp((uint32_t)std::round(color.b * 255.0f), 0u, 255u);

        // 行优先存储，像素 (x, y) 的下标 = y * mViewportWidth + x
        // 按位左移把 RGB 通道打包成 0x00RRGGBB
        mBuffer[y * mViewportWidth + x] = (r << 16) | (g << 8) | (b);
    }
}

Color Renderer::RenderPixel(int x, int y)
{
    // SSAA：每个像素内多次随机采样后取平均，实现超采样抗锯齿
    const int N = SamplePerPixel; // 每个像素采样的次数
    Color resultColor(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < N; i++)
    {
        // (x, y) - (x+1, y+1) 范围内随机采样一个点:
        float px = x + glm::linearRand(0.0f, 1.0f);
        float py = y + glm::linearRand(0.0f, 1.0f);

        Color color = RenderSubPixel(px, py);
        resultColor += (color / (float)N);
    }

    return resultColor; // 取平均值，得到最终颜色
}

Color Renderer::RenderSubPixel(float x, float y)
{
    Ray ray = mScene->GetCamera().GetRay(x, y); // 世界空间射线
    Intersection isect;
    Color color(0.0f, 0.0f, 0.0f);

    // 与场景在世界空间下求最近交点：
    //   Scene::Intersect 内部用 isect.t 收缩 ray.maxt，返回最近命中的场景对象（供后续取材质着色）
    //bool bHit = false;
    //for (const auto& primitive : mPrimitives)
    //{
    //    if (primitive->Intersect(ray, isect))
    //    {
    //        ray.maxt = isect.t;
    //        bHit = true;
    //    }
    //}
    //if (bHit)
    //{
    //    color = isect.normal * 0.5f + 0.5f; // 将法线向量映射到[0, 1]范围内，作为颜色输出
    //}
    if (mScene->Intersect(ray, isect))
    {
        color = isect.normal * 0.5f + 0.5f; // 将法线向量映射到[0, 1]范围内，作为颜色输出
    }

    return color;
}