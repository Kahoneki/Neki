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
		virtual void Run() = 0;


	protected:
		std::vector<UniquePtr<Scene>> m_scenes;
		std::size_t m_activeScene;
	};
	
}