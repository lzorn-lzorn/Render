#include "VulkanRHI.h"

namespace render::rhi
{

namespace
{
// Compile-time smoke test: forces the compiler to resolve Vulkan and VMA types.
[[maybe_unused]] void VulkanCompileSmokeTest()
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;
	(void)buffer;
	(void)allocation;
}
} // namespace

} // namespace render::rhi
