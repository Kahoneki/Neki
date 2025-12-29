#pragma once

#include <glm/glm.hpp>


namespace NK
{

	struct CBoxCollider final
	{
	public:
		glm::vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
	};
	
}