#pragma once

#include "ContextConfig.h"
#include "Debug/ILogger.h"
#include "Memory/IAllocator.h"


namespace NK
{
	struct CLight;

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
		[[nodiscard]] inline static CLight* GetActiveLightView() { return m_activeLightView; }
		[[nodiscard]] inline static bool GetEditorActive() { return m_editorActive; }
		[[nodiscard]] inline static bool GetPaused() { return m_paused; }
		[[nodiscard]] inline static bool GetPopupOpen() { return m_popupOpen; }
		[[nodiscard]] inline static float GetFixedUpdateTimestep() { return m_fixedUpdateTimestep; }
		
		inline static void SetLayerUpdateState(const LAYER_UPDATE_STATE _state) { m_layerUpdateState = _state; }
		inline static void SetActiveLightView(CLight* _light) { m_activeLightView = _light; }
		inline static void SetEditorActive(const bool _active) { m_editorActive = _active; }
		inline static void SetPaused(const bool _paused) { m_paused = _paused; }
		inline static void SetPopupOpen(const bool _inputting) { m_popupOpen = _inputting; }
		inline static void SetFixedUpdateTimestep(const float _timestep) { m_fixedUpdateTimestep = _timestep; }


	protected:
		static ILogger* m_logger;
		static IAllocator* m_allocator;
		static LAYER_UPDATE_STATE m_layerUpdateState;
		static CLight* m_activeLightView; //todo: this is very ugly, this shouldn't be here, find a better way of doing this
		static bool m_editorActive; //todo: this is very ugly, this shouldn't be here, find a better way of doing this
		static bool m_paused; //todo: this is very ugly, this shouldn't be here, find a better way of doing this
		static bool m_popupOpen; //todo: this is very ugly, this shouldn't be here, find a better way of doing this
		static float m_fixedUpdateTimestep;
	};
	
}