#pragma once

#include "Primitive.h"

// 三角形：三个顶点在对象空间定义
//   构造时传入 SceneObject，决定三角形在世界空间中的摆放
class Triangle : public Primitive
{
public:
    // v0/v1/v2：三角形的三个顶点（对象空间，局部坐标）
    Triangle(SceneObject* pSceneObject, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2);

    // 输入/输出的 ray / isect 都在世界空间
    virtual bool Intersect(Ray ray, Intersection& isect) const override;

private:
    Vector3f mVertices[3]; // 三角形的三个顶点（对象空间，局部坐标）
};
