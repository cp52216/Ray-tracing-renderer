#include "SceneObject.h"

SceneObject::~SceneObject()
{
    // 场景对象拥有挂在其下的图元，负责在自身销毁时释放它们
    for (auto primitive : mPrimitives)
    {
        if (primitive)
            delete primitive;
    }
}

// 在世界空间下与挂在本对象下的所有图元求最近交点。
// 命中后用 isect.t 收缩 ray.maxt，保证后续只考虑更近的交点。
bool SceneObject::Intersect(Ray ray, Intersection& isect) const
{
    bool hit = false;
    for (const auto& primitive : mPrimitives)
    {
        if (primitive->Intersect(ray, isect))
        {
            ray.maxt = isect.t;
            hit = true;
        }
    }

    return hit;
}
