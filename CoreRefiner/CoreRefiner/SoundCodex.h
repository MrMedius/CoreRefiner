#pragma once
#pragma comment(lib, "Xaudio2.lib")

#include <wrl/client.h>
#include <windows.h>
#include <xaudio2.h>
#include <X3DAudio.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "SoundPaths.h"

class SoundCodex
{
public:
    using SeHandle = uint64_t;

public:
    // Global Singleton
    static SoundCodex& Get() noexcept
    {
        static SoundCodex inst;
        return inst;
    }

    SoundCodex(const SoundCodex&) = delete;
    SoundCodex& operator=(const SoundCodex&) = delete;

    // Explicit initialization / completion
    bool Init(HWND hWnd);
    void UnInit();

    // Per-frame call: Recycle the played SE voice
    void Update();

    // BUS volume
    void SetMasterVolume(float vol); // 0..1
    void SetBgmVolume(float vol);    // 0..1
    void SetSeVolume(float vol);     // 0..1

    // BGM
    bool PlayBGM(std::string path, int loopCount = -1);
    void StopBGM();

    // SE(2D)
    SeHandle PlaySE(const std::string& path, int loopCount = 0, float volume = 1.0f);
    void StopAllSE();
    // SE(3D)
    SeHandle PlaySE3D(const std::string& path, int loopCount, float volume,
        float emitterX, float emitterY, float emitterZ,
        bool enableDistanceAtten = true, float minDistance = 1.0f, float maxDistance = 50.0f,
        float rolloff = 1.0f);
    void UpdateSE3D(SeHandle h, float emitterX, float emitterY, float emitterZ);
    // listener
    void SetListenerPosition(float x, float y, float z);
    void SetListenerPosition(DirectX::XMFLOAT3 pos) { SetListenerPosition(pos.x, pos.y, pos.z); }
    void SetListenerTransform(float px, float py, float pz,
        float fx, float fy, float fz,
        float ux, float uy, float uz);

    // playback state
    bool IsSePlaying(SeHandle h) const;

    // imgui window
    void SpawnWindow() noexcept;

private:
    SoundCodex() = default;
    ~SoundCodex() = default;

private:
    struct AudioClip
    {
        WAVEFORMATEX fmt{};
        std::vector<BYTE> data;
    };

    struct SeVoiceSlot
    {
        IXAudio2SourceVoice* voice = nullptr;
        WAVEFORMATEX fmt{}; // format in which the voice was created
        bool inUse = false;

        float volume = 1.0f;
        std::shared_ptr<AudioClip> clip; // keep only during playback

        uint64_t lastUseTick = 0;

        // handle
        SeHandle handle = 0;

        // 3D state
        bool is3D = false;
        bool enableDistanceAtten = true;
        float minDistance = 1.0f;
        float maxDistance = 50.0f;
        float rolloff = 1.0f;

        UINT32 srcChannels = 1;
        X3DAUDIO_EMITTER emitter{};
        std::vector<float> matrix;
    };

private:
    // get cache (if not, load and store)
    std::shared_ptr<AudioClip> GetOrLoadClip_(const std::string path);
    // wav import
    std::shared_ptr<AudioClip> LoadWavClip_(const std::string path);

    // voice create / destroy
    IXAudio2SourceVoice* CreateSourceVoice_(const WAVEFORMATEX& fmt, IXAudio2Voice* dstBus);
    void DestroySourceVoice_(IXAudio2SourceVoice*& v);
    
    // slot acquire (free / add / replace)
    SeVoiceSlot* AcquireSeSlot_(const WAVEFORMATEX& fmt);
    
    // wav Chunk loading
    HRESULT CheckChunk_(HANDLE hFile, DWORD format, DWORD* pChunkSize, DWORD* pChunkDataPosition);
    HRESULT ReadChunkData_(HANDLE hFile, void* pBuffer, DWORD dwBuffersize, DWORD dwBufferoffset);
    
    // safer format compare than memcmp (WAVEFORMATEX)
    static bool FormatEquals_(const WAVEFORMATEX& a, const WAVEFORMATEX& b);

    // 3D apply
    void Apply3D_(SeVoiceSlot& slot);
    static float ComputeDistanceAtten_(float d, float minD, float maxD, float rolloff);

private:
    // XAudio2 core
    Microsoft::WRL::ComPtr<IXAudio2> XAudio2;
    IXAudio2MasteringVoice* masterVoice = nullptr;

    //BUS (SubmixVoice)
    IXAudio2SubmixVoice* bgmBus = nullptr;
    IXAudio2SubmixVoice* seBus  = nullptr;

    // bus volumes
    float masterVolume = 1.0f;
    float bgmBusVolume = 1.0f;
    float seBusVolume  = 1.0f;

    // cache => key = path string
    std::unordered_map<std::string, std::shared_ptr<AudioClip>> clipCache;

    // BGM
    IXAudio2SourceVoice* bgmVoice = nullptr;
    std::shared_ptr<AudioClip> bgmClip;
    float bgmVolume = 1.0f;

    // SE pool
    std::vector<SeVoiceSlot> sePool;
    // SE slot limit + tick counter
    static constexpr size_t kMaxSeVoices = 64;
    uint64_t tick_ = 1;

    // handle generator
    SeHandle nextHandle_ = 1;

    // X3DAudio
    X3DAUDIO_HANDLE x3d_{};
    X3DAUDIO_LISTENER listener_{};
    DWORD channelMask_ = 0;
    UINT32 dstChannels_ = 2;

	// initialization flag
    bool inited = false;
};