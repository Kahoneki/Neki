#pragma once

#include <Core/Utils/ModelLoader.h>
#include <Core/Utils/ImageLoader.h>
#include <Graphics/GPUUploader.h>
#include <Types/NekiTypes.h>

#include <string>
#include <unordered_map>
#include <memory>


namespace NK
{
    
    class StreamingManager final
    {
    public:
        //Registers interest in asset
        //Returns asset pointer if resident on GPU, otherwise returns nullptr
        //If not loaded, begins the loading process
        static const GPUModel* RequestModel(const std::string& _filepath);
        
        //Registers interest in asset
        //Returns asset pointer if resident on GPU, otherwise returns nullptr
        //If not loaded, begins the loading process
        static const ITexture* RequestTexture(const std::string& _filepath);
        
        //Decrements reference count. If 0, moves to STREAMING_STATE::PENDING_UNLOAD
        static void ReleaseModel(const std::string& _filepath);
        
        //Decreements reference count. If 0, moves to STREAMING_STATE::PENDING_UNLOAD
        static void ReleaseTexture(const std::string& _filepath);
        
        //To be called once per frame
        //-Loads STREAMING_STATE::PENDING_LOAD assets from disk
        //-Pushes STREAMING_STATE::LOADED_RAM assets to _uploader if within VRAM budget
        //-Updates STREAMING_STATE::UPLOADING assets to STREAMING_STATE::READY based on the state of _uploader's fence
        //-Frees CPU memory for STREAMING_STATE::READY assets
        //-Handles unloading timer for STREAMING_STATE::PENDING_UNLOAD assets
        static void Update(GPUUploader& _uploader);
        
        
    private:
        struct StreamingInfoBase
        {
            STREAMING_STATE state{ STREAMING_STATE::UNLOADED };
            std::uint32_t refCount{ 0 };
            float timeSinceLastUse{ 0.0f }; //for unload timer
        };

        struct ModelStreamingInfo : public StreamingInfoBase
        {
            const CPUModel* cpuModel{ nullptr };
            UniquePtr<GPUModel> gpuModel{ nullptr };
        };

        struct TextureStreamingInfo : public StreamingInfoBase
        {
            ImageData* cpuImage{ nullptr };
            UniquePtr<ITexture> gpuTexture{ nullptr };
        };

        inline static std::unordered_map<std::string, ModelStreamingInfo> m_models{};
        inline static std::unordered_map<std::string, TextureStreamingInfo> m_textures{};

        
        //Config
        static constexpr std::uint32_t VRAM_BUDGET{ 4 * 1024 * 1024 }; //4GiB
        static constexpr std::uint32_t MAX_UPLOAD_BYTES_PER_FRAME{ 20 * 1024 * 1024 }; //20 MiB
        static constexpr float UNLOAD_TIME{ 5.0f }; //Time (in seconds) to keep non-visible assets
    };
    
}