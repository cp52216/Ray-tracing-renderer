#include "Triangle.h"
#include "SceneObject.h"

Triangle::Triangle(SceneObject* pSceneObject, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2)
    : Primitive(pSceneObject)
{
    // 顶点保存在对象空间，求交时再经 SceneObject 变换到世界空间
    mVertices[0] = v0;
    mVertices[1] = v1;
    mVertices[2] = v2;
}

bool Triangle::Intersect(Ray ray, Intersection& isect) const
{
    // Möller–Trumbore 射线-三角形求交算法（世界空间计算）
    // 顶点用 w=1 变换到世界空间（每次求交时变换，SceneObject 变化后仍正确）
    const Matrix4x4& objectToWorld = m_pSceneObject->GetObjectToWorld();
    Vector3f p0 = Vector3f(objectToWorld * Vector4f(mVertices[0], 1.0f));
    Vector3f p1 = Vector3f(objectToWorld * Vector4f(mVertices[1], 1.0f));
    Vector3f p2 = Vector3f(objectToWorld * Vector4f(mVertices[2], 1.0f));

    Vector3f e1 = p1 - p0;
    Vector3f e2 = p2 - p0;
    Vector3f s = ray.o - p0;

    // 世界空间法线：边向量叉积后归一化（方向依赖顶点绕序）
    Vector3f normal = glm::normalize(glm::cross(e1, e2));

    Vector3f s1 = glm::cross(ray.d, e2);
    Vector3f s2 = glm::cross(s, e1);

    float det = glm::dot(s1, e1);
    if (fabs(det) < 1e-6f) // 射线与三角形所在平面平行，无交点
        return false;

    float invDet = 1.0f / det;
    float b1 = glm::dot(s1, s) * invDet;     // 重心坐标分量 b1
    float b2 = glm::dot(s2, ray.d) * invDet; // 重心坐标分量 b2
    float t = glm::dot(s2, e2) * invDet;     // 射线参数 t

    if (t < ray.mint || t > ray.maxt) // 交点不在射线的有效范围内
        return false;

    float b0 = 1.0f - b1 - b2;
    if (b1 < 0.0f || b2 < 0.0f || b0 < 0.0f) // 交点在三角形外（重心坐标出现负分量）
        return false;

    isect.position = ray.o + t * ray.d; // 交点位置
    isect.normal = normal;              // 交点法线（世界空间）
    isect.t = t;

    return true;
}
