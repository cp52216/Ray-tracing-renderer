#pragma once

#include "SceneObject.h"
#include "Light.h"
#include "Camera.h"
#include <vector>
#include <utility>

// 场景：管理 相机 + 所有场景对象（SceneObject） + 所有光源（Light）
//   - 相机的位置/朝向等参数都在世界坐标系中
//   - SceneObject 的摆放属性（position/euler/scale）也相对世界坐标系
//   - 光源的位置/方向也是世界坐标系
//   - Scene 拥有 SceneObject 与 Light，析构时统一释放
class Scene
{
public:
    // 从 XML 文件加载场景（文件缺失/解析失败返回 nullptr）：
    //   filepath ：XML 路径（相对工作目录）
    //   W, H     ：视口宽高（相机 Initialize 需要用来生成投影/视口矩阵）
    static Scene* LoadSceneFromXML(const char* filepath, int W, int H);

    // 设置相机：camera 内部的位置/目标/上向量均为世界空间
    void SetCamera(const Camera& camera) { mCamera = camera; }

    // 获取相机（供渲染端生成世界空间光线）
    const Camera& GetCamera() const { return mCamera; }

    // 创建场景对象并纳入场景管理：
    //   position：物体中心在世界坐标系中的平移位置（世界空间）
    //   euler   ：物体相对于世界坐标轴的欧拉角（弧度，世界空间下的朝向）
    //   scale   ：物体在自身对象空间各轴上的缩放比例（对象空间）
    SceneObject* CreateSceneObject(const Vector3f& position, const Vector3f& euler, float scale);

    // 创建光源并纳入场景管理：内部 new 出具体光源并 push 进 mLights，返回 T*。
    //   T    ：光源类型（DirectionalLight / PointLight / SpotLight）
    //   args ：光源构造参数（自动转发）
    //   返回值：新创建的光源指针（所有权归本 Scene）
    template<typename T, typename... Args>
    T* CreateLight(Args&&... args)
    {
        Light* light = new T(std::forward<Args>(args)...);
        mLights.push_back(light);
        return static_cast<T*>(light);
    }

    // 获取所有光源（供渲染端做直接光照 / 阴影计算）
    const std::vector<Light*>& GetLights() const { return mLights; }

    // 与场景中所有对象求最近交点（ray/isect 均在世界空间）：
    //   命中后用 isect.t 收缩 ray.maxt，返回命中的场景对象（供后续着色/取材质），未命中返回 nullptr
    SceneObject* Intersect(Ray ray, Intersection& isect) const;

    // 析构：释放所有场景对象与光源
    ~Scene();

private:
    Camera mCamera;                         // 场景相机（世界空间）
    std::vector<SceneObject*> mSceneObjects; // 场景中所有对象（Scene 拥有）
    std::vector<Light*>       mLights;      // 场景中所有光源（Scene 拥有）
};
