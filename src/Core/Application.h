#pragma once

#include "Scene.h"
#include "Layers/ILayer.h"


namespace NK
{

	class Application
	{
		friend class Engine;
		
		
	public:
		Application() = default;
		virtual ~Application() = default;

		inline virtual void PreUpdate() const final
		{
			for (const UniquePtr<ILayer>& l : m_preAppLayers)
			{
				l->Update(m_scenes[m_activeScene]->m_reg);
			}
		}

		//Call m_scenes[m_activeScene]->Update() somewhere in your inherited Application::Update() method
		inline virtual void Update() = 0;

		inline virtual void PostUpdate() const final
		{
			for (const UniquePtr<ILayer>& l : m_postAppLayers)
			{
				l->Update(m_scenes[m_activeScene]->m_reg);
			}
		}


	protected:
		std::vector<UniquePtr<ILayer>> m_preAppLayers;
		std::vector<UniquePtr<ILayer>> m_postAppLayers;

		std::vector<UniquePtr<Scene>> m_scenes;
		std::size_t m_activeScene;

		//Set to true when you want to shut down the application, the program will exit at the start of the next frame
		bool m_shutdown{ false };
	};
	
}