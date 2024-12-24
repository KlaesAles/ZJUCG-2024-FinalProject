负责人：Klaes

-----

**最近更改：**
libs文件夹中添加了nlohmann，以实现json相关功能

---

#### 已完成模块：

- Collision

  - BoundingBox——生成包围盒
  - CollisionManager——计算包围盒重合，处理碰撞
  - 非常不完善

- Controller

  - Camera——Fps摄像机

- Lighting

  - Light——管理光源类型与数据

  - LightManager——管理场景内光源，传递光源数据给Shader

  - ShadowManager——传入场景与光源，为其生成深度贴图并绑定 （点光源阴影未完成）


- Model
  - mesh和model——基于LearnOpenGL实现进行修改，可绑定导入obj文件的纹理和各种数据（目前支持漫反射，镜面光，法线和高度映射四种纹理），并生成包围盒

- Render
  - Renderer——初步从主函数中分离出场景与UI的渲染逻辑

- Scene
  
  - Scene——统一处理场景内物体，生成json文件记录场景，支持导入导出场景
  
    - json记录场景内使用到的模型路径，以及大小方向位置等
  
  - GameObject——保存物体数据，设置大小位置等
  
- Shader
  - Shader——导入生成Shader，设置Shader内参数
  - Model Shader——根据目前实现的功能编写的大概能用的Shader
  - Shadow Shader——用于生成深度贴图的Shader

- stb_image——避免重复定义stb_image

---

#### 目前进度：

**待完善：**

- Light
- LightManager
- ShadowManager（点光源阴影）
- Model Shader
- GameObject
- Scene

**待修改、添加功能：**

- Collision

----

可在Sandbox中查看该分支目前实现效果

- 光照/阴影
- 外部导入模型渲染
- 保存场景/导入场景
- Tab键控制鼠标捕获
- fps摄像机
- imgui功能
  - 查看深度贴图
  - 查看材质贴图
  - 调节视角聚光
  - Save/Load场景
