#pragma once

#include <cstdint>
#include <GLFW/glfw3.h>


namespace NK
{

	class Timer final
	{
	public:
		//Start a timer for _time milliseconds
		explicit Timer(const std::uint64_t _time);

		//Query the timer's completion
		[[nodiscard]] inline bool IsComplete() const { return (m_timeLeft < 0); }

		//Query how much time the timer has left (in milliseconds)
		[[nodiscard]] inline bool TimeLeft() const { return m_timeLeft; }

		//Reset the timer to _time milliseconds
		inline void Reset(const std::uint64_t _time) { *this = Timer(_time); }

		//Update the state of the timer
		void Update();


	private:
		double m_timeLeft;
		double m_lastTime;
	};
	
}