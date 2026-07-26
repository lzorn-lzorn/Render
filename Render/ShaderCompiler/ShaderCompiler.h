
#pragma once 

#include <filesystem>
#include <vector>
#include <cstdint>
#include <string>

#include "../RHI/Definitions.h"
namespace render
{

using rhi::EShaderStage;

enum class EShaderDiagnosticSeverity
{
	Info,
	Warning,
	Error
};

struct RShaderDiagnosticInfo
{
	EShaderDiagnosticSeverity Severity;
	std::filesystem::path FilePath;
	std::uint32_t Line;
	std::uint32_t Column;
	std::string Message;
};

struct RDescriptorRequirement
{
	uint32_t Set;
	uint32_t Binding;
	uint32_t Count;
	uint32_t DescriptorType;
	std::string name;
};

struct RShaderReflectionInfo
{
	std::vector<RDescriptorRequirement> Descriptors;
	uint32_t PushConstantSize;
	std::vector<uint32_t> VertexInputLocations;
};

struct RCompiledShader
{
	std::vector<uint32_t> SpirvCode;
	std::vector<RShaderDiagnosticInfo> Diagnostics;
	RShaderReflectionInfo Reflection;
	uint64_t SourceHash;
	bool IsSuccess;
};

struct RShaderCompileRequest 
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

class RShaderCompiler
{

public:
	[[nodiscard]] RCompiledShader compile(const RShaderCompileRequest& Request);
};


}