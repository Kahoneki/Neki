#include "RenderGraph.h"

#include <RHI/ICommandBuffer.h>
#include <RHI/RHIUtils.h>


namespace NK
{

	RenderGraphDesc* RenderGraphDesc::AddNode(const NODE_NAME& _name, const std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>& _resourceStates, const std::function<void(ICommandBuffer*, const BindingMap<IBuffer>&, const BindingMap<ITexture>&, const BindingMap<IBufferView>&, const BindingMap<ITextureView>&, const BindingMap<ISampler>&)>& _execFunc, bool _forceRequire)
	{
		nodes.push_back({ _name, Node(_resourceStates, _forceRequire) });
		resources[_name] = _resourceStates;
		executionFunctions[_name] = _execFunc;
		return this; //For chaining
	}



	RenderGraphDesc* RenderGraphDesc::AddNode(const NODE_NAME& _name, const std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>& _resourceStates, bool _forceRequire)
	{
		nodes.push_back({ _name, Node(_resourceStates, _forceRequire) });
		resources[_name] = _resourceStates;
		return this; //For chaining
	}



	RenderGraph::RenderGraph(RenderGraphDesc& _desc)
	{
		//Get required nodes (a node is considered not required if it doesn't contribute to the root node either directly or indirectly)
		//The root node is, hence, obviously required
		m_requiredNodes.push_back(_desc.nodes.back());

		//Add all inputs to the root node as required resources
		for (const std::pair<BINDING_NAME, RESOURCE_STATE>& resource : _desc.resources[m_requiredNodes[0].first])
		{
			m_requiredResources[m_requiredNodes[0].first].push_back(resource);
		}

		//Add the root node's execution function as required execution function if it's not an empty (transition-only) node
		if (m_requiredExecutionFunctions.contains(m_requiredNodes[0].first))
		{
			m_requiredExecutionFunctions[m_requiredNodes[0].first] = _desc.executionFunctions[m_requiredNodes[0].first];
		}


		//todo: this whole process could be sped up by a bunch with bitwise operations (https://poniesandlight.co.uk/reflect/island_rendergraph_1/)
		//todo: ^for now, since this is a one-time setup thing and the graphs aren't like 10'000 nodes long, it's fine

		//Iterate in reverse through the nodes - skipping the first (back) node (it's the root node)
		for (std::vector<std::pair<NODE_NAME, Node>>::reverse_iterator nodeIt{ _desc.nodes.rbegin() + 1 }; nodeIt != _desc.nodes.rend(); ++nodeIt)
		{
			//Whether or not the node is required
			//Initialise to forceRequire then only run requirement check logic if forceRequire is false
			bool required{ nodeIt->second.forceRequire };

			if (!nodeIt->second.forceRequire)
			{
				//Go through all required resources and see if the current node writes to any of them
				for (std::unordered_map<NODE_NAME, std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>>::const_iterator allReqResIt{ m_requiredResources.begin() }; allReqResIt != m_requiredResources.end(); ++allReqResIt)
				{
					for (const std::pair<BINDING_NAME, RESOURCE_STATE> requiredResource : allReqResIt->second)
					{
						//starting to heavily consider those bitwise operations....
						for (const std::pair<BINDING_NAME, RESOURCE_STATE>& nodeRes : _desc.resources[nodeIt->first])
						{
							if (nodeRes.first == requiredResource.first)
							{
								//Current node uses a resource in the required resource list, is it writing to it?
								const RESOURCE_ACCESS_TYPE accessType{ RHIUtils::GetResourceAccessType(nodeRes.second) };
								if (accessType == RESOURCE_ACCESS_TYPE::WRITE || accessType == RESOURCE_ACCESS_TYPE::READ_WRITE)
								{
									//Node is writing to a resource in the required resource list, which means it itself is required
									required = true;
								}
							}
						}
					}
				}
			}

			if (required)
			{
				m_requiredNodes.push_back(*nodeIt);

				//Add all of this node's resources as required
				for (const std::pair<BINDING_NAME, RESOURCE_STATE>& resource : _desc.resources[nodeIt->first])
				{
					m_requiredResources[nodeIt->first].push_back(resource);
				}

				//Add the node's execution function as required execution function
				m_requiredExecutionFunctions[nodeIt->first] = _desc.executionFunctions[nodeIt->first];
			}
		}


		//Nodes were added in reverse order, so reverse the list
		std::ranges::reverse(m_requiredNodes);
	}



	void RenderGraph::Execute(RenderGraphExecutionDesc& _desc) const
	{
		for (const std::pair<NODE_NAME, Node>& node : m_requiredNodes)
		{
			if (!_desc.commandBuffers.contains(node.first))
			{
				throw std::invalid_argument("RenderGraph::Execute() - no command buffer was bound to node \"" + node.first + "\" - all nodes require a command buffer");
			}
			
			const std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>& nodeRequiredResources{ m_requiredResources.at(node.first) };
			
			
			//Perform all resource transitions
			for (const std::pair<BINDING_NAME, RESOURCE_STATE>& reqRes : nodeRequiredResources)
			{
				const BINDING_NAME& reqResName{ reqRes.first };
				const RESOURCE_STATE reqResState{ reqRes.second };

				std::unordered_map<BINDING_NAME, IBuffer*>::iterator bufIt{ _desc.buffers.bindings.find(reqResName) };
				if (bufIt != _desc.buffers.bindings.end())
				{
					//This resource is a buffer
					//Shortcircuit to allow null-set buffers
					if (bufIt->second && bufIt->second->GetState() != reqResState)
					{
						_desc.commandBuffers.at(node.first)->TransitionBarrier(bufIt->second, bufIt->second->GetState(), reqResState);
					}
					continue;
				}

				std::unordered_map<BINDING_NAME, ITexture*>::iterator texIt{ _desc.textures.bindings.find(reqResName) };
				if (texIt != _desc.textures.bindings.end())
				{
					//This resource is a texture
					//Shortcircuit to allow null-set textures
					if (texIt->second && texIt->second->GetState() != reqResState)
					{
						_desc.commandBuffers.at(node.first)->TransitionBarrier(texIt->second, texIt->second->GetState(), reqResState);
					}
					continue;
				}

				//This resource is not bound
				throw std::invalid_argument("RenderGraph::Execute() - The resource " + reqResName + " (required for node " + node.first + ") was not bound in the _desc provided.");
			}


			//Execute node if it's not an empty (transition-only) node
			if (m_requiredExecutionFunctions.contains(node.first))
			{
				m_requiredExecutionFunctions.at(node.first)(_desc.commandBuffers.at(node.first), _desc.buffers, _desc.textures, _desc.bufferViews, _desc.textureViews, _desc.samplers);
			}
		}
	}

}