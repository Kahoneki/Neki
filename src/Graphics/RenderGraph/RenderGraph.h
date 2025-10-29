#pragma once

#include "Node.h"

#include <RHI/IBuffer.h>
#include <RHI/ITexture.h>

#include <unordered_map>
#include <Core/Memory/Allocation.h>


namespace NK
{

	typedef std::string NODE_NAME;
	typedef std::string RESOURCE_NAME;
	typedef std::string NODE_DATA_NAME;
	struct RenderGraphDesc final
	{
		friend class RenderGraph;


	public:
		RenderGraphDesc* AddGraphicsNode(const NODE_NAME& _name, const std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>& _resources);
		RenderGraphDesc* AddComputeNode(const NODE_NAME& _name, const std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>& _resources);
		RenderGraphDesc* AddTransferNode(const NODE_NAME& _name, const std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>& _resources);


	private:
		//Parallel vectors - 1 entry for each node
		std::vector<std::pair<NODE_NAME, Node>> nodes; //node-semantic-name and node - vector to preserve order of nodes - last node in `nodes` is the root node
		std::unordered_map<const NODE_NAME&, std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>> resources; //Node-semantic-name to all resources accessed in that node (resource semantic name and required state)
	};


	typedef std::variant<IPipeline*, IShader*, std::function<void(const std::unordered_map<RESOURCE_NAME, RESOURCE_STATE>&)>> NODE_DATA_VARIANT;
	typedef std::variant<IBuffer*, ITexture*> RENDER_DATA_VARIANT;
	struct RenderGraphExecutionDesc
	{
		std::unordered_map<NODE_NAME, NODE_DATA_VARIANT> nodeData; //Node-semantic-name to actual data (pipeline, shader, or function)
		std::unordered_map<RESOURCE_NAME, RENDER_DATA_VARIANT> resourceData; //Resource-semantic name to actual data (buffer or texture)
	};


	typedef std::pair<std::string, std::pair<RESOURCE_STATE, RESOURCE_STATE>> RESOURCE_TRANSITION;
	class RenderGraph final
	{
	public:
		explicit RenderGraph(RenderGraphDesc& _desc);


	private:
		std::vector<std::pair<NODE_NAME, Node>> m_requiredNodes;
		std::unordered_map<const NODE_NAME&, std::vector<std::pair<RESOURCE_NAME, RESOURCE_STATE>>> m_requiredResources; //Subset of RenderGraphDesc::resources for all required nodes
		std::unordered_map<const NODE_NAME&, std::vector<RESOURCE_TRANSITION>> m_transitionTable; //Node-semantic-name to transition-to-be-made after that node has been executed
	};

}