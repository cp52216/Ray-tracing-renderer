#pragma once

#include "Common.h"
#include "Ray.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

class Camera
{
public:

    // 初始化相机（所有空间参数都在世界坐标系中）
    //   p      ：相机位置（世界空间）
    //   target ：观察目标点（世界空间）
    //   up     ：世界上方向（左手坐标系）
    //   fov    ：垂直视场角（弧度）
    //   n/f    ：近/远裁剪面距离（视图空间）
    //   W/H    ：视口像素宽高
    void Initialize(const Vector3f& p, const Vector3f& target, const Vector3f& up,
                    float fov, float n, float f, int W, int H);

    // 生成穿过像素 (x, y) 的世界空间光线（整型像素中心）
    Ray GetRay(int x, int y) const;

    // 生成穿过亚像素坐标 (x, y) 的世界空间光线（浮点，供 SSAA 随机采样使用）
    Ray GetRay(float x, float y) const;

private:
    Vector3f  mPosition;         // 相机在世界空间中的位置
    Matrix4x4 mCombinedMatrix;     // viewport * projection * view（世界空间 → 屏幕像素空间）
    Matrix4x4 mInvCombinedMatrix;  // 逆矩阵：屏幕像素空间 → 世界空间

};
