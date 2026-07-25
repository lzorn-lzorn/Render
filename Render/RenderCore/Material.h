#pragma once

#include "RHI/Definitions.h"
namespace render
{


/**
 * @brief 将一次绘制调用的"材质相关状态"打包成一个不可变的, 可哈希的, 可缓存的键,
 *        用于快速查找或创建对应的图形管线(Pipeline State Object), 并在绘制时绑定资源
 */
struct RMaterialBindingDescriptor
{
	RShaderHandle VertexShader;
	RShaderHandle PixelShader;

	RDescriptorSet DescriptorSet;
	RPipelineLayout PipelineLayout;

	EFormat ColorFormat;
	EFormat DepthFormat;
	ESampleCount SampleCount;	

	EPrimitiveTopology PrimitiveTopology;
	ECullMode CullMode;
	bool IsFrontCounterClockwise; // Vulkan 的 FRONT_FACE_CLOCKWISE
	bool IsDepthTestEnable;
	bool IsDepthWriteEnable;
	ECompareOp DepthCompare;
};



}