#pragma once

#include "RHI/RHI.h"
namespace render
{

struct RGeometryView
{
	std::span<const rhi::RVertexBufferView> VertexBuffers;
	std::span<const rhi::RVertexAttribute> VertexAttributes;
	
	rhi::RIndexBufferView IndexBuffer;
	uint32_t VertexCount = 0;
	uint32_t FirstVertex = 0;
	uint32_t FirstInstance = 0;
	uint32_t InstanceCount = 1;

	[[nodiscard]] bool isIndexed() const { return IndexBuffer.isValid(); }
};
}