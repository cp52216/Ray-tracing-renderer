#pragma once

#include "SceneObject.h"
#include "Camera.h"
#include <vector>

// 场景：管理 相机 + 所有场景对象（SceneObject）
//   - 相机的位置/朝向等参数都在世界坐标系中
//   - SceneObject 的摆放属性（position/euler/scale）也相对世界坐标系
//   - Scene 拥有所有 SceneObject，析构时统一释放
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

    // 与场景中所有对象求最近交点（ray/isect 均在世界空间）：
    //   命中后用 isect.t 收缩 ray.maxt，返回命中的场景对象（供后续着色/取材质），未命中返回 nullptr
    SceneObject* Intersect(Ray ray, Intersection& isect) const;

    // 析构：释放所有场景对象（SceneObject 析构会继续释放其下挂接的图元）
    ~Scene();

private:
    Camera mCamera;                         // 场景相机（世界空间）
    std::vector<SceneObject*> mSceneObjects; // 场景中所有对象（Scene 拥有）
};
