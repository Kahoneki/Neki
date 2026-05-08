#pragma once

#include <Core/Memory/Allocation.h>

#include <string>


namespace NK::Neural
{
	
	class NTCLoader final
	{
	public:
		~NTCLoader();
		
		[[nodiscard]] static NTCModel* LoadMaterial(const std::string& _filepath);
		static void FreeMaterial(NTCModel* _modelData);
		static void ClearCache();
		
		
	private:
		//To avoid unnecessary duplicate loads
		static std::unordered_map<std::string, UniquePtr<NTCModel>> m_filepathToNTCModelCache;
	};
	
}