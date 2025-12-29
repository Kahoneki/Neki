#pragma once

#include "Scene.h"
#include "Layers/ILayer.h"


namespace NK
{

	class Application
	{
		friend class Engine;
		
		
	public:
		explicit Application(const std::size_t _maxEntities) : m_reg(_maxEntities) {}
		virtual ~Application() = default;

		inline virtual void PreFixedUpdate() const final
		{
			for (ILayer* const l : m_preAppLayers)
			{
				l->FixedUpdate();
			}
		}
		
		inline virtual void PreUpdate() const final
		{
			for (ILayer* const l : m_preAppLayers)
			{
				l->Update();
			}
		}

		//Call m_scenes[m_activeScene]->Update() somewhere in your inherited Application::Update() method
		inline virtual void Update() {}
		//Call m_scenes[m_activeScene]->FixedUpdate() somewhere in your inherited Application::FixedUpdate() method
		inline virtual void FixedUpdate() {}

		inline virtual void PostFixedUpdate() const final
		{
			for (ILayer* const l : m_postAppLayers)
			{
				l->FixedUpdate();
			}
		}
		
		inline virtual void PostUpdate() const final
		{
			for (ILayer* const l : m_postAppLayers)
			{
				l->Update();
			}
		}
		


	protected:
		//Non-owning pointers, inherited application class should own these
		std::vector<ILayer*> m_preAppLayers;
		std::vector<ILayer*> m_postAppLayers;

		std::vector<UniquePtr<Scene>> m_scenes;
		std::size_t m_activeScene{ 0 };

		//Set to true when you want to shut down the application, the program will exit at the start of the next frame
		bool m_shutdown{ false };

		Registry m_reg;
	};
	
}