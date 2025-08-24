#include "VulkanDebugAllocator.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#if defined(_WIN32)
	#include <malloc.h>
#endif

namespace NK
{


	VulkanDebugAllocator::VulkanDebugAllocator(const VULKAN_ALLOCATOR_TYPE _type) : m_callbacks{}, m_type(_type)
	{
		m_callbacks.pUserData = static_cast<void*>(this);
		m_callbacks.pfnAllocation = &Allocation;
		m_callbacks.pfnReallocation = &Reallocation;
		m_callbacks.pfnFree = &Free;

		if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - Initialised.\n"; }
	}



	VulkanDebugAllocator::~VulkanDebugAllocator()
	{
		//Check for memory leaks
		if (!m_allocationSizes.empty())
		{
			std::cerr << "ERR::VulkanDebugAllocator::~VulkanDebugAllocator::MEMORY_LEAK::ALLOCATOR_DESTRUCTED_WITH_" << std::to_string(m_allocationSizes.size()) << "_ALLOCATIONS_UNFREED" << std::endl;
		}

		if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - Destroyed.\n"; }
	}


	
	void* VKAPI_CALL VulkanDebugAllocator::Allocation(void* _pUserData, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		return static_cast<VulkanDebugAllocator*>(_pUserData)->AllocationImpl(_size, _alignment, _allocationScope);
	}



	void* VKAPI_CALL VulkanDebugAllocator::Reallocation(void* _pUserData, void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		return static_cast<VulkanDebugAllocator*>(_pUserData)->ReallocationImpl(_pOriginal, _size, _alignment, _allocationScope);
	}



	void VKAPI_CALL VulkanDebugAllocator::Free(void* _pUserData, void* _pMemory)
	{
		return static_cast<VulkanDebugAllocator*>(_pUserData)->FreeImpl(_pMemory);
	}



	void* VulkanDebugAllocator::AllocationImpl(std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - AllocationImpl called for " << _size << " bytes aligned to " << _alignment << " bytes with " << _allocationScope << " allocation scope.\n"; }

	#if defined(_WIN32)
		void* ptr{ _aligned_malloc(_size, _alignment) };
		if (ptr == nullptr)
	#else
		void* ptr{ nullptr };
		if (posix_memalign(&ptr, _alignment, _size) != 0)
	#endif
		{
			std::cerr << "ERR::VulkanDebugAllocator::AllocationImpl::ALLOCATION_FAILED::RETURNING_NULLPTR" << std::endl;
			return nullptr;
		}

		std::lock_guard<std::mutex> lock(m_allocationSizesMtx);
		m_allocationSizes[ptr] = _size;

		return ptr;
	}



	void* VulkanDebugAllocator::ReallocationImpl(void* _pOriginal, std::size_t _size, std::size_t _alignment, VkSystemAllocationScope _allocationScope)
	{
		if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - ReallocationImpl called for address " << _pOriginal << " to the new pool of " << _size << " bytes aligned to " << _alignment << " bytes with " << _allocationScope << " allocation scope.\n"; }

		if (_pOriginal == nullptr)
		{
			if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - ReallocationImpl - _pOriginal == nullptr, falling back to VulkanDebugAllocator::Allocation\n"; }
			return AllocationImpl(_size, _alignment, _allocationScope);
		}

		if (_size == 0)
		{
			if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - ReallocationImpl - _size == 0, falling back to VulkanDebugAllocator::Free and returning nullptr\n"; }
			FreeImpl(_pOriginal);
			return nullptr;
		}

		//Get size of old allocation
		std::size_t oldSize; {
			std::lock_guard<std::mutex> lock(m_allocationSizesMtx);
			const std::unordered_map<void*, std::size_t>::iterator it{ m_allocationSizes.find(_pOriginal) };
			if (it == m_allocationSizes.end())
			{
				std::cerr << "ERR::VulkanDebugAllocator::ReallocationImpl::PROVIDED_DATA_NOT_ORIGINALLY_ALLOCATED_BY_VulkanDebugAllocator::RETURNING_NULLPTR" << std::endl;
				return nullptr;
			}
			oldSize = it->second;
		}

		//Allocate a new block
		void* newPtr{ AllocationImpl(_size, _alignment, _allocationScope) };
		if (newPtr == nullptr)
		{
			std::cerr << "ERR::VulkanDebugAllocator::ReallocationImpl::ALLOCATION_RETURNED_NULLPTR::RETURNING_NULLPTR" << std::endl;
			return nullptr;
		}

		//Copy data and free old block
		const std::size_t bytesToCopy{ (oldSize < _size) ? oldSize : _size };
		memcpy(newPtr, _pOriginal, bytesToCopy);
		FreeImpl(_pOriginal);

		return newPtr;
	}



	void VulkanDebugAllocator::FreeImpl(void* _pMemory)
	{
		if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - FreeImpl called for address " << _pMemory << "\n"; }

		if (_pMemory == nullptr)
		{
			if (m_type == VULKAN_ALLOCATOR_TYPE::DEBUG_VERBOSE) { std::cout << "VulkanDebugAllocator - FreeImpl - _pMemory == nullptr, returning.\n"; }
			return;
		}

		//Remove from m_allocationSizes map
		std::lock_guard<std::mutex> lock(m_allocationSizesMtx);
		const std::unordered_map<void*, std::size_t>::iterator it{ m_allocationSizes.find(_pMemory) };
		if (it == m_allocationSizes.end())
		{
			std::cerr << "ERR::VulkanDebugAllocator::FreeImpl::PROVIDED_DATA_NOT_ORIGINALLY_ALLOCATED_BY_VulkanDebugAllocator::RETURNING" << std::endl;
			return;
		}
		m_allocationSizes.erase(it);

	#if defined(_WIN32)
		_aligned_free(_pMemory);
	#else
		free(_pMemory);
	#endif

	}



}