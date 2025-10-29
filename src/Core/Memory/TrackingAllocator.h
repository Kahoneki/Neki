#pragma once

#include "IAllocator.h"

#include <Core/Debug/ILogger.h>

#include <mutex>
#include <unordered_map>


namespace NK
{

	class TrackingAllocator final : public IAllocator
	{
	public:
		explicit TrackingAllocator(ILogger& _logger, const TrackingAllocatorConfig& _desc);
		virtual ~TrackingAllocator() override;
		virtual void* Allocate(const std::size_t _size, const char* _file, const int _line, const bool _static) override;
		virtual void* Reallocate(void* _original, const std::size_t _size, const char* _file, const int _line, const bool _static) override;
		virtual void Free(void* _ptr, bool _static) override;

		std::size_t GetTotalMemoryAllocated(); //Returns the total amount of memory allocated through this allocator (in bytes)


	private:
		//Holds information about an allocation for tracking memory leaks
		struct AllocationInfo
		{
			ALLOCATION_SOURCE source{ ALLOCATION_SOURCE::UNKNOWN };
			std::size_t size{ 0 };
			const char* file{ nullptr };
			int line{ 0 };
		};
		
		
		#if NEKI_VULKAN_SUPPORTED
			//Static C-Style Vulkan Host-Callbacks
			//todo: maybe add internal allocation notification callbacks ?
			static void* VKAPI_CALL Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
			static void* VKAPI_CALL Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope);
			static void VKAPI_CALL Free(void* _pUserData, void* _pMemory);
			static std::string VulkanAllocationScopeToString(VkSystemAllocationScope _scope);

			//Static C-Style VMA Device-Callbacks
			static void VKAPI_PTR VMADeviceMemoryAllocation(VmaAllocator VMA_NOT_NULL _allocator, std::uint32_t _memType, VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE _memory, VkDeviceSize _size, void* VMA_NULLABLE _pUserData);
			static void VKAPI_PTR VMADeviceMemoryFree(VmaAllocator VMA_NOT_NULL _allocator, std::uint32_t _memType, VkDeviceMemory VMA_NOT_NULL_NON_DISPATCHABLE _memory, VkDeviceSize _size, void* VMA_NULLABLE _pUserData);
		
			#define TRACK_DEVICE_ALLOCATIONS

		#elif NEKI_D3D12_SUPPORTED
			//Static C-Style D3D12MA Host-Callbacks
			static void* Allocation(std::size_t _Size, std::size_t _Alignment, void* _pPrivateData);
			static void Free(void* _pMemory, void* _pPrivateData);

		#endif

		//Impl
		void* AllocateAligned(const std::size_t _size, const std::size_t _alignment);
		void* ReallocateAligned(void* _original, const std::size_t _size, const std::size_t _alignment);
		void FreeAligned(void* _ptr);

		
		//Dependency injections
		ILogger& m_logger;

		static inline constexpr std::size_t m_defaultAlignment{ 16 };

		//Track allocations for memory leak detection
		std::unordered_map<void*, AllocationInfo> m_hostAllocationMap;
		std::mutex m_hostAllocationMapMtx;

		#ifdef TRACK_DEVICE_ALLOCATIONS
			typedef VkDeviceMemory GPU_POINTER
			std::unordered_map<GPU_POINTER, AllocationInfo> m_deviceAllocationMap;
			std::mutex m_deviceAllocationMapMtx;
		#endif

		bool m_engineVerbose; //Whether or not to output engine internals
		bool m_vulkanVerbose; //Whether or not to output vulkan internals
		bool m_vmaVerbose; //Whether or not to output vma internals
		bool m_d3d12maVerbose; //Whether or not to output D3D12MA internals
	};
	
}