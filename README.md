负责人：Klaes

---

#### 已完成功能：

- Light类——管理光源类型与数据

- LightManager类——管理场景内光源，传递光源数据给Shader

- ShadowManager类——传入场景与光源，为其生成深度贴图并绑定 （点光源阴影未完成）

- GameObject类——统一处理场景内物体（设置大小位置等功能待完善）

- Mesh类和Model类——基于LearnOpenGL实现进行修改，可绑定导入obj文件的纹理和各种数据（目前支持漫反射，镜面光，法线和高度映射四种纹理）

- Shader类和Camera类——直接使用的LearnOpenGL提供的，未进行修改

- Model Shader——根据目前实现的功能编写的大概能用的Shader

- Shadow Shader——用于生成深度贴图的Shader

---

#### 目前进度：

**待完善：**

- Light类
- LightManager类
- ShadowManager类（点光源）
- Model Shader

**待修改、添加功能：**

- GameObject类

**开发中：**

- Scene类用于场景管理？

- Render类封装渲染流程

----

可在Sandbox中查看该分支目前实现效果
