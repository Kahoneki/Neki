#include "StreamingManager.h"


namespace NK
{
    
    const GPUModel* StreamingManager::RequestModel(const std::string& _filepath)
    {
        if (m_models[_filepath].state == STREAMING_STATE::UNLOADED)
        {
            m_models[_filepath].state = STREAMING_STATE::PENDING_LOAD;
        }
        ++m_models[_filepath].refCount;
        return m_models[_filepath].gpuModel.get();
    }

    
    
    const ITexture* StreamingManager::RequestTexture(const std::string& _filepath)
    {
        if (m_textures[_filepath].state == STREAMING_STATE::UNLOADED)
        {
            m_textures[_filepath].state = STREAMING_STATE::PENDING_LOAD;
        }
        ++m_textures[_filepath].refCount;
        return m_textures[_filepath].gpuTexture.get();
    }

    
    
    void StreamingManager::ReleaseModel(const std::string& _filepath)
    {
        if (!m_models.contains(_filepath))
        {
            throw std::invalid_argument("StreamingManager::ReleaseModel() - provided _filepath (" + _filepath + ") is not in database.\n");
        }
        
        --m_models[_filepath].refCount;
        if (m_models[_filepath].refCount == 0 && m_models[_filepath].state != STREAMING_STATE::UNLOADED)
        {
            m_models[_filepath].state = STREAMING_STATE::PENDING_UNLOAD;
        }
    }

    
    
    void StreamingManager::ReleaseTexture(const std::string& _filepath)
    {
        if (!m_textures.contains(_filepath))
        {
            throw std::invalid_argument("StreamingManager::ReleaseTexture() - provided _filepath (" + _filepath + ") is not in database.\n");
        }
        
        --m_textures[_filepath].refCount;
        if (m_textures[_filepath].refCount == 0 && m_textures[_filepath].state != STREAMING_STATE::UNLOADED)
        {
            m_textures[_filepath].state = STREAMING_STATE::PENDING_UNLOAD;
        }
    }

    
    
    void StreamingManager::Update(GPUUploader& _uploader)
    {
        for (std::unordered_map<std::string, ModelStreamingInfo>::iterator it{ m_models.begin() }; it != m_models.end(); ++it)
        {
            switch (it->second.state)
            {
            case STREAMING_STATE::UNLOADED:
            {
                continue;
            }
            case STREAMING_STATE::PENDING_LOAD:
            {
                //Load STREAMING_STATE::PENDING_LOAD assets from disk
                m_models[it->first].cpuModel = ModelLoader::LoadModel(it->first);
                break;
            }
            case STREAMING_STATE::LOADED_RAM:
            {
                
            }
            }
        }
        
        //-Pushes STREAMING_STATE::LOADED_RAM assets to _uploader if within VRAM budget
        //-Updates STREAMING_STATE::UPLOADING assets to STREAMING_STATE::READY based on the state of _uploader's fence
        //-Frees CPU memory for STREAMING_STATE::READY assets
        //-Handles unloading timer for STREAMING_STATE::PENDING_UNLOAD assets
    }
    
    
}
