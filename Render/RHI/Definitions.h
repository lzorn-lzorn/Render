
#pragma once

#include <cstdint>
namespace render::rhi
{

enum class EResourceType
{
	Buffer,
	Texture,
	Sampler,
	Shader,
	Pipeline,
	BindGroup,
	BindGroupLayout,
	Fence,
	Swapchain
};

enum class EResourceState {
    Undefined,
    Common,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,
    ShaderResource,
    UnorderedAccess,
    Target,
    DepthWrite,
    DepthRead,
    CopySrc,
    CopyDst,
    Present,
};

enum class EFormat : uint32_t
{
	Unkown,
	RGBA8_UNorm,
	RGBA8_sRGB,
	BGRA8_UNorm,
	RGBA16_Float,
	RGBA32_Float,
	D24_UNorm_S8_UInt,
	D32_Float,
	// ...
};

enum class EBufferUsage : uint32_t
{
	Node        = 0,
	Vertex      = 1 << 0,
	Index       = 1 << 1,
	Uniform     = 1 << 2,
	Storage     = 1 << 3,
	Indirect    = 1 << 4,
	TransferSrc = 1 << 5,
	TransferDst = 1 << 6,

};

inline EBufferUsage operator|(EBufferUsage a, EBufferUsage b)
{
	return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class ETextureUsage : uint32_t
{
	None         = 0,
	Sampled      = 1 << 0,
	Storage      = 1 << 1,
	Target = 1 << 2,
	DepthStencil = 1 << 3,
	TransferSrc  = 1 << 4,
	TransferDst  = 1 << 5,
	Present      = 1 << 6,
};
inline ETextureUsage operator|(ETextureUsage a, ETextureUsage b)
{
	return static_cast<ETextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

enum class EShaderStage : uint32_t
{
	Vertex        = 1 << 0,
	Pixel         = 1 << 1,
	Compute       = 1 << 2,
	Geometry      = 1 << 3,
	Mesh	      = 1 << 4,
	Amplification = 1 << 5,
};

enum class ECommandQueueType
{
	Graphics,
	Compute,
	Copy
};

enum class ELoadOp
{
	Load,
	Clear,
	DontCare
};

enum class EStoreOp
{
	Store,
	DontCare
};

enum class EIndexFormat
{
	UInt16,
	UInt32
};

enum class EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriangleList,
	TriangleStrip
};

enum class EDescriptorType
{
	UniformBuffer,
	StorageBuffer,
	Sampler,
	SampledTexture,
	StorageTexture
};

enum class EBlendFactor
{
	Zero,
	One,
	SrcAlpha,
	OneMinusSrcAlpha,
	/* ... */
};

enum class EBlendOp
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
	/* ... */
};

enum class ECompareOp
{
	Less,
	LessEqual,
	Equal,
	NotEqual,
	GreaterEqual,
	Greater,
	Always,
	Never
	/* ... */
};

enum class EStencilOp
{
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	IncrementAndWrap,
	DecrementAndWrap
};

enum class ECullMode
{
	None,
	Front,
	Back
};

enum class EFillMode
{
	Solid,
	Wireframe
};

enum class ESampleCount
{
	Count1 = 1,
	Count2 = 2,
	Count4 = 4,
	Count8 = 8,
	Count16 = 16,
	Count32 = 32,
	Count64 = 64
};

} // namespace render::rhi