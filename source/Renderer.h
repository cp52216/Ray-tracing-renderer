#pragma once

#include "Common.h"
#include "Camera.h"
#include "Primitive.h"
#include "SceneObject.h"
#include "Sphere.h"
#include "Disk.h"
#include "Triangle.h"

#include <atomic>
#include <thread>
#include <vector>

class Renderer
{
public:

    Renderer(int w = 800, int h = 600, int samplePerPixel = 10);
    virtual ~Renderer();

    void Run();

    // 返回像素 (x, y) 的颜色（线性 RGB，分量范围 0.0~1.0）
    //   (x, y) 是屏幕像素坐标，原点在左上角
    virtual Color RenderPixel(int x, int y);

    // 返回亚像素坐标 (x, y) 的颜色：生成世界空间光线、与场景求交并着色
    virtual Color RenderSubPixel(float x, float y);

    // 渲染线程的入口函数，负责执行渲染循环
    void RunRenderThread();

protected:
    int mViewportWidth  = 800;  // 视口宽度（像素）
    int mViewportHeight = 600;  // 视口高度（像素）

    // 屏幕上每个像素的采样次数 (SPP)：
    int SamplePerPixel = 100;

    // 像素缓冲（行优先，格式 0x00RRGGBB）
    uint32_t* mBuffer = nullptr;

    // 渲染队列：原子递增的像素下标，多个渲染线程通过 fetch_add 抢占下一个像素
    std::atomic<int> mCurrentPixelIndex{0};

    // 相机：Initialize 后可通过 GetRay(x, y) 生成光线
    Camera mCamera;

    // 当前测试用的单个场景对象（包含多个基本图元）
    SceneObject* mTestSceneObject = nullptr;

};