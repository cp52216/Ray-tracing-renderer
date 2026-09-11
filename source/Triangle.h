#pragma once

#include "Primitive.h"

// 三角形：三个顶点在对象空间定义
//   构造时传入 SceneObject，决定三角形在世界空间中的摆放
//
// ---------- 性能优化：世界空间缓存 ----------
// 为什么要这样做：
//   旧实现里 Triangle::Intersect 每被一条光线命中，都要把 3 个顶点各自乘一次
//   4x4 矩阵（对象空间 → 世界空间）。但顶点是"常量"——SceneObject 构造后变换矩阵
//   就再也不变，同一三角形被第 1 条和第 100 万条光线求交，变换结果一模一样。
//   路径追踪中三角形求交是全程序调用频率最高的代码（每像素 SPP 条主光线 ×
//   最多 mMaxDepth 层弹射 × 每层还有 shadow ray，全部都要遍历三角形），
//   把"每条光线 3 次矩阵乘法"变成"每个三角形构造时 3 次"，摊销后几乎为零成本。
// 为什么安全（结果不变）：
//   预变换用的是同一矩阵 GetObjectToWorld()，只是把计算从"每次求交"挪到
//   "构造时"，数值路径完全一致，属于纯性能优化，渲染画面不变。
//   前提是 SceneObject 构造后不被修改（当前项目满足：矩阵只在构造函数里赋值）。
class Triangle : public Primitive
{
public:
    // v0/v1/v2：三角形的三个顶点（对象空间，局部坐标）
    Triangle(SceneObject* pSceneObject, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2);

    // 输入/输出的 ray / isect 都在世界空间
    virtual bool Intersect(Ray ray, Intersection& isect) const override;

private:
    Vector3f mVertices[3];      // 三角形的三个顶点（对象空间，局部坐标，保留作原始数据备查）
    Vector3f mWorldVertices[3]; // 【缓存】顶点预变换到世界空间（构造时算一次，求交时直接读）
    Vector3f mWorldNormal;      // 【缓存】世界空间法线（构造时叉积+归一化算一次；旧实现每次求交都叉积）
};
