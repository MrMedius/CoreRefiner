#include "DWriteCustomFontCollection.h"

#include <stdexcept>
#include <cstring>

namespace Text
{
    // key bytes: [uint32 count][ (uint32 len)(wchar_t[len]) ... ]
    std::vector<uint8_t> BuildFontCollectionKeyBytes(const FontCollectionKey& key)
    {
        uint32_t count = static_cast<uint32_t>(key.filePaths.size());
        size_t total = sizeof(uint32_t);

        for (const auto& s : key.filePaths)
        {
            total += sizeof(uint32_t);
            total += sizeof(wchar_t) * s.size();
        }

        std::vector<uint8_t> bytes(total);
        uint8_t* p = bytes.data();

        std::memcpy(p, &count, sizeof(uint32_t));
        p += sizeof(uint32_t);

        for (const auto& s : key.filePaths)
        {
            uint32_t len = static_cast<uint32_t>(s.size());
            std::memcpy(p, &len, sizeof(uint32_t));
            p += sizeof(uint32_t);

            if (len > 0)
            {
                std::memcpy(p, s.data(), sizeof(wchar_t) * len);
                p += sizeof(wchar_t) * len;
            }
        }

        return bytes;
    }

    static std::vector<std::wstring> ParseFontCollectionKeyBytes(const void* key, UINT32 keySize)
    {
        if (!key || keySize < sizeof(uint32_t))
            throw std::runtime_error("FontCollection key invalid");

        const uint8_t* p = reinterpret_cast<const uint8_t*>(key);
        const uint8_t* end = p + keySize;

        uint32_t count = 0;
        std::memcpy(&count, p, sizeof(uint32_t));
        p += sizeof(uint32_t);

        std::vector<std::wstring> out;
        out.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            if (p + sizeof(uint32_t) > end)
                throw std::runtime_error("FontCollection key truncated");

            uint32_t len = 0;
            std::memcpy(&len, p, sizeof(uint32_t));
            p += sizeof(uint32_t);

            const size_t bytesLen = size_t(len) * sizeof(wchar_t);
            if (p + bytesLen > end)
                throw std::runtime_error("FontCollection key truncated (string)");

            std::wstring s;
            if (len > 0)
            {
                s.assign(reinterpret_cast<const wchar_t*>(p), reinterpret_cast<const wchar_t*>(p + bytesLen));
                p += bytesLen;
            }
            out.push_back(std::move(s));
        }

        return out;
    }

    FontFileEnumerator::FontFileEnumerator(IDWriteFactory* factory, std::vector<std::wstring> files)
        : factory_(factory), files_(std::move(files))
    {
    }

    HRESULT __stdcall FontFileEnumerator::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteFontFileEnumerator))
        {
            *ppvObject = static_cast<IDWriteFontFileEnumerator*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG __stdcall FontFileEnumerator::AddRef() { return ++ref_; }

    ULONG __stdcall FontFileEnumerator::Release()
    {
        const ULONG r = --ref_;
        if (r == 0) delete this;
        return r;
    }

    HRESULT __stdcall FontFileEnumerator::MoveNext(BOOL* hasCurrentFile)
    {
        if (!hasCurrentFile) return E_POINTER;

        current_.Reset();

        if (index_ >= files_.size())
        {
            *hasCurrentFile = FALSE;
            return S_OK;
        }

        HRESULT hr = factory_->CreateFontFileReference(files_[index_].c_str(), nullptr, &current_);
        if (FAILED(hr))
            return hr;

        ++index_;
        *hasCurrentFile = TRUE;
        return S_OK;
    }

    HRESULT __stdcall FontFileEnumerator::GetCurrentFontFile(IDWriteFontFile** fontFile)
    {
        if (!fontFile) return E_POINTER;
        if (!current_) return E_FAIL;

        *fontFile = current_.Get();
        (*fontFile)->AddRef();
        return S_OK;
    }

    HRESULT __stdcall FontCollectionLoader::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (!ppvObject) return E_POINTER;
        *ppvObject = nullptr;

        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteFontCollectionLoader))
        {
            *ppvObject = static_cast<IDWriteFontCollectionLoader*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG __stdcall FontCollectionLoader::AddRef() { return ++ref_; }

    ULONG __stdcall FontCollectionLoader::Release()
    {
        const ULONG r = --ref_;
        if (r == 0) delete this;
        return r;
    }

    HRESULT __stdcall FontCollectionLoader::CreateEnumeratorFromKey(
        IDWriteFactory* factory,
        const void* collectionKey,
        UINT32 collectionKeySize,
        IDWriteFontFileEnumerator** fontFileEnumerator)
    {
        if (!factory || !fontFileEnumerator) return E_INVALIDARG;
        *fontFileEnumerator = nullptr;

        try
        {
            auto files = ParseFontCollectionKeyBytes(collectionKey, collectionKeySize);
            *fontFileEnumerator = new FontFileEnumerator(factory, std::move(files));
            return S_OK;
        }
        catch (...)
        {
            return E_FAIL;
        }
    }
}