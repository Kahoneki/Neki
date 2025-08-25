#include "TrackingAllocator.h"

#include <cstring>
#include <stdexcept>
#include <oneapi/tbb/info.h>

#include "Core/Utils/FormatUtils.h"

namespace NK
{

	TrackingAllocator::TrackingAllocator(ILogger& _logger, bool _verbose, bool _vulkanVerbose)
	: m_logger(_logger), m_verbose(_verbose), m_vulkanVerbose(_vulkanVerbose)
	{
		m_vulkanCallbacks.pUserData = static_cast<void*>(this);
		m_vulkanCallbacks.pfnAllocation = &Allocation;
		m_vulkanCallbacks.pfnReallocation = &Reallocation;
		m_vulkanCallbacks.pfnFree = &Free;
	}



	TrackingAllocator::~TrackingAllocator()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TRACKING_ALLOCATOR, "Shutting Down TrackingAllocator\n");
		//Report memory leaks
		if (!m_allocationMap.empty())
		{
			m_logger.Indent();
			
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, std::to_string(m_allocationMap.size()) + " memory leak" + (m_allocationMap.size() == 1 ? "" : "s") + " detected");
			if (!m_verbose)
			{
				m_logger.RawLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\n");
				return;
			}

			//Display all memory leaks
			//Figure out how many are to be displayed based on the state of m_vulkanVerbose and display the count to the user
			std::size_t displayCount{ m_vulkanVerbose ? m_allocationMap.size() : 0 };
			if (!m_vulkanVerbose)
			{
				//Not displaying vulkan internal memory leaks, need to count the number of non-vulkan-internal memory leaks
				for (const std::pair<void*, AllocationInfo> alloc : m_allocationMap)
				{
					if (alloc.second.file)
					{
						++displayCount;
					}
				}
			}
			m_logger.RawLog(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, " - Displaying " + std::to_string(displayCount) + "/" + std::to_string(m_allocationMap.size()) + " based on current verbosity settings\n", 0);

			m_logger.Indent();
			for (const std::pair<void*, AllocationInfo> alloc : m_allocationMap)
			{
				if (!alloc.second.file)
				{
					//Vulkan internal memory leak
					if (!m_vulkanVerbose) { continue; }
				}
				std::string size{ FormatUtils::GetSizeString(alloc.second.size) };
				std::string file{ alloc.second.file ? alloc.second.file : "Vulkan Internal" };
				std::string line{ alloc.second.file ? std::to_string(alloc.second.line) : "N/A" };
				m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "    " + size + " (" + file + " - line " + line + ")\n");
			}
			m_logger.Unindent();

			m_logger.Unindent();
		}

		m_logger.Unindent();
	}



	void* TrackingAllocator::Allocate(std::size_t _size, const char* _file, int _line)
	{
		if (m_verbose) { m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Allocation: " + std::string(_file) + " - line " + std::to_string(_line) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + ")\n"); }
		void* ptr{ AllocateAligned(_size, m_defaultAlignment) };

		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		m_allocationMap[ptr] = { _size, _file, _line };
		
		return ptr;
	}



	void* TrackingAllocator::Reallocate(void* _original, std::size_t _size, const char* _file, int _line)
	{
		m_logger.Indent();
		
		if (m_verbose) { m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Reallocation: " + std::string(_file) + " - line " + std::to_string(_line) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + "). Freeing previous " + FormatUtils::GetSizeString(m_allocationMap[_original].size) + " from " + m_allocationMap[_original].file + "- line " + std::to_string(m_allocationMap[_original].line)); }
		void* ptr{ ReallocateAligned(_original, _size, m_defaultAlignment) };

		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		m_allocationMap[ptr] = { _size, _file, _line };
		if (_original) { m_allocationMap.erase(_original); }

		m_logger.Unindent();
		
		return ptr;
	}



	void TrackingAllocator::Free(void* _ptr)
	{
		m_logger.Indent();
		
		if (!m_allocationMap.contains(_ptr))
		{
			m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "Free - _ptr does not point to memory allocated through this tracking allocator");
		}
		const std::string size{ FormatUtils::GetSizeString(m_allocationMap[_ptr].size) };
		const std::string file{ m_allocationMap[_ptr].file };
		const std::string line{ std::to_string(m_allocationMap[_ptr].line) };
		if (m_verbose) { m_logger.Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Free: " + file + " - line " + line + " --- Freeing " + size + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + ")\n"); }

		FreeAligned(_ptr);
		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		m_allocationMap.erase(_ptr);

		m_logger.Unindent();
	}



	std::size_t TrackingAllocator::GetTotalMemoryAllocated()
	{
		std::size_t counter{ 0 };
		for (const std::pair<void*, AllocationInfo> alloc : m_allocationMap)
		{
			counter += alloc.second.size;
		}

		return counter;
	}



	void* VKAPI_CALL TrackingAllocator::Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		TrackingAllocator* allocator{ reinterpret_cast<TrackingAllocator*>(_pUserData) };
		if (allocator->m_vulkanVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Allocation: " + VulkanAllocationScopeToString(_allocationScope) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (aligned to " + FormatUtils::GetSizeString(_alignment) + ")\n"); }
		void* ptr{ allocator->AllocateAligned(_size, _alignment) };

		std::lock_guard<std::mutex> lock(allocator->m_allocationMapMtx);
		allocator->m_allocationMap[ptr] = { _size, nullptr, 0 };
		
		return ptr;
	}



	void* VKAPI_CALL TrackingAllocator::Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		TrackingAllocator* allocator{ reinterpret_cast<TrackingAllocator*>(_pUserData) };
		if (allocator->m_vulkanVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Reallocation: " + VulkanAllocationScopeToString(_allocationScope) + " --- Request for " + FormatUtils::GetSizeString(_size) + " (aligned to " + FormatUtils::GetSizeString(_alignment) + ")\n"); }
		void* ptr{ allocator->ReallocateAligned(_pOriginal, _size, _alignment) };

		std::lock_guard<std::mutex> lock(allocator->m_allocationMapMtx);
		allocator->m_allocationMap[ptr] = { _size, nullptr, 0 };
		if (_pOriginal) { allocator->m_allocationMap.erase(_pOriginal); }

		return ptr;
	}



	void VKAPI_CALL TrackingAllocator::Free(void* _pUserData, void* _pMemory)
	{
		TrackingAllocator* allocator{ reinterpret_cast<TrackingAllocator*>(_pUserData) };
		if (allocator->m_vulkanVerbose) { allocator->m_logger.IndentLog(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Free " + std::string("--- Freeing ") + FormatUtils::GetSizeString(allocator->m_allocationMap[_pMemory].size) + "\n"); }
		allocator->FreeAligned(_pMemory);

		std::lock_guard<std::mutex> lock(allocator->m_allocationMapMtx);
		allocator->m_allocationMap.erase(_pMemory);
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
			if (!m_verbose) { m_logger.Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "Set TrackingAllocator::m_verbose flag to see more detailed output"); }
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
		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		const std::size_t oldSize{ m_allocationMap[_original].size };

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