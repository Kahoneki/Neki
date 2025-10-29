#include "TrackingAllocator.h"

#include <Core/Utils/EnumUtils.h>
#include <Core/Utils/FormatUtils.h>

#include <cstring>
#include <stdexcept>


namespace NK
{

	TrackingAllocator::TrackingAllocator(ILogger& _logger, const TrackingAllocatorConfig& _desc)
	: m_logger(_logger),
	  m_engineVerbose(EnumUtils::Contains(_desc.verbosityFlags, TRACKING_ALLOCATOR_VERBOSITY_FLAGS::ENGINE)),
	  m_vulkanVerbose(EnumUtils::Contains(_desc.verbosityFlags, TRACKING_ALLOCATOR_VERBOSITY_FLAGS::VULKAN)),
	  m_vmaVerbose(EnumUtils::Contains(_desc.verbosityFlags, TRACKING_ALLOCATOR_VERBOSITY_FLAGS::VMA)),
	  m_d3d12maVerbose(EnumUtils::Contains(_desc.verbosityFlags, TRACKING_ALLOCATOR_VERBOSITY_FLAGS::D3D12MA))
	{
		#if NEKI_VULKAN_SUPPORTED
			m_vulkanCallbacks.pUserData = static_cast<void*>(this);
			m_vulkanCallbacks.pfnAllocation = &TrackingAllocator::Allocation;
			m_vulkanCallbacks.pfnReallocation = &TrackingAllocator::Reallocation;
			m_vulkanCallbacks.pfnFree = &TrackingAllocator::Free;

			m_vmaCallbacks.pfnAllocate = &TrackingAllocator::VMADeviceMemoryAllocation;
			m_vmaCallbacks.pfnFree = &TrackingAllocator::VMADeviceMemoryFree;
			m_vmaCallbacks.pUserData = static_cast<void*>(this);

		#elif NEKI_D3D12_SUPPORTED
			m_d3d12maCallbacks.pPrivateData = static_cast<void*>(this);
			m_d3d12maCallbacks.pAllocate = &TrackingAllocator::Allocation;
			m_d3d12maCallbacks.pFree = &TrackingAllocator::Free;

		#endif
	}



	TrackingAllocator::~TrackingAllocator()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TRACKING_ALLOCATOR, "Shutting Down TrackingAllocator\n");


		//Report host memory leaks - strictly speaking I don't think VMA has host allocation callbacks specific to it (it just uses vulkan's callbacks), but it's still in here for consistency
		if (!m_hostAllocationMap.empty())
		{
			m_logger.Indent();
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, std::to_string(m_hostAllocationMap.size()) + " host memory leak" + (m_hostAllocationMap.size() == 1 ? "" : "s") + " detected");
			
			//Display all host memory leaks
			//Figure out how many are to be displayed based on the verbosity settings and display the count to the user
			std::size_t displayCount{ 0 };
			for (const std::pair<void*, AllocationInfo>& hostAlloc : m_hostAllocationMap)
			{
				if		(hostAlloc.second.source == ALLOCATION_SOURCE::UNKNOWN) { ++displayCount; } //Shouldn't be logically possible
				else if	(hostAlloc.second.source == ALLOCATION_SOURCE::ENGINE)	{ if (m_engineVerbose)	{ ++displayCount; } }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::VULKAN)	{ if (m_vulkanVerbose)	{ ++displayCount; } }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::VMA)		{ if (m_vmaVerbose)		{ ++displayCount; } }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::D3D12MA)	{ if (m_d3d12maVerbose) { ++displayCount; } }
			}
			m_logger.RawLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, " - Displaying " + std::to_string(displayCount) + "/" + std::to_string(m_hostAllocationMap.size()) + " based on current verbosity settings\n", 0);

			for (const std::pair<void*, AllocationInfo>& hostAlloc : m_hostAllocationMap)
			{
				if		(hostAlloc.second.source == ALLOCATION_SOURCE::ENGINE)	{ if (!m_engineVerbose)		{ continue; } }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::VULKAN)	{ if (!m_vulkanVerbose)		{ continue; } }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::VMA)		{ if (!m_vmaVerbose)		{ continue; } }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::D3D12MA) { if (!m_d3d12maVerbose)	{ continue; } }
				
				std::string location;
				if		(hostAlloc.second.source == ALLOCATION_SOURCE::UNKNOWN)	{ location = "UNKNOWN (LOGICAL ERROR)"; } //Shouldn't be logically possible
				else if	(hostAlloc.second.source == ALLOCATION_SOURCE::ENGINE)	{ location = hostAlloc.second.file + std::string(" - line ") + std::to_string(hostAlloc.second.line); }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::VULKAN)	{ location = "VULKAN INTERNAL"; }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::VMA)		{ location = "VMA INTERNAL"; }
				else if (hostAlloc.second.source == ALLOCATION_SOURCE::D3D12MA) { location = "D3D12MA INTERNAL"; }
				std::string size{ FormatUtils::GetSizeString(hostAlloc.second.size) };
				m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, location + " - " + size + "\n");
			}

			m_logger.Unindent();
		}


		#ifdef TRACK_DEVICE_ALLOCATIONS
			//Report device memory leaks - strictly speaking I think VMA is the only one that does device allocation callbacks, but the others are still in here for consistency
			if (!m_deviceAllocationMap.empty())
			{
				m_logger.Indent();
				m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, std::to_string(m_hostAllocationMap.size()) + " device memory leak" + (m_hostAllocationMap.size() == 1 ? "" : "s") + " detected");
			
				//Display all device memory leaks
				//Figure out how many are to be displayed based on the verbosity settings and display the count to the user
				std::size_t displayCount{ 0 };
				for (const std::pair<GPU_POINTER, AllocationInfo>& deviceAlloc : m_deviceAllocationMap)
				{
					if		(deviceAlloc.second.source == ALLOCATION_SOURCE::UNKNOWN) { ++displayCount; } //Shouldn't be logically possible
					else if	(deviceAlloc.second.source == ALLOCATION_SOURCE::ENGINE)	{ if (m_engineVerbose)	{ ++displayCount; } }
					else if (deviceAlloc.second.source == ALLOCATION_SOURCE::VULKAN)	{ if (m_vulkanVerbose)	{ ++displayCount; } }
					else if (deviceAlloc.second.source == ALLOCATION_SOURCE::VMA)		{ if (m_vmaVerbose)		{ ++displayCount; } }
				}
				m_logger.RawLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, " - Displaying " + std::to_string(displayCount) + "/" + std::to_string(m_hostAllocationMap.size()) + " based on current verbosity settings\n", 0);

				for (const std::pair<GPU_POINTER, AllocationInfo>& deviceAlloc : m_deviceAllocationMap)
				{
					if		(deviceAlloc.second.source == ALLOCATION_SOURCE::ENGINE)	{ if (!m_engineVerbose)	{ continue; } }
					else if (deviceAlloc.second.source == ALLOCATION_SOURCE::VULKAN)	{ if (!m_vulkanVerbose)	{ continue; } }
					else if (deviceAlloc.second.source == ALLOCATION_SOURCE::VMA)		{ if (!m_vmaVerbose)	{ continue; } }
				
					std::string location;
					if		(deviceAlloc.second.source == ALLOCATION_SOURCE::UNKNOWN)	{ location = "UNKNOWN (LOGICAL ERROR)"; } //Shouldn't be logically possible
					else if	(deviceAlloc.second.source == ALLOCATION_SOURCE::ENGINE)	{ location = deviceAlloc.second.file + std::string(" - line ") + std::to_string(deviceAlloc.second.line); }
					else if (deviceAlloc.second.source == ALLOCATION_SOURCE::VULKAN)	{ location = "VULKAN INTERNAL"; }
					else if (deviceAlloc.second.source == ALLOCATION_SOURCE::VMA)		{ location = "VMA INTERNAL"; }
					std::string size{ FormatUtils::GetSizeString(deviceAlloc.second.size) };
					m_logger.IndentLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, location + " - " + size + "\n");
				}

				m_logger.Unindent();
			}
		#endif

		
		m_logger.Unindent();
	}



	void* TrackingAllocator::Allocate(const std::size_t _size, const char* _file, const int _line, const bool _static)
	{
		if (m_engineVerbose) { m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Allocation: " + std::string(_file) + " - line " + std::to_string(_line) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + ")\n"); }
		if (_static) { m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "TrackingAllocator::Allocate() - _static flag set to true which can be dangerous, was this intended?\n"); }
		void* ptr{ AllocateAligned(_size, m_defaultAlignment) };

		if (!_static)
		{
			std::lock_guard<std::mutex> lock(m_hostAllocationMapMtx);
			m_hostAllocationMap[ptr] = { ALLOCATION_SOURCE::ENGINE, _size, _file, _line };
		}
		
		return ptr;
	}



	void* TrackingAllocator::Reallocate(void* _original, const std::size_t _size, const char* _file, const int _line, const bool _static)
	{
		m_logger.Indent();
		
		if (m_engineVerbose) { m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Reallocation: " + std::string(_file) + " - line " + std::to_string(_line) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + "). Freeing previous " + FormatUtils::GetSizeString(m_hostAllocationMap[_original].size) + " from " + m_hostAllocationMap[_original].file + "- line " + std::to_string(m_hostAllocationMap[_original].line)); }
		if (_static) { m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "TrackingAllocator::Reallocate() - _static flag set to true which can be dangerous, was this intended?\n"); }
		void* ptr{ ReallocateAligned(_original, _size, m_defaultAlignment) };

		if (!_static)
		{
			std::lock_guard<std::mutex> lock(m_hostAllocationMapMtx);
			m_hostAllocationMap[ptr] = { ALLOCATION_SOURCE::ENGINE, _size, _file, _line };
			if (_original) { m_hostAllocationMap.erase(_original); }
		}

		m_logger.Unindent();
		
		return ptr;
	}



	void TrackingAllocator::Free(void* _ptr, const bool _static)
	{
		m_logger.Indent();
		
		if (!m_hostAllocationMap.contains(_ptr))
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "Free - _ptr does not point to memory allocated through this tracking allocator");
			throw std::runtime_error("");
		}
		if (_static) { m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "TrackingAllocator::Free() - _static flag set to true which can be dangerous, was this intended?\n"); }

		const std::string size{ FormatUtils::GetSizeString(m_hostAllocationMap[_ptr].size) };
		const std::string file{ m_hostAllocationMap[_ptr].file };
		const std::string line{ std::to_string(m_hostAllocationMap[_ptr].line) };
		if (m_engineVerbose) { m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Free: " + file + " - line " + line + " --- Freeing " + size + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + ")\n"); }

		FreeAligned(_ptr);

		if (!_static)
		{
			std::lock_guard<std::mutex> lock(m_hostAllocationMapMtx);
			m_hostAllocationMap.erase(_ptr);
		}

		m_logger.Unindent();
	}



	std::size_t TrackingAllocator::GetTotalMemoryAllocated()
	{
		std::size_t counter{ 0 };
		for (const std::pair<void*, AllocationInfo>& alloc : m_hostAllocationMap)
		{
			counter += alloc.second.size;
		}

		return counter;
	}



	#if NEKI_VULKAN_SUPPORTED
		void* VKAPI_CALL TrackingAllocator::Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pUserData) };
			if (allocator->m_vulkanVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Allocation: " + VulkanAllocationScopeToString(_allocationScope) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (aligned to " + FormatUtils::GetSizeString(_alignment) + ")\n"); }
			void* ptr{ allocator->AllocateAligned(_size, _alignment) };

			std::lock_guard<std::mutex> lock(allocator->m_hostAllocationMapMtx);
			allocator->m_hostAllocationMap[ptr] = { ALLOCATION_SOURCE::VULKAN, _size, nullptr, 0 };
		
			return ptr;
		}



		void* VKAPI_CALL TrackingAllocator::Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pUserData) };
			if (allocator->m_vulkanVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Reallocation: " + VulkanAllocationScopeToString(_allocationScope) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (aligned to " + FormatUtils::GetSizeString(_alignment) + ")\n"); }
			void* ptr{ allocator->ReallocateAligned(_pOriginal, _size, _alignment) };

			std::lock_guard<std::mutex> lock(allocator->m_hostAllocationMapMtx);
			allocator->m_hostAllocationMap[ptr] = { ALLOCATION_SOURCE::VULKAN, _size, nullptr, 0 };
			if (_pOriginal) { allocator->m_hostAllocationMap.erase(_pOriginal); }

			return ptr;
		}



		void VKAPI_CALL TrackingAllocator::Free(void* _pUserData, void* _pMemory)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pUserData) };
			if (allocator->m_vulkanVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Free --- Freeing " + FormatUtils::GetSizeString(allocator->m_hostAllocationMap[_pMemory].size) + "\n"); }
			allocator->FreeAligned(_pMemory);

			std::lock_guard<std::mutex> lock(allocator->m_hostAllocationMapMtx);
			allocator->m_hostAllocationMap.erase(_pMemory);
		}



		std::string TrackingAllocator::VulkanAllocationScopeToString(VkSystemAllocationScope _scope)
		{
			switch (_scope)
			{
			case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND: return "COMMAND";
			case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT: return "SCOPE";
			case VK_SYSTEM_ALLOCATION_SCOPE_CACHE: return "CACHE";
			case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE: return "DEVICE";
			case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE: return "INSTANCE";
			default: return "UNDEFINED";
			}
		}



		void TrackingAllocator::VMADeviceMemoryAllocation(VmaAllocator _allocator, std::uint32_t _memType, VkDeviceMemory _memory, VkDeviceSize _size, void* _pUserData)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pUserData) };
			if (allocator->m_vmaVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "VMADeviceMemoryAllocation --- Allocating " + FormatUtils::GetSizeString(_size) + " on memory type index " + std::to_string(_memType) + "\n"); }

			//Not implemented
		
			std::lock_guard<std::mutex> lock(allocator->m_deviceAllocationMapMtx);
			allocator->m_deviceAllocationMap[_memory] = { ALLOCATION_SOURCE::VMA, _size, nullptr, 0 };
		}



		void TrackingAllocator::VMADeviceMemoryFree(VmaAllocator _allocator, std::uint32_t _memType, VkDeviceMemory _memory, VkDeviceSize _size, void* _pUserData)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pUserData) };
			if (allocator->m_vmaVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "VMADeviceMemoryFree --- Freeing " + FormatUtils::GetSizeString(_size) + " on memory type index " + std::to_string(_memType) + "\n"); }

			//Not implemented
		
			std::lock_guard<std::mutex> lock(allocator->m_deviceAllocationMapMtx);
			allocator->m_deviceAllocationMap.erase(_memory);
		}



	#elif NEKI_D3D12_SUPPORTED
		void* TrackingAllocator::Allocation(std::size_t _Size, std::size_t _Alignment, void* _pPrivateData)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pPrivateData) };
			if (allocator->m_d3d12maVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "D3D12MA Host-Allocation --- Request for " + FormatUtils::GetSizeString(_Size) + " (aligned to " + FormatUtils::GetSizeString(_Alignment) + ")\n"); }
			void* ptr{ allocator->AllocateAligned(_Size, _Alignment) };

			std::lock_guard<std::mutex> lock(allocator->m_hostAllocationMapMtx);
			allocator->m_hostAllocationMap[ptr] = { ALLOCATION_SOURCE::D3D12MA, _Size, nullptr, 0 };

			return ptr;
		}


		void TrackingAllocator::Free(void* _pMemory, void* _pPrivateData)
		{
			TrackingAllocator* allocator{ static_cast<TrackingAllocator*>(_pPrivateData) };
			if (allocator->m_d3d12maVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "D3D12MA Host-Free --- Freeing " + FormatUtils::GetSizeString(allocator->m_hostAllocationMap[_pMemory].size) + "\n"); }
			
			//0-Byte Free calls permitted by D3D12MA, just skip the actual free (as instructed by the docs)
			if (_pMemory)
			{
				allocator->FreeAligned(_pMemory);
			}

			std::lock_guard<std::mutex> lock(allocator->m_hostAllocationMapMtx);
			allocator->m_hostAllocationMap.erase(_pMemory);
		}

	#endif



	void* TrackingAllocator::AllocateAligned(std::size_t _size, std::size_t _alignment)
	{
		m_logger.Indent();
		
		#if defined(_WIN32)
			void* ptr{ _aligned_malloc(_size, _alignment) };
			if (ptr == nullptr)
		#else
			void* ptr{ nullptr };
			if (posix_memalign(&ptr, _alignment, _size) != 0)
		#endif
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "AllocateAligned - Allocation failed.\n");
			if (!m_engineVerbose) { m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "Set verbose flags flag to see more detailed output"); }
			throw std::runtime_error("");
		}

		m_logger.Unindent();
		
		return ptr;
	}



	void* TrackingAllocator::ReallocateAligned(void* _original, std::size_t _size, std::size_t _alignment)
	{
		m_logger.Indent();
		
		if (_original == nullptr)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "  ReallocationAligned - _original was nullptr, falling back to AllocationAligned.\n");
			return AllocateAligned(_size, _alignment);
		}
		
		if (_size == 0)
		{
			m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "  ReallocationImpl - _size was 0, falling back to FreeAligned (returning nullptr)\n");
			FreeAligned(_original);
			return nullptr;
		}

		//Get old size
		std::lock_guard<std::mutex> lock(m_hostAllocationMapMtx);
		const std::size_t oldSize{ m_hostAllocationMap[_original].size };

		//Allocate a new block
		void* newPtr{ AllocateAligned(_size, _alignment) };
		if (newPtr == nullptr)
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "  ReallocateAligned - Allocation returned nullptr.\n");
			throw std::runtime_error("");
		}

		//Copy data and free old block
		const std::size_t bytesToCopy{ (oldSize < _size) ? oldSize : _size };
		memcpy(newPtr, _original, bytesToCopy);
		FreeAligned(_original);

		m_logger.Unindent();
		
		return newPtr;
	}



	void TrackingAllocator::FreeAligned(void* _ptr)
	{
		if (_ptr == nullptr)
		{
			return;
		}

		#if defined(_WIN32)
			_aligned_free(_ptr);
		#else
			free(_ptr);
		#endif
	}
	
}