#pragma once

#include <cstdint>
#include <GLFW/glfw3.h>


namespace NK
{

	class Timer final
	{
	public:
		//Start a timer for _time milliseconds
		explicit Timer(const double _time);

		//Query the timer's completion
		[[nodiscard]] inline bool IsComplete() const { return (m_timeLeft < 0); }

		//Query how much time the timer has left (in seconds)
		[[nodiscard]] inline bool TimeLeft() const { return m_timeLeft; }

		//Reset the timer to _time seconds
		inline void Reset(const double _time) { *this = Timer(_time); }

		//Update the state of the timer
		void Update();


	private:
		double m_timeLeft;
		double m_lastTime;
	};
	
}