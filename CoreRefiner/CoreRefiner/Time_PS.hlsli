// 像素着色器的时间。TimeCbuf_PS 写入寄存器 b1。
cbuffer TimeCBuf : register(b1)
{
    // 缩放后的累计时间，单位秒。
    float totalTime;
    // 缩放后的帧间隔，单位秒。
    float deltaTime;
    // 未缩放的帧间隔，单位秒。
    float unscaledDeltaTime;
    // 时间缩放。1 是正常速度。
    float timeScale;
};
