#pragma once

namespace NK
{
	//All the states that a resource can occupy
	enum class RESOURCE_STATE
	{
		UNDEFINED,
		
		//Read-only
		VERTEX_BUFFER,
		INDEX_BUFFER,
		CONSTANT_BUFFER,
		INDIRECT_BUFFER,
		SHADER_RESOURCE, //Equivalent to readonly vulkan storage buffer
		COPY_SOURCE,
		DEPTH_READ,

		//Read/write
		RENDER_TARGET,
		UNORDERED_ACCESS, //Equivalent to readwrite vulkan storage buffer
		COPY_DEST,
		DEPTH_WRITE,

		PRESENT,
	};
}