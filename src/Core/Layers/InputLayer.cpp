#include "InputLayer.h"

#include <Components/CInput.h>
#include <Managers/InputManager.h>


namespace NK
{

	InputLayer::InputLayer(Registry& _reg, const InputLayerDesc& _desc)
	: ILayer(_reg), m_desc(_desc)
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::INPUT_LAYER, "Initialising Input Layer\n");

		InputManager::SetWindow(m_desc.window);
		
		m_logger.Unindent();
	}



	InputLayer::~InputLayer()
	{
		m_logger.Indent();
		m_logger.Log(LOGGER_CHANNEL::HEADING, LOGGER_LAYER::INPUT_LAYER, "Shutting Down Input Layer\n");

		m_logger.Unindent();
	}



	void InputLayer::Update()
	{
		InputManager::Update();
		
		//Get all input components and populate their action states with the input manager
		for (auto&& [input] : m_reg.get().View<CInput>())
		{
			for (std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT>::iterator it{ input.actionStates.begin() }; it != input.actionStates.end(); ++it)
			{
				switch (InputManager::GetActionInputType(it->first))
				{
				case INPUT_BINDING_TYPE::UNBOUND:
				{
					//It's fine for the user to not have an input bound to an action, provide a warning, not an error
					m_logger.IndentLog(LOGGER_CHANNEL::WARNING, LOGGER_LAYER::INPUT_LAYER, "Unbound action found - type: " + std::to_string(it->first.first) + ", element: " + std::to_string(it->first.second) + "\n");
					break;
				}
				case INPUT_BINDING_TYPE::BUTTON:
				{
					it->second = InputManager::GetButtonState(it->first);
					break;
				}
				case INPUT_BINDING_TYPE::AXIS_1D:
				{
					it->second = InputManager::GetAxis1DState(it->first);
					break;
				}
				case INPUT_BINDING_TYPE::AXIS_2D:
				{
					it->second = InputManager::GetAxis2DState(it->first);
					break;
				}
				}
			}
		}
	}

}