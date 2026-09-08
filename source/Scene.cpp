#include "Scene.h"
#include "Material.h"

// 创建场景对象：new 出来后交给 mSceneObjects 统一管理（Scene 拥有其生命周期）
SceneObject* Scene::CreateSceneObject(const Vector3f& position, const Vector3f& euler, float scale)
{
    SceneObject* pSceneObject = new SceneObject(position, euler, scale);
    mSceneObjects.push_back(pSceneObject);
    return pSceneObject;
}

// 析构：释放所有材质、光源与场景对象
Scene::~Scene()
{
    for (auto& pair : mMaterials)
    {
        delete pair.second;
    }
    mMaterials.clear();

    for (Light* light : mLights)
    {
        delete light;
    }
    mLights.clear();

    for (SceneObject* pSceneObject : mSceneObjects)
    {
        delete pSceneObject;
    }
    mSceneObjects.clear();
}

// 在世界空间下遍历所有场景对象，求最近交点：
//   每次命中后用 isect.t 收缩 ray.maxt，保证后续命中一定更近，
//   最终 isect 保存最近交点，pHitObject 即最近命中的对象（供后续着色/取材质）
SceneObject* Scene::Intersect(Ray ray, Intersection& isect) const
{
    SceneObject* pHitObject = nullptr;
    for (const auto pSceneObject : mSceneObjects)
    {
        if (pSceneObject->Intersect(ray, isect))
        {
            ray.maxt = isect.t;
            pHitObject = pSceneObject;
        }
    }

    return pHitObject;
}
