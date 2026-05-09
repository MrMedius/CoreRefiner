#pragma once
#include "Win.h"
#include "WRL.h"

#include <dwrite.h>
#include <string>
#include <vector>

namespace Text
{
    struct FontCollectionKey
    {
        std::vector<std::wstring> filePaths;
    };

    std::vector<uint8_t> BuildFontCollectionKeyBytes(const FontCollectionKey& key);

    class FontFileEnumerator final : public IDWriteFontFileEnumerator
    {
    public:
        FontFileEnumerator(IDWriteFactory* factory, std::vector<std::wstring> files);

        // IUnknown
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
        ULONG __stdcall AddRef() override;
        ULONG __stdcall Release() override;

        // IDWriteFontFileEnumerator
        HRESULT __stdcall MoveNext(BOOL* hasCurrentFile) override;
        HRESULT __stdcall GetCurrentFontFile(IDWriteFontFile** fontFile) override;

    private:
        ~FontFileEnumerator() = default;

    private:
        ULONG ref_ = 1;
        Microsoft::WRL::ComPtr<IDWriteFactory> factory_;
        std::vector<std::wstring> files_;
        size_t index_ = 0;
        Microsoft::WRL::ComPtr<IDWriteFontFile> current_;
    };

    class FontCollectionLoader final : public IDWriteFontCollectionLoader
    {
    public:
        FontCollectionLoader() = default;

        // IUnknown
        HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
        ULONG __stdcall AddRef() override;
        ULONG __stdcall Release() override;

        // IDWriteFontCollectionLoader
        HRESULT __stdcall CreateEnumeratorFromKey(
            IDWriteFactory* factory,
            const void* collectionKey,
            UINT32 collectionKeySize,
            IDWriteFontFileEnumerator** fontFileEnumerator) override;

    private:
        ~FontCollectionLoader() = default;

    private:
        ULONG ref_ = 1;
    };
}