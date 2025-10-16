#pragma once

#include <RHI/IShader.h>


namespace NK
{

	class D3D12Shader final : public IShader
	{
	public:
		//Path relative to project top-level-directory not including the file extension (i.e.: for TLD/Shaders/a/b.hlsl, pass in "Shaders/a/b")
		D3D12Shader(ILogger& _logger, const ShaderDesc& _desc);
	};
	
}