#pragma once 

#include <unordered_map>
#include "../../Definitions.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{

inline std::unordered_map<EIndexFormat, VkIndexType> IndexFormatToVkIndexTypeMap = {
	{ EIndexFormat::UInt16, VK_INDEX_TYPE_UINT16 },
	{ EIndexFormat::UInt32, VK_INDEX_TYPE_UINT32 }
};

inline std::unordered_map<EFormat, VkFormat> FormatToVkFormatMap = {
	{ EFormat::Undefined, VK_FORMAT_UNDEFINED },
	{ EFormat::RGBA8_UNorm, VK_FORMAT_R8G8B8A8_UNORM },
	{ EFormat::RGBA8_sRGB, VK_FORMAT_R8G8B8A8_SRGB },
	{ EFormat::BGRA8_UNorm, VK_FORMAT_B8G8R8A8_UNORM },
	{ EFormat::RGBA16_Float, VK_FORMAT_R16G16B16A16_SFLOAT },
	{ EFormat::RGBA32_Float, VK_FORMAT_R32G32B32A32_SFLOAT },
	{ EFormat::D24_UNorm_S8_UInt, VK_FORMAT_D24_UNORM_S8_UINT },
	{ EFormat::D32_Float, VK_FORMAT_D32_SFLOAT }
};

inline std::unordered_map<EBlendFactor, VkBlendFactor> BlendFactorToVkBlendFactorMap = {
	{ EBlendFactor::Zero, VK_BLEND_FACTOR_ZERO },
	{ EBlendFactor::One, VK_BLEND_FACTOR_ONE },
	{ EBlendFactor::SrcAlpha, VK_BLEND_FACTOR_SRC_ALPHA },
	{ EBlendFactor::OneMinusSrcAlpha, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA }
};

inline std::unordered_map<EBlendOp, VkBlendOp> BlendOpToVkBlendOpMap = {
	{ EBlendOp::Add, VK_BLEND_OP_ADD },
	{ EBlendOp::Subtract, VK_BLEND_OP_SUBTRACT },
	{ EBlendOp::ReverseSubtract, VK_BLEND_OP_REVERSE_SUBTRACT },
	{ EBlendOp::Min, VK_BLEND_OP_MIN },
	{ EBlendOp::Max, VK_BLEND_OP_MAX }
};

inline std::unordered_map<ECompareOp, VkCompareOp> CompareOpToVkCompareOpMap = {
	{ ECompareOp::Less, VK_COMPARE_OP_LESS },
	{ ECompareOp::LessEqual, VK_COMPARE_OP_LESS_OR_EQUAL },
	{ ECompareOp::Equal, VK_COMPARE_OP_EQUAL },
	{ ECompareOp::NotEqual, VK_COMPARE_OP_NOT_EQUAL },
	{ ECompareOp::GreaterEqual, VK_COMPARE_OP_GREATER_OR_EQUAL },
	{ ECompareOp::Greater, VK_COMPARE_OP_GREATER },
	{ ECompareOp::Always, VK_COMPARE_OP_ALWAYS },
	{ ECompareOp::Never, VK_COMPARE_OP_NEVER }
};

inline std::unordered_map<ECullMode, VkCullModeFlagBits> CullModeToVkCullModeMap = {
	{ ECullMode::None, VK_CULL_MODE_NONE },
	{ ECullMode::Front, VK_CULL_MODE_FRONT_BIT },
	{ ECullMode::Back, VK_CULL_MODE_BACK_BIT }
};

inline std::unordered_map<EFillMode, VkPolygonMode> FillModeToVkPolygonModeMap = {
	{ EFillMode::Solid, VK_POLYGON_MODE_FILL },
	{ EFillMode::Wireframe, VK_POLYGON_MODE_LINE }
};

inline std::unordered_map<EStencilOp, VkStencilOp> StencilOpToVkStencilOpMap = {
	{ EStencilOp::Keep, VK_STENCIL_OP_KEEP },
	{ EStencilOp::Zero, VK_STENCIL_OP_ZERO },
	{ EStencilOp::Replace, VK_STENCIL_OP_REPLACE },
	{ EStencilOp::IncrementAndClamp, VK_STENCIL_OP_INCREMENT_AND_CLAMP },
	{ EStencilOp::DecrementAndClamp, VK_STENCIL_OP_DECREMENT_AND_CLAMP },
	{ EStencilOp::Invert, VK_STENCIL_OP_INVERT },
	{ EStencilOp::IncrementAndWrap, VK_STENCIL_OP_INCREMENT_AND_WRAP },
	{ EStencilOp::DecrementAndWrap, VK_STENCIL_OP_DECREMENT_AND_WRAP }
};

class VulkanDevice;
class VulkanTextureView;

class VulkanDescriptorSetLayout : public RDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(VulkanDevice* InDevice, const RDescriptorSetLayoutDescriptor& InDescriptor);
	~VulkanDescriptorSetLayout();
	virtual EResourceType getType() const override { return EResourceType::DescriptorSetLayout; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RDescriptorSetLayoutDescriptor Descriptor;

};

class VulkanDescriptorSet : public RDescriptorSet
{
public:
	VulkanDescriptorSet(VulkanDevice* InDevice, const RDescriptorSetDescriptor& InDescriptor);
	~VulkanDescriptorSet();
	virtual EResourceType getType() const override { return EResourceType::DescriptorSet; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RDescriptorSetDescriptor Descriptor;

};


} // namespace render::rhi
