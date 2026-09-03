#pragma once

#include "Primitive.h"

// 圆盘：在对象空间 z=0 平面，圆心在原点，半径 mRadius
//   构造时传入 SceneObject，决定圆盘在世界空间中的摆放
class Disk : public Primitive
{
public:
    Disk(SceneObject* pSceneObject, float radius);

    // 输入/输出的 ray / isect 都在世界空间
    virtual bool Intersect(Ray ray, Intersection& isect) const override;

private:
    float mRadius; // 圆盘在对象空间的半径
};
