#include "SoundCodex.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include "Util.h"
#include "Math.h"
#include "imgui/imgui.h"
#include "XMath.h"

bool SoundCodex::Init(HWND hWnd)
{
    if (inited) return true;

    // XAudio2 init
    HRESULT hr;
    {
        hr = XAudio2Create(XAudio2.GetAddressOf(), 0);
        if (FAILED(hr))
        {
            MessageBox(hWnd, "Failed to create XAudio2 object!", "WARNING�I", MB_ICONWARNING);
            return false;
        }

        hr = XAudio2->CreateMasteringVoice(&masterVoice);
        if (FAILED(hr))
        {
            MessageBox(hWnd, "Master voice generation failed!", "WARNING�I", MB_ICONWARNING);
            XAudio2.Reset();
            return false;
        }
    }

    // Submix(Bus) init
    XAUDIO2_VOICE_DETAILS mv{};
    {
        masterVoice->GetVoiceDetails(&mv);

        hr = XAudio2->CreateSubmixVoice(&bgmBus, mv.InputChannels, mv.InputSampleRate);
        if (FAILED(hr))
        {
            MessageBoxA(hWnd, "Failed to create BGM bus (SubmixVoice)!", "WARNING", MB_ICONWARNING);
            masterVoice->DestroyVoice();
            masterVoice = nullptr;
            XAudio2.Reset();
            return false;
        }
        hr = XAudio2->CreateSubmixVoice(&seBus, mv.InputChannels, mv.InputSampleRate);
        if (FAILED(hr))
        {
            MessageBoxA(hWnd, "Failed to create SE bus (SubmixVoice)!", "WARNING", MB_ICONWARNING);
            if (bgmBus) { bgmBus->DestroyVoice(); bgmBus = nullptr; }
            masterVoice->DestroyVoice();
            masterVoice = nullptr;
            XAudio2.Reset();
            return false;
        }

        masterVoice->SetVolume(std::clamp(masterVolume, 0.0f, 1.0f));
        bgmBus->SetVolume(std::clamp(bgmBusVolume, 0.0f, 1.0f));
        seBus->SetVolume(std::clamp(seBusVolume, 0.0f, 1.0f));
    }

    // X3DAudio init
    {
        dstChannels_ = mv.InputChannels;

        HRESULT hrMask = masterVoice->GetChannelMask(&channelMask_);
        if (FAILED(hrMask) || channelMask_ == 0)
        {
            // fallback
            channelMask_ = SPEAKER_STEREO;
        }

        X3DAudioInitialize(channelMask_, X3DAUDIO_SPEED_OF_SOUND, x3d_);

        // listener defaults
        listener_.Position = { 0,0,0 };
        listener_.OrientFront = { 0,0,1 };
        listener_.OrientTop = { 0,1,0 };
        listener_.Velocity = { 0,0,0 };
    }

    inited = true;
    return true;
}

void SoundCodex::UnInit()
{
    if (!inited) return;

    StopBGM();
    StopAllSE();

    // SE voice discard
    for (auto& s : sePool)
    {
        DestroySourceVoice_(s.voice);
        s.inUse = false;
        s.clip.reset();
        std::memset(&s.fmt, 0, sizeof(WAVEFORMATEX));
        s.matrix.clear();
    }
    sePool.clear();
    // discard the cache (AudioClip is a shared_ptr, so it is released here)
    clipCache.clear();

    // Bus discard
    if (seBus)  { seBus->DestroyVoice();  seBus = nullptr; }
    if (bgmBus) { bgmBus->DestroyVoice(); bgmBus = nullptr; }


    if (masterVoice)
    {
        masterVoice->DestroyVoice();
        masterVoice = nullptr;
    }
    XAudio2.Reset();

    inited = false;
}

void SoundCodex::Update()
{
    if (!inited) return;

    // return the SE slot to "empty" after playback is complete
    for (auto& s : sePool)
    {
        if (!s.voice || !s.inUse) continue;

        // 3D update first (so moving sounds spatialize while playing)
        if (s.is3D) Apply3D_(s);

        XAUDIO2_VOICE_STATE st{};
        s.voice->GetState(&st);

        if (st.BuffersQueued == 0)
        {
            // end of playback
            s.inUse = false;
            s.clip.reset();
            s.is3D = false;
            s.handle = 0;
            // just to be safe
            s.voice->FlushSourceBuffers();
        }
    }
}


// ------------------------------------------
// BUS volume
void SoundCodex::SetMasterVolume(float vol)
{
    masterVolume = std::clamp(vol, 0.0f, 1.0f);
    if (masterVoice) masterVoice->SetVolume(masterVolume);
}

void SoundCodex::SetBgmVolume(float vol)
{
    bgmBusVolume = std::clamp(vol, 0.0f, 1.0f);
    if (bgmBus) bgmBus->SetVolume(bgmBusVolume);
}

void SoundCodex::SetSeVolume(float vol)
{
    seBusVolume = std::clamp(vol, 0.0f, 1.0f);
    if (seBus) seBus->SetVolume(seBusVolume);
}


// ------------------------------------------
// BGM
bool SoundCodex::PlayBGM(const std::string path, int loopCount)
{
    if (!inited) return false;

    auto clip = GetOrLoadClip_(path);
    if (!clip) return false;

    // stop the existing BGM and replace it
    StopBGM();

    // set BGM voice output destination to bgmBus
    bgmVoice = CreateSourceVoice_(clip->fmt, bgmBus);
    if (!bgmVoice) return false;

    bgmClip = clip;

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = static_cast<UINT32>(clip->data.size());
    buf.pAudioData = clip->data.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;
    if (loopCount < 0) loopCount = XAUDIO2_LOOP_INFINITE;
    buf.LoopCount = loopCount;

    bgmVoice->SubmitSourceBuffer(&buf);
    bgmVoice->SetVolume(1.0f); // bus / master handle overall volume
    bgmVoice->Start(0);

    return true;
}

void SoundCodex::StopBGM()
{
    if (bgmVoice)
    {
        bgmVoice->Stop(0);
        bgmVoice->FlushSourceBuffers();
        bgmVoice->DestroyVoice();
        bgmVoice = nullptr;
    }
    bgmClip.reset();
}


// ------------------------------------------
// SE(2D)
SoundCodex::SeHandle SoundCodex::PlaySE(const std::string& path, int loopCount, float volume)
{
    if (!inited) return 0;

    auto clip = GetOrLoadClip_(path);
    if (!clip) return 0;

    volume = clamp01(volume);

    SeVoiceSlot* slot = AcquireSeSlot_(clip->fmt);
    if (!slot || !slot->voice) return 0;

    // safety precautions when reusing
    slot->voice->Stop(0);
    slot->voice->FlushSourceBuffers();

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = static_cast<UINT32>(clip->data.size());
    buf.pAudioData = clip->data.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;
    if (loopCount < 0) loopCount = XAUDIO2_LOOP_INFINITE;
    buf.LoopCount = loopCount;

    slot->clip = clip;
    slot->inUse = true;
    slot->volume = volume;
    slot->lastUseTick = tick_++;
    slot->handle = nextHandle_++;
    slot->is3D = false;

    // reset 3D output matrix->make sure this voice plays as normal 2D
    XAUDIO2_VOICE_DETAILS vd{};
    slot->voice->GetVoiceDetails(&vd);
    const UINT32 srcCh = vd.InputChannels;
    XAUDIO2_VOICE_DETAILS busd{};
    seBus->GetVoiceDetails(&busd);
    const UINT32 dstCh = busd.InputChannels;
    // Neutral matrix: distribute each input channel evenly to all output channels
    std::vector<float> m(static_cast<size_t>(srcCh) * dstCh, 1.0f / float(dstCh));
    slot->voice->SetOutputMatrix(seBus, srcCh, dstCh, m.data());
    

    slot->voice->SubmitSourceBuffer(&buf);
    // leave the individual SE volume to voice and the overall volume to seBus
    slot->voice->SetVolume(volume);
    slot->voice->Start(0);

    return slot->handle;
}

void SoundCodex::StopAllSE()
{
    for (auto& s : sePool)
    {
        if (s.voice)
        {
            s.voice->Stop(0);
            s.voice->FlushSourceBuffers();
        }
        s.inUse = false;
        s.clip.reset();
        s.is3D = false;
        s.handle = 0;
    }
}


// ------------------------------------------
// SE(3D)
SoundCodex::SeHandle SoundCodex::PlaySE3D(const std::string& path, int loopCount, float volume,
    float emitterX, float emitterY, float emitterZ,
    bool enableDistanceAtten, float minDistance, float maxDistance,
    float rolloff)
{
    if (!inited) return 0;

    auto clip = GetOrLoadClip_(path);
    if (!clip) return 0;

    volume = clamp01(volume);

    SeVoiceSlot* slot = AcquireSeSlot_(clip->fmt);
    if (!slot || !slot->voice) return 0;

    slot->voice->Stop(0);
    slot->voice->FlushSourceBuffers();

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = static_cast<UINT32>(clip->data.size());
    buf.pAudioData = clip->data.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;
    if (loopCount < 0) loopCount = XAUDIO2_LOOP_INFINITE;
    buf.LoopCount = loopCount;

    slot->clip = clip;
    slot->inUse = true;
    slot->volume = volume;
    slot->lastUseTick = tick_++;
    slot->handle = nextHandle_++;

    // 3D params
    slot->is3D = true;
    slot->enableDistanceAtten = enableDistanceAtten;
    slot->minDistance = max(0.001f, minDistance);
    slot->maxDistance = max(slot->minDistance, maxDistance);
    slot->rolloff = max(0.0f, rolloff);

    // emitter init
    slot->emitter = {};
    slot->emitter.OrientFront = { 0,0,1 };
    slot->emitter.OrientTop = { 0,1,0 };
    slot->emitter.Position = { emitterX, emitterY, emitterZ };
    slot->emitter.Velocity = { 0,0,0 };
    slot->emitter.ChannelCount = slot->srcChannels;
    slot->emitter.CurveDistanceScaler = 1.0f;
    slot->emitter.DopplerScaler = 1.0f;

    slot->voice->SubmitSourceBuffer(&buf);

    Apply3D_(*slot);
    slot->voice->Start(0);
    return slot->handle;
}

void SoundCodex::UpdateSE3D(SeHandle h, float emitterX, float emitterY, float emitterZ)
{
    if (!inited || h == 0) return;

    for (auto& s : sePool)
    {
        if (!s.inUse || !s.is3D || s.handle != h) continue;
        s.emitter.Position = { emitterX, emitterY, emitterZ };
        // basically leave the work to Update()
        Apply3D_(s);
        return;
    }
}


// ----------------------------------------
// listener
void SoundCodex::SetListenerPosition(float x, float y, float z)
{
    listener_.Position = { x,y,z };
}

void SoundCodex::SetListenerTransform(float px, float py, float pz,
    float fx, float fy, float fz,
    float ux, float uy, float uz)
{
    listener_.Position = { px,py,pz };
    listener_.OrientFront = { fx,fy,fz };
    listener_.OrientTop = { ux,uy,uz };
}


// ----------------------------------------
// state query
bool SoundCodex::IsSePlaying(SeHandle h) const
{
    if (!inited || h == 0) return false;

    for (const auto& s : sePool)
    {
        if (!s.inUse || !s.voice) continue;
        if (s.handle != h) continue;

        XAUDIO2_VOICE_STATE st{};
        s.voice->GetState(&st);
        return (st.BuffersQueued != 0);
    }
    return false;
}


// ------------------------------------------
// cache / load
std::shared_ptr<SoundCodex::AudioClip> SoundCodex::GetOrLoadClip_(std::string path)
{
    if (path.empty()) return nullptr;

    auto it = clipCache.find(path);
    if (it != clipCache.end())
        return it->second;

    auto clip = LoadWavClip_(path);
    if (!clip) return nullptr;

    clipCache.emplace(path, clip);
    return clip;
}

std::shared_ptr<SoundCodex::AudioClip> SoundCodex::LoadWavClip_(std::string path)
{
    DWORD dwChunkSize = 0;
    DWORD dwChunkPosition = 0;
    DWORD dwFiletype = 0;
    WAVEFORMATEXTENSIBLE wfx{};
    DWORD audioSize = 0;

    std::wstring wpath = ToWideUtf8(path);
    HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return nullptr;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        return nullptr;
    }

    HRESULT hr = S_OK;

    // RIFF
    hr = CheckChunk_(hFile, 'FFIR', &dwChunkSize, &dwChunkPosition);
    if (FAILED(hr))
    {
        CloseHandle(hFile);
        return nullptr;
    }
    hr = ReadChunkData_(hFile, &dwFiletype, sizeof(DWORD), dwChunkPosition);
    if (FAILED(hr) || dwFiletype != 'EVAW')
    {
        CloseHandle(hFile);
        return nullptr;
    }

    // fmt
    hr = CheckChunk_(hFile, ' tmf', &dwChunkSize, &dwChunkPosition);
    if (FAILED(hr))
    {
        CloseHandle(hFile);
        return nullptr;
    }
    hr = ReadChunkData_(hFile, &wfx, dwChunkSize, dwChunkPosition);
    if (FAILED(hr))
    {
        CloseHandle(hFile);
        return nullptr;
    }

    // data
    hr = CheckChunk_(hFile, 'atad', &audioSize, &dwChunkPosition);
    if (FAILED(hr) || audioSize == 0)
    {
        CloseHandle(hFile);
        return nullptr;
    }

    auto clip = std::make_shared<AudioClip>();
    clip->fmt = wfx.Format;
    clip->data.resize(audioSize);

    hr = ReadChunkData_(hFile, clip->data.data(), audioSize, dwChunkPosition);
    CloseHandle(hFile);

    if (FAILED(hr))
        return nullptr;

    return clip;
}


// ----------------------------------------
// voice create / destroy
IXAudio2SourceVoice* SoundCodex::CreateSourceVoice_(const WAVEFORMATEX& fmt, IXAudio2Voice* dstBus)
{
    if (!XAudio2 || !dstBus) return nullptr;

    IXAudio2SourceVoice* v = nullptr;

    XAUDIO2_SEND_DESCRIPTOR sendDesc{};
    sendDesc.Flags = 0;
    sendDesc.pOutputVoice = dstBus;

    XAUDIO2_VOICE_SENDS sends{};
    sends.SendCount = 1;
    sends.pSends = &sendDesc;

    HRESULT hr = XAudio2->CreateSourceVoice(
        &v,
        &fmt,
        0,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        nullptr,
        &sends,
        nullptr
    );

    if (FAILED(hr) || !v) return nullptr;
    return v;
}

void SoundCodex::DestroySourceVoice_(IXAudio2SourceVoice*& v)
{
    if (!v) return;
    v->Stop(0);
    v->FlushSourceBuffers();
    v->DestroyVoice();
    v = nullptr;
}

// ------------------------------------------
    // slot acquire (free / add / replace)
SoundCodex::SeVoiceSlot* SoundCodex::AcquireSeSlot_(const WAVEFORMATEX& fmt)
{
    // 1) free slot with same fmt
    for (auto& s : sePool)
    {
        if (!s.inUse && s.voice && FormatEquals_(s.fmt, fmt))
            return &s;
    }

    // 2) check if can add new
    if (sePool.size() < kMaxSeVoices)
    {
        SeVoiceSlot slot{};
        slot.voice = CreateSourceVoice_(fmt, seBus);
        if (!slot.voice) return nullptr;

        slot.fmt = fmt;

        XAUDIO2_VOICE_DETAILS vd{};
        slot.voice->GetVoiceDetails(&vd);
        slot.srcChannels = vd.InputChannels;

        sePool.push_back(std::move(slot));
        return &sePool.back();
    }

    // 3) replace oldest same fmt
    SeVoiceSlot* bestSame = nullptr;
    for (auto& s : sePool)
    {
        if (!s.voice) continue;
        if (!FormatEquals_(s.fmt, fmt)) continue;
        if (!bestSame || s.lastUseTick < bestSame->lastUseTick)
            bestSame = &s;
    }

    if (bestSame)
    {
        bestSame->voice->Stop(0);
        bestSame->voice->FlushSourceBuffers();
        bestSame->inUse = false;
        bestSame->clip.reset();
        bestSame->is3D = false;
        bestSame->handle = 0;
        return bestSame;
    }

    // 4) replace oldest any (recreate voice)
    SeVoiceSlot* bestAny = &sePool[0];
    for (auto& s : sePool)
    {
        if (s.lastUseTick < bestAny->lastUseTick)
            bestAny = &s;
    }

    if (bestAny->voice)
        DestroySourceVoice_(bestAny->voice);

    bestAny->voice = CreateSourceVoice_(fmt, seBus);
    if (!bestAny->voice) return nullptr;

    bestAny->fmt = fmt;

    XAUDIO2_VOICE_DETAILS vd{};
    bestAny->voice->GetVoiceDetails(&vd);
    bestAny->srcChannels = vd.InputChannels;

    bestAny->inUse = false;
    bestAny->clip.reset();
    bestAny->is3D = false;
    bestAny->handle = 0;
    bestAny->matrix.clear();

    return bestAny;
}


// ------------------------------------------
// 3D apply
void SoundCodex::Apply3D_(SeVoiceSlot& slot)
{
    if (!slot.voice || !slot.is3D || !seBus) return;

    // stereo input azimuths
    static float s_azimuthsStereo[2] = { -X3DAUDIO_PI / 2.0f, X3DAUDIO_PI / 2.0f };
    if (slot.srcChannels == 2)
        slot.emitter.pChannelAzimuths = s_azimuthsStereo;
    else
        slot.emitter.pChannelAzimuths = nullptr;

    slot.matrix.resize(static_cast<size_t>(slot.srcChannels) * dstChannels_);

    X3DAUDIO_DSP_SETTINGS dsp{};
    dsp.SrcChannelCount = slot.srcChannels;
    dsp.DstChannelCount = dstChannels_;
    dsp.pMatrixCoefficients = slot.matrix.data();

    X3DAudioCalculate(
        x3d_,
        &listener_,
        &slot.emitter,
        X3DAUDIO_CALCULATE_MATRIX,
        &dsp
    );

    // apply matrix to the send to seBus
    slot.voice->SetOutputMatrix(
        seBus,
        slot.srcChannels,
        dstChannels_,
        slot.matrix.data()
    );

    // distance attenuation (simple & practical)
    float atten = 1.0f;
    if (slot.enableDistanceAtten)
    {
        const Vec3 emitterPos{
            slot.emitter.Position.x,
            slot.emitter.Position.y,
            slot.emitter.Position.z
        };
        const Vec3 listenerPos{
            listener_.Position.x,
            listener_.Position.y,
            listener_.Position.z
        };
        const float d = (emitterPos - listenerPos).Length();
        atten = ComputeDistanceAtten_(d, slot.minDistance, slot.maxDistance, slot.rolloff);
    }

    slot.voice->SetVolume(clamp01(slot.volume * atten));
}

float SoundCodex::ComputeDistanceAtten_(float d, float minD, float maxD, float rolloff)
{
    // clamp range
    if (d <= minD) return 1.0f;
    if (d >= maxD) return 0.0f;

    // very simple rolloff curve:
    // t in [0,1], atten = (1 - t)^(rolloff)
    const float t = (d - minD) / (maxD - minD);
    const float p = max(0.001f, rolloff);
    return std::pow(1.0f - t, p);
}


// ------------------------------------------
// format compare
bool SoundCodex::FormatEquals_(const WAVEFORMATEX& a, const WAVEFORMATEX& b)
{
    return a.wFormatTag      == b.wFormatTag &&
           a.nChannels       == b.nChannels &&
           a.nSamplesPerSec  == b.nSamplesPerSec &&
           a.wBitsPerSample  == b.wBitsPerSample &&
           a.nBlockAlign     == b.nBlockAlign &&
           a.nAvgBytesPerSec == b.nAvgBytesPerSec &&
           a.cbSize          == b.cbSize;
}

// ------------------------------------------
// WAV Chunk loading
HRESULT SoundCodex::CheckChunk_(HANDLE hFile, DWORD format, DWORD* pChunkSize, DWORD* pChunkDataPosition)
{
    HRESULT hr = S_OK;
    DWORD dwRead = 0;
    DWORD dwChunkType = 0;
    DWORD dwChunkDataSize = 0;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType = 0;
    DWORD dwBytesRead = 0;
    DWORD dwOffset = 0;

    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        return HRESULT_FROM_WIN32(GetLastError());

    while (hr == S_OK)
    {
        if (ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL) == 0)
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL) == 0)
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case 'FFIR':
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL) == 0)
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT) == INVALID_SET_FILE_POINTER)
                return HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        dwOffset += sizeof(DWORD) * 2;
        if (dwChunkType == format)
        {
            *pChunkSize = dwChunkDataSize;
            *pChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;
        dwBytesRead = dwOffset;
        if (dwBytesRead >= dwRIFFDataSize)
            return S_FALSE;
    }

    return hr;
}

HRESULT SoundCodex::ReadChunkData_(HANDLE hFile, void* pBuffer, DWORD dwBuffersize, DWORD dwBufferoffset)
{
    DWORD dwRead = 0;

    if (SetFilePointer(hFile, dwBufferoffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        return HRESULT_FROM_WIN32(GetLastError());

    if (ReadFile(hFile, pBuffer, dwBuffersize, &dwRead, NULL) == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}


// imgui window
void SoundCodex::SpawnWindow() noexcept
{
    ImGui::Begin("Audio");

    float master = masterVolume;
    float bgm = bgmBusVolume;
    float se = seBusVolume;

    bool changed = false;

    changed |= ImGui::SliderFloat("Master", &master, 0.0f, 1.0f, "%.2f");
    changed |= ImGui::SliderFloat("BGM Bus", &bgm, 0.0f, 1.0f, "%.2f");
    changed |= ImGui::SliderFloat("SE Bus", &se, 0.0f, 1.0f, "%.2f");

    if (changed)
    {
        auto& snd = SoundCodex::Get();
        snd.SetMasterVolume(master);
        snd.SetBgmVolume(bgm);
        snd.SetSeVolume(se);
    }

    if (ImGui::Button("Mute All"))
    {
        master = 0.0f;
        bgm = 0.0f;
        se = 0.0f;

        auto& snd = SoundCodex::Get();
        snd.SetMasterVolume(master);
        snd.SetBgmVolume(bgm);
        snd.SetSeVolume(se);
    }

    ImGui::End();
}