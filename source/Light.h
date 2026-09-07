#pragma once

#include "Common.h"

// 光源抽象基类：所有具体光源（平行光/点光/聚光）都给出
//   - 在给定点 p 处的入射辐射（radiance）Color
//   - 以及该辐射"来自"的位置 sourcePos（供阴影光线追踪使用）
//   位置/方向等参数都属于世界坐标系；Radiance/Intensity 为线性 RGB 颜色
class Light
{
public:
    // 对于点 p，求它的入射辐射与光源位置
    //   p        ：被着色点（世界空间）
    //   sourcePos：输出，光源在世界空间中的位置（点光/聚光为光源位置；平行光取沿光线反推的一点）
    //   返回值   ：入射到 p 的辐射（Color）
    virtual Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const = 0;
};

// 平行光：来自无穷远、方向固定的光（模拟太阳光）
class DirectionalLight : public Light
{
public:
    // direction：光线方向（世界空间，将被归一化）
    // radiance ：入射辐射强度（线性 RGB）
    DirectionalLight(const Vector3f& direction, const Color& radiance)
        : mDirection(glm::normalize(direction))
        , mRadiance(radiance)
    {
    }

    Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const override
    {
        // 假设光源在无限远处，沿光线反方向取一个很远的点作为 sourcePos
        sourcePos = p - mDirection * 100000.0f;
        return mRadiance;
    }

private:
    Vector3f mDirection; // 光线方向（世界空间，单位向量）
    Color   mRadiance;  // 入射辐射强度（线性 RGB）
};

// 点光源：从一个位置向四周均匀辐射、按距离衰减的光
class PointLight : public Light
{
public:
    // position   ：光源位置（世界空间）
    // intensity  ：光源强度（线性 RGB）
    // attenuations：衰减系数 (A, B, C)，实际衰减 = 1 / (C + B*R + A*R²)
    PointLight(const Vector3f& position, const Color& intensity, const Vector3f& attenuations)
        : mPosition(position)
        , mIntensity(intensity)
        , mAttenuations(attenuations)
    {
    }

    Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const override
    {
        sourcePos = mPosition;
        float R = glm::length(p - mPosition);
        // 防止距离为 0 导致除 0：给 R 加一个极小下界
        R = glm::max(R, 1e-4f);
        float attenuation = 1.0f / (mAttenuations.z + mAttenuations.y * R + mAttenuations.x * R * R);
        return mIntensity * attenuation;
    }

private:
    Vector3f mPosition;      // 光源位置（世界空间）
    Color   mIntensity;     // 光照强度（线性 RGB）
    Vector3f mAttenuations; // 衰减系数 (A, B, C)
};

// 聚光灯：在点光源基础上加上圆锥角度限制（内/外锥之间的角度有平滑过渡）
class SpotLight : public Light
{
public:
    // position     ：光源位置（世界空间）
    // direction    ：聚光朝向（世界空间，将被归一化）
    // intensity    ：光源强度（线性 RGB）
    // innerAngle   ：内锥半角（弧度 alpha）
    // outerAngle   ：外锥半角（弧度 beta，alpha < beta）
    // attenuations ：衰减系数 (A, B, C)
    SpotLight(const Vector3f& position, const Vector3f& direction, const Color& intensity,
              float innerAngle, float outerAngle, const Vector3f& attenuations)
        : mPosition(position)
        , mDirection(glm::normalize(direction))
        , mIntensity(intensity)
        , mCosInnerAngle(std::cos(innerAngle))
        , mCosOuterAngle(std::cos(outerAngle))
        , mAttenuations(attenuations)
    {
    }

    Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const override
    {
        sourcePos = mPosition;

        // 距离衰减 k1
        float R = glm::length(p - mPosition);
        R = glm::max(R, 1e-4f);
        float k1 = 1.0f / (mAttenuations.z + mAttenuations.y * R + mAttenuations.x * R * R);

        // 角度衰减 k2：cosTheta 在 [cosOuterAngle, cosInnerAngle] 之间时从 0 平滑过渡到 1
        Vector3f L = glm::normalize(p - mPosition);
        float cosTheta = glm::dot(L, mDirection);
        float k2 = (cosTheta - mCosOuterAngle) / (mCosInnerAngle - mCosOuterAngle);

        return mIntensity * k1 * glm::clamp(k2, 0.0f, 1.0f);
    }

private:
    Vector3f mDirection;      // 光线方向（世界空间，单位向量）
    Vector3f mPosition;       // 光源位置（世界空间）
    Color   mIntensity;      // 光照强度（线性 RGB）
    float   mCosInnerAngle;  // 内锥半角的余弦（alpha）
    float   mCosOuterAngle;  // 外锥半角的余弦（beta）
    Vector3f mAttenuations;  // 衰减系数 (A, B, C)
};
