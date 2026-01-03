#include "Engine.h"
#include "RAIIContext.h"


#include <Components/CBoxCollider.h>
#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CLight.h>
#include <Components/CModelRenderer.h>
#include <Components/CNetworkSync.h>
#include <Components/CPhysicsBody.h>
#include <Components/CSelected.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Components/CWindow.h>
#include <Graphics/Camera/Camera.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Graphics/Lights/DirectionalLight.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>

#include <cereal/types/polymorphic.hpp>



//To be implemented by user
[[nodiscard]] extern NK::ContextConfig CreateContext();
[[nodiscard]] extern NK::EngineConfig CreateEngine();


CEREAL_REGISTER_TYPE(NK::DirectionalLight);
CEREAL_REGISTER_TYPE(NK::PointLight);
CEREAL_REGISTER_TYPE(NK::SpotLight);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NK::Light, NK::DirectionalLight)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NK::Light, NK::PointLight)
CEREAL_REGISTER_POLYMORPHIC_RELATION(NK::Light, NK::SpotLight)



int main()
{
	const NK::ContextConfig contextConfig{ CreateContext() };
	NK::RAIIContext context{ contextConfig };
	
	const NK::EngineConfig engineConfig{ CreateEngine() };
	NK::Engine engine{ engineConfig };
	
	NK::TypeRegistry::Register<NK::CBoxCollider>("C_BOX_COLLIDER");
	NK::TypeRegistry::Register<NK::CCamera>("C_CAMERA");
	NK::TypeRegistry::Register<NK::CInput>("C_INPUT");
	NK::TypeRegistry::Register<NK::CLight>("C_LIGHT");
	NK::TypeRegistry::Register<NK::CModelRenderer>("C_MODEL_RENDERER");
	NK::TypeRegistry::Register<NK::CNetworkSync>("C_NETWORK_SYNC");
	NK::TypeRegistry::Register<NK::CPhysicsBody>("C_PHYSICS_BODY");
	NK::TypeRegistry::Register<NK::CSelected>("C_SELECTED");
	NK::TypeRegistry::Register<NK::CSkybox>("C_SKYBOX");
	NK::TypeRegistry::Register<NK::CTransform>("C_TRANSFORM");
	NK::TypeRegistry::Register<NK::CWindow>("C_WINDOW");
	
	engine.Run();
}