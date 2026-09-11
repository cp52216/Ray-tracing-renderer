#include "Triangle.h"
#include "SceneObject.h"

Triangle::Triangle(SceneObject* pSceneObject, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2)
    : Primitive(pSceneObject)
{
    // 顶点保存原始对象空间数据（备查）
    mVertices[0] = v0;
    mVertices[1] = v1;
    mVertices[2] = v2;

    // ===== 性能优化：预变换缓存（见 Triangle.h 头部注释）=====
    // 为什么能加速：求交是"每条光线都执行"的热路径，而顶点变换是"每个三角形只需一次"的
    // 冷数据准备。把矩阵乘法从热路径挪到构造函数，调用次数从 O(光线数) 降到 O(三角形数)。
    // 路径追踪下光线数 = 像素数 × SPP × (弹射层数 + shadow ray 数)，是天文数字，
    // 而三角形数只有个位数到几百——这笔账就是全部收益来源。
    const Matrix4x4& objectToWorld = m_pSceneObject->GetObjectToWorld();
    mWorldVertices[0] = Vector3f(objectToWorld * Vector4f(v0, 1.0f)); // w=1：按"点"变换（含平移）
    mWorldVertices[1] = Vector3f(objectToWorld * Vector4f(v1, 1.0f));
    mWorldVertices[2] = Vector3f(objectToWorld * Vector4f(v2, 1.0f));

    // 法线同样缓存：旧实现每次求交都叉积+归一化，这里构造时算一次。
    // （注意：三角形是平面，法线处处相同，所以整个三角形缓存一份就够）
    Vector3f e1 = mWorldVertices[1] - mWorldVertices[0];
    Vector3f e2 = mWorldVertices[2] - mWorldVertices[0];
    mWorldNormal = glm::normalize(glm::cross(e1, e2)); // 方向依赖顶点绕序（v0→v1→v2 右手系）
}

bool Triangle::Intersect(Ray ray, Intersection& isect) const
{
    // Möller–Trumbore 射线-三角形求交算法（世界空间计算）
    //
    // 性能优化生效处：p0/p1/p2 直接引用构造时缓存的 mWorldVertices（const 引用，零拷贝），
    // 对比旧实现——每次求交要 3 次 objectToWorld * Vector4f(v, 1) 的矩阵乘法。
    // 这里是全程序执行次数最多的函数，省下的乘法在 SPP=1000 时是每秒数亿次级别。
    const Vector3f& p0 = mWorldVertices[0];
    const Vector3f& p1 = mWorldVertices[1];
    const Vector3f& p2 = mWorldVertices[2];

    Vector3f e1 = p1 - p0;
    Vector3f e2 = p2 - p0;
    Vector3f s  = ray.o - p0;

    Vector3f s1 = glm::cross(ray.d, e2);
    Vector3f s2 = glm::cross(s, e1);

    float det = glm::dot(s1, e1);
    if (fabs(det) < 1e-6f) // 射线与三角形所在平面平行，无交点
        return false;

    float invDet = 1.0f / det;
    float b1 = glm::dot(s1, s) * invDet;     // 重心坐标分量 b1
    float b2 = glm::dot(s2, ray.d) * invDet; // 重心坐标分量 b2
    float t  = glm::dot(s2, e2) * invDet;    // 射线参数 t

    if (t < ray.mint || t > ray.maxt) // 交点不在射线的有效范围内
        return false;

    float b0 = 1.0f - b1 - b2;
    if (b1 < 0.0f || b2 < 0.0f || b0 < 0.0f) // 交点在三角形外（重心坐标出现负分量）
        return false;

    isect.position = ray.o + t * ray.d; // 交点位置（世界空间）
    isect.normal = mWorldNormal;        // 交点法线（世界空间）——直接读构造时缓存的常量，
                                        // 旧实现这里每次都要 cross(e1,e2)+normalize，现在一行拷贝
    isect.t = t;

    return true;
}
