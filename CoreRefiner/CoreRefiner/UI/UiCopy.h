#pragma once
#include <filesystem>
#include <string>
#include <string_view>

// 从 JSON 加载界面词条。失败时表被清空
bool LoadUiCopy(const std::filesystem::path& path);

// 三路径回退加载 UiCopy.json。已加载则直接成功
bool LoadUiCopy();

[[nodiscard]] bool IsUiCopyLoaded() noexcept;

// 按 GameStatsCodex::GetLanguage() 取词条；缺 key 时回退 En、Zh，再没有则返回 key 本身
[[nodiscard]] std::string GetUiCopy(std::string_view key);

// 将词条中第一个 {0} 替换为 arg0
[[nodiscard]] std::string GetUiCopy(std::string_view key, std::string_view arg0);

[[nodiscard]] std::string GetUiCopy(std::string_view key, int arg0);
