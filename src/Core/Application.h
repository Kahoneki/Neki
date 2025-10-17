#pragma once


namespace NK
{

	class Application
	{
	public:
		virtual void Run() = 0;
		virtual ~Application() = default;
	};
	
}