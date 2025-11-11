#pragma once

#include <glm/glm.hpp>


namespace NK
{

	struct Prefabs
	{
		explicit Prefabs() = default;
		explicit Prefabs(const glm::vec3& _normal, const float _distanceAlongNormal) : normal(_normal), t(_distanceAlongNormal) {}
		explicit Prefabs(const glm::vec4& _normalAndDistanceAlongNormal) : normal(_normalAndDistanceAlongNormal.x, _normalAndDistanceAlongNormal.y, _normalAndDistanceAlongNormal.z), t(_normalAndDistanceAlongNormal.w) {}

		glm::vec3 normal;
		float t;

		inline void Normalise()
		{
			const float mag{ glm::length(normal) };
			normal /= mag;
			t /= mag;
		}
	};

	
	struct ViewFrustum final
	{
	public:
		inline void Update(const glm::mat4& _m)
		{
			//Adapted from Gribb-Hartmann's "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix" (Subsection 2. Plane Extraction in Direct3D) and (Appendix B.2 Plane Extraction for Direct3D)
			//https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

			glm::mat4 m{ glm::transpose(_m) }; //Convert to row-major
			
			planes[0] = Prefabs(m[3] + m[0]);
			planes[1] = Prefabs(m[3] - m[0]);
			planes[2] = Prefabs(m[3] + m[1]);
			planes[3] = Prefabs(m[3] - m[1]);
			planes[4] = Prefabs(m[2]);
			planes[5] = Prefabs(m[3] - m[2]);
		}


		inline void Normalise()
		{
			for (std::size_t i{ 0 }; i < 6; ++i)
			{
				planes[i].Normalise();
			}
		}

		
		[[nodiscard]] inline bool BoxVisible(const glm::vec3& _min, const glm::vec3& _max) const
		{
			//Go through all planes and, if box is outside any of them, box isn't visible
			for (std::size_t i{ 0 }; i < 6; ++i)
			{
				//Adapted from Gribb-Hartmann's "Fast Extraction of Viewing Frustum Planes from the World-View-Projection Matrix" Appendix A.3
				//https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

				const float minPointD{ glm::dot(planes[i].normal, _min) + planes[i].t };
				const float maxPointD{ glm::dot(planes[i].normal, _max) + planes[i].t };
				if (minPointD < 0 && maxPointD < 0)
				{
					//Box is fully outside of this plane
					return false;
				}
			}
			
			//Box is inside all planes
			return true;
		}
		

	private:
		Prefabs planes[6]; //left, right, bottom, top, near, far
	};
}