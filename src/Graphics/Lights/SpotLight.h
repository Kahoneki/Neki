#pragma once

#include "PointLight.h"


namespace NK
{

	class SpotLight final : public PointLight
	{
	public:
		explicit SpotLight() = default;
		virtual ~SpotLight() override = default;

		//Set the inner angle of the spot light, in radians
		inline void SetInnerAngle(const float _angle) { m_innerAngle = _angle; m_dirty = true; }
		//Set the outer angle of the spot light, in radians
		inline void SetOuterAngle(const float _angle) { m_outerAngle = _angle; m_dirty = true; }

		//Get the inner angle of the spot light, in radians
		[[nodiscard]] inline float GetInnerAngle() const { return m_innerAngle; }
		//Get the outer angle of the spot light, in radians
		[[nodiscard]] inline float GetOuterAngle() const { return m_outerAngle; }

		SERIALISE_MEMBER_FUNC(cereal::base_class<PointLight>(this), m_innerAngle, m_outerAngle)
		

	private:
		//Inner angle of the spot light, in radians
		float m_innerAngle{ glm::radians(30.0f) };

		//Outer angle of the spot light, in radians
		float m_outerAngle{ glm::radians(60.0f) };
	};
	
}