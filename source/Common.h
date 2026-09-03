#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;
using Vector2i = glm::ivec2;
using Vector3i = glm::ivec3;
using Vector4i = glm::ivec4;
using Matrix3x3 = glm::mat3;
using Matrix4x4 = glm::mat4;
using Color     = glm::vec3;

//构造局部→世界的平移矩阵（GLM 列主序：每 4 个参数构成一列）
//  t：对象在世界坐标系中的平移量（世界空间）
//  列0=(1,0,0,0) 列1=(0,1,0,0) 列2=(0,0,1,0) 列3=(t.x,t.y,t.z,1)
inline Matrix4x4 MakeTranslation(const Vector3f& t)
{
    return Matrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
                     0.0f, 1.0f, 0.0f, 0.0f,
                     0.0f, 0.0f, 1.0f, 0.0f,
                     t.x,  t.y,  t.z,  1.0f);
}

//构造局部→世界的旋转矩阵（传入欧拉角，弧度）
//  euler：对象相对于世界坐标轴的欧拉角（世界空间下的朝向）
//先绕 x 轴旋转，再绕 y 轴旋转，最后绕 z 轴旋转（作用于向量时）
//即最终 R = Rz * Ry * Rx，矩阵从右往左作用到列向量上
inline Matrix4x4 MakeRotation(const Vector3f& euler)
{
    //先绕x轴旋转
    float cx = cosf(euler.x);
    float sx = sinf(euler.x);
    Matrix4x4 rx(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,  cx,  sx, 0.0f,
        0.0f, -sx,  cx, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    //再绕y轴旋转
    float cy = cosf(euler.y);
    float sy = sinf(euler.y);
    Matrix4x4 ry(
         cy, 0.0f, -sy, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
         sy, 0.0f,  cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    //最后绕z轴旋转
    float cz = cosf(euler.z);
    float sz = sinf(euler.z);
    Matrix4x4 rz(
         cz,  sz, 0.0f, 0.0f,
        -sz,  cz, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

    return rz * ry * rx; //注意矩阵乘法的顺序
}

//构造局部→世界的缩放矩阵（列主序），传入缩放因子 V3
//  s：对象在自身局部坐标系各轴上的缩放比例（对象空间）
//  列0=(s.x,0,0,0) 列1=(0,s.y,0,0) 列2=(0,0,s.z,0) 列3=(0,0,0,1)
inline Matrix4x4 MakeScale(const Vector3f& s)
{
    return Matrix4x4(s.x, 0.0f, 0.0f, 0.0f,
                     0.0f, s.y, 0.0f, 0.0f,
                     0.0f, 0.0f, s.z, 0.0f,
                     0.0f, 0.0f, 0.0f, 1.0f);
}

//构造对象空间→世界空间的世界变换矩阵（TRS 组合）：M = T * R * S
//  position：对象在世界坐标系中的平移位置（世界空间）
//  euler   ：对象相对于世界坐标轴的欧拉角（弧度，世界空间下的朝向）
//  scale   ：对象在自身局部坐标轴上的缩放比例（对象空间）
//作用于向量时：先缩放 S、再旋转 R、最后平移 T（矩阵从右往左作用）
inline Matrix4x4 MakeWorld(const Vector3f& position, const Vector3f& euler, const Vector3f& scale)
{
    Matrix4x4 T = MakeTranslation(position);
    Matrix4x4 R = MakeRotation(euler);
    Matrix4x4 S = MakeScale(scale);
    return T * R * S;
}

//便捷重载：三轴等比缩放（统一缩放因子）
inline Matrix4x4 MakeWorldTransform(const Vector3f& position, const Vector3f& euler, float scale)
{
    return MakeWorld(position, euler, Vector3f(scale));
}

//构造的新坐标系到世界坐标系的变换矩阵：
inline Matrix3x3 MakeCoordinateSystem(const Vector3f& w)
{
    // u, v
    Vector3f u(1.0f, 0.0f, 0.0f);

    if (glm::dot(w, u) > 0.99f)
    {
        u = Vector3f(0.0f, 1.0f, 0.0f);
    }

    Vector3f v = glm::cross(w, u);
    u = glm::cross(v, w);

    u = glm::normalize(u);
    v = glm::normalize(v);

    return Matrix3x3(u, v, w); //列主序
}