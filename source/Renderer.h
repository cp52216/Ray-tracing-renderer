#pragma once

#include "Common.h"
#include "Scene.h"
#include "Ray.h"

#include <atomic>
#include <thread>
#include <vector>

class Renderer
{
public:

    // filepath：场景 XML 路径（相对工作目录，如 "../scenes/scene01.xml"）
    // maxDepth：路径追踪的最大递归深度（避免光线无限弹射）
    Renderer(int w, int h, int maxDepth, int samplePerPixel, const char* filepath);
    virtual ~Renderer();

    void Run();

private:
    // 返回像素 (x, y) 的颜色（线性 RGB，分量范围 0.0~1.0）
    //   (x, y) 是屏幕像素坐标，原点在左上角
    virtual Color RenderPixel(int x, int y);

    // 返回亚像素坐标 (x, y) 的颜色：生成世界空间光线并交给 GetRadiance 求着色
    virtual Color RenderSubPixel(float x, float y);

    // 给定一条世界空间光线，递归地求它沿出射方向返回的辐射 Lo（路径追踪）：
    //   - 背景（未命中）返回黑色
    //   - 命中后在命中点处建局部坐标系，把 wo/wi 转到局部后调用材质的 BRDF：
    //       * 直接光照 Lo += Σ BRDF * L_i * max(cosθ, 0)，带阴影光线追踪
    //       * 间接光照 蒙特卡洛：对半球 N 个方向随机采 wi，递归 GetRadiance，
    //         用 Lo += sum * π² / N 估计 f_r * cosθ * dω 的积分
    //   - depth 超过 mMaxDepth 时直接返回黑色，防止无限递归
    Color GetRadiance(const Ray& ray, int depth);

    // 给定一条世界空间光线，求它在场景中交点的入射辐照度 E(p)（不含材质反射）：
    //   - 背景（未命中）返回黑色
    //   - 命中后遍历所有光源累加 E += L * max(cosθ, 0)，带阴影光线追踪
    //   （不乘 BRDF，是"到达该点的光"，与 GetRadiance 的"从该点反射出去的光"相对）
    Color GetIrradiance(const Ray& ray);

    // 渲染线程的入口函数，负责执行渲染循环
    void RunRenderThread();

    int mViewportWidth  = 800;  // 视口宽度（像素）
    int mViewportHeight = 600;  // 视口高度（像素）

    // 屏幕上每个像素的采样次数 (SPP)：
    int SamplePerPixel = 100;

    // 路径追踪的最大递归深度（光线弹射上限）
    int mMaxDepth = 10;

    // 像素缓冲（行优先，格式 0x00RRGGBB）
    uint32_t* mBuffer = nullptr;

    // 渲染队列：原子递增的像素下标，多个渲染线程通过 fetch_add 抢占下一个像素
    std::atomic<int> mCurrentPixelIndex{0};

    // 场景：持有相机与所有场景对象（Scene 拥有它们的生命周期）
    Scene* mScene = nullptr;
};
