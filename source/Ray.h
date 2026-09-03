#pragma once

#include "Common.h"
#include <cfloat>

struct Ray
{
    Vector3f o; // 射线起点（世界空间，通常是相机位置）
    Vector3f d; // 射线方向（世界空间单位向量）

    float mint = 0.0f;    // 有效区间下界
    float maxt = FLT_MAX; // 有效区间上界
};

// 用矩阵变换射线：o 当点（w=1），d 当方向（w=0）
inline Ray operator*(const Matrix4x4& m, const Ray& r)
{
    Ray result;
    result.o = Vector3f(m * Vector4f(r.o, 1.0f));
    result.d = Vector3f(m * Vector4f(r.d, 0.0f));
    result.mint = r.mint;
    result.maxt = r.maxt;
    return result;
}

struct Intersection
{
    Vector3f position; // 交点位置（世界空间）
    Vector3f normal;   // 交点法线（世界空间，单位向量）
    float t;           // 射线参数t值，即交点到射线原点的距离
};
