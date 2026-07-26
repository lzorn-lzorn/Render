#pragma once

#include "../RHI/Definitions.h"
namespace render
{


/**
 * @brief 将一次绘制调用的"材质相关状态"打包成一个不可变的, 可哈希的, 可缓存的键,
 *        用于快速查找或创建对应的图形管线(Pipeline State Object), 并在绘制时绑定资源
 */
struct RMaterialBindingDescriptor
{
	rhi::RShaderHandle VertexShader;
	rhi::RShaderHandle PixelShader;

// TODO: 需要接入 Handle 支持
  	rhi::RDescriptorSet* DescriptorSet;
// rhi 不提供 PipelineLayout 的封装
//	rhi::RPipelineLayout PipelineLayout;

	rhi::EFormat ColorFormat;
	rhi::EFormat DepthFormat;
	rhi::ESampleCount SampleCount;

	rhi::EPrimitiveTopology PrimitiveTopology;
	rhi::ECullMode CullMode;
	bool IsFrontCounterClockwise; // Vulkan 的 FRONT_FACE_CLOCKWISE
	bool IsDepthTestEnable;
	bool IsDepthWriteEnable;
	rhi::ECompareOp DepthCompare;
};



} // namespace render