#include "Sphere.h"
#include "SceneObject.h"

Sphere::Sphere(SceneObject* pSceneObject, float R)
    : Primitive(pSceneObject)
    , mRadius(R)
{
}

bool Sphere::Intersect(Ray ray, Intersection& isect) const
{
    // ray转到球体的局部空间（变换矩阵由所属 SceneObject 提供）:
    Ray r = m_pSceneObject->GetWorldToObject() * ray;

    // 解二次方程 A·t^2 + B·t + C = 0（球心在原点：dot(o+t·d, o+t·d) = R^2）
    float A = glm::dot(r.d, r.d);
    float B = 2.0f * glm::dot(r.d, r.o);
    float C = glm::dot(r.o, r.o) - mRadius * mRadius;

    // 判别式 < 0：无交点
    float delta = B * B - 4.0f * A * C;
    if (delta < 0.0f)
        return false;

    float sqrtDelta = sqrtf(delta);
    float t1 = (-B - sqrtDelta) / (2.0f * A);
    float t2 = (-B + sqrtDelta) / (2.0f * A);

    // t1 < t2：整个区间 [t1, t2] 在有效范围之外则无交点
    if (t2 < r.mint)
        return false;

    if (t1 > r.maxt)
        return false;

    // 取有效范围内最近的 t：优先 t1，不满足再用 t2
    float t = t1;
    if (t < r.mint)
    {
        t = t2;
        if (t > r.maxt)
            return false;
    }

    Vector3f p = r.o + t * r.d;       // 交点位置（局部空间）
    Vector3f n = glm::normalize(p);   // 交点法线（局部空间，球心在原点即位置归一化）

    // 交点属性变换回世界空间：位置用 w=1，法线用 w=0（只用旋转部分）再归一化
    isect.position = Vector3f(m_pSceneObject->GetObjectToWorld() * Vector4f(p, 1.0f));
    isect.normal = glm::normalize(Vector3f(m_pSceneObject->GetObjectToWorld() * Vector4f(n, 0.0f)));
    isect.t = t;

    return true;
}
