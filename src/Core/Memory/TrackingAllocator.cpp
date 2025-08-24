#include "TrackingAllocator.h"

#include <cstring>
#include <stdexcept>
#include <oneapi/tbb/info.h>

#include "Core/Utils/FormatUtils.h"

namespace NK
{

	TrackingAllocator::TrackingAllocator(ILogger* _logger, bool _verbose)
	: m_logger(_logger), m_verbose(_verbose)
	{
		m_vulkanCallbacks.pUserData = static_cast<void*>(this);
		m_vulkanCallbacks.pfnAllocation = &Allocation;
		m_vulkanCallbacks.pfnReallocation = &Reallocation;
		m_vulkanCallbacks.pfnFree = &Free;
	}



	TrackingAllocator::~TrackingAllocator()
	{
		m_logger->Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::TRACKING_ALLOCATOR, "Shutting Down TrackingAllocator\n");
		//Report memory leaks
		if (!m_allocationMap.empty())
		{
			m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tMemory leaks detected (" + std::to_string(m_allocationMap.size()) + ")\n");
			for (const std::pair<void*, AllocationInfo> alloc : m_allocationMap)
			{
				std::string size{ FormatUtils::GetSizeString(alloc.second.size) };
				std::string file{ alloc.second.file ? alloc.second.file : "Vulkan Internal" };
				std::string line{ alloc.second.file ? std::to_string(alloc.second.line) : "N/A" };
				m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\t\t" + size + " (" + file + " - line " + line + ")");
			}
		}
	}



	void* TrackingAllocator::Allocate(std::size_t _size, const char* _file, int _line)
	{
		if (m_verbose) { m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Allocation: " + std::string(_file) + " - line " + std::to_string(_line) + "--- Request for " + FormatUtils::GetSizeString(_size) + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + ")\n"); }
		void* ptr{ AllocateAligned(_size, m_defaultAlignment) };

		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		m_allocationMap[ptr] = { _size, _file, _line };

		return ptr;
	}



	void* TrackingAllocator::Reallocate(void* _original, std::size_t _size, const char* _file, int _line)
	{
		if (m_verbose) { m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Reallocation: " + std::string(_file) + " - line " + std::to_string(_line) + "--- Request for " + FormatUtils::GetSizeString(_size) + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + "). Freeing previous " + FormatUtils::GetSizeString(m_allocationMap[_original].size) + " from " + m_allocationMap[_original].file + "- line " + std::to_string(m_allocationMap[_original].line)); }
		void* ptr{ ReallocateAligned(_original, _size, m_defaultAlignment) };

		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		m_allocationMap[ptr] = { _size, _file, _line };
		if (_original) { m_allocationMap.erase(_original); }

		return ptr;
	}



	void TrackingAllocator::Free(void* _ptr)
	{
		if (!m_allocationMap.contains(_ptr))
		{
			m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "Free - _ptr does not point to memory allocated through this tracking allocator");
		}
		const std::string size{ FormatUtils::GetSizeString(m_allocationMap[_ptr].size) };
		const std::string file{ m_allocationMap[_ptr].file };
		const std::string line{ std::to_string(m_allocationMap[_ptr].line) };
		if (m_verbose) { m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Application Free: " + file + " - line " + line + "--- Freeing " + size + " (implicitly aligned to " + FormatUtils::GetSizeString(m_defaultAlignment) + ")\n"); }

		FreeAligned(_ptr);
		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		m_allocationMap.erase(_ptr);
	}



	void* VKAPI_CALL TrackingAllocator::Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		TrackingAllocator* allocator{ reinterpret_cast<TrackingAllocator*>(_pUserData) };
		if (allocator->m_verbose) { allocator->m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Allocation: " + VulkanAllocationScopeToString(_allocationScope) + "--- Request for " + FormatUtils::GetSizeString(_size) + " (aligned to " + FormatUtils::GetSizeString(_alignment) + ")\n"); }
		void* ptr{ allocator->AllocateAligned(_size, _alignment) };

		std::lock_guard<std::mutex> lock(allocator->m_allocationMapMtx);
		allocator->m_allocationMap[ptr] = { _size, nullptr, 0 };

		return ptr;
	}



	void* VKAPI_CALL TrackingAllocator::Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		TrackingAllocator* allocator{ reinterpret_cast<TrackingAllocator*>(_pUserData) };
		if (allocator->m_verbose) { allocator->m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Reallocation: " + VulkanAllocationScopeToString(_allocationScope) + "--- Request for " + FormatUtils::GetSizeString(_size) + " (aligned to " + FormatUtils::GetSizeString(_alignment) + ")\n"); }
		void* ptr{ allocator->ReallocateAligned(_pOriginal, _size, _alignment) };

		std::lock_guard<std::mutex> lock(allocator->m_allocationMapMtx);
		allocator->m_allocationMap[ptr] = { _size, nullptr, 0 };
		if (_pOriginal) { allocator->m_allocationMap.erase(_pOriginal); }

		return ptr;
	}



	void VKAPI_CALL TrackingAllocator::Free(void* _pUserData, void* _pMemory)
	{
		TrackingAllocator* allocator{ reinterpret_cast<TrackingAllocator*>(_pUserData) };
		if (allocator->m_verbose) { allocator->m_logger->Log(LOGGER_CHANNEL::INFO, LOGGER_LAYER::TRACKING_ALLOCATOR, "Vulkan Free" + std::string("--- Freeing ") + FormatUtils::GetSizeString(allocator->m_allocationMap[_pMemory].size) + "\n"); }
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
		#if defined(_WIN32)
			void* ptr{ _aligned_malloc(_size, _alignment) };
			if (ptr == nullptr)
		#else
			void* ptr{ nullptr };
			if (posix_memalign(&ptr, _alignment, _size) != 0)
		#endif
		{
			m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tAllocateAligned - Allocation failed.\n");
			if (!m_verbose) { m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tSet TrackingAllocator::m_verbose flag to see more detailed output"); }
			throw std::runtime_error("");
		}

		return ptr;
	}



	void* TrackingAllocator::ReallocateAligned(void* _original, std::size_t _size, std::size_t _alignment)
	{
		if (_original == nullptr)
		{
			m_logger->Log(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tReallocationAligned - _original was nullptr, falling back to AllocationAligned.\n");
			return AllocateAligned(_size, _alignment);
		}
		
		if (_size == 0)
		{
			m_logger->Log(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tReallocationImpl - _size was 0, falling back to FreeAligned (returning nullptr)\n");
			FreeAligned(_original);
			return nullptr;
		}

		//Get old size
		std::lock_guard<std::mutex> lock(m_allocationMapMtx);
		std::size_t oldSize{ m_allocationMap[_original].size };

		//Allocate a new block
		void* newPtr{ AllocateAligned(_size, _alignment) };
		if (newPtr == nullptr)
		{
			m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tReallocateAligned - Allocation returned nullptr.\n");
			throw std::runtime_error("");
		}

		//Copy data and free old block
		const std::size_t bytesToCopy{ (oldSize < _size) ? oldSize : _size };
		memcpy(newPtr, _original, bytesToCopy);
		FreeAligned(_original);

		return newPtr;
	}



	void TrackingAllocator::FreeAligned(void* _ptr)
	{
		if (_ptr == nullptr)
		{
			m_logger->Log(LOGGER_CHANNEL::ERROR, LOGGER_LAYER::TRACKING_ALLOCATOR, "\tFreeAligned - _ptr was nullptr.\n");
			throw std::runtime_error("");
		}

		#if defined(_WIN32)
			_aligned_free(_ptr);
		#else
			free(_ptr);
		#endif
	}
}