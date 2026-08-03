
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <cstddef>
#include <type_traits>
#include <concepts>
namespace render::rhi
{

using DeviceSizeType = uint64_t; // device memory size and offset values
// Vk: typedef uint64_t VkDeviceSize;

enum class ESupportedBackendAPI
{
	Vulkan
};
enum class EResourceType
{
	Buffer,
	Texture,
	Sampler,
	Shader,
	Pipeline,
	DescriptorSet,
	DescriptorSetLayout,
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
	Undefined,
	RGBA8_UNorm,
	RGBA8_sRGB,
	BGRA8_UNorm,
	RGBA16_Float,
	RGBA32_Float,
	D16_UNorm,
	D24_UNorm_S8_UInt,
	D32_Float,
	// ...
};

inline bool IsDepthFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt || Format == EFormat::D32_Float;
}

inline bool IsDepthOnlyFormat(EFormat Format)
{
	switch (Format) {
        case EFormat::D16_UNorm:
        case EFormat::D32_Float:
            return true;
        default:
            return false;
    }
}

inline bool IsStencilOnlyFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt;
}

inline bool IsDepthStencilFormat(EFormat Format)
{
	return Format == EFormat::D24_UNorm_S8_UInt || Format == EFormat::D32_Float;
}

inline uint32_t CalPixelSizeFormEFormat(EFormat Format)
{
	switch (Format)
	{
	case EFormat::RGBA8_UNorm:
	case EFormat::RGBA8_sRGB:
	case EFormat::BGRA8_UNorm:
		return 4;
	case EFormat::RGBA16_Float:
		return 8;
	case EFormat::RGBA32_Float:
		return 16;
	case EFormat::D24_UNorm_S8_UInt:
		return 4;
	case EFormat::D32_Float:
		return 4;
	default:
		return 0;
	}
}

enum class EMemoryProperty : uint8_t
{
    None           = 0,
    DeviceLocal    = 1 << 0,  // 位于 GPU 显存，访问最快
    HostVisible    = 1 << 1,  // CPU 可映射访问（必须配合 HostVisible 才能用 vkMapMemory）
    HostCoherent   = 1 << 2,  // 自动同步 CPU/GPU 缓存（免去手动 Flush/Invalidate）
    HostCached     = 1 << 3,  // CPU 缓存中保留副本（适合频繁读回的场景）
    LazilyAllocated = 1 << 4, // 惰性分配（用于深度/模板缓冲，节省显存）
};

inline bool operator==(EMemoryProperty a, EMemoryProperty b)
{
	return static_cast<uint8_t>(a) == static_cast<uint8_t>(b);
}
inline bool operator!=(EMemoryProperty a, EMemoryProperty b)
{
	return static_cast<uint8_t>(a) != static_cast<uint8_t>(b);
}

inline EMemoryProperty operator|(EMemoryProperty a, EMemoryProperty b)
{
	return static_cast<EMemoryProperty>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool operator&(EMemoryProperty a, EMemoryProperty b)
{
	return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

enum class EBufferUsage : uint32_t
{
	None        = 0,
	Vertex      = 1 << 0,
	Index       = 1 << 1,
	Uniform     = 1 << 2,
	Storage     = 1 << 3,
	Indirect    = 1 << 4,
	TransferSrc = 1 << 5,
	TransferDst = 1 << 6,
};

enum class EBufferMapMode : uint8_t
{
	Read,
	Write,
	ReadWrite,
	WriteDiscard
};

inline EBufferUsage operator|(EBufferUsage a, EBufferUsage b)
{
	return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EBufferUsage operator&(EBufferUsage a, EBufferUsage b)
{
	return static_cast<EBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class ETextureUsage : uint32_t
{
	None         = 0,
	Sampled      = 1 << 0,
	Storage      = 1 << 1,
	Target       = 1 << 2,
	DepthStencil = 1 << 3,
	TransferSrc  = 1 << 4,
	TransferDst  = 1 << 5,
	Present      = 1 << 6,
};
inline ETextureUsage operator|(ETextureUsage a, ETextureUsage b)
{
	return static_cast<ETextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ETextureUsage operator&(ETextureUsage a, ETextureUsage b)
{
	return static_cast<ETextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

enum class ETextureDimension : uint8_t
{
    Texture1D,          // 1D 纹理
    Texture1DArray,     // 1D 纹理数组
    Texture2D,          // 2D 纹理
    Texture2DArray,     // 2D 纹理数组
    Texture3D,          // 3D 纹理(整个 Depth 当作第三维，不能单独切片)
    Cube,               // 立方体贴图(6 个面，不可独立扩展)
    CubeArray           // 立方体贴图数组(6 的整数倍面)
};

enum class ETextureAspect : uint8_t
{
	Auto,
	Color,
	Depth,
	Stencil,
	DepthStencil
};

enum class ESharingMode
{
	Exclusive,   // GPU 独占模式, 性能更高
	Concurrent   // GPU 并发模式, 允许多个队列同时访问资源, 但性能较低
};

enum class EShaderStage : uint32_t
{
	Vertex        = 1 << 0,
	Pixel         = 1 << 1,
	Compute       = 1 << 2,
	Geometry      = 1 << 3,
	Mesh	      = 1 << 4,
	Amplification = 1 << 5,
};

inline EShaderStage operator|(EShaderStage a, EShaderStage b)
{
	return static_cast<EShaderStage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline EShaderStage operator&(EShaderStage a, EShaderStage b)
{
	return static_cast<EShaderStage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

template <typename EnumT>
inline bool hasAnyFlags(EnumT Value, EnumT Mask)
{
	using UIntT = std::underlying_type_t<EnumT>;
	return (static_cast<UIntT>(Value) & static_cast<UIntT>(Mask)) != 0;
}

enum class ECommandQueueType
{
	Graphics,
	Compute,
	Copy
};

enum class EPipelineType
{
	None,
	Graphics,
	Compute
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
	None,
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

// 向上取整到指定对齐值的整数倍
template <std::integral Ty>
inline Ty Align(Ty Value, Ty Alignment) {
    return (Value + Alignment - 1) / Alignment * Alignment;
}

/**
 * @brief RResource 内封装的是一个 GPU 资源的句柄对象.
 */
class RResource
{
public:
	virtual ~RResource() = default;
	virtual EResourceType getType() const = 0;
	virtual bool isValid() const = 0;
	void setName(const std::string& InName) 
	{ 
		Name = InName; 
		setDebugName(Name);
	}
	const std::string& getName() const { return Name; }
protected:
	virtual void setDebugName(const std::string& InName) = 0;
	std::string Name;
};

class RBuffer : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Buffer; }
	virtual uint64_t getSize() const = 0;
	virtual bool updateData(uint64_t Offset, const void* Data, uint64_t Size)
	{
		(void)Offset;
		(void)Data;
		(void)Size;
		return false;
	}
	virtual void* mapRange(EBufferMapMode MapMode, uint64_t Offset = 0, uint64_t Size = 0)
	{
		(void)MapMode;
		(void)Offset;
		(void)Size;
		return nullptr;
	}
	virtual void unmap() {}
	virtual bool flushMappedRange(uint64_t Offset = 0, uint64_t Size = 0)
	{
		(void)Offset;
		(void)Size;
		return false;
	}
	virtual bool invalidateMappedRange(uint64_t Offset = 0, uint64_t Size = 0)
	{
		(void)Offset;
		(void)Size;
		return false;
	}
	virtual bool isCpuAccessible() const { return false; }
};


struct RBufferDescriptor
{
	EBufferUsage Usage = EBufferUsage::None;
	ESharingMode SharingMode = ESharingMode::Exclusive;
	EMemoryProperty MemoryProperties = EMemoryProperty::DeviceLocal;
	uint64_t Size = 0;
	
	const void* InitialData = nullptr;
	uint64_t InitialDataSize = 0;
	std::string Name;

	bool isCpuAccessible() const noexcept
	{
		return (MemoryProperties & EMemoryProperty::HostVisible) != 0;
	}
};

struct RTextureDescriptor
{
	ETextureUsage Usage = ETextureUsage::None;
	EFormat Format = EFormat::BGRA8_UNorm;
	ETextureDimension Dimension = ETextureDimension::Texture2D;
	uint32_t Width = 1;
	uint32_t Height = 1;
	uint32_t Depth = 1;
	uint32_t MipLevels = 1;
	uint32_t ArrayLayers = 1;
	ESampleCount SampleCount = ESampleCount::Count1;
	ESharingMode SharingMode = ESharingMode::Exclusive;
	bool ShouldGenerateMipmaps = false;
	std::string Name;
};

struct RTextureBulkData
{
	const void *Data = nullptr;
	uint32_t RowPitch = 0; // 每行数据的字节数
	uint32_t SlicePitch = 0; // 每一层(深度)的字节数
	bool hasData() const noexcept { return Data != nullptr && RowPitch > 0 && SlicePitch > 0; }
	
};

struct TextureHelper
{
	struct SubresourceInfo {
		const void* Data;
		uint32_t RowPitch;
		uint32_t SlicePitch;
	};

	// 根据指定的 Mip 级别和数组层索引从一整块连续的初始纹理数据中,切分出对应的子资源数
    // 据块,并提供其数据指针,行跨距和层跨距
	static SubresourceInfo getSubresourceData(const RTextureDescriptor& Desc, const RTextureBulkData* BulkDataPtr, uint32_t MipLevel, uint32_t ArrayLayer)
	{
		if (!BulkDataPtr || MipLevel >= Desc.MipLevels || ArrayLayer >= Desc.ArrayLayers)
        {
			return { nullptr, 0, 0 };
		}

		uint32_t pixel_size = CalPixelSizeFormEFormat(Desc.Format);
		const uint8_t* base_ptr = static_cast<const uint8_t*>(BulkDataPtr->Data);
		uint32_t current_offset = 0;

		// 标准内存布局：mip 优先,同一 mip 内所有 array layer 连续,各层内部按 2D 行/层排列
		for (uint32_t m = 0; m < Desc.MipLevels; ++m) {
			uint32_t mip_width = std::max(1u, Desc.Width  >> m);
			uint32_t mip_height = std::max(1u, Desc.Height >> m);
			uint32_t mip_depth = (Desc.Depth > 0) ? std::max(1u, Desc.Depth >> m) : 1;

			// 计算该 mip 的子资源跨距(需考虑设备对齐要求,这里用 256 字节对齐为例)
			uint32_t row_pitch = Align(mip_width * pixel_size, 256u);
			uint32_t slice_pitch = row_pitch * mip_height; // 适用于 2D 和 3D(3D 的每一层即一张 2D 切片)

			if (m == MipLevel) {
				// 定位到目标 layer
				current_offset += ArrayLayer * slice_pitch * mip_depth;
				return {
					base_ptr + current_offset,
					row_pitch,
					slice_pitch
				};
			} else {
				// 跳过整个 mip 的所有 layer
				current_offset += Desc.ArrayLayers * slice_pitch * mip_depth;
			}
		}
		return { nullptr, 0, 0 };
	}
};



// 纹理更新区域描述
struct RTextureUpdateRegion
{
	uint32_t MipIndex; // 要更新的 mip 级别(0 为最高分辨率)
	uint32_t ArrayLayer; // 纹理数组 / Cube 的面索引(2D 纹理固定为 0)
	uint32_t DstX, DstY, DstZ; // 目标纹理的起始位置
	uint32_t Width, Height, Depth; // 更新区域的尺寸
};


struct RSamplerDescriptor
{
	enum class EFilter
	{
		Nearest,
		Linear
	};
	enum class EAddressMode
	{
		Repeat,
		Mirror,
		Clamp
	};
	
	EFilter MinFilter = EFilter::Linear;
	EFilter MipFilter = EFilter::Linear;
	EFilter MagFilter = EFilter::Linear;
	EAddressMode AddressModeU = EAddressMode::Repeat;
	EAddressMode AddressModeV = EAddressMode::Repeat;
	EAddressMode AddressModeW = EAddressMode::Repeat;
	std::string Name;
};

struct RShaderDescriptor
{
	EShaderStage Stage;
	std::vector<std::byte> ByteCodes;
	std::string Name;
	std::string EntryPoint = "main";
	std::string SourceCode;
};

struct RShaderHandle
{
	uint64_t Value = 0;
};

struct RVertexAttribute
{
	uint32_t Location;
	uint32_t Binding;
	EFormat Format;
	uint32_t Offset;
};

struct RVertexBindingDescriptor
{
	enum class EVertexInputRate
	{
		// 着色器每处理一个顶点, 就会从绑定的缓冲区中按 Stride 前进一次, 读取下一组顶点数据
		Vertex, 
		// 着色器每处理一个实例,才按 stride 前进一次. 也就是说, 同一实例内的所有顶点共享同一份实例数据
		Instance
	};
	uint32_t Binding;
	uint32_t Stride;

	// 着色器在绘制多少个实例后, 才从该绑定中读取下一组实例数据, 仅在 Instance 模式下有效
	uint32_t InstanceStepRate;
	EVertexInputRate Rate = EVertexInputRate::Vertex;
};

struct RVertexBufferView
{
	RBuffer* VertexBuffer;
	DeviceSizeType Size;
	RVertexBindingDescriptor BindingDescriptor; 
	bool isValid() const noexcept { return VertexBuffer && VertexBuffer->isValid(); }
};

struct RIndexBufferView
{
	RBuffer* IndexBuffer;
	DeviceSizeType Offset;
	DeviceSizeType Size;
	EIndexFormat Format;

	bool isValid() const noexcept { return IndexBuffer && IndexBuffer->isValid(); }
};


struct RVertexInputLayout
{
	std::vector<RVertexAttribute> Attributes;
	std::vector<RVertexBindingDescriptor> Bindings;
};


struct RRasterizerState
{
	EFillMode FillMode = EFillMode::Solid;
	ECullMode CullMode = ECullMode::Back;
	bool FrontCounterClockwise = false;
	int32_t DepthBias = 0;
	float DepthBiasClamp = 0.0f;
	float DepthBiasSlopeFactor = 0.0f;
	float SlopeScaledDepthBias = 0.0f;
	bool DepthClipEnable = true;
	bool ScissorEnable = false;
	bool MultisampleEnable = false;
	bool AntialiasedLineEnable = false;
};

struct RBlendAttachment
{
	bool BlendEnable = false;
	EBlendFactor SrcColorBlendFactor = EBlendFactor::One;
	EBlendFactor DstColorBlendFactor = EBlendFactor::Zero;
	EBlendOp ColorBlendOp = EBlendOp::Add;
	EBlendFactor SrcAlphaBlendFactor = EBlendFactor::One;
	EBlendFactor DstAlphaBlendFactor = EBlendFactor::Zero;
	EBlendOp AlphaBlendOp = EBlendOp::Add;
};

struct RBlendState
{
	bool BlendEnable = false;
	std::vector<RBlendAttachment> Attachments;
};

struct RDepthStencilState
{
	bool DepthTestEnable = true;
	bool DepthWriteEnable = true;
	ECompareOp DepthFunc = ECompareOp::Less;
	bool StencilTestEnable = false;
	uint32_t StencilReadMask = 0xFF;
	uint32_t StencilWriteMask = 0xFF;
	struct StencilOpState
	{
		EStencilOp StencilFailOp = EStencilOp::Keep;
		EStencilOp StencilDepthFailOp = EStencilOp::Keep;
		EStencilOp StencilPassOp = EStencilOp::Keep;
		ECompareOp StencilFunc = ECompareOp::Always;
	};
	StencilOpState FrontFace;
	StencilOpState BackFace;

};

struct RGraphicsPipelineDescriptor
{
	class RShader* VertexShader = nullptr;
	class RShader* PixelShader = nullptr;
	class RShader* GeometryShader = nullptr;
	class RShader* MeshShader = nullptr;
	class RShader* AmplificationShader = nullptr;
	
	RVertexInputLayout VertexInputLayout;
	RRasterizerState RasterizerState;
	RBlendState BlendState;
	RDepthStencilState DepthStencilState;

	std::array<EFormat, 8> RenderTargetFormats = {};
	uint32_t RenderTargetCount = 0;
	EFormat DepthStencilFormat = EFormat::Undefined;
	
	EPrimitiveTopology PrimitiveTopology = EPrimitiveTopology::TriangleList;
	uint32_t SampleCount = 1;
};

struct RComputePipelineDescriptor
{
	class RShader* ComputeShader = nullptr;
};




struct RTextureViewDescriptor
{
	enum class EViewType
	{
		SRV,
		UAV,
		RTV,
		DSV
	};
	EViewType Type = EViewType::SRV;
	ETextureAspect Aspect = ETextureAspect::Auto;
	EFormat Format = EFormat::Undefined;
	uint32_t MipLevel = 0;
	uint32_t MipLevelCount = 1;
	uint32_t ArrayLayer = 0;
	uint32_t ArrayLayerCount = 1;
	ETextureDimension Dimension = ETextureDimension::Texture2D;
};

struct RClearValue
{
	std::array<float, 4> Color = {0.0f, 0.0f, 0.0f, 1.0f};
	float Depth = 1.0f;
	uint32_t Stencil = 0;
};

struct RRenderTargetAttachment
{
	class RTextureView* TextureView = nullptr;
	ELoadOp LoadOp = ELoadOp::Clear;
	EStoreOp StoreOp = EStoreOp::Store;
	RClearValue ClearValue;
};

struct RDepthStencilAttachment
{
	class RTextureView* TextureView = nullptr;
	ELoadOp DepthLoadOp = ELoadOp::Clear;
	EStoreOp DepthStoreOp = EStoreOp::Store;
	ELoadOp StencilLoadOp = ELoadOp::Clear;
	EStoreOp StencilStoreOp = EStoreOp::Store;
	RClearValue ClearValue;
};

struct RRenderPassDescriptor
{
	std::array<RRenderTargetAttachment, 8> ColorAttachments;
	uint32_t ColorAttachmentCount = 0;
	RDepthStencilAttachment* DepthStencilAttachment = nullptr;
};

struct RResourceBarrier
{
	class RTexture* Texture = nullptr;
	EResourceState Before = EResourceState::Undefined;
	EResourceState After = EResourceState::Common;
};

struct RDescriptorSetLayoutEntry
{
	uint32_t Binding;
	EShaderStage Stage;
	EDescriptorType Type;
	uint32_t Count = 1;
};

using RDescriptorSetLayoutDescriptor = std::vector<RDescriptorSetLayoutEntry>;

struct RDescriptorBinding
{
	union {
		class RBuffer* Buffer;
		class RTexture* Texture;
		class RSampler* Sampler;
	};
	uint32_t ArrayIndex = 0;
};

struct RDescriptorSetDescriptor
{
	class RDescriptorSetLayout* Layout = nullptr;
	std::vector<RDescriptorBinding> Bindings;
};

struct RPushConstantRangeLimits
{
	uint32_t MaxSize = 128;
	uint32_t MaxRanges = 4;
};
struct RPushConstantRange
{
	EShaderStage VisibleStage;
	uint32_t Offset;
	uint32_t Size;
};

struct RPipelineLayoutDescriptor
{
	std::vector<RDescriptorSetLayout*> DescriptorSetLayouts;
	std::vector<RPushConstantRange> PushConstantRanges;
};

struct RPipelineLayout : public RResource
{
	virtual uint32_t getDescriptorSetLayoutCount() const = 0;
	virtual RDescriptorSetLayout* getDescriptorSetLayout(uint32_t Index) const = 0;
	virtual uint32_t getPushConstantRangeCount() const = 0;

};

class RPipeline : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Pipeline; }
	virtual bool isGraphicsPipeline() const = 0;
	virtual bool isComputePipeline() const = 0;
	virtual EPipelineType getPipelineType() const = 0;
};

struct RViewport 
{
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	float MinDepth = 0.0f;
	float MaxDepth = 1.0f;
};

struct RRect
{
	int32_t X = 0;
	int32_t Y = 0;
	uint32_t Width = 0;
	uint32_t Height = 0;
};

struct RBufferCopyDescriptor
{
	uint32_t SrcOffset = 0;
	uint32_t DstOffset = 0;
	uint32_t Size = 0;
};

struct RTextureCopyDescriptor
{
	uint32_t SrcMipLevel = 0;
	uint32_t SrcArrayLayer = 0;
	uint32_t DstMipLevel = 0;
	uint32_t DstArrayLayer = 0;
	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t Depth = 1;
};




class RTexture : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Texture; }
	virtual class RTextureView* createView(const RTextureViewDescriptor& Descriptor) = 0;
	virtual const RTextureDescriptor& getDescriptor() const = 0;

	/**
	 * @brief 更新纹理数据
	 * @param Region 更新区域描述
	 * @param SrcData CPU端源数据指针, 数据格式应与纹理格式
	 * 	指向连续排列的像素数据, 按行,层顺序存储, 中间没有额外填充, 数据类型格式必须与纹
	 *	理创建时指定的 EFormat 匹配
	 * @param SrcRowPitch 源数据每行的字节数, 如果为 0 则按纹理格式计算
	 * 	一行像素占用的字节数, 可能大于 SrcWidth * 每像素字节数
	 * @param SrcDepthPitch 源数据每层(深度)的字节数, 如果为 0 则按纹理格式计算
	 * 	仅用于 3D 纹理或纹理数组一次更新多个深度和层, 其表示从一层(slice)的起始位置到下
	 *  一层的起始位置的字节数. 对于 2D 纹理或一次只更新一层, 此值为 0
	 */
	virtual void updateTexture(const RTextureUpdateRegion& Region, const void* SrcData, uint32_t SrcRowPitch, uint32_t SrcDepthPitch = 0) = 0;

	/**
	 * @brief GPU自动生成纹理的mipmap
	 */
	virtual void generateMipmaps() = 0;
	virtual ETextureUsage getUsage() const { return getDescriptor().Usage; }
	virtual uint32_t getMipLevels() const { return getDescriptor().MipLevels; }
	virtual uint32_t getArrayLayers() const { return getDescriptor().ArrayLayers; }
	virtual ETextureDimension getDimension() const { return getDescriptor().Dimension; }
	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getDepth() const = 0;
	virtual EFormat getFormat() const = 0;
};

class RTextureView : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Texture; }
	virtual RTexture* getTexture() const = 0;
	virtual RTextureViewDescriptor getDescriptor() const = 0;
};

class RSampler : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Sampler; }
};


class RShader: public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::Shader; }
	virtual EShaderStage getStage() const = 0;
};


class RDescriptorSetLayout : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::DescriptorSetLayout; }

};

class RDescriptorSet : public RResource
{
public:
	virtual EResourceType getType() const override { return EResourceType::DescriptorSet; }

};



class RCommandList 
{
public:
	virtual ~RCommandList() = default;
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual void beginRenderPass(const RRenderPassDescriptor& Descriptor) = 0;
	virtual void endRenderPass() = 0;

	virtual void setGraphicsPipeline(class RPipeline* Pipeline) = 0;
	virtual void setComputePipeline(class RPipeline* Pipeline) = 0;
	virtual void setDescriptorSet(uint32_t Index, class RDescriptorSet* DescriptorSet) = 0;
	virtual void setVertexBuffer(uint32_t Slot, class RBuffer* Buffer) = 0;
	virtual void setIndexBuffer(class RBuffer* Buffer, EIndexFormat Format) = 0;
	virtual void setViewport(const RViewport& Viewport) = 0;
	virtual void setScissorRect(const RRect& Rect) = 0;
	virtual void setPushConstants(EShaderStage stage, uint32_t offset, uint32_t size, const void* data) = 0;

	virtual void draw(uint32_t VertexCount, uint32_t InstanceCount = 1, uint32_t FirstVertex = 0, uint32_t FirstInstance = 0) = 0;
	virtual void drawIndexed(uint32_t IndexCount, uint32_t InstanceCount = 1, uint32_t FirstIndex = 0, int32_t VertexOffset = 0, uint32_t FirstInstance = 0) = 0;

	virtual void dispatch(uint32_t GroupX, uint32_t GroupY = 1, uint32_t GroupZ = 1) = 0;
	
	virtual void copyBuffer(RBuffer* Src, RBuffer* Dst, const RBufferCopyDescriptor& Descriptor) = 0;
	virtual void copyTexture(RTexture* Src, RTexture* Dst, const RTextureCopyDescriptor& Descriptor) = 0;

	virtual void resourceBarrier(const RResourceBarrier& Barriers) = 0;
	virtual void resourceBarriers(std::span<const RResourceBarrier> Barriers) = 0;
};

class RSwapchain
{
public:
	virtual ~RSwapchain() = default;
	virtual EResourceType getType() const { return EResourceType::Swapchain; }
	virtual RTexture* acquireNextTexture() = 0;
	virtual void present() = 0;
	virtual void resize(uint32_t Width, uint32_t Height) = 0;
	virtual uint32_t getCurrentTextureIndex() const = 0;
	virtual uint32_t getTextureCount() const = 0;
	virtual EFormat getFormat() const = 0;
	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
};

class RFence 
{
public:
	virtual ~RFence() = default;
	virtual EResourceType getType() const { return EResourceType::Fence; }
	virtual void signal(uint64_t Value) = 0;
	virtual uint64_t getCompletedValue() const = 0;
	virtual void wait(uint64_t Value) = 0;

};

struct RSwapchainDescriptor
{
	uint32_t Width = 800;
	uint32_t Height = 600;
	EFormat Format = EFormat::BGRA8_UNorm;
	uint32_t BufferCount = 3;
	bool VSync = true;
	void* NativeWindowHandle = nullptr;
	std::string Name;
};

class RDevice
{
public:
	virtual ~RDevice() = default;

	// 资源创建
	virtual RBuffer* createBuffer(const RBufferDescriptor& Descriptor) = 0;
	virtual RBuffer* createStagingBuffer(const void* Data, uint64_t Size, uint64_t InitialDataSize = 0) = 0;
	virtual RTexture* createTexture(const RTextureDescriptor& Descriptor) = 0;
	virtual RTexture* createTexture(const RTextureDescriptor& Descriptor, RTextureBulkData data) = 0;

	virtual RSampler* createSampler(const RSamplerDescriptor& Descriptor) = 0;
	virtual RShader* createShader(const RShaderDescriptor& Descriptor) = 0;
	virtual RPipeline* createGraphicsPipeline(const RGraphicsPipelineDescriptor& Descriptor) = 0;
	virtual RPipeline* createComputePipeline(const RComputePipelineDescriptor& Descriptor) = 0;
	virtual RDescriptorSetLayout* createDescriptorSetLayout(const RDescriptorSetLayoutDescriptor& Descriptor) = 0;
	virtual RDescriptorSet* createDescriptorSet(const RDescriptorSetDescriptor& Descriptor) = 0;
	
	// 命令与同步
	virtual RCommandList* createCommandList(ECommandQueueType Type) = 0;
	virtual RSwapchain* createSwapchain(const RSwapchainDescriptor& Descriptor) = 0;
	virtual RFence* createFence() = 0;

	// 队列提交
	struct QueueSubmitDescriptor
	{
		RCommandList** CommandLists;
		uint32_t CommandListCount;
		RFence* SignalFence = nullptr;
		uint64_t SignalValue = 0;
	};
	virtual void submitCommandLists(ECommandQueueType Type, const QueueSubmitDescriptor& Descriptor) = 0;

	// 同步
	virtual void waitIdle() = 0;

	// 析构资源
	virtual void destroyResource(RResource* Resource) = 0;
};

std::unique_ptr<RDevice> CreateDevice(ESupportedBackendAPI API);
std::unique_ptr<RDevice> CreateDevice(const char* APIName);

} // namespace render::rhi