#pragma once

// 读写 Saves/settings.json，字段写入 GameStatsCodex。
// 路径回退：Saves/、CoreRefiner/Saves/、CoreRefiner/CoreRefiner/Saves/
bool LoadSettings();

// 把 Codex 当前设置写回 Saves/settings.json；没有文件夹会创建
bool SaveSettings();
