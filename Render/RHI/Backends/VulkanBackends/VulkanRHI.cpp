#include "VulkanRHI.h"

#include "VulkanDevice.h"

#include <memory>

namespace render::rhi
{

namespace
{

template <typename EnumT>
bool HasFlag(EnumT Value, EnumT Flag)
{
	using UIntT = std::underlying_type_t<EnumT>;
	return (static_cast<UIntT>(Value) & static_cast<UIntT>(Flag)) != 0;
}

} // namespace

VkFormat ToVkFormat(EFormat Fomat)
{
	switch (Fomat)
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

VkIndexType ToVkIndexType(EIndexFormat Fomat)
{
	switch (Fomat)
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

VkShaderStageFlags ToVkShaderStageFlags(EShaderStage Stage)
{
	VkShaderStageFlags result = 0;
	if (HasFlag(Stage, EShaderStage::Vertex)) result |= VK_SHADER_STAGE_VERTEX_BIT;
	if (HasFlag(Stage, EShaderStage::Pixel)) result |= VK_SHADER_STAGE_FRAGMENT_BIT;
	if (HasFlag(Stage, EShaderStage::Compute)) result |= VK_SHADER_STAGE_COMPUTE_BIT;
	if (HasFlag(Stage, EShaderStage::Geometry)) result |= VK_SHADER_STAGE_GEOMETRY_BIT;
	#ifdef VK_SHADER_STAGE_MESH_BIT_EXT
	if (HasFlag(Stage, EShaderStage::Mesh)) result |= VK_SHADER_STAGE_MESH_BIT_EXT;
	#endif
	#ifdef VK_SHADER_STAGE_TASK_BIT_EXT
	if (HasFlag(Stage, EShaderStage::Amplification)) result |= VK_SHADER_STAGE_TASK_BIT_EXT;
	#endif
	return result;
}

VkBufferUsageFlags ToVkBufferUsage(EBufferUsage Usage)
{
	VkBufferUsageFlags flags = 0;
	if (HasFlag(Usage, EBufferUsage::Vertex)) flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (HasFlag(Usage, EBufferUsage::Index)) flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (HasFlag(Usage, EBufferUsage::Uniform)) flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (HasFlag(Usage, EBufferUsage::Storage)) flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (HasFlag(Usage, EBufferUsage::Indirect)) flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	if (HasFlag(Usage, EBufferUsage::TransferSrc)) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (HasFlag(Usage, EBufferUsage::TransferDst)) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	return flags;
}

VkImageUsageFlags ToVkImageUsage(ETextureUsage Usage)
{
	VkImageUsageFlags flags = 0;
	if (HasFlag(Usage, ETextureUsage::Sampled)) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (HasFlag(Usage, ETextureUsage::Storage)) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (HasFlag(Usage, ETextureUsage::Target)) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (HasFlag(Usage, ETextureUsage::DepthStencil)) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (HasFlag(Usage, ETextureUsage::TransferSrc)) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	if (HasFlag(Usage, ETextureUsage::TransferDst)) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	return flags;
}

VkPrimitiveTopology ToVkPrimitiveTopology(EPrimitiveTopology Topology)
{
	switch (Topology)
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

VkPolygonMode ToVkPolygonMode(EFillMode Mode)
{
	return Mode == EFillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

VkCullModeFlags ToVkCullMode(ECullMode Mode)
{
	switch (Mode)
	{
	case ECullMode::Front: return VK_CULL_MODE_FRONT_BIT;
	case ECullMode::Back: return VK_CULL_MODE_BACK_BIT;
	case ECullMode::None:
	default:
		return VK_CULL_MODE_NONE;
	}
}

VkCompareOp ToVkCompareOp(ECompareOp Op)
{
	switch (Op)
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

VkStencilOp ToVkStencilOp(EStencilOp Op)
{
	switch (Op)
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

VkBlendOp ToVkBlendOp(EBlendOp Op)
{
	switch (Op)
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

VkImageLayout ToVkImageLayout(EResourceState State)
{
	switch (State)
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

VkPipelineStageFlags ToVkPipelineStage(EResourceState State)
{
	switch (State)
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

VkAccessFlags ToVkAccessMask(EResourceState State)
{
	switch (State)
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

VkSharingMode ToVkSharingMode(ESharingMode Mode)
{
	return (Mode == ESharingMode::Exclusive) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
}

VkMemoryPropertyFlags ToVkMemoryPropertyFlags(EMemoryProperty Properties)
{
    VkMemoryPropertyFlags Flags = 0;

    if (static_cast<bool>(Properties & EMemoryProperty::DeviceLocal))
        Flags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    if (static_cast<bool>(Properties & EMemoryProperty::HostVisible))
        Flags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    
    if (static_cast<bool>(Properties & EMemoryProperty::HostCoherent))
        Flags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    
    if (static_cast<bool>(Properties & EMemoryProperty::HostCached))
        Flags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    
    if (static_cast<bool>(Properties & EMemoryProperty::LazilyAllocated))
        Flags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

    return Flags;
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


std::unique_ptr<RDevice> CreateDevice(ESupportedBackendAPI API)
{
	if (API == ESupportedBackendAPI::Vulkan)
	{
		return CreateVulkanDevice();
	}
	return nullptr;
}

std::unique_ptr<RDevice> CreateDevice(const char* APIName)
{
	if (!APIName)
	{
		return nullptr;
	}

	if (strcmp(APIName, "Vulkan") == 0)
	{
		return CreateVulkanDevice();
	}
	return nullptr;
}
} // namespace render::rhi
