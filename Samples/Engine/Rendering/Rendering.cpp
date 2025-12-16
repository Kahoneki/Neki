#include <filesystem>
#include <Components/CCamera.h>
#include <Components/CInput.h>
#include <Components/CLight.h>
#include <Components/CModelRenderer.h>
#include <Components/CSkybox.h>
#include <Components/CTransform.h>
#include <Core/EngineConfig.h>
#include <Core/RAIIContext.h>
#include <Core/Layers/InputLayer.h>
#include <Core/Layers/ModelVisibilityLayer.h>
#include <Core/Layers/PlayerCameraLayer.h>
#include <Core/Layers/RenderLayer.h>
#include <Core/Layers/WindowLayer.h>
#include <Graphics/Camera/PlayerCamera.h>
#include <Graphics/Lights/DirectionalLight.h>
#include <Graphics/Lights/PointLight.h>
#include <Graphics/Lights/SpotLight.h>
#include <Managers/InputManager.h>
#include <Managers/TimeManager.h>

#include <imgui.h>


class GameScene final : public NK::Scene
{
public:
	explicit GameScene() : Scene(8), m_playerCamera(NK::UniquePtr<NK::PlayerCamera>(NK_NEW(NK::PlayerCamera, glm::vec3(0, 0, 3), -90.0f, 0, 0.01f, 1000.0f, 90.0f, WIN_ASPECT_RATIO, 30.0f, 0.05f)))
	{
		m_skyboxEntity = m_reg.Create();
		NK::CSkybox& skybox{ m_reg.AddComponent<NK::CSkybox>(m_skyboxEntity) };
		skybox.SetSkyboxFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/skybox.ktx");
		skybox.SetIrradianceFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/irradiance.ktx");
		skybox.SetPrefilterFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/prefilter.ktx");

		
		m_lightEntity1 = m_reg.Create();
		m_reg.AddComponent<NK::CTransform>(m_lightEntity1);
		NK::CLight& redLight{ m_reg.AddComponent<NK::CLight>(m_lightEntity1) };
		redLight.lightType = NK::LIGHT_TYPE::DIRECTIONAL;
		redLight.light = NK::UniquePtr<NK::Light>(NK_NEW(NK::DirectionalLight));
		redLight.light->SetColour({ 1,0,0 });
		redLight.light->SetIntensity(0.75f);

		
		m_lightEntity2 = m_reg.Create();
		m_reg.AddComponent<NK::CTransform>(m_lightEntity2);
		NK::CLight& spotLight{ m_reg.AddComponent<NK::CLight>(m_lightEntity2) };
		spotLight.lightType = NK::LIGHT_TYPE::SPOT;
		spotLight.light = NK::UniquePtr<NK::Light>(NK_NEW(NK::SpotLight));
		spotLight.light->SetColour({ 0,1,0 });
		spotLight.light->SetIntensity(1.0f);
		dynamic_cast<NK::SpotLight*>(spotLight.light.get())->SetConstantAttenuation(1.0f);
		dynamic_cast<NK::SpotLight*>(spotLight.light.get())->SetLinearAttenuation(0.1f);
		dynamic_cast<NK::SpotLight*>(spotLight.light.get())->SetQuadraticAttenuation(0.01f);
		dynamic_cast<NK::SpotLight*>(spotLight.light.get())->SetInnerAngle(glm::radians(30.0f));
		dynamic_cast<NK::SpotLight*>(spotLight.light.get())->SetOuterAngle(glm::radians(60.0f));
		
		m_lightEntity3 = m_reg.Create();
		m_reg.AddComponent<NK::CTransform>(m_lightEntity3);
		NK::CLight& pointLight{ m_reg.AddComponent<NK::CLight>(m_lightEntity3) };
		pointLight.lightType = NK::LIGHT_TYPE::POINT;
		pointLight.light = NK::UniquePtr<NK::Light>(NK_NEW(NK::PointLight));
		pointLight.light->SetColour({ 0,0,1 });
		pointLight.light->SetIntensity(1.0f);
		dynamic_cast<NK::PointLight*>(pointLight.light.get())->SetConstantAttenuation(1.0f);
		dynamic_cast<NK::PointLight*>(pointLight.light.get())->SetLinearAttenuation(0.1f);
		dynamic_cast<NK::PointLight*>(pointLight.light.get())->SetQuadraticAttenuation(0.01f);
		
		
		//preprocessing step - ONLY NEEDS TO BE CALLED ONCE - serialises the model into a persistent .nkmodel file that can then be loaded
//		std::filesystem::path serialisedModelOutputPath{ std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel") };
//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/DamagedHelmet/DamagedHelmet.gltf", serialisedModelOutputPath.string(), true, true);
//		serialisedModelOutputPath = std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/Prefabs/Cube.nkmodel");
//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/Prefabs/Cube.gltf", serialisedModelOutputPath.string(), true, true);
//		serialisedModelOutputPath = std::filesystem::path(NEKI_SOURCE_DIR) / std::string("Samples/Resource-Files/nkmodels/Sponza/Sponza.nkmodel");
//		NK::ModelLoader::SerialiseNKModel("Samples/Resource-Files/Sponza/Sponza.gltf", serialisedModelOutputPath, true, true);
		
		m_helmetEntity = m_reg.Create();
		NK::CModelRenderer& helmetModelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_helmetEntity) };
		helmetModelRenderer.modelPath = "Samples/Resource-Files/nkmodels/DamagedHelmet/DamagedHelmet.nkmodel";
		helmetModelRenderer.waveAmplitude = 0.2f;
		NK::CTransform& helmetTransform{ m_reg.AddComponent<NK::CTransform>(m_helmetEntity) };
		helmetTransform.SetPosition(glm::vec3(0, 0, -5));
		helmetTransform.SetScale({ 5.0f, 5.0f, 5.0f });
		helmetTransform.SetRotation({ glm::radians(70.0f), glm::radians(-30.0f), glm::radians(180.0f) });

//		m_groundEntity = m_reg.Create();
//		NK::CModelRenderer& groundModelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_groundEntity) };
//		groundModelRenderer.modelPath = "Samples/Resource-Files/nkmodels/Prefabs/Cube.nkmodel";
//		NK::CTransform& groundTransform{ m_reg.AddComponent<NK::CTransform>(m_groundEntity) };
////		groundTransform.SetScale({ 0.01f, 0.01f, 0.01f });
//		groundTransform.SetPosition(glm::vec3(0, -3, -3));
//		groundTransform.SetScale({ 5.0f, 0.2f, 5.0f });
//
//		m_wallEntity = m_reg.Create();
//		NK::CModelRenderer& groundModelRenderer2{ m_reg.AddComponent<NK::CModelRenderer>(m_wallEntity) };
//		groundModelRenderer2.modelPath = "Samples/Resource-Files/nkmodels/Prefabs/Cube.nkmodel";
//		NK::CTransform& groundTransform2{ m_reg.AddComponent<NK::CTransform>(m_wallEntity) };
//		//		groundTransform.SetScale({ 0.01f, 0.01f, 0.01f });
//		groundTransform2.SetPosition(glm::vec3(0, 1.8, -8));
//		groundTransform2.SetScale({ 5.0f, 0.2f, 5.0f });
//		groundTransform2.SetRotation({ glm::radians(90.0f), 0.0f, 0.0f });


		m_sponzaEntity = m_reg.Create();
		NK::CModelRenderer& sponzaModelRenderer{ m_reg.AddComponent<NK::CModelRenderer>(m_sponzaEntity) };
		sponzaModelRenderer.modelPath = "Samples/Resource-Files/nkmodels/Sponza/Sponza.nkmodel";
		sponzaModelRenderer.waveAmplitude = 0.0f;
		NK::CTransform& sponzaTransform{ m_reg.AddComponent<NK::CTransform>(m_sponzaEntity) };
		sponzaTransform.SetScale({ 0.1f, 0.1f, 0.1f });
		sponzaTransform.SetPosition(glm::vec3(0, -3, -3));
		

		m_cameraEntity = m_reg.Create();
		NK::CCamera& camera{ m_reg.AddComponent<NK::CCamera>(m_cameraEntity) };
		camera.camera = m_playerCamera.get();


//		//Inputs
		NK::ButtonBinding aBinding{ NK::KEYBOARD::A };
		NK::ButtonBinding dBinding{ NK::KEYBOARD::D };
		NK::ButtonBinding sBinding{ NK::KEYBOARD::S };
		NK::ButtonBinding wBinding{ NK::KEYBOARD::W };
		NK::Axis1DBinding camMoveHorizontalBinding{ { aBinding, dBinding }, { -1, 1 } };
		NK::Axis1DBinding camMoveVerticalBinding{ { sBinding, wBinding }, { -1, 1 } };
		NK::Axis2DBinding camMoveBinding{ NK::Axis2DBinding({ camMoveHorizontalBinding, camMoveVerticalBinding }) };
		NK::Axis2DBinding mouseDiffBinding{ NK::Axis2DBinding(NK::MOUSE::POSITION_DIFFERENCE) };
		NK::InputManager::BindActionToInput(NK::PLAYER_CAMERA_ACTIONS::MOVE, camMoveBinding);
		NK::InputManager::BindActionToInput(NK::PLAYER_CAMERA_ACTIONS::YAW_PITCH, mouseDiffBinding);

		NK::CInput& input{ m_reg.AddComponent<NK::CInput>(m_cameraEntity) };
		input.AddActionToMap(NK::PLAYER_CAMERA_ACTIONS::MOVE);
		input.AddActionToMap(NK::PLAYER_CAMERA_ACTIONS::YAW_PITCH);
	}

	
	virtual void Update() override
	{
		NK::CTransform& helmetTransform{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity) };
		constexpr float speed{ 50.0f };
		const float rotationAmount{ glm::radians(speed * static_cast<float>(NK::TimeManager::GetDeltaTime())) };
		helmetTransform.SetRotation(helmetTransform.GetRotation() + glm::vec3(0,rotationAmount,0));


		//ImGui for lights
		if (ImGui::Begin("Light Controls"))
		{
			auto DrawLightNode{ [&](const char* label, NK::Entity entity) 
			{
				if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::PushID(static_cast<int>(entity));

					//Position
					NK::CTransform& transform = m_reg.GetComponent<NK::CTransform>(entity);
					glm::vec3 pos{ transform.GetPosition() };
					if (ImGui::DragFloat3("Position", &pos.x, 0.05f)) 
					{
						transform.SetPosition(pos);
					}
					
					//Rotation
					glm::vec3 rot{ glm::degrees(transform.GetRotation()) };
					if (ImGui::DragFloat3("Rotation", &rot.x, 0.05f)) 
					{
						transform.SetRotation(glm::radians(rot));
					}

					
					//Light properties
					NK::CLight& lightComp = m_reg.GetComponent<NK::CLight>(entity);
					if (lightComp.light)
					{
						//Colour
						glm::vec3 color = lightComp.light->GetColour();
						if (ImGui::ColorEdit3("Colour", &color.x))
						{
							lightComp.light->SetColour(color);
						}

						//Intensity
						float intensity = lightComp.light->GetIntensity();
						if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f))
						{
							lightComp.light->SetIntensity(intensity);
						}
						
						if (NK::PointLight* pointLight{ dynamic_cast<NK::PointLight*>(lightComp.light.get()) })
						{
							//Attenuation
							float constant{ pointLight->GetConstantAttenuation() };
							if (ImGui::DragFloat("Constant", &constant, 0.01f, 0.0f, 100.0f)) { pointLight->SetConstantAttenuation(constant); }
							float linear{ pointLight->GetLinearAttenuation() };
							if (ImGui::DragFloat("Linear", &linear, 0.01f, 0.0f, 100.0f)) { pointLight->SetLinearAttenuation(linear); }
							float quadratic{ pointLight->GetQuadraticAttenuation() };
							if (ImGui::DragFloat("Quadratic", &quadratic, 0.01f, 0.0f, 100.0f)) { pointLight->SetQuadraticAttenuation(quadratic); }
							
							if (NK::SpotLight* spotLight{ dynamic_cast<NK::SpotLight*>(pointLight) })
							{
								//Angles
								float innerAngle{ glm::degrees(spotLight->GetInnerAngle()) };
								if (ImGui::DragFloat("Inner (deg)", &innerAngle, 0.01f, 0.0f, 360.0f)) { spotLight->SetInnerAngle(glm::radians(innerAngle)); }
								float outerAngle{ glm::degrees(spotLight->GetOuterAngle()) };
								if (ImGui::DragFloat("Outer (deg)", &outerAngle, 0.01f, 0.0f, 360.0f)) { spotLight->SetOuterAngle(glm::radians(outerAngle)); }
							}
						}
					}
					ImGui::PopID();
				}
			}};

			DrawLightNode("Directional Light", m_lightEntity1);
			DrawLightNode("Spot Light", m_lightEntity2);
			DrawLightNode("Point Light", m_lightEntity3);

			if (ImGui::CollapsingHeader("Helmet", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID(static_cast<int>(m_helmetEntity));
				NK::CTransform& transform{ m_reg.GetComponent<NK::CTransform>(m_helmetEntity) };
				glm::vec3 pos{ transform.GetPosition() };
				if (ImGui::DragFloat3("Position", &pos.x, 0.05f)) 
				{
					transform.SetPosition(pos);
				}
				NK::CModelRenderer& modelRenderer{ m_reg.GetComponent<NK::CModelRenderer>(m_helmetEntity) };
				ImGui::DragFloat("Wave Amplitude", &modelRenderer.waveAmplitude, 0.001f);
				ImGui::PopID();
			}

			if (ImGui::CollapsingHeader("Sponza", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID(static_cast<int>(m_sponzaEntity));
				NK::CTransform& transform = m_reg.GetComponent<NK::CTransform>(m_sponzaEntity);
				glm::vec3 pos = transform.GetPosition();
				if (ImGui::DragFloat3("Position", &pos.x, 0.05f)) 
				{
					transform.SetPosition(pos);
				}
				ImGui::PopID();
			}
			
			if (ImGui::Button("Swap Skybox"))
			{
				NK::CSkybox& skybox{ m_reg.GetComponent<NK::CSkybox>(m_skyboxEntity) };
				if (skybox.GetSkyboxFilepath() == "Samples/Resource-Files/Skyboxes/The Sky is On Fire/skybox.ktx")
				{
					skybox.SetSkyboxFilepath("Samples/Resource-Files/Skyboxes/Shanghai Bund/skybox.ktx");
					skybox.SetIrradianceFilepath("Samples/Resource-Files/Skyboxes/Shanghai Bund/irradiance.ktx");
					skybox.SetPrefilterFilepath("Samples/Resource-Files/Skyboxes/Shanghai Bund/prefilter.ktx");
				}
				else
				{
					skybox.SetSkyboxFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/skybox.ktx");
					skybox.SetIrradianceFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/irradiance.ktx");
					skybox.SetPrefilterFilepath("Samples/Resource-Files/Skyboxes/The Sky is On Fire/prefilter.ktx");
				}
			}
		}
		ImGui::End();

		
//		const double time{ NK::TimeManager::GetTotalTime() };
//		constexpr float radius{ 10.0f };
//		constexpr float speed{ 3.0f };
//		constexpr glm::vec3 centre{ 0.0f, 0.0f, -3.0f };
//		
//		const double xRed{ centre.x + radius * cos(time * speed) };
//		const double zRed{ centre.z + radius * sin(time * speed) };
//		const double xGreen{ centre.x - radius * cos(time * speed) };
//		const double zGreen{ centre.z - radius * sin(time * speed) };
//
//		NK::CTransform& redLightTransform{ m_reg.GetComponent<NK::CTransform>(m_redLightEntity) };
//		redLightTransform.SetPosition({ xRed, centre.y, zRed });
//
//		NK::CTransform& greenLightTransform{ m_reg.GetComponent<NK::CTransform>(m_greenLightEntity) };
//		greenLightTransform.SetPosition({ xGreen, centre.y, zGreen });
	}


private:
	NK::Entity m_skyboxEntity;
	NK::Entity m_helmetEntity;
	NK::Entity m_sponzaEntity;
	NK::Entity m_groundEntity;
	NK::Entity m_wallEntity;
	NK::Entity m_lightEntity1;
	NK::Entity m_lightEntity2;
	NK::Entity m_lightEntity3;
	NK::Entity m_cameraEntity;
	NK::UniquePtr<NK::PlayerCamera> m_playerCamera;
};


class GameApp final : public NK::Application
{
public:
	explicit GameApp() : Application(1)
	{
		//Register types
		NK::TypeRegistry::Register<NK::PLAYER_CAMERA_ACTIONS>("PLAYER_CAMERA_ACTIONS");

		m_scenes.push_back(NK::UniquePtr<NK::Scene>(NK_NEW(GameScene)));
		m_activeScene = 0;


		//Window
		NK::WindowDesc windowDesc;
		windowDesc.name = "Rendering Sample";
		windowDesc.size = { 1920, 1080 };
		m_window = NK::UniquePtr<NK::Window>(NK_NEW(NK::Window, windowDesc));
		m_window->SetCursorVisibility(false);

		m_windowEntity = m_reg.Create();
		NK::CWindow& windowComponent{ m_reg.AddComponent<NK::CWindow>(m_windowEntity) };
		windowComponent.window = m_window.get();
		

		//Pre-app layers
		m_windowLayer = NK::UniquePtr<NK::WindowLayer>(NK_NEW(NK::WindowLayer, m_reg));
		NK::InputLayerDesc inputLayerDesc{ m_window.get() };
		m_inputLayer = NK::UniquePtr<NK::InputLayer>(NK_NEW(NK::InputLayer, m_scenes[m_activeScene]->m_reg, inputLayerDesc));
		NK::RenderLayerDesc renderLayerDesc{};
		renderLayerDesc.backend = NK::GRAPHICS_BACKEND::VULKAN;
		renderLayerDesc.enableMSAA = false;
		renderLayerDesc.msaaSampleCount = NK::SAMPLE_COUNT::BIT_8;
		renderLayerDesc.enableSSAA = true;
		renderLayerDesc.ssaaMultiplier = 4;
		renderLayerDesc.window = m_window.get();
		renderLayerDesc.framesInFlight = 3;
		m_renderLayer = NK::UniquePtr<NK::RenderLayer>(NK_NEW(NK::RenderLayer, m_scenes[m_activeScene]->m_reg,  renderLayerDesc));
		m_playerCameraLayer = NK::UniquePtr<NK::PlayerCameraLayer>(NK_NEW(NK::PlayerCameraLayer, m_scenes[m_activeScene]->m_reg));

		m_preAppLayers.push_back(m_windowLayer.get());
		m_preAppLayers.push_back(m_inputLayer.get());
		m_preAppLayers.push_back(m_renderLayer.get());
		m_preAppLayers.push_back(m_playerCameraLayer.get());
		
		
		//Post-app layers
		m_postAppLayers.push_back(m_renderLayer.get());
	}



	virtual void Update() override
	{
		m_scenes[m_activeScene]->Update();

		glfwSetWindowShouldClose(m_window->GetGLFWWindow(), NK::InputManager::GetKeyPressed(NK::KEYBOARD::ESCAPE));
		if (NK::InputManager::GetKeyPressed(NK::KEYBOARD::E) && !m_editorActiveKeyPressedLastFrame)
		{
			NK::Context::SetEditorActive(!NK::Context::GetEditorActive());
		}
		m_editorActiveKeyPressedLastFrame = NK::InputManager::GetKeyPressed(NK::KEYBOARD::E);
		
		m_window->SetCursorVisibility(NK::Context::GetEditorActive());
		
		m_shutdown = m_window->ShouldClose();
	}


private:
	NK::UniquePtr<NK::Window> m_window;
	NK::Entity m_windowEntity;

	bool m_editorActiveKeyPressedLastFrame{ false };
	
	//Pre-app layers
	NK::UniquePtr<NK::WindowLayer> m_windowLayer;
	NK::UniquePtr<NK::InputLayer> m_inputLayer;
	NK::UniquePtr<NK::PlayerCameraLayer> m_playerCameraLayer;

	//Post-app layers
	NK::UniquePtr<NK::ModelVisibilityLayer> m_modelVisibilityLayer;
	NK::UniquePtr<NK::RenderLayer> m_renderLayer;
};



[[nodiscard]] NK::ContextConfig CreateContext()
{
	NK::LoggerConfig loggerConfig{ NK::LOGGER_TYPE::CONSOLE, true };
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_GENERAL, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::VULKAN_VALIDATION, NK::LOGGER_CHANNEL::INFO | NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);
	loggerConfig.SetLayerChannelBitfield(NK::LOGGER_LAYER::TRACKING_ALLOCATOR, NK::LOGGER_CHANNEL::WARNING | NK::LOGGER_CHANNEL::ERROR);

	constexpr NK::TrackingAllocatorConfig trackingAllocatorConfig{ NK::TRACKING_ALLOCATOR_VERBOSITY_FLAGS::ALL };
	constexpr NK::AllocatorConfig allocatorConfig{ NK::ALLOCATOR_TYPE::TRACKING, trackingAllocatorConfig };

	return NK::ContextConfig(loggerConfig, allocatorConfig);
}



[[nodiscard]] NK::EngineConfig CreateEngine()
{
	return NK::EngineConfig(NK_NEW(GameApp));
}