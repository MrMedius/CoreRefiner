#pragma once

class Window;

/**
 * @brief 把 Codex 里的全屏开关落到窗口，再以窗口实际状态写回 Codex。
 */
void ApplyFullscreen(Window& wnd, bool enable);

/**
 * @brief 把窗口档写入 Codex，并按档位设置窗口客户区尺寸。
 * @param index 0=1280x720，1=1600x900，2=1920x1080。
 */
void ApplyWindowSizeIndex(Window& wnd, int index);
