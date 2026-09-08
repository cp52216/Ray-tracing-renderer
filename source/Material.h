#pragma once

#include "Ray.h"

// 材质抽象基类：所有材质给出给定出射方向 wo 和入射方向 wi 的 BRDF 值
//   wo：出射方向（指向相机，世界空间）
//   wi：入射方向（指向光源，世界空间）
class Material
{
public:
    virtual Color BRDF(const Vector3f& wo, const Vector3f& wi) const = 0;
};

// Lambert 漫反射材质：BRDF = albedo / PI（均匀向所有方向散射）
class LambertMaterial : public Material
{
public:
    LambertMaterial(const Color& albedo) : mAlbedo(albedo) {}

    virtual Color BRDF(const Vector3f& wo, const Vector3f& wi) const override
    {
        return mAlbedo * INV_PI;
    }

private:
    Color mAlbedo; // 反射率（线性 RGB）
};
