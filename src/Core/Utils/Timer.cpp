#include "Timer.h"


namespace NK
{

	Timer::Timer(const std::uint64_t _time)
	{
		m_lastTime = glfwGetTime();
		m_timeLeft = _time;
	}



	void Timer::Update()
	{
		const double currentTime{ glfwGetTime() };
		const double dt{ m_lastTime - currentTime };
		m_timeLeft -= dt;
		m_lastTime = currentTime;
	}
	
}