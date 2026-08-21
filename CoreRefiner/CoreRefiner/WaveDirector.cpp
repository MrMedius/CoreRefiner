#include "WaveDirector.h"

void WaveDirector::Reset() noexcept
{
	phase_ = GamePhase::Prep;
	waveIndex_ = 0;
	remainSec_ = 0.0f;
	currentSpec_ = {};
}

void WaveDirector::Update(float dt)
{
	if (phase_ != GamePhase::Combat || dt <= 0.0f)
	{
		return;
	}

	remainSec_ -= dt;
	if (remainSec_ <= 0.0f)
	{
		remainSec_ = 0.0f;
		EndWave_();
	}
}

int WaveDirector::GetNextWaveIndex() const noexcept
{
	if (waveIndex_ >= WaveRules::kTotalWaves)
	{
		return WaveRules::kTotalWaves;
	}
	return waveIndex_ + 1;
}

void WaveDirector::RequestStartWave()
{
	if (phase_ != GamePhase::Prep)
	{
		return;
	}

	const int next = waveIndex_ + 1;
	if (next > WaveRules::kTotalWaves)
	{
		phase_ = GamePhase::Victory;
		return;
	}

	StartWave_(next);
}

void WaveDirector::NotifyPlayerDead()
{
	if (phase_ == GamePhase::Defeat || phase_ == GamePhase::Victory)
	{
		return;
	}
	phase_ = GamePhase::Defeat;
	remainSec_ = 0.0f;
}

void WaveDirector::DebugSkipPhase()
{
	switch (phase_)
	{
	case GamePhase::Combat:
		remainSec_ = 0.0f;
		EndWave_();
		break;
	case GamePhase::Prep:
		RequestStartWave();
		break;
	default:
		break;
	}
}

void WaveDirector::StartWave_(int wave)
{
	waveIndex_ = wave;
	currentSpec_ = WaveRules::GetSpec(wave);
	remainSec_ = currentSpec_.duration;
	phase_ = GamePhase::Combat;
	if (onWaveStart)
	{
		onWaveStart(currentSpec_);
	}
}

void WaveDirector::EndWave_()
{
	const int ended = waveIndex_;
	if (ended >= WaveRules::kTotalWaves)
	{
		phase_ = GamePhase::Victory;
	}
	else
	{
		phase_ = GamePhase::Prep;
	}
	remainSec_ = 0.0f;
	if (onWaveEnd)
	{
		onWaveEnd(ended);
	}
}
