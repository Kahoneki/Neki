#pragma once


namespace NK
{
	
	class TimeManager final
	{
	public:
		static void Update();
		[[nodiscard]] static inline double GetDeltaTime() { return dt; }
		
		
	private:
		inline static double dt{ 0.0 };
		inline static double lastTime{ 0.0 };
	};

}