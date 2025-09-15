#pragma once
#include "IDevice.h"

namespace NK
{

	class IFence
	{
	public:
		virtual ~IFence() = default;

		virtual void Wait() = 0;
		virtual void Reset() = 0;


	protected:
		explicit IFence(ILogger& _logger) : m_logger(_logger) {}


		//Dependency injections
		ILogger& m_logger;
	};

}