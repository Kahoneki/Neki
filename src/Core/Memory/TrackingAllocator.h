#pragma once
#include <mutex>
#include <unordered_map>

#include "IAllocator.h"
#include <vulkan/vulkan.h>

#include "../Debug/ILogger.h"

namespace NK
{

	//Holds information about an allocation for tracking memory leaks
	struct AllocationInfo
	{
		std::size_t size{ 0 };
		const char* file{ nullptr };
		int line{ 0 };
	};

	class TrackingAllocator final : public IAllocator
	{
	public:
		explicit TrackingAllocator(ILogger& _logger, bool _verbose, bool _vulkanVerbose);
		virtual ~TrackingAllocator() override;
		virtual void* Allocate(std::size_t _size, const char* _file, int _line) override;
		virtual void* Reallocate(void* _original, std::size_t _size, const char* _file, int _line) override;
		virtual void Free(void* _ptr) override;

		[[nodiscard]] virtual inline const VkAllocationCallbacks* GetVulkanCallbacks() const override { return &m_vulkanCallbacks; }
		std::size_t GetTotalMemoryAllocated(); //Returns the total amount of memory allocated through this allocator (in bytes)


	private:
		//Static C-Style Vulkan Callbacks
		//todo: maybe add internal allocation notification callbacks ?
		static void* VKAPI_CALL Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
		static void* VKAPI_CALL Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
		static void VKAPI_CALL Free(void* _pUserData, void* _pMemory);
		static std::string VulkanAllocationScopeToString(VkSystemAllocationScope _scope);

		//Impl
		void* AllocateAligned(std::size_t _size, std::size_t _alignment);
		void* ReallocateAligned(void* _original, std::size_t _size, std::size_t _alignment);
		void FreeAligned(void* _ptr);

		//Dependency injections
		ILogger& m_logger;

		static inline constexpr std::size_t m_defaultAlignment{ 16 };

		//Track allocations for memory leak detection
		std::unordered_map<void*, AllocationInfo> m_allocationMap;
		std::mutex m_allocationMapMtx;

		bool m_verbose;
		bool m_vulkanVerbose; //Whether or not to output vulkan internals
	};
}