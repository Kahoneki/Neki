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
		inline void SetDirty(const bool _dirty) { m_dirty = _dirty; }
		inline void SetShadowMapIndex(const ResourceIndex _index) { m_shadowMapIndex = _index; m_shadowMapDirty = false; }
		inline void SetShadowMapVectorIndex(const std::size_t _index) { m_shadowMapVectorIndex = _index; }
		
		[[nodiscard]] inline glm::vec3 GetColour() const { return m_colour; }
		[[nodiscard]] inline float GetIntensity() const { return m_intensity; }
		[[nodiscard]] inline bool GetDirty() const { return m_dirty; }
		[[nodiscard]] inline bool GetShadowMapDirty() const { return m_shadowMapDirty; }
		[[nodiscard]] inline ResourceIndex GetShadowMapIndex() const { return m_shadowMapIndex; }
		[[nodiscard]] inline std::size_t GetShadowMapVectorIndex() const { return m_shadowMapVectorIndex; }

		
	protected:
		explicit Light() = default;

		glm::vec3 m_colour;
		float m_intensity;

		//True if any values have changed
		//Gets cleaned by the render layer when the updated values are sent to the gpu
		bool m_dirty{ true };

		//True if the shadow map needs to be created (dirtied when CLight.lightType changes or the light has just been created this frame)
		//^todo: changing light type and removing lights is currently not supported
		//Gets cleaned when SetShadowMapIndex() is called
		bool m_shadowMapDirty{ true };

		ResourceIndex m_shadowMapIndex;
		std::size_t m_shadowMapVectorIndex;
	};
	
}