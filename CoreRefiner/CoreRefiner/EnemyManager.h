#pragma once
#include "Graphics.h"
#include "RenderGraph.h"
#include "Player.h"
#include "Enemy.h"
#include "WaveRules.h"

#include <random>
#include <vector>

class EnemyManager
{
public:
	EnemyManager(Graphics& gfx, Rgph::RenderGraph& rg);
	~EnemyManager() = default;

	void Update(float dt);
	void Submit(void);
	void Reset(void);

	// 应用本波规则：清场、按预算扩池、按 spawnInterval 开始生成。
	void SetWave(const WaveSpec& spec);
	// 停用场上敌人并清空跟踪表；不改变当前 WaveSpec。
	void ClearAll();
	// 停止本波继续刷怪（波末收尾用）。
	void HaltSpawning() noexcept;

private:
	void EnsurePool_(int count);
	void TrySpawnOne_();
	[[nodiscard]] DirectX::XMFLOAT3 PickSpawnPos_() noexcept;

	Graphics& gfx;
	Rgph::RenderGraph& rg;

	std::vector<Enemy*> enemies;
	std::mt19937 rng_;

	WaveSpec spec_{};
	bool waveActive_{ false };
	int spawnedThisWave_{ 0 };
	float Interval{ 1.5f };
	float CountDown{ 0.0f };
	bool NewEnemy{ false };

	Player* pPlayer;

	static constexpr int kInitialPool_ = 10;
	static constexpr float kSpawnMinRadius_ = 20.0f;
	static constexpr float kSpawnMaxRadius_ = 40.0f;
	static constexpr float kSpawnHeight_ = 10.0f;
	static constexpr float kFieldHalf_ = 46.0f;
};
