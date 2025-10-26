#include "ModelVisibilityLayer.h"

#include <Components/CModelRenderer.h>
#include <Components/CTransform.h>


namespace NK
{

	ModelVisibilityLayer::ModelVisibilityLayer(Registry& _reg) : ILayer(_reg), m_frustum()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::MODEL_VISIBILITY_LAYER, "Initialising Model Visibility Layer\n");

		m_logger.Unindent();
	}



	void ModelVisibilityLayer::Update()
	{
		//Get the camera's view frustum
		bool found{ false };
		for (auto&& [camera] : m_reg.get().View<CCamera>())
		{
			if (found)
			{
				//Multiple cameras, notify the user only the first one is being considered for rendering - todo: change this (probably let the user specify on CModelRenderer which camera they want to use)
				m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::MODEL_VISIBILITY_LAYER, "Multiple `CCamera`s found in registry. Note that currently, only one camera is supported - only the first camera will be used.\n");
				break;
			}
			found = true;
			m_frustum = camera.camera->GetFrustum();
		}
		if (!found)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::MODEL_VISIBILITY_LAYER, "No `CCamera`s found in registry.\n");
		}


		//Get all CModelRenderers and get their boundaries' min and max points to check against the camera's frustum for visibility
		//TODO: THIS DOESN'T WORK FOR ROTATED MODELS - I HAVE NO CLUE HOW TO SOLVE THIS :(
		//todo: ^it'd maybe be a bit to get set up but i reckon a gpu occlusion-query style prepass could work, there's something there
		//todo: ^maybe even use the stencil buffer to draw a mask of where the objects are? and the values in the stencil buffer could be the Entity ids (do you get a 32-bit stencil buffer?)
		//todo: ^doing it on the gpu would let us have much more accurate visibility testing (with like depth testing and whatnot)
		for (auto&& [modelRenderer, transform] : m_reg.get().View<CModelRenderer, CTransform>())
		{
			constexpr float epsilon{ 1e-3 };
			if (modelRenderer.worldBoundaryHalfExtents.x < epsilon && modelRenderer.worldBoundaryHalfExtents.y < epsilon && modelRenderer.worldBoundaryHalfExtents.z < epsilon)
			{
				//Model boundary hasn't been set yet
				CPUModel_SerialisedHeader header{ ModelLoader::GetNKModelHeader(modelRenderer.modelPath) };
				modelRenderer.worldBoundaryHalfExtents = transform.GetScale() * header.halfExtents;
				modelRenderer.worldBoundaryPosition = transform.GetPosition();
			}
			
			const glm::vec3 minPoint{ modelRenderer.worldBoundaryPosition - modelRenderer.worldBoundaryHalfExtents };
			const glm::vec3 maxPoint{ modelRenderer.worldBoundaryPosition + modelRenderer.worldBoundaryHalfExtents };
			modelRenderer.visible = m_frustum.BoxVisible(minPoint, maxPoint);
		}
	}
	
}