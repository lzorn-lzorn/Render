#include "VulkanRHI.h"

#include "VulkanDevice.h"
#include "../../VulkanFactory.h"

#include <memory>

namespace render::rhi
{

namespace
{

template <typename EnumT>
bool HasFlag(EnumT value, EnumT flag)
{
	using UIntT = std::underlying_type_t<EnumT>;
	return (static_cast<UIntT>(value) & static_cast<UIntT>(flag)) != 0;
}

} // namespace

VkFormat ToVkFormat(EFormat format)
{
	switch (format)
	{
	case EFormat::RGBA8_UNorm: return VK_FORMAT_R8G8B8A8_UNORM;
	case EFormat::RGBA8_sRGB: return VK_FORMAT_R8G8B8A8_SRGB;
	case EFormat::BGRA8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
	case EFormat::RGBA16_Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
	case EFormat::RGBA32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
	case EFormat::D24_UNorm_S8_UInt: return VK_FORMAT_D24_UNORM_S8_UINT;
	case EFormat::D32_Float: return VK_FORMAT_D32_SFLOAT;
	case EFormat::Undefined:
	default:
		return VK_FORMAT_UNDEFINED;
	}
}

VkIndexType ToVkIndexType(EIndexFormat format)
{
	switch (format)
	{
	case EIndexFormat::UInt16: return VK_INDEX_TYPE_UINT16;
	case EIndexFormat::UInt32: return VK_INDEX_TYPE_UINT32;
	case EIndexFormat::None:
	default:
	#ifdef VK_INDEX_TYPE_NONE_KHR
		return VK_INDEX_TYPE_NONE_KHR;
	#else
		return VK_INDEX_TYPE_UINT32;
	#endif
	}
}

VkShaderStageFlags ToVkShaderStageFlags(EShaderStage stage)
{
	VkShaderStageFlags result = 0;
	if (HasFlag(stage, EShaderStage::Vertex)) result |= VK_SHADER_STAGE_VERTEX_BIT;
	if (HasFlag(stage, EShaderStage::Pixel)) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (HasFlag(stage, EShaderStage::Compute)) result |= VK_SHADER_STAGE_COMPUTE_BIT;
	if (HasFlag(stage, EShaderStage::Geometry)) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
	#ifdef VK_SHADER_STAGE_MESH_BIT_EXT
	if (HasFlag(stage, EShaderStage::Mesh)) result |= VK_SHADER_STAGE_MESH_BIT_EXT;
	#endif
	#ifdef VK_SHADER_STAGE_TASK_BIT_EXT
	if (HasFlag(stage, EShaderStage::Amplification)) result |= VK_SHADER_STAGE_TASK_BIT_EXT;
	#endif
	return result;
}

VkBufferUsageFlags ToVkBufferUsage(EBufferUsage usage)
{
	VkBufferUsageFlags flags = 0;
	if (HasFlag(usage, EBufferUsage::Vertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (HasFlag(usage, EBufferUsage::Index)) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (HasFlag(usage, EBufferUsage::Uniform)) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (HasFlag(usage, EBufferUsage::Storage)) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (HasFlag(usage, EBufferUsage::Indirect)) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if (HasFlag(usage, EBufferUsage::TransferSrc)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (HasFlag(usage, EBufferUsage::TransferDst)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	return flags;
}

VkImageUsageFlags ToVkImageUsage(ETextureUsage usage)
{
	VkImageUsageFlags flags = 0;
	if (HasFlag(usage, ETextureUsage::Sampled)) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (HasFlag(usage, ETextureUsage::Storage)) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (HasFlag(usage, ETextureUsage::Target)) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (HasFlag(usage, ETextureUsage::DepthStencil)) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (HasFlag(usage, ETextureUsage::TransferSrc)) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (HasFlag(usage, ETextureUsage::TransferDst)) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	return flags;
}

VkPrimitiveTopology ToVkPrimitiveTopology(EPrimitiveTopology topology)
{
	switch (topology)
	{
	case EPrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	case EPrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	case EPrimitiveTopology::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	case EPrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	case EPrimitiveTopology::TriangleList:
	default:
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
}

VkPolygonMode ToVkPolygonMode(EFillMode mode)
{
	return mode == EFillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

VkCullModeFlags ToVkCullMode(ECullMode mode)
{
	switch (mode)
	{
	case ECullMode::Front: return VK_CULL_MODE_FRONT_BIT;
	case ECullMode::Back: return VK_CULL_MODE_BACK_BIT;
	case ECullMode::None:
	default:
		return VK_CULL_MODE_NONE;
	}
}

VkCompareOp ToVkCompareOp(ECompareOp op)
{
	switch (op)
	{
	case ECompareOp::Less: return VK_COMPARE_OP_LESS;
	case ECompareOp::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
	case ECompareOp::Equal: return VK_COMPARE_OP_EQUAL;
	case ECompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
	case ECompareOp::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
	case ECompareOp::Greater: return VK_COMPARE_OP_GREATER;
	case ECompareOp::Never: return VK_COMPARE_OP_NEVER;
	case ECompareOp::Always:
	default:
		return VK_COMPARE_OP_ALWAYS;
	}
}

VkStencilOp ToVkStencilOp(EStencilOp op)
{
	switch (op)
	{
	case EStencilOp::Keep: return VK_STENCIL_OP_KEEP;
	case EStencilOp::Zero: return VK_STENCIL_OP_ZERO;
	case EStencilOp::Replace: return VK_STENCIL_OP_REPLACE;
	case EStencilOp::IncrementAndClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
	case EStencilOp::DecrementAndClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
	case EStencilOp::Invert: return VK_STENCIL_OP_INVERT;
	case EStencilOp::IncrementAndWrap: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
	case EStencilOp::DecrementAndWrap: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
	default:
		return VK_STENCIL_OP_KEEP;
	}
}

VkSampleCountFlagBits ToVkSampleCount(ESampleCount count)
{
	switch (count)
	{
	case ESampleCount::Count2: return VK_SAMPLE_COUNT_2_BIT;
	case ESampleCount::Count4: return VK_SAMPLE_COUNT_4_BIT;
	case ESampleCount::Count8: return VK_SAMPLE_COUNT_8_BIT;
	case ESampleCount::Count16: return VK_SAMPLE_COUNT_16_BIT;
	case ESampleCount::Count32: return VK_SAMPLE_COUNT_32_BIT;
	case ESampleCount::Count64: return VK_SAMPLE_COUNT_64_BIT;
	case ESampleCount::Count1:
	default:
		return VK_SAMPLE_COUNT_1_BIT;
	}
}

VkBlendFactor ToVkBlendFactor(EBlendFactor factor)
{
	switch (factor)
	{
	case EBlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
	case EBlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
	case EBlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	case EBlendFactor::One:
	default:
		return VK_BLEND_FACTOR_ONE;
	}
}

VkBlendOp ToVkBlendOp(EBlendOp op)
{
	switch (op)
	{
	case EBlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
	case EBlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
	case EBlendOp::Min: return VK_BLEND_OP_MIN;
	case EBlendOp::Max: return VK_BLEND_OP_MAX;
	case EBlendOp::Add:
	default:
		return VK_BLEND_OP_ADD;
	}
}

VkImageLayout ToVkImageLayout(EResourceState state)
{
	switch (state)
	{
	case EResourceState::Target: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case EResourceState::DepthWrite: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case EResourceState::DepthRead: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	case EResourceState::ShaderResource: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case EResourceState::CopySrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	case EResourceState::CopyDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	case EResourceState::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	case EResourceState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
	case EResourceState::Common:
	default:
		return VK_IMAGE_LAYOUT_GENERAL;
	}
}

VkPipelineStageFlags ToVkPipelineStage(EResourceState state)
{
	switch (state)
	{
	case EResourceState::VertexBuffer:
	case EResourceState::IndexBuffer:
		return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
	case EResourceState::ConstantBuffer:
	case EResourceState::ShaderResource:
		return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	case EResourceState::UnorderedAccess:
		return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	case EResourceState::Target:
		return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	case EResourceState::DepthWrite:
	case EResourceState::DepthRead:
		return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	case EResourceState::CopySrc:
	case EResourceState::CopyDst:
		return VK_PIPELINE_STAGE_TRANSFER_BIT;
	case EResourceState::Present:
		return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	case EResourceState::Undefined:
	case EResourceState::Common:
	default:
		return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	}
}

VkAccessFlags ToVkAccessMask(EResourceState state)
{
	switch (state)
	{
	case EResourceState::VertexBuffer: return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	case EResourceState::IndexBuffer: return VK_ACCESS_INDEX_READ_BIT;
	case EResourceState::ConstantBuffer: return VK_ACCESS_UNIFORM_READ_BIT;
	case EResourceState::ShaderResource: return VK_ACCESS_SHADER_READ_BIT;
	case EResourceState::UnorderedAccess: return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	case EResourceState::Target: return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	case EResourceState::DepthWrite: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	case EResourceState::DepthRead: return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	case EResourceState::CopySrc: return VK_ACCESS_TRANSFER_READ_BIT;
	case EResourceState::CopyDst: return VK_ACCESS_TRANSFER_WRITE_BIT;
	case EResourceState::Present:
	case EResourceState::Undefined:
	case EResourceState::Common:
	default:
		return 0;
	}
}

bool IsDepthFormat(EFormat format)
{
	return format == EFormat::D24_UNorm_S8_UInt || format == EFormat::D32_Float;
}

std::unique_ptr<RDevice> CreateVulkanDevice()
{
	auto device = std::make_unique<VulkanDevice>();
	if (!device->isValid())
	{
		return nullptr;
	}
	return device;
}

} // namespace render::rhi
