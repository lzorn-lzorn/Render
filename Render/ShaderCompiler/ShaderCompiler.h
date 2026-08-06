
#pragma once 

#include <filesystem>
#include <vector>
#include <cstdint>
#include <string>

#include "../RHI/Definitions.h"
namespace render
{

using rhi::EShaderStage;
using rhi::EDescriptorType;

enum class EShaderDiagnosticSeverity
{
	Info,
	Warning,
	Error
};

struct ShaderDiagnosticInfo
{
	EShaderDiagnosticSeverity Severity;
	std::filesystem::path FilePath;
	std::uint32_t Line;
	std::uint32_t Column;
	std::string Message;
};

struct DescriptorRequirement
{
	uint32_t Set;
	uint32_t Binding;
	uint32_t Count;
	EDescriptorType DescriptorType;
	std::string name;
};

struct ShaderReflectionInfo
{
	std::vector<DescriptorRequirement> Descriptors;
	uint32_t PushConstantSize;
	std::vector<uint32_t> VertexInputLocations;
};

struct CompiledShader
{
	std::vector<uint32_t> SpirvCode;
	std::vector<ShaderDiagnosticInfo> Diagnostics;
	ShaderReflectionInfo Reflection;
	uint64_t SourceHash;
	bool IsSuccess;
};

struct ShaderCompileRequest 
{
	std::filesystem::path FilePath;
	std::string Source;
	std::string EntryPoint;
	EShaderStage Stage;
	std::vector<std::filesystem::path> IncludeDirectories;
	std::vector<std::string> Defines;
	bool IsInDebugMode;
	bool IsOpenCodeOptimization;
};

class ShaderCompiler
{

public:
	[[nodiscard]] CompiledShader compile(const ShaderCompileRequest& Request);
};


}