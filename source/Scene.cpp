#include "Scene.h"
#include "tinyxml2.h"
#include "Sphere.h"
#include "Disk.h"
#include "Triangle.h"
#include <cstdio>

// ---------- XML 解析辅助函数 ----------

// 解析 "x, y, z" 形式的文本为 Vector3f（缺省返回全 0）
static Vector3f ParseVector3f(const char* text)
{
    Vector3f result(0.0f);
    if (text)
    {
        sscanf(text, "%f , %f , %f", &result.x, &result.y, &result.z);
    }
    return result;
}

// 获取子元素的文本内容（子元素不存在返回 nullptr）
static const char* GetChildText(tinyxml2::XMLElement* pParent, const char* name)
{
    tinyxml2::XMLElement* pElem = pParent->FirstChildElement(name);
    return pElem ? pElem->GetText() : nullptr;
}

// 获取子元素的 float 值（子元素缺失或无文本时返回默认值）
static float GetChildFloat(tinyxml2::XMLElement* pParent, const char* name, float defaultValue)
{
    tinyxml2::XMLElement* pElem = pParent->FirstChildElement(name);
    return (pElem && pElem->GetText()) ? (float)atof(pElem->GetText()) : defaultValue;
}

// ---------- 场景加载 ----------

// 从 XML 文件加载整个场景（相机 + 场景对象 + 图元），结构见 scenes/*.xml
Scene* Scene::LoadSceneFromXML(const char* filepath, int W, int H)
{
    if (!filepath)
        return nullptr;

    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filepath) != tinyxml2::XML_SUCCESS)
        return nullptr;

    tinyxml2::XMLElement* pRoot = doc.FirstChildElement("Scene");
    if (!pRoot)
        return nullptr;

    Scene* pScene = new Scene();

    // 解析 Camera（所有参数均为世界空间；Fov 单位为角度）
    tinyxml2::XMLElement* pCameraElem = pRoot->FirstChildElement("Camera");
    if (pCameraElem)
    {
        Vector3f position = ParseVector3f(GetChildText(pCameraElem, "Position"));
        Vector3f target   = ParseVector3f(GetChildText(pCameraElem, "Target"));
        Vector3f up       = ParseVector3f(GetChildText(pCameraElem, "Up"));
        float nearZ = GetChildFloat(pCameraElem, "NearZ", 0.1f);
        float farZ  = GetChildFloat(pCameraElem, "FarZ", 1000.0f);
        float fovDeg = GetChildFloat(pCameraElem, "Fov", 45.0f);
        float fovRad = glm::radians(fovDeg);

        Camera camera;
        camera.Initialize(position, target, up, fovRad, nearZ, farZ, W, H);
        pScene->SetCamera(camera);
    }

    // 解析 SceneObjects
    tinyxml2::XMLElement* pSceneObjectsElem = pRoot->FirstChildElement("SceneObjects");
    if (pSceneObjectsElem)
    {
        for (tinyxml2::XMLElement* pObjElem = pSceneObjectsElem->FirstChildElement("SceneObject");
             pObjElem != nullptr;
             pObjElem = pObjElem->NextSiblingElement("SceneObject"))
        {
            SceneObject* pSceneObject = nullptr;

            // 解析 Transform（物体相对世界坐标系的摆放属性）
            tinyxml2::XMLElement* pTransformElem = pObjElem->FirstChildElement("Transform");
            if (pTransformElem)
            {
                Vector3f position = ParseVector3f(GetChildText(pTransformElem, "Position"));
                Vector3f rotation = ParseVector3f(GetChildText(pTransformElem, "Rotation"));
                float scale = GetChildFloat(pTransformElem, "Scale", 1.0f);

                // XML 里旋转写的是角度，这里转成弧度
                rotation = glm::radians(rotation);
                pSceneObject = pScene->CreateSceneObject(position, rotation, scale);
            }

            if (!pSceneObject)
                continue;

            // 解析 Primitives（图元几何参数均在对象空间）
            tinyxml2::XMLElement* pPrimitivesElem = pObjElem->FirstChildElement("Primitives");
            if (pPrimitivesElem)
            {
                // Sphere：半径
                for (tinyxml2::XMLElement* pElem = pPrimitivesElem->FirstChildElement("Sphere");
                     pElem != nullptr;
                     pElem = pElem->NextSiblingElement("Sphere"))
                {
                    float radius = GetChildFloat(pElem, "Radius", 1.0f);
                    pSceneObject->CreatePrimitive<Sphere>(radius);
                }

                // Disk：半径
                for (tinyxml2::XMLElement* pElem = pPrimitivesElem->FirstChildElement("Disk");
                     pElem != nullptr;
                     pElem = pElem->NextSiblingElement("Disk"))
                {
                    float radius = GetChildFloat(pElem, "Radius", 1.0f);
                    pSceneObject->CreatePrimitive<Disk>(radius);
                }

                // Triangle：按顺序读取三个 Vertex
                for (tinyxml2::XMLElement* pElem = pPrimitivesElem->FirstChildElement("Triangle");
                     pElem != nullptr;
                     pElem = pElem->NextSiblingElement("Triangle"))
                {
                    tinyxml2::XMLElement* pV0 = pElem->FirstChildElement("Vertex");
                    tinyxml2::XMLElement* pV1 = pV0 ? pV0->NextSiblingElement("Vertex") : nullptr;
                    tinyxml2::XMLElement* pV2 = pV1 ? pV1->NextSiblingElement("Vertex") : nullptr;

                    if (pV0 && pV1 && pV2)
                    {
                        Vector3f v0 = ParseVector3f(pV0->GetText());
                        Vector3f v1 = ParseVector3f(pV1->GetText());
                        Vector3f v2 = ParseVector3f(pV2->GetText());
                        pSceneObject->CreatePrimitive<Triangle>(v0, v1, v2);
                    }
                }
            }
        }
    }

    // 解析 Lights
    tinyxml2::XMLElement* pLightsElem = pRoot->FirstChildElement("Lights");
    if (pLightsElem)
    {
        // DirectionalLight
        for (tinyxml2::XMLElement* pElem = pLightsElem->FirstChildElement("DirectionalLight");
             pElem != nullptr;
             pElem = pElem->NextSiblingElement("DirectionalLight"))
        {
            Vector3f direction = ParseVector3f(GetChildText(pElem, "Direction"));
            Color    radiance  = ParseVector3f(GetChildText(pElem, "Radiance"));
            pScene->CreateLight<DirectionalLight>(
                direction, // direction：光线方向（世界空间，将被归一化）
                radiance); // radiance ：入射辐射强度（线性 RGB）
        }

        // PointLight
        for (tinyxml2::XMLElement* pElem = pLightsElem->FirstChildElement("PointLight");
             pElem != nullptr;
             pElem = pElem->NextSiblingElement("PointLight"))
        {
            Vector3f position     = ParseVector3f(GetChildText(pElem, "Position"));
            Color    intensity    = ParseVector3f(GetChildText(pElem, "Intensity"));
            Vector3f attenuations = ParseVector3f(GetChildText(pElem, "Attenuations"));
            pScene->CreateLight<PointLight>(
                position,     // position    ：光源位置（世界空间）
                intensity,    // intensity   ：光源强度（线性 RGB）
                attenuations);// attenuations：衰减系数 (A, B, C)
        }

        // SpotLight（角度在 XML 里以"度"为单位，这里转成弧度后传入）
        for (tinyxml2::XMLElement* pElem = pLightsElem->FirstChildElement("SpotLight");
             pElem != nullptr;
             pElem = pElem->NextSiblingElement("SpotLight"))
        {
            Vector3f position     = ParseVector3f(GetChildText(pElem, "Position"));
            Vector3f direction    = ParseVector3f(GetChildText(pElem, "Direction"));
            Color    intensity    = ParseVector3f(GetChildText(pElem, "Intensity"));
            float    innerAngle   = glm::radians(GetChildFloat(pElem, "InnerAngle", 0.0f));
            float    outerAngle   = glm::radians(GetChildFloat(pElem, "OuterAngle", 0.0f));
            Vector3f attenuations = ParseVector3f(GetChildText(pElem, "Attenuations"));
            pScene->CreateLight<SpotLight>(
                position,     // position    ：光源位置（世界空间）
                direction,    // direction   ：聚光朝向（世界空间，将被归一化）
                intensity,    // intensity   ：光源强度（线性 RGB）
                innerAngle,   // innerAngle  ：内锥半角（弧度，alpha）
                outerAngle,   // outerAngle  ：外锥半角（弧度，beta，alpha < beta）
                attenuations);// attenuations：衰减系数 (A, B, C)
        }
    }

    return pScene;
}

// 创建场景对象：new 出来后交给 mSceneObjects 统一管理（Scene 拥有其生命周期）
SceneObject* Scene::CreateSceneObject(const Vector3f& position, const Vector3f& euler, float scale)
{
    SceneObject* pSceneObject = new SceneObject(position, euler, scale);
    mSceneObjects.push_back(pSceneObject);
    return pSceneObject;
}

// 析构：释放所有光源与场景对象
Scene::~Scene()
{
    for (Light* light : mLights)
    {
        delete light;
    }

    for (SceneObject* pSceneObject : mSceneObjects)
    {
        delete pSceneObject;
    }
}

// 在世界空间下遍历所有场景对象，求最近交点：
//   每次命中后用 isect.t 收缩 ray.maxt，保证后续命中一定更近，
//   最终 isect 保存最近交点，pHitObject 即最近命中的对象（供后续着色）
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
