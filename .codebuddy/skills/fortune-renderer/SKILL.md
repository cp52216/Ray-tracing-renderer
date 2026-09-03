---
name: fortune-renderer
description: FortuneRenderer 软件光栅化/光线追踪渲染器项目的模块知识库。解释项目各模块（数学库 Common.h、渲染器框架 Renderer）的代码含义与设计；每次为项目新增或修改功能后，必须把变更同步写入对应模块参考文档和 changelog。当用户要求解释该项目代码、新增渲染功能、或提到"写入 skill"时使用本技能。
---

# Fortune Renderer 项目知识库

本项目是一个基于 minifb 窗口库 + GLM 数学库的 C++17 软件渲染器（Windows，CMake 构建），正在向光线追踪渲染器演进。

## 模块地图

| 模块 | 文件 | 参考文档 |
|------|------|----------|
| 数学库 | `source/Common.h` | [references/common-math.md](references/common-math.md) |
| 渲染器框架 | `source/Renderer.h` / `source/Renderer.cpp` | [references/renderer-core.md](references/renderer-core.md) |
| 相机与射线 | `source/Camera.h/.cpp`、`source/Ray.h` | [references/camera.md](references/camera.md) |
| 几何求交 | `source/Primitive.h`（抽象基类）、`source/SceneObject.h`（变换容器）、`source/Sphere.h/.cpp`、`source/Disk.h/.cpp`、`source/Triangle.h/.cpp` | [references/geometry-primitive.md](references/geometry-primitive.md)、[references/geometry-sphere.md](references/geometry-sphere.md)、[references/geometry-disk.md](references/geometry-disk.md)、[references/geometry-triangle.md](references/geometry-triangle.md) |
| 场景 | `source/Scene.h/.cpp` | [references/scene.md](references/scene.md) |
| 程序入口 | `source/main.cpp` | 见 renderer-core.md |

## 使用流程

1. **解释代码**：先读对应模块的参考文档获得设计背景，再结合源码解释。
2. **新增/修改功能**：修改源码后，必须执行"技能同步"流程（见下）。

## 技能同步流程（每次改动后必须执行）

每当为项目新增或修改了功能，按以下步骤更新本技能：

1. 更新受影响模块的参考文档（`references/common-math.md` 或 `references/renderer-core.md`）：
   - 新增的类/函数/成员：补充到对应章节，写明签名、职责、参数含义。
   - 修改的行为：直接改写文档中过时的描述，不要留旧内容。
   - 删除的内容：从文档中移除。
2. 在 `references/changelog.md` 顶部按日期追加一条变更记录（做了什么、为什么、涉及哪些文件）。
3. 保持文档简练：只记录"是什么、为什么"，不复述整段代码。

## 构建说明

- CMake 项目，`build.bat` 一键生成（配置阶段），在 `build/` 下已有 VS 解决方案可直接编译。
- 头文件包含路径：`minifb/include`、`glm`、`tinyxml2`（见 CMakeLists.txt 的 `target_include_directories`）。
- `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` 收集 `source/` 下所有 `.cpp/.h/.hpp`，新增文件会被自动纳入构建。
- 场景描述：`scenes/*.xml`（tinyxml2 解析），运行时相对工作目录传给 Renderer 构造函数（VS 调试用 `../scenes/scene01.xml`）。

## 关键设计约定

- **调用 `Scene::CreateSceneObject` / `SceneObject::CreatePrimitive` 时，每个实参必须加行尾注释**，标明含义与所属坐标系（见 references/scene.md 的编码规范）。
- GLM 矩阵为列主序：构造函数每 4 个参数构成一列；下标访问 `m[col][row]`。
- 像素缓冲 `mBuffer` 行优先存储，一维下标 `y * mViewportWidth + x`，打包格式 `0x00RRGGBB`。
- `Color` 为线性 RGB（分量 0.0~1.0），由量化函数（round + clamp）转成 0~255 整数。
- 多线程渲染采用"原子像素队列"模式：`mCurrentPixelIndex.fetch_add(1)` 抢占像素，无线程间锁。
