#pragma once

#include "Light.h"


namespace NK
{

	class DirectionalLight final : public Light
	{
	public:
		explicit DirectionalLight() = default;
		virtual ~DirectionalLight() override = default;

		//Set the half-dimensions of the orthographic projection matrix in world units (centred on the light's position)
		inline void SetDimensions(const glm::vec3 _dimensions) { m_dimensions = _dimensions; m_dirty = true; }

		//Get the half-dimensions of the orthographic projection matrix in world units (centred on the light's position)
		[[nodiscard]] inline glm::vec3 GetDimensions() const { return m_dimensions; }
		
		SERIALISE_MEMBER_FUNC(cereal::base_class<Light>(this), m_dimensions)
		

	private:
		glm::vec3 m_dimensions;
	};
	
}