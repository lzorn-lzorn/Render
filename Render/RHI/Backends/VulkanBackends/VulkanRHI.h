#pragma once 
#include "../../Definitions.h"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace render::rhi
{


class VulkanDevice;
class VulkanTextureView;

class VulkanBindGroupLayout : public RBindGroupLayout
{
public:
	VulkanBindGroupLayout(VulkanDevice* InDevice, const RBindGroupLayoutDescriptor& InDescriptor);
	~VulkanBindGroupLayout();
	virtual EResourceType getType() const override { return EResourceType::BindGroupLayout; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RBindGroupLayoutDescriptor Descriptor;

};

class VulkanBindGroup : public RBindGroup
{
public:
	VulkanBindGroup(VulkanDevice* InDevice, const RBindGroupDescriptor& InDescriptor);
	~VulkanBindGroup();
	virtual EResourceType getType() const override { return EResourceType::BindGroup; }

private:
	void SetDebugName(const std::string& Name) override;
	VulkanDevice* Device;
	RBindGroupDescriptor Descriptor;

};


} // namespace render::rhi
