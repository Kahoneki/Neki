#pragma once
#define BCDEC_IMPLEMENTATION
#include <bcdec.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
};

struct DDS_HEADER {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    uint32_t reserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

struct DDS_HEADER_DXT10 {
    uint32_t dxgiFormat;
    uint32_t resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};
#pragma pack(pop)

// FourCC helpers
constexpr uint32_t MakeFourCC(char a, char b, char c, char d) {
    return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) |
           (static_cast<uint32_t>(c) << 16) | (static_cast<uint32_t>(d) << 24);
}

enum class DDSFormat {
    BC1, BC2, BC3, BC4, BC5, BC6H, BC7,
    RGB8, RGBA8, RG8,
    Unknown
};

struct DDSLoadResult {
    std::vector<unsigned char> pixels;
    int width = 0;
    int height = 0;
    int channels = 0; // output channels (always decompressed to this)
};

// --- DXGI_FORMAT enums (only the ones we care about) ---
enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8_UNORM = 31,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_BC6H_UF16 = 95,
    DXGI_FORMAT_BC6H_SF16 = 96,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    DXGI_FORMAT_B8G8R8A8_UNORM = 86,
};

static DDSFormat DetectDX10Format(uint32_t dxgiFormat) {
    switch (dxgiFormat) {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB: return DDSFormat::BC1;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB: return DDSFormat::BC2;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB: return DDSFormat::BC3;
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:      return DDSFormat::BC4;
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:      return DDSFormat::BC5;
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:      return DDSFormat::BC6H;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB: return DDSFormat::BC7;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM: return DDSFormat::RGBA8;
    case DXGI_FORMAT_R8G8_UNORM:     return DDSFormat::RG8;
    default: return DDSFormat::Unknown;
    }
}

static DDSFormat DetectDDSFormat(const DDS_HEADER& hdr) {
    const auto& pf = hdr.ddspf;
    if (pf.flags & 0x4) { // DDPF_FOURCC
        // Check for DX10 header first
        if (pf.fourCC == MakeFourCC('D', 'X', '1', '0')) {
            // Format will be resolved by DX10 header parsing later
            // We return Unknown here because DetectDDSFormat can't see the DX10 header.
            // The caller handles this.
            return DDSFormat::Unknown; 
        }

        switch (pf.fourCC) {
        case MakeFourCC('D', 'X', 'T', '1'): return DDSFormat::BC1;
        case MakeFourCC('D', 'X', 'T', '3'): return DDSFormat::BC2;
        case MakeFourCC('D', 'X', 'T', '5'): return DDSFormat::BC3;
        case MakeFourCC('B', 'C', '4', 'U'): return DDSFormat::BC4;
        case MakeFourCC('B', 'C', '5', 'U'): return DDSFormat::BC5;
        case MakeFourCC('B', 'C', '6', 'H'): return DDSFormat::BC6H;
        case MakeFourCC('B', 'C', '7', '\0'): return DDSFormat::BC7;
        // Older D3D9 FourCCs for BC4/BC5
        case MakeFourCC('A', 'T', 'I', '1'): return DDSFormat::BC4;
        case MakeFourCC('A', 'T', 'I', '2'): return DDSFormat::BC5;
        default: return DDSFormat::Unknown;
        }
    }
    // Uncompressed RGB/RGBA
    if (pf.flags & 0x40) { // DDPF_RGB
        if (pf.aBitMask) return DDSFormat::RGBA8;
        return DDSFormat::RGB8;
    }
    return DDSFormat::Unknown;
}

static DDSLoadResult LoadDDS(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("LoadDDS() - failed to open: " + filepath);
    }

    // Verify magic "DDS "
    char magic[4];
    file.read(magic, 4);
    if (memcmp(magic, "DDS ", 4) != 0) {
        throw std::runtime_error("LoadDDS() - not a DDS file: " + filepath);
    }

    DDS_HEADER hdr{};
    file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (hdr.size != 124) {
        throw std::runtime_error("LoadDDS() - invalid DDS header size");
    }

    // Check if we have a DX10 header
    const bool isDX10 = (hdr.ddspf.flags & 0x4) && (hdr.ddspf.fourCC == MakeFourCC('D', 'X', '1', '0'));
    DDS_HEADER_DXT10 dx10hdr{};
    
    if (isDX10) {
        file.read(reinterpret_cast<char*>(&dx10hdr), sizeof(dx10hdr));
        if (file.gcount() != sizeof(dx10hdr)) {
            throw std::runtime_error("LoadDDS() - failed to read DX10 header");
        }
    }

    // Determine format
    DDSFormat fmt = isDX10 ? DetectDX10Format(dx10hdr.dxgiFormat) : DetectDDSFormat(hdr);
    if (fmt == DDSFormat::Unknown) {
        throw std::runtime_error("LoadDDS() - unsupported or unknown DDS format in: " + filepath);
    }
    
    int outChannels = 4;
    if (fmt == DDSFormat::BC4)       outChannels = 1;
    else if (fmt == DDSFormat::BC5)  outChannels = 2;
    else if (fmt == DDSFormat::RG8)  outChannels = 2;
    else if (fmt == DDSFormat::RGB8) outChannels = 3;

    const uint32_t w = hdr.width;
    const uint32_t h = hdr.height;

    // Calculate total header size to skip to pixel data
    const size_t headerSize = 4 + sizeof(DDS_HEADER) + (isDX10 ? sizeof(DDS_HEADER_DXT10) : 0);

    // Read all remaining data
    file.seekg(0, std::ios::end);
    const auto fileSize = static_cast<std::streamsize>(file.tellg());
    std::vector<unsigned char> fileData(fileSize - headerSize);
    file.seekg(headerSize, std::ios::beg);
    file.read(reinterpret_cast<char*>(fileData.data()), fileData.size());

    DDSLoadResult result;
    result.width = static_cast<int>(w);
    result.height = static_cast<int>(h);
    result.channels = outChannels;
    result.pixels.resize(w * h * outChannels);

    // Decompress base level (level 0)
    const uint32_t blocksX = (w + 3) / 4;
    const uint32_t blocksY = (h + 3) / 4;
    size_t srcOffset = 0;

    switch (fmt) {
    case DDSFormat::BC1: {
        constexpr size_t bs = 8;
        for (uint32_t by = 0; by < blocksY; ++by) {
            for (uint32_t bx = 0; bx < blocksX; ++bx) {
                unsigned char rgba[4 * 4 * 4]; // 4x4 pixels, 4 channels
                bcdec_bc1(rgba, fileData.data() + srcOffset, 4); // pitch is 4 pixels
                // Copy into result
                for (uint32_t y = 0; y < 4; ++y) {
                    for (uint32_t x = 0; x < 4; ++x) {
                        const uint32_t px = bx * 4 + x;
                        const uint32_t py = by * 4 + y;
                        if (px < w && py < h) {
                            size_t dstIdx = (py * w + px) * outChannels;
                            size_t srcIdx = (y * 4 + x) * 4;
                            for (int c = 0; c < outChannels; ++c) {
                                result.pixels[dstIdx + c] = rgba[srcIdx + c];
                            }
                        }
                    }
                }
                srcOffset += bs;
            }
        }
        break;
    }
    case DDSFormat::BC3: {
        for (uint32_t by = 0; by < blocksY; ++by) {
            for (uint32_t bx = 0; bx < blocksX; ++bx) {
                unsigned char rgba[4 * 4 * 4];
                bcdec_bc3(rgba, fileData.data() + srcOffset, 4);
                for (uint32_t y = 0; y < 4; ++y) {
                    for (uint32_t x = 0; x < 4; ++x) {
                        const uint32_t px = bx * 4 + x;
                        const uint32_t py = by * 4 + y;
                        if (px < w && py < h) {
                            size_t dstIdx = (py * w + px) * outChannels;
                            size_t srcIdx = (y * 4 + x) * 4;
                            for (int c = 0; c < outChannels; ++c) {
                                result.pixels[dstIdx + c] = rgba[srcIdx + c];
                            }
                        }
                    }
                }
                srcOffset += 16;
            }
        }
        break;
    }
    case DDSFormat::BC4: {
        for (uint32_t by = 0; by < blocksY; ++by) {
            for (uint32_t bx = 0; bx < blocksX; ++bx) {
                unsigned char r[4 * 4];
                bcdec_bc4(r, fileData.data() + srcOffset, 4);
                for (uint32_t y = 0; y < 4; ++y) {
                    for (uint32_t x = 0; x < 4; ++x) {
                        const uint32_t px = bx * 4 + x;
                        const uint32_t py = by * 4 + y;
                        if (px < w && py < h) {
                            result.pixels[(py * w + px) * outChannels] = r[y * 4 + x];
                        }
                    }
                }
                srcOffset += 8;
            }
        }
        break;
    }
    case DDSFormat::BC5: {
        for (uint32_t by = 0; by < blocksY; ++by) {
            for (uint32_t bx = 0; bx < blocksX; ++bx) {
                unsigned char rg[4 * 4 * 2];
                bcdec_bc5(rg, fileData.data() + srcOffset, 4);
                for (uint32_t y = 0; y < 4; ++y) {
                    for (uint32_t x = 0; x < 4; ++x) {
                        const uint32_t px = bx * 4 + x;
                        const uint32_t py = by * 4 + y;
                        if (px < w && py < h) {
                            result.pixels[(py * w + px) * outChannels + 0] = rg[(y * 4 + x) * 2 + 0];
                            result.pixels[(py * w + px) * outChannels + 1] = rg[(y * 4 + x) * 2 + 1];
                        }
                    }
                }
                srcOffset += 16;
            }
        }
        break;
    }
    case DDSFormat::BC7: {
        for (uint32_t by = 0; by < blocksY; ++by) {
            for (uint32_t bx = 0; bx < blocksX; ++bx) {
                unsigned char rgba[4 * 4 * 4];
                bcdec_bc7(rgba, fileData.data() + srcOffset, 4);
                for (uint32_t y = 0; y < 4; ++y) {
                    for (uint32_t x = 0; x < 4; ++x) {
                        const uint32_t px = bx * 4 + x;
                        const uint32_t py = by * 4 + y;
                        if (px < w && py < h) {
                            size_t dstIdx = (py * w + px) * outChannels;
                            size_t srcIdx = (y * 4 + x) * 4;
                            for (int c = 0; c < outChannels; ++c) {
                                result.pixels[dstIdx + c] = rgba[srcIdx + c];
                            }
                        }
                    }
                }
                srcOffset += 16;
            }
        }
        break;
    }
    case DDSFormat::RGB8: {
        const size_t rowBytes = w * 3;
        for (uint32_t y = 0; y < h; ++y) {
            memcpy(result.pixels.data() + y * rowBytes, fileData.data() + y * rowBytes, rowBytes);
        }
        break;
    }
    case DDSFormat::RGBA8:
    case DDSFormat::RG8: {
        // Tightly packed, just copy all bytes
        memcpy(result.pixels.data(), fileData.data(), result.pixels.size());
        break;
    }
    default:
        throw std::runtime_error("LoadDDS() - unsupported or unimplemented DDS format");
    }

    return result;
}