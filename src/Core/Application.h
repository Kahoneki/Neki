#pragma once

#include "Scene.h"


namespace NK
{

	class Application
	{
		friend class Engine;
		
		
	public:
		Application() = default;
		virtual ~Application() = default;

		inline virtual void Update()
		{
			m_scenes[m_activeScene]->Update();
		}


	protected:
		std::vector<UniquePtr<Scene>> m_scenes;
		std::size_t m_activeScene;
	};
	
}