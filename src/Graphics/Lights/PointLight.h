#pragma once

#include "Light.h"


namespace NK
{

	class PointLight : public Light
	{
	public:
		explicit PointLight() = default;
		virtual ~PointLight() override = default;

		//Set the A term in attenuation formula 1 / (A + Bd + Cd^2)
		inline void SetConstantAttenuation(const float _attenuation) { m_constantAttenuation = _attenuation; m_dirty = true; }
		//Set the B term in attenuation formula 1 / (A + Bd + Cd^2)
		inline void SetLinearAttenuation(const float _attenuation) { m_linearAttenuation = _attenuation; m_dirty = true; }
		//Set the C term in attenuation formula 1 / (A + Bd + Cd^2)
		inline void SetQuadraticAttenuation(const float _attenuation) { m_quadraticAttenuation = _attenuation; m_dirty = true; }
		
		//Get the A term in attenuation formula 1 / (A + Bd + Cd^2)
		[[nodiscard]] inline float GetConstantAttenuation() const { return m_constantAttenuation; }
		//Get the B term in attenuation formula 1 / (A + Bd + Cd^2)
		[[nodiscard]] inline float GetLinearAttenuation() const { return m_linearAttenuation; }
		//Get the C term in attenuation formula 1 / (A + Bd + Cd^2)
		[[nodiscard]] inline float GetQuadraticAttenuation() const { return m_quadraticAttenuation; }

		SERIALISE_MEMBER_FUNC(cereal::base_class<Light>(this), m_constantAttenuation, m_linearAttenuation, m_quadraticAttenuation)


	private:
		float m_constantAttenuation{ 0.1f };
		float m_linearAttenuation{ 0.01f };
		float m_quadraticAttenuation{ 0.001f };
	};
	
}