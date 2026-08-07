#pragma once

#include "../../RHI.h"
#include "VulkanRHI.h"


namespace render::rhi
{


class VulkanBuffer : public RBuffer
{
public:
	VulkanBuffer(class VulkanDevice* InDevice, const RBufferDescriptor& InBufferDesc);
	~VulkanBuffer() override;

	bool isValid() const override;
	uint64_t getSize() const override;
	bool updateData(uint64_t Offset, const void* Data, uint64_t Size) override;
	void* mapRange(EBufferMapMode MapMode, uint64_t Offset = 0, uint64_t Size = 0) override;
	void unmap() override;
	bool flushMappedRange(uint64_t Offset = 0, uint64_t Size = 0) override;
	bool invalidateMappedRange(uint64_t Offset = 0, uint64_t Size = 0) override;
	bool isCpuAccessible() const override;

	VkBuffer getVkBuffer() const noexcept { return Buffer; }
	VkDeviceMemory getVkDeviceMemory() const noexcept { return Memory; }
	const RBufferDescriptor& getDesc() const noexcept { return BufferDesc; }
private:
	void setDebugName(const std::string& Name) override;

	bool allocateMemory();
    bool uploadInitialData();

	class VulkanDevice* Device = nullptr;
	RBufferDescriptor BufferDesc{};
	VkBuffer Buffer = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	void* MappedPtr = nullptr;
	uint64_t MappedOffset = 0;
	uint64_t MappedSize = 0;
};



}