#pragma once

#include "Ray.h"
#include "Primitive.h"
#include "Material.h"
#include <vector>
#include <utility>

// 场景对象：描述“场景里的一个物体”在世界坐标系中的摆放
//   - position / euler / scale 都是这个物体相对于世界坐标系的属性
//   - 图元（Primitive）以对象空间定义，通过 mObjectToWorld 变换到世界空间
class SceneObject
{
public:
    // 构造一个场景对象
    //   position：物体中心在世界坐标系中的平移位置（世界空间）
    //   euler   ：物体相对于世界坐标轴的欧拉角（弧度，世界空间下的朝向）
    //   scale   ：物体在自身对象空间各轴上的缩放比例（对象空间）
    SceneObject(const Vector3f& position, const Vector3f& euler, const Vector3f& scale)
    {
        // mObjectToWorld：把图元的局部坐标变换到世界坐标
        // mWorldToObject：把世界坐标的射线变换到图元的局部坐标
        mObjectToWorld = MakeWorld(position, euler, scale);
        mWorldToObject = glm::inverse(mObjectToWorld);
    }

    // 便捷构造：统一缩放版本（内部走 MakeWorldTransform）
    SceneObject(const Vector3f& position, const Vector3f& euler, float scale)
    {
        mObjectToWorld = MakeWorldTransform(position, euler, scale);
        mWorldToObject = glm::inverse(mObjectToWorld);
    }

    // 与挂在本对象下的所有图元求最近交点
    //   ray / isect 均在 世界空间 中
    bool Intersect(Ray ray, Intersection& isect) const;

    // 把已创建好的图元挂到本对象下
    void AddPrimitive(Primitive* primitive) { mPrimitives.push_back(primitive); }

    // 创建图元并自动挂到本对象下：
    //   T        ：图元类型（Sphere/Disk/Triangle/...）
    //   args     ：图元构造参数（不含 SceneObject*，由本函数自动传入 this）
    //   返回值   ：新创建的图元指针（所有权归本 SceneObject）
    template<typename T, typename... Args>
    T* CreatePrimitive(Args&&... args)
    {
        T* primitive = new T(this, std::forward<Args>(args)...);
        mPrimitives.push_back(primitive);
        return primitive;
    }

    // SceneObject 拥有其下挂接的图元，析构时统一释放
    virtual ~SceneObject();

    Matrix4x4 GetObjectToWorld() const { return mObjectToWorld; }
    Matrix4x4 GetWorldToObject() const { return mWorldToObject; }

    // 材质（SceneObject 拥有引用，材质对象由 Scene 统一管理生命周期）
    void SetMaterial(Material* pMaterial) { m_pMaterial = pMaterial; }
    Material* GetMaterial() const { return m_pMaterial; }

private:
    Matrix4x4 mObjectToWorld; // 对象空间 → 世界空间 的变换矩阵
    Matrix4x4 mWorldToObject; // 世界空间 → 对象空间 的逆变换矩阵

    Material* m_pMaterial = nullptr; // 本场景对象使用的材质

    std::vector<Primitive*> mPrimitives; // 本场景对象包含的所有基本图元
};
