#include "Timer.h"


namespace NK
{

	Timer::Timer(const double _time)
	{
		m_lastTime = glfwGetTime();
		m_timeLeft = _time;
	}



	void Timer::Update()
	{
		const double currentTime{ glfwGetTime() };
		const double dt{ currentTime - m_lastTime };
		m_timeLeft -= dt;
		m_lastTime = currentTime;
	}
	
}