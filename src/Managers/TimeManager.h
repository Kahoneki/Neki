#pragma once


namespace NK
{
	
	class TimeManager final
	{
	public:
		static void Update();
		[[nodiscard]] static inline double GetDeltaTime() { return m_dt; }
		[[nodiscard]] static inline double GetTotalTime() { return m_totalTime; }
		
		
	private:
		inline static double m_dt{ 0.0 };
		inline static double m_lastTime{ 0.0 };
		inline static double m_totalTime{ 0.0 };
	};

}