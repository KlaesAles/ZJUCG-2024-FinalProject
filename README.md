# ZJUCG-2024-FinalProject
#### 项目结构
```
project/
├── src/                # 源代码
│   ├── main.cpp        # 主程序入口
│   ├── modules/        # 各模块代码
│   │   ├── GameController/
│   │   ├── GUI/
│   │   ├── Scene/
│   │   ├── Modeling/
│   │   └── Lighting/
│   │   └── .../
├── assets/             # 模型和纹理资源
├── docs/               # 设计文档
├── build/              # 编译生成的文件
└── README.md           # 项目介绍
```
#### 项目要求

**分支管理**：

- **主分支**（`main`）：仅存储稳定版本。
- **开发分支**（`dev`）：进行团队整体开发。
- **功能分支**（`feature/xxx`）：每个功能由对应的分支开发，完成后合并到`dev`。

**模块文档**：

- **功能概述**：简要描述模块的功能，例如：“负责场景中地形的生成与管理”。
- **对外接口**：
  - 列出模块提供的所有公共函数和方法。
  - 对每个接口，说明其：
    - **名称**：函数或方法名称。
    - **参数**：每个参数的含义、数据类型和可选值。
    - **返回值**：说明返回的数据类型和意义。
- **对外数据结构**（如有）：
  - 说明模块提供的数据结构或类，包括关键字段的解释。

#### 项目分工

 ##### **Basic**

- **GUI**
  
     - 实现用户界面，包括材质选择、光照调整、模型加载等功能。
     - 提供直观的菜单和工具栏界面。
     -  **负责成员**：______
     
- **控制器 (Controller)**
  
     - 管理游戏状态（编辑模式/漫游模式）。
     - 处理键盘和鼠标输入事件，控制模式切换。
     - **负责成员**：______
- **模型加载**
  
     - 实现 3D 模型的加载。
     - 支持.obj 文件等。
     - **负责成员**：______
     
- **Mesh与Model类**
    - 对接模型加载数据。
    - 实现模型的存储与绘制（VAO，VBO，etc.)  
    - **负责成员**：______
  
- **Shader 类**
     - 管理着色器程序的加载、编译和使用。
     - 提供通用接口，如设置 Uniform 变量函数。
     - **负责成员**：______

- **Camera 类**
     - 管理视图变换，包括摄像机位置、方向和视场角。
     - 实现视角移动、缩放和旋转的功能。
     - **负责成员**：______

- **材质**
     - 提供基础材质（如金属、木材、玻璃等），并支持纹理映射。
     - 支持材质参数的调整（留下相应的接口），如反射率、透明度等。
     - **负责成员**：______

- **光照**
     - 实现光照模型，包括点光源、方向光、聚光灯等。
     - 支持动态调整光源的方向、强度和颜色。
     - **负责成员**：DDKing

- **阴影**
     - 实现实时阴影映射，包括深度缓冲生成和阴影绘制。
     - 支持不同光源类型的阴影计算。
     - **负责成员**：Klaes

- **天空盒**
     - 提供基础天空盒资源（如白天、夜晚、晴天等）。
     - 支持动态加载和切换天空盒纹理。
     - **负责成员**：LYT

- **Scene 类**
    - 管理场景中的所有数据（模型、灯光、摄像机等。
    - 提供场景的加载、保存和更新功能。
    - **负责成员**：_____

- **存储**
    - 实现模型文件的导入导出功能。
    - 支持场景数据存储与读取（如 JSON 格式）。
    - **负责成员**：______

- **渲染模块**
    - 管理场景渲染逻辑，包括光照、阴影、天空盒的渲染。
    - 提供针对复杂场景的性能优化。
    - **负责成员**：______

- **拾取 (MousePicker) 类**
    - 从鼠标点击位置投射光线，判断拾取的地面或物体位置。
    - 实现物体的精确选中和操作功能。
    - **负责成员**：______

- **Record 类**
    - 实现屏幕截图和保存功能。
    - 支持生成场景静态图片以便后续使用。
    - **负责成员**：______

------

##### **Bonus**

- **碰撞箱**
    - 提供物体的碰撞检测功能。
    - 用于角色与场景交互、物理模拟。
    - **负责成员**：______
- **角色控制器与状态机模块**
    - 实现角色的移动、旋转与交互控制。
    - 管理角色的状态（如站立、移动、跳跃等）和动画播放。
    - **负责成员**：______
- **预设游戏场景模块**
    - 制作预定义游戏场景
    - 简单交互与游戏设计（可能需要编写交互类）
    - 支持快速加载这些场景进行展示和测试。
    - **负责成员**：______
- **其他功能（待补充）**
