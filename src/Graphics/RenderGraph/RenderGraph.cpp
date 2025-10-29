#include "RenderGraph.h"

#include <RHI/RHIUtils.h>


namespace NK
{

	RenderGraphDesc* RenderGraphDesc::AddGraphicsNode(const NODE_NAME& _name, const std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>& _resources)
	{
		nodes.push_back({ _name, Node(COMMAND_TYPE::GRAPHICS, _resources) });
		resources[_name] = _resources;
		return this; //For chaining
	}

	RenderGraphDesc* RenderGraphDesc::AddComputeNode(const NODE_NAME& _name, const std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>& _resources)
	{
		nodes.push_back({ _name, Node(COMMAND_TYPE::COMPUTE, _resources) });
		resources[_name] = _resources;
		return this; //For chaining
	}

	RenderGraphDesc* RenderGraphDesc::AddTransferNode(const NODE_NAME& _name, const std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>& _resources)
	{
		nodes.push_back({ _name, Node(COMMAND_TYPE::TRANSFER, _resources) });
		resources[_name] = _resources;
		return this; //For chaining
	}



	RenderGraph::RenderGraph(RenderGraphDesc& _desc)
	{
		//Get required nodes (a node is considered not required if it doesn't contribute to the root node either directly or indirectly)
		//The root node is, hence, obviously required
		m_requiredNodes.push_back(_desc.nodes.back());
		m_requiredResources[m_requiredNodes[0].first] = _desc.resources[m_requiredNodes[0].first];


		//todo: this whole process could be sped up by a bunch with bitwise operations (https://poniesandlight.co.uk/reflect/island_rendergraph_1/)
		//todo: ^for now, since this is a one-time setup thing and the graphs aren't like 10'000 nodes long, it's fine

		//Iterate in reverse through the nodes - skipping the first (back) node (it's the root node)
		for (std::vector<std::pair<NODE_NAME, Node>>::reverse_iterator nodeIt{ _desc.nodes.rbegin() + 1 }; nodeIt != _desc.nodes.rend(); ++nodeIt)
		{
			//Whether or not the node is required
			bool required{ false };

			//Go through all required resources and see if the current node writes to any of them
			for (std::unordered_map<const NODE_NAME&, std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>>::const_iterator allReqResIt{ m_requiredResources.begin() }; allReqResIt != m_requiredResources.end(); ++allReqResIt)
			{
				for (const std::pair<RESOURCE_NAME, RESOURCE_STATE> requiredResourcesCurrent : allReqResIt->second)
				{
					//starting to heavily consider those bitwise operations....
					for (const std::pair<RESOURCE_NAME, RESOURCE_STATE>& nodeRes : _desc.resources[nodeIt->first])
					{
						if (nodeRes.first == requiredResourcesCurrent.first)
						{
							//Current node uses a resource in the required resource list, is it writing to it?
							const RESOURCE_ACCESS_TYPE accessType{ RHIUtils::GetResourceAccessType(nodeRes.second) };
							if (accessType == RESOURCE_ACCESS_TYPE::WRITE || accessType == RESOURCE_ACCESS_TYPE::READ_WRITE)
							{
								//Node is writing to a resource in the required resource list, which means it itself is required
								required = true;
								
								//Store the transition that needs to occur for this resource after the execution of this node
								RESOURCE_TRANSITION trans{ nodeRes.first, { nodeRes.second, requiredResourcesCurrent.second } };
								m_transitionTable[nodeIt->first].push_back(std::move(trans));
							}
						}
					}
				}
			}
			
			if (required)
			{
				m_requiredNodes.push_back(*nodeIt);
				m_requiredResources[nodeIt->first] = _desc.resources[nodeIt->first];
			}
		}
	}

}