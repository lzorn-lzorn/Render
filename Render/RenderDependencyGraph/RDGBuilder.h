
#pragma once

namespace render
{

class RDGTexture;
class RDGBuilder
{
public:
	static RDGTexture createTexture();
public:
	RDGBuilder() = default;

	RDGBuilder(RDGBuilder&&) = default;
	RDGBuilder& operator=(RDGBuilder&&) = default;

	RDGBuilder(const RDGBuilder&) = delete;
	RDGBuilder& operator=(const RDGBuilder&) = delete;

public:
	RDGBuilder& addRenderPass();

};

} // namespace render