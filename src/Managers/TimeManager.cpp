#include "TimeManager.h"

#include <GLFW/glfw3.h>


namespace NK
{
	
	void TimeManager::Update()
	{
		m_totalTime = glfwGetTime();
		m_dt = m_totalTime - m_lastTime;
		m_lastTime = m_totalTime;
	}
	
}