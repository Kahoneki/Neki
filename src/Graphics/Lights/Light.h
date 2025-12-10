#pragma once

#include <glm/glm.hpp>


namespace NK
{

	class Light
	{
	public:
		virtual ~Light() = default;

		inline void SetColour(const glm::vec3 _colour) { m_colour = _colour; m_dirty = true; }
		inline void SetIntensity(const float _intensity) { m_intensity = _intensity; m_dirty = true; }
		inline void SetDirty(const bool _dirty) { m_dirty = _dirty; m_dirty = true; }
		
		[[nodiscard]] inline glm::vec3 GetColour() const { return m_colour; }
		[[nodiscard]] inline float GetIntensity() const { return m_intensity; }
		[[nodiscard]] inline bool GetDirty() const { return m_dirty; }
		
		
	protected:
		explicit Light() = default;

		glm::vec3 m_colour;
		float m_intensity;

		//True if any values have changed
		//Gets cleaned by the render layer when the updated values are sent to the gpu
		bool m_dirty{ true };
	};
	
}