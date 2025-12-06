#pragma once

#include <RHI/IBuffer.h>
#include <RHI/IPipeline.h>
#include <RHI/ITexture.h>
#include <Types/NekiTypes.h>

#include <functional>
#include <unordered_map>


namespace NK
{
	typedef std::string BINDING_NAME;
	struct Node
	{
		explicit Node(const std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>& _resources, const bool _forceRequire)
		: resources(_resources), forceRequire(_forceRequire) {}

		std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>> resources; //Pair of resource semantic name and state the node needs it to be in prior to execution

		//kind of a hacky workaround for multi-frame graphs
		//^todo: get a better system for handling this
		bool forceRequire;
	};
	

	template<typename T>
	struct BindingMap
	{
		friend class RenderGraph;
		
		
	public:
		[[nodiscard]] inline T* Get(BINDING_NAME _name) const
		{
			if (!bindings.contains(_name))
			{
				throw std::invalid_argument("BindingMap::Get() - _name (" + _name + ")" + " was not bound");
			}
			return bindings.at(_name);
		}
		inline void Set(BINDING_NAME _name, T* _binding) { bindings[_name] = _binding; }
		inline void Remove(BINDING_NAME _name) { bindings.erase(_name); }


	private:
		std::unordered_map<BINDING_NAME, T*> bindings;
	};

	
	typedef std::string NODE_NAME;
	
	struct RenderGraphDesc final
	{
		friend class RenderGraph;


	public:
		//_forceRequire flag is kind of a hacky workaround for multi-frame graphs
		//^todo: get a better system for handling this
		RenderGraphDesc* AddNode(const NODE_NAME& _name, const std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>& _resourceStates, const std::function<void(ICommandBuffer*, const BindingMap<IBuffer>&, const BindingMap<ITexture>&, const BindingMap<IBufferView>&, const BindingMap<ITextureView>&, const BindingMap<ISampler>&)>& _execFunc, bool _forceRequire = false);
		RenderGraphDesc* AddNode(const NODE_NAME& _name, const std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>& _resourceStates, bool _forceRequire = false); //For transitions only


	private:
		std::vector<std::pair<NODE_NAME, Node>> nodes; //node-semantic-name and node - vector to preserve order of nodes - last node in `nodes` is the root node
		std::unordered_map<NODE_NAME, std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>> resources; //Node-semantic-name to all resources accessed in that node (resource semantic name and required state)
		std::unordered_map<NODE_NAME, std::function<void(ICommandBuffer*, const BindingMap<IBuffer>&, const BindingMap<ITexture>&, const BindingMap<IBufferView>&, const BindingMap<ITextureView>&, const BindingMap<ISampler>&)>> executionFunctions; //Node-semantic-name to callback function for that node - does not include entries for empty (transition-only) nodes
	};
	
	
	struct RenderGraphExecutionDesc
	{
		std::unordered_map<NODE_NAME, ICommandBuffer*> commandBuffers;
		BindingMap<IBuffer> buffers;
		BindingMap<ITexture> textures;
		BindingMap<IBufferView> bufferViews;
		BindingMap<ITextureView> textureViews;
		BindingMap<ISampler> samplers;
	};


	class RenderGraph final
	{
	public:
		explicit RenderGraph(RenderGraphDesc& _desc);
		//Does not call Begin() or End() on the provided command buffer(s)
		void Execute(RenderGraphExecutionDesc& _desc) const;


	private:
		std::vector<std::pair<NODE_NAME, Node>> m_requiredNodes;
		std::unordered_map<NODE_NAME, std::vector<std::pair<BINDING_NAME, RESOURCE_STATE>>> m_requiredResources; //Subset of RenderGraphDesc::resources for all required nodes
		std::unordered_map<NODE_NAME, std::function<void(ICommandBuffer*, const BindingMap<IBuffer>&, const BindingMap<ITexture>&, const BindingMap<IBufferView>&, const BindingMap<ITextureView>&, const BindingMap<ISampler>&)>> m_requiredExecutionFunctions; //Subset of RenderGraphDesc::executionFunctions for all required execution functions
	};

}