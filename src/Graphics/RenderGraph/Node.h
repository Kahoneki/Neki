#pragma once

#include <RHI/IPipeline.h>
#include <Types/NekiTypes.h>

#include <functional>


namespace NK
{

	struct Node
	{
		explicit Node(COMMAND_TYPE _type, const std::vector<std::pair<std::string, RESOURCE_STATE>>& _resources)
		: type(_type), resources(_resources) {}

		COMMAND_TYPE type;
		std::vector<std::pair<std::string, RESOURCE_STATE>> resources; //Pair of resource semantic name and state the node needs it to be in prior to execution
	};

}