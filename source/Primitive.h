#pragma once

#include "Ray.h"

class SceneObject; // 前向声明，避免循环包含

// 图元抽象基类：所有可求交的几何体（Sphere/Disk/Triangle/...）的公共接口
//   - 图元在对象空间定义，不持有变换矩阵
//   - 摆放信息由所属 SceneObject 提供（对象空间 → 世界空间）
class Primitive
{
public:
    Primitive(SceneObject* pSceneObject) : m_pSceneObject(pSceneObject) {}
    virtual ~Primitive() {}

    // 在世界空间下与射线求交；命中时把 isect 填为世界空间属性并返回 true
    virtual bool Intersect(Ray ray, Intersection& isect) const = 0;

protected:
    // 所属场景对象，提供 GetObjectToWorld() / GetWorldToObject() 两组变换矩阵
    SceneObject* m_pSceneObject = nullptr;
};
