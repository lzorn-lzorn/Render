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

VkFormat toVkFormat(EFormat Fomat)
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

VkIndexType toVkIndexType(EIndexFormat Fomat)
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

VkShaderStageFlags toVkShaderStageFlags(EShaderStage Stage)
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

VkBufferUsageFlags toVkBufferUsage(EBufferUsage Usage)
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

VkImageUsageFlags toVkImageUsage(ETextureUsage Usage)
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

VkPrimitiveTopology toVkPrimitiveTopology(EPrimitiveTopology Topology)
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

VkPolygonMode toVkPolygonMode(EFillMode Mode)
{
	return Mode == EFillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
}

VkCullModeFlags toVkCullMode(ECullMode Mode)
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

VkCompareOp toVkCompareOp(ECompareOp Op)
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

VkStencilOp toVkStencilOp(EStencilOp Op)
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

VkSampleCountFlagBits toVkSampleCount(ESampleCount count)
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

VkBlendFactor toVkBlendFactor(EBlendFactor factor)
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

VkBlendOp toVkBlendOp(EBlendOp Op)
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

VkImageLayout toVkImageLayout(EResourceState State)
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

VkPipelineStageFlags toVkPipelineStage(EResourceState State)
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

VkAccessFlags toVkAccessMask(EResourceState State)
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

VkSharingMode toVkSharingMode(ESharingMode Mode)
{
	return (Mode == ESharingMode::Exclusive) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
}

VkMemoryPropertyFlags toVkMemoryPropertyFlags(EMemoryProperty Properties)
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

VkImageAspectFlags toVkImageAspectMask(const RTextureViewDescriptor& ViewDescriptor, EFormat TextureFormat)
{
	if (ViewDescriptor.Aspect != ETextureAspect::Auto)
	{
		return toVkImageAspectMask(ViewDescriptor.Aspect, TextureFormat);
	}

	switch (ViewDescriptor.Type)
	{
	case RTextureViewDescriptor::EViewType::DSV:
		if (isDepthOnlyFormat(TextureFormat))
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		if (isStencilOnlyFormat(TextureFormat))
		{
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		if (isDepthStencilFormat(TextureFormat))
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return 0;
	case RTextureViewDescriptor::EViewType::UAV:
	case RTextureViewDescriptor::EViewType::SRV:
	case RTextureViewDescriptor::EViewType::RTV:
	default:
		return toVkImageAspectMask(ETextureAspect::Auto, TextureFormat);
	}
}


VkImageAspectFlags toVkImageAspectMask(ETextureAspect Aspect, EFormat TextureFormat)
{
	switch (Aspect)
	{
	case ETextureAspect::Color:
		return VK_IMAGE_ASPECT_COLOR_BIT;
	case ETextureAspect::Depth:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	case ETextureAspect::Stencil:
		return VK_IMAGE_ASPECT_STENCIL_BIT;
	case ETextureAspect::DepthStencil:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	case ETextureAspect::Auto:
	default:
		if (isDepthStencilFormat(TextureFormat))
		{
			if (isDepthOnlyFormat(TextureFormat))
			{
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if (isStencilOnlyFormat(TextureFormat))
			{
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}

uint64_t clampCopySize(uint64_t Offset, uint64_t RequestedSize, uint64_t MaxSize)
{
	if (Offset >= MaxSize)
	{
		return 0;
	}

	const uint64_t available_size = MaxSize - Offset;
	if (RequestedSize == 0)
	{
		return available_size;
	}
	return std::min<uint64_t>(RequestedSize, available_size);
}


bool buildMappedMemoryRange(
	VulkanDevice* Device,
	VkDeviceMemory Memory,
	uint64_t BufferSize,
	uint64_t Offset,
	uint64_t Size,
	VkMappedMemoryRange& OutRange)
{
	if (!Device || Memory == VK_NULL_HANDLE)
	{
		return false;
	}

	const uint64_t range_size = clampCopySize(Offset, Size, BufferSize);
	if (range_size == 0)
	{
		return false;
	}

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(Device->getVkPhysicalDevice(), &properties);

	const uint64_t atom_size = std::max<uint64_t>(1, properties.limits.nonCoherentAtomSize);
	const uint64_t aligned_offset = Offset - (Offset % atom_size);
	const uint64_t aligned_end = Align<uint64_t>(Offset + range_size, atom_size);
	const uint64_t clamped_end = std::min<uint64_t>(aligned_end, BufferSize);
	if (clamped_end <= aligned_offset)
	{
		return false;
	}

	OutRange = {};
	OutRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	OutRange.memory = Memory;
	OutRange.offset = static_cast<VkDeviceSize>(aligned_offset);
	OutRange.size = static_cast<VkDeviceSize>(clamped_end - aligned_offset);
	return true;
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
