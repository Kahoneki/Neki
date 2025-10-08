#include "TimeManager.h"

#include <GLFW/glfw3.h>
#include <iostream>

namespace NK
{
	
	void TimeManager::Update()
	{
		const double currentTime{ glfwGetTime() };
		dt = currentTime - lastTime;
		lastTime = currentTime;
	}
	
}