#pragma once

#include "ContextConfig.h"
#include "Debug/ILogger.h"
#include "Memory/IAllocator.h"


namespace NK
{
	
	//Global static context class
	class Context
	{
	public:
		static void Initialise(const ContextConfig& _config);
		static void Shutdown();
		
		Context() = delete;
		~Context() = delete;

		[[nodiscard]] inline static ILogger* GetLogger() { return m_logger; }
		[[nodiscard]] inline static IAllocator* GetAllocator() { return m_allocator; }
		[[nodiscard]] inline static LAYER_UPDATE_STATE GetLayerUpdateState() { return m_layerUpdateState; }
		[[nodiscard]] inline static bool GetEditorActive() { return m_editorActive; }
		[[nodiscard]] inline static float GetFixedUpdateTimestep() { return m_fixedUpdateTimestep; }
		
		inline static void SetLayerUpdateState(const LAYER_UPDATE_STATE _state) { m_layerUpdateState = _state; }
		inline static void SetEditorActive(const bool _active) { m_editorActive = _active; }


	protected:
		static ILogger* m_logger;
		static IAllocator* m_allocator;
		static LAYER_UPDATE_STATE m_layerUpdateState;
		static bool m_editorActive; //todo: this is very ugly, this shouldn't be here, find a better way of doing this
		static float m_fixedUpdateTimestep;
	};
	
}