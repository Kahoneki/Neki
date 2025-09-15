#pragma once
#include "IDevice.h"

namespace NK
{

	class ISemaphore
	{
	public:
		virtual ~ISemaphore() = default;


	protected:
		explicit ISemaphore(ILogger& _logger) : m_logger(_logger) {}


		//Dependency injections
		ILogger& m_logger;
	};

}