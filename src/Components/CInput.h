#pragma once

#include "CImGuiInspectorRenderable.h"

#include <Core/Utils/Serialisation/TypeRegistry.h>
#include <Managers/InputManager.h>
#include <Types/NekiTypes.h>

#include <stdexcept>
#include <unordered_map>


namespace NK
{
	
	struct CInput final : public CImGuiInspectorRenderable
	{
		friend class ClientNetworkLayer;
		friend class ServerNetworkLayer;
		friend class InputLayer;
		friend class PlayerCameraLayer;
		
		
	public:
		//Helper function to add an action state to the actionStates map
		template<typename ActionType>
		inline void AddActionToMap(ActionType _action)
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };
			actionStates[key] = {};
		}


		//Helper function to remove an action state from the actionStates map
		template<typename ActionType>
		inline void RemoveActionFromMap(ActionType _action)
		{
			const ActionTypeMapKey key{ std::type_index(typeid(ActionType)), std::to_underlying(_action) };
			const std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT>::iterator it{ actionStates.find(key) };
			if (it == actionStates.end()) { throw std::invalid_argument("CInput::RemoveActionFromMap() - invalid action provided - type: " + std::string(_action.name()) + ", element: " + std::to_string(_action)); }
			actionStates.erase(it);
		}

		
		//Helper function to safely get the state
		//Fun fact apparently this compiles despite seemingly creating multiple identical functions only separated by return type because templates will change the name of the function
		template<typename T, typename ActionType>
		T GetActionState(ActionType _action) const
		{
			const ActionTypeMapKey key{ TypeRegistry::GetConstant(std::type_index(typeid(ActionType))), std::to_underlying(_action) };
			const std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT>::const_iterator it{ actionStates.find(key) };
			if (it == actionStates.end()) { return T{}; }
			return std::get<T>(it->second);
		}
		

		[[nodiscard]] inline static std::string GetStaticName() { return "Input"; }

		
		SERIALISE_MEMBER_FUNC(actionStates)
		
		
	private:
		[[nodiscard]] virtual inline std::string GetComponentName() const override { return "Input"; }
		virtual inline ImGuiTreeNodeFlags GetTreeNodeFlags() const override { return ImGuiTreeNodeFlags_DefaultOpen; }

		
		virtual inline void RenderImGuiInspectorContents(Registry& _reg) override
		{
			std::unordered_map<std::string, std::pair<std::type_index, std::uint32_t>>& registeredActions{ TypeRegistry::GetInputActions() };

			if (ImGui::BeginTable("InputActionsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed, 20.0f);

				std::vector<ActionTypeMapKey> toRemove{};

				for (std::pair<ActionTypeMapKey, INPUT_STATE_VARIANT> boundAction : actionStates)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);

					std::string displayName{ "Unknown Action" };
					for (const std::pair<std::string, std::pair<std::type_index, std::uint32_t>> registeredAction : registeredActions)
					{
						if (TypeRegistry::GetConstant(registeredAction.second.first) == boundAction.first.first && registeredAction.second.second == boundAction.first.second)
						{
							displayName = registeredAction.first;
							break;
						}
					}

					ImGui::AlignTextToFramePadding();
					ImGui::Text("%s", displayName.c_str());
					if (ImGui::IsItemHovered())
					{
						std::string stateType = "Empty";
						if (std::holds_alternative<ButtonState>(boundAction.second))		{ stateType = "Button"; }
						else if (std::holds_alternative<Axis1DState>(boundAction.second))	{ stateType = "Axis 1D"; }
						else if (std::holds_alternative<Axis2DState>(boundAction.second))	{ stateType = "Axis 2D"; }
						ImGui::SetTooltip("%s\nEnum Value: %u", stateType.c_str(), boundAction.first.second);
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::PushID(static_cast<int>(boundAction.first.first ^ boundAction.first.second));
					if (ImGui::Button("X"))
					{
						toRemove.push_back(boundAction.first);
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
				
				for (const ActionTypeMapKey& k : toRemove)
				{
					actionStates.erase(k);
				}
			}

			ImGui::Spacing();

			if (ImGui::Button("Add Action"))
			{
				ImGui::OpenPopup("AddActionPopup");
			}

			if (ImGui::BeginPopup("AddActionPopup"))
			{
				if (registeredActions.empty())
				{
					ImGui::TextDisabled("No actions registered via TypeRegistry");
				}
				else
				{
					for (const std::pair<const std::string, std::pair<std::type_index, std::uint32_t>>& registeredAction : registeredActions)
					{
						const ActionTypeMapKey key{ TypeRegistry::GetConstant(registeredAction.second.first), registeredAction.second.second };

						if (actionStates.contains(key)) { continue; }

						if (ImGui::Selectable(registeredAction.first.c_str()))
						{
							const INPUT_BINDING_TYPE type{ InputManager::GetActionInputType(key) };
							
							switch (type)
							{
							case INPUT_BINDING_TYPE::AXIS_1D:
								actionStates[key] = Axis1DState{};
								break;
							case INPUT_BINDING_TYPE::AXIS_2D:
								actionStates[key] = Axis2DState{};
								break;
							case INPUT_BINDING_TYPE::BUTTON:
							case INPUT_BINDING_TYPE::UNBOUND:
							default:
								actionStates[key] = ButtonState{};
								break;
							}
						}
					}
				}
				ImGui::EndPopup();
			}
		}
		
		
		//Map from user's action-types enum (represented as a pair of both the enum class' type index and the std::uint32_t underlying type) to its current state
		std::unordered_map<ActionTypeMapKey, INPUT_STATE_VARIANT> actionStates;
	};
	
}
