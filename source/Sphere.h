#pragma once

#include "Primitive.h"

class SceneObject;

// 球体：在对象空间 centered at origin，半径 mRadius
//   构造时传入 SceneObject，决定球体在世界空间中的摆放
class Sphere : public Primitive
{
public:
    Sphere(SceneObject* pSceneObject, float R);

    // 输入/输出的 ray / isect 都在世界空间
    virtual bool Intersect(Ray ray, Intersection& isect) const override;

private:
    float mRadius; // 球体在对象空间的半径
};
