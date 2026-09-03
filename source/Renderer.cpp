#include "Renderer.h"
#include "Common.h"
#include "MiniFB.h"
#include <cmath>
#include <chrono>
#include <glm/gtc/random.hpp>

Renderer::Renderer(int w, int h, int samplePerPixel)
    : mViewportWidth(w)
    , mViewportHeight(h)
{
    SamplePerPixel = samplePerPixel; // 屏幕上每个像素的采样次数 (SPP)

    mCamera.Initialize(
        Vector3f(0.0f, 0.0f, 0.0f),    // 相机位置
        Vector3f(0.0f, 0.0f, 1.0f),    // 目标位置
        Vector3f(0.0f, 1.0f, 0.0f),    // 上向量
        glm::radians(60.0f),           // FOV
        0.1f,                          // 近裁剪面
        1000.0f,                       // 远裁剪面
        w, h);                         // 视口宽高

    // 测试对象：一个位于 (0,0,5) 的矩形，由两个三角形拼成
    //   position/euler/scale 都是该矩形在世界坐标系中的摆放属性
    mTestSceneObject = new SceneObject(Vector3f(0, 0, 5), Vector3f(0, 0, 0), 2.0f);

    // CreatePrimitive 会自动 new 图元、传入 this 并挂到对象下
    mTestSceneObject->CreatePrimitive<Triangle>(
        Vector3f(-1.0f, -1.0f, 0.0f),
        Vector3f( 1.0f, -1.0f, 0.0f),
        Vector3f( 1.0f,  1.0f, 0.0f));

    mTestSceneObject->CreatePrimitive<Triangle>(
        Vector3f(-1.0f, -1.0f, 0.0f),
        Vector3f( 1.0f,  1.0f, 0.0f),
        Vector3f(-1.0f,  1.0f, 0.0f));
}

Renderer::~Renderer()
{
    // SceneObject 析构时会释放其下挂接的所有图元
    delete mTestSceneObject;
    mTestSceneObject = nullptr;

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
    Ray ray = mCamera.GetRay(x, y); // 世界空间射线
    Intersection isect;
    Color color(0.0f, 0.0f, 0.0f);

    // 与测试场景对象在世界空间下求最近交点：
    // SceneObject::Intersect 内部已用 isect.t 收缩 ray.maxt，最终 isect 保存最近交点
    bool bHit = false;
    if (mTestSceneObject && mTestSceneObject->Intersect(ray, isect))
    {
        bHit = true;
    }

    if (bHit)
    {
        color = isect.normal * 0.5f + 0.5f; // 将法线向量映射到[0, 1]范围内，作为颜色输出
    }

    return color;
}