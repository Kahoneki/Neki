#pragma once

#include <RHI/IShader.h>


namespace NK
{
	
	class VulkanShader final : public IShader
	{
	public:
		//Path relative to project top-level-directory not including the file extension (i.e.: for TLD/Shaders/a/b.hlsl, pass in "Shaders/a/b")
		VulkanShader(ILogger& _logger, const ShaderDesc& _desc);
	};

}