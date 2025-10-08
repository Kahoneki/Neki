#pragma once
#include <cstdint>
#include <Core/Debug/ILogger.h>
#include <Core/Memory/IAllocator.h>

namespace NK
{
	class IDevice;
	enum class PIPELINE_BIND_POINT;

	struct RootSignatureDesc
	{
		PIPELINE_BIND_POINT bindPoint;
		std::uint32_t num32BitPushConstantValues;
	};
	
	
	class IRootSignature
	{
	public:
		virtual ~IRootSignature() = default;

		[[nodiscard]] inline PIPELINE_BIND_POINT GetBindPoint() const { return m_bindPoint; }
		[[nodiscard]] inline std::uint32_t GetNum32BitValues() const { return m_actualNum32BitPushConstantValues; }
		//Will differ from GetNum32BitValues() if _desc.num32BitPushConstantValues < 128 and backend API is Vulkan (Vulkan requires minimum push constant range size to be 128)
		[[nodiscard]] inline std::uint32_t GetProvidedNum32BitValues() const { return m_providedNum32BitPushConstantValues; }


	protected:
		explicit IRootSignature(ILogger& _logger, IAllocator& _allocator, IDevice& _device, const RootSignatureDesc& _desc)
		: m_logger(_logger), m_allocator(_allocator), m_device(_device),
		  m_bindPoint(_desc.bindPoint), m_providedNum32BitPushConstantValues(_desc.num32BitPushConstantValues) {}


		//Dependency injections
		ILogger& m_logger;
		IAllocator& m_allocator;
		IDevice& m_device;

		PIPELINE_BIND_POINT m_bindPoint;
		std::uint32_t m_providedNum32BitPushConstantValues;
		std::uint32_t m_actualNum32BitPushConstantValues;
	};
}