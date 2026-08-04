
# 项目描述
该项目是 PotatoEngine 的渲染部分, 定位为一个高性能渲染系统, 内部包括 
- RHI
- RenderGraph
- RenderComponent
- Offline Render 

等功能.

其中 
- SandBox/ 中集成了 SDL3 窗口环境用于测试, 以及相关的 3D 模型文件的解析工具(assimp, ufbx)
- Assets/ 中是一些用于测试的 3D 模型资产

# 关于 MacOS
## DXC 编译器
在 MacOS 应该使用 Mental API, 但是目前该 Render 并没有实现对于 Mental 的封装. 所以临
时使用 MoltenVk. 由于该 Render 使用 HLSL, 但是在 macOS 上 DXC 没有现成的包的版本, 所
以需要从微软仓库中拉取源码进行编译.

```bash
git clone github.com/microsofr/DirectXShaderCompiler.git
cd DirectXShaderCompiler
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release -GNinja
```

## clang
在 macOS 的平台下需要使用 clang 编译器, 原因是 SDL3 在 macOS 下需要使用到 -fobjc-arc 
而 Homebrew GCC/G++ 不支持这个选项, 即便是原本 GCC 起 Object-C 运行时也是第三方开源的
Object-C 运行时, 和苹果自己的运行时也不一样.

所谓 -fobjc-arc 是在编译 Object-C 时才会生效的选项, 用于开启自动应用计数

# 名称规范
## 变量
局部变量: 下划线
函数形参: 大驼峰, 必要时可以使用 In- Out- 前缀和成员变量进行区分
成员变量: 小驼峰
类型名: 小驼峰

## 函数
函数普遍以小驼峰命名, 开头词为动词; 对于名词开头的函数使用大驼峰, 且返回值只能表征执行过程成功与否
而不能直接返回结果