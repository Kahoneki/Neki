#pragma once
#include <vector>

#include "Core/Debug/ILogger.h"

namespace NK
{
	
	enum class SHADER_TYPE
	{
		VERTEX,
		TESS_CTRL,
		TESS_EVAL,
		FRAGMENT,

		COMPUTE,
	};

	struct ShaderDesc
	{
		SHADER_TYPE type;

		//Path relative to project top-level-directory not including the file extension (i.e.: for TLD/Shaders/a/b.hlsl, path="Shaders/a/b")
		//For the cmake generation process, shader file names should be suffixed according to their type
		//I.e. shader.hlsl should be named:
		//VERTEX: shader_vs.hlsl
		//TESS_CTRL: shader_tcs.hlsl
		//TESS_EVAL: shader_tes.hlsl
		//FRAGMENT: shader_fs.hlsl
		//COMPUTE: shader_cs.hlsl
		std::string filepath;
	};

	
	class IShader
	{
	public:
		virtual ~IShader() = default;

		
	protected:
		IShader(ILogger& _logger, const ShaderDesc& _desc)
		: m_logger(_logger), m_type(_desc.type) {}


		//Dependency injections
		ILogger& m_logger;
		
		SHADER_TYPE m_type;
		std::vector<char> m_bytecode;
	};
	
}