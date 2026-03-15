#include "StreamingManager.h"

#include "TimeManager.h"

#include <RHI/IFence.h>


namespace NK
{
    
    void StreamingManager::SetDevice(IDevice* _device)
    {
        m_device = _device;
    }
    
    

    const GPUModel* StreamingManager::RequestModel(const std::string& _filepath)
    {
        if (m_models[_filepath].state == STREAMING_STATE::UNLOADED)
        {
            m_models[_filepath].state = STREAMING_STATE::PENDING_LOAD;
        }
        if (m_models[_filepath].memoryCost == 0)
        {
            //todo: get mem cost from header file
        }
        if (m_models[_filepath].vramUploadStatusFence == nullptr)
        {
            m_models[_filepath].vramUploadStatusFence = m_device->CreateFence({ false });
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
        if (m_textures[_filepath].memoryCost == 0)
        {
            //todo: get mem cost from header file
        }
        if (m_models[_filepath].vramUploadStatusFence == nullptr)
        {
            m_models[_filepath].vramUploadStatusFence = m_device->CreateFence({ false });
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
        if (m_models[_filepath].refCount == 0)
        {
            throw std::invalid_argument("StreamingManager::ReleaseModel() - ref count of provided _filepath (" + _filepath + ") is already 0.\n");
        }
        if (m_models[_filepath].state == STREAMING_STATE::UNLOADED)
        {
            throw std::invalid_argument("StreamingManager::ReleaseModel() - state of provided _filepath (" + _filepath + ") is already STREAMING_STATE::UNLOADED - this implies an internal engine issue since the above check for ref count == 0 should've passed.\n");
        }
        
        --m_models[_filepath].refCount;
        if (m_models[_filepath].refCount == 0)
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
                it->second.state = STREAMING_STATE::LOADED_RAM; //todo: this shouldn't be done instantly, it's only okay right now because ModelLoader::LoadModel() is synchronous - it should be asynchronous and go to LOADING state
                it->second.timeSinceLastUse = UNLOAD_TIME;
                break;
            }
            case STREAMING_STATE::LOADED_RAM:
            {
                if (m_vramUsage + it->second.memoryCost <= VRAM_BUDGET)
                {
                    it->second.gpuModel = _uploader.EnqueueModelDataUpload(it->second.cpuModel);
                    _uploader.Flush(false, it->second.vramUploadStatusFence.get(), nullptr);
                    it->second.state = STREAMING_STATE::UPLOADING;
                }
                break;
            }
            case STREAMING_STATE::UPLOADING:
            {
                if (it->second.vramUploadStatusFence->GetSignalled())
                {
                    //Model has finished being loaded to VRAM
                    it->second.state = STREAMING_STATE::READY;
                }
                break;
            }
            case STREAMING_STATE::READY:
            {
                //free ram
                
            }
            case STREAMING_STATE::PENDING_UNLOAD:
            {
                it->second.timeSinceLastUse -= TimeManager::GetDeltaTime();
                if (it->second.timeSinceLastUse <= 0.0f)
                {
                    ModelLoader::UnloadModel(it->first);
                    it->second.state = STREAMING_STATE::UNLOADED;
                }
                break;
            }
            }
        }
        
        //-Pushes STREAMING_STATE::LOADED_RAM assets to _uploader if within VRAM budget
        //-Updates STREAMING_STATE::UPLOADING assets to STREAMING_STATE::READY based on the state of _uploader's fence
        //-Frees CPU memory for STREAMING_STATE::READY assets
        //-Handles unloading timer for STREAMING_STATE::PENDING_UNLOAD assets
    }
    
    
}
