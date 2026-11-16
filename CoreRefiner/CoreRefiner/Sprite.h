#pragma once
#include "Drawable.h"
#include "Transformation.h"
#include <memory>

namespace dx = DirectX;

namespace Bind
{
	class DynamicTexture;
	class SpriteUVCbuf;
}

class Sprite : public Drawable
{
public:
    Sprite() = default;
    ~Sprite() = default;
    void Update(float dt);
    // transformation
    void SetPosition(float x, float y, float z) noexcept;
    void SetPosition(dx::XMFLOAT3 pos) noexcept { SetPosition(pos.x, pos.y, pos.z); }
    void SetPosition(float x, float y) noexcept { SetPosition(x, y, 0.0f); };
    void SetPosition(dx::XMFLOAT2 pos) noexcept { SetPosition(pos.x, pos.y); }
    void SetRotation(float x, float y, float z) noexcept;
    void SetRotation(dx::XMFLOAT3 rot) noexcept { SetRotation(rot.x, rot.y, rot.z); }
    void SetRotation(float degrees) noexcept    { SetRotation(0.0f, 0.0f, degrees); }
    void SetScale(float x, float y, float z) noexcept;
    void SetScale(dx::XMFLOAT3 size) noexcept   { SetScale(size.x, size.y, size.z); }
    void SetScale(float x, float y) noexcept    { SetScale(x, y, 0.0f); }
    void SetScale(dx::XMFLOAT2 size) noexcept   { SetScale(size.x, size.y); }
    dx::XMFLOAT3 GetPosition(void) noexcept { return trans.GetPosition(); }
    dx::XMFLOAT3 GetRotation(void) noexcept { return trans.GetRotation(); }
    dx::XMFLOAT3 GetScale(void) noexcept    { return trans.GetScale(); }
    dx::XMFLOAT3 GetBasePosition(void) noexcept { return transBase.position; }
    dx::XMFLOAT3 GetBaseRotation(void) noexcept { return transBase.rotation; }
    dx::XMFLOAT3 GetBaseScale(void) noexcept    { return transBase.scale; }
    // auto animation
    void SetFrameAuto(int numU_, int numV_, int startTex_, int totalTex_, int indexTex_, float fps, bool isFlip_ = false, bool isLoop_ = true);
    void SetAtlas(int numU_, int numV_, int startTex_, int totalTex_) noexcept;
    void SetFPS(float fps) noexcept;
    // manual animation
    void SetFrame(int numU_, int numV_, int startTex_, int totalTex_, int countTex_, int indexTex_, bool isFlip_ = false) noexcept;
    void SetRatioOffset(float ratioX, float ratioY) noexcept;
    // setters
    void SetFlip(bool isFlip_) noexcept { isFlip = isFlip_; }
    void SetLoop(bool isLoop_) noexcept { isLoop = isLoop_; }
    void SetTextureFrameIndex(int i) noexcept;
    // getters
    bool ClipFinished() const noexcept { return frame >= totalTex; }
    int GetCurrentFrame() const noexcept { return frame; }
    int GetTexIndex() const noexcept { return texIndex; }
protected:
    // 留住这一份精灵实际绑定的帧贴图和 UV 缓冲。dynTex 是帧贴图，uvcb 是 UV 缓冲。
    void AttachFrameBinds(std::shared_ptr<Bind::DynamicTexture> dynTex, std::shared_ptr<Bind::SpriteUVCbuf> uvcb) noexcept;
private:
    void UpdateUV(void) noexcept;
    // 把当前帧号和 UV 写进留住的缓冲。指针还没接上时跳过。
    void SyncFrameBinds_() noexcept;
private:
    // transformation
    Transformation trans;
    TransInfo transBase;
    dx::XMFLOAT2 uvOffset = { 0.0f,0.0f };
    dx::XMFLOAT2 uvScale = { 1.0f,1.0f };
    // frame info
    int numU = 1;
    int numV = 1;
    int startTex = 1;
    int totalTex = 1;
    // anime play
    int frame = 1;
    float frameTime = 0.1f;
    float accum = 0.0f;
    // anime state
    bool isFlip = false;
    bool isLoop = false;
    // texture num
    int texIndex = 0;
    // 这一份精灵正在使用的帧贴图。还没接上时为空。
    std::shared_ptr<Bind::DynamicTexture> dynTex_;
    // 这一份精灵正在使用的 UV 缓冲。还没接上时为空。
    std::shared_ptr<Bind::SpriteUVCbuf> uvcb_;
};