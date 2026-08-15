#pragma once
#include <algorithm>
#include "imgui/imgui.h"

struct PerformanceData
{
    int score{ 0 };

    int totalDefeat{ 0 };
    int rDefeat{ 0 };
    int gDefeat{ 0 };
    int bDefeat{ 0 };

    int rHighestCombo{ 0 };
    int gHighestCombo{ 0 };
    int bHighestCombo{ 0 };

    int totalWeapon{ 0 };
    int rWeapon{ 0 };
    int gWeapon{ 0 };
    int bWeapon{ 0 };

    float lifeTime{ 0.0f };
    bool gameClear{ false };

    float outputDamage{ 0.0f };
    float inputDamage{ 0.0f };

    void Reset() noexcept
    {
        score = 0;

        totalDefeat = rDefeat = gDefeat = bDefeat = 0;

        rHighestCombo = gHighestCombo = bHighestCombo = 0;

        totalWeapon = rWeapon = gWeapon = bWeapon = 0;

        lifeTime = 0.0f;
        gameClear = false;

        outputDamage = 0.0f;
        inputDamage = 0.0f;
    }
};

struct TutorialData
{
    bool isTutorial{ true };

    bool isMoved{ false };
    bool isDashed{ false };
    bool isAttacked{ false };
    bool isSwitched{ false };
    bool isSkilled{ false };
    bool isFevered{ false };

    bool isLearnt{ false };
    bool isFinishPart{ false };

    void Reset() noexcept
    {
        isTutorial = true;

        isMoved = false;
        isDashed = false;
        isAttacked = false;
        isSwitched = false;
        isSkilled = false;
        isFevered = false;

        isLearnt = false;
        isFinishPart = false;
    }
};

struct CurrencyData
{
    int amount{ 0 };

    void Reset() noexcept
    {
        amount = 0;
    }
};


class GameStatsCodex
{
public:
    // read only
    static const PerformanceData& Get() noexcept { return Get_().pData; }

    // write
    static void Reset() noexcept 
    { 
        Get_().pData.Reset(); 
        Get_().tData.Reset();
        Get_().cData.Reset();
    }

    /////////////////////////////////////////////////////////
    // PerformanceData
    /////////////////////////////////////////////////////////
    static bool GetIsTutorial(void) noexcept     { return Get_().tData.isTutorial; }
    static void SetFinishTutorial(void) noexcept { Get_().tData.isTutorial = false; }

    static bool GetIsMoved(void) noexcept { return Get_().tData.isMoved; }
    static void SetMoved(void) noexcept   { Get_().tData.isMoved = true; }

    static bool GetIsDashed(void) noexcept { return Get_().tData.isDashed; }
    static void SetDashed(void) noexcept   { Get_().tData.isDashed = true; }

    static bool GetIsAttacked(void) noexcept { return Get_().tData.isAttacked; }
    static void SetAttacked(void) noexcept   { Get_().tData.isAttacked = true; }

    static bool GetIsSwitched(void) noexcept { return Get_().tData.isSwitched; }
    static void SetSwitched(void) noexcept   { Get_().tData.isSwitched = true; }

    static bool GetIsSkilled(void) noexcept { return Get_().tData.isSkilled; }
    static void SetSkilled(void) noexcept   { Get_().tData.isSkilled = true; }

    static bool GetIsFevered(void) noexcept { return Get_().tData.isFevered; }
    static void SetFevered(void) noexcept { Get_().tData.isFevered = true; }

    static bool GetIsLearnt(void) noexcept { return Get_().tData.isLearnt; }
    static void SetLearnt(void) noexcept { Get_().tData.isLearnt = true; }

    static bool GetIsFinishPart(void) noexcept { return Get_().tData.isFinishPart; }
    static void SetFinishPart(void) noexcept { Get_().tData.isFinishPart = true; }




    /////////////////////////////////////////////////////////
    // PerformanceData
    /////////////////////////////////////////////////////////
    static void UpdateLifeTime(float dt) noexcept
    {
        if (dt > 0.0f && !Get_().tData.isTutorial)
            Get_().pData.lifeTime += dt;
    }

    static void AddScore(int s) noexcept { if (!Get_().tData.isTutorial) Get_().pData.score += s; }
    static void SetGameClear() noexcept  { if (!Get_().tData.isTutorial) Get_().pData.gameClear = true; }

    static void AddTotalDefeat(int c = 1) noexcept { if (!Get_().tData.isTutorial) Get_().pData.totalDefeat += c; }
    static void AddRedDefeat(int c = 1) noexcept   { if (!Get_().tData.isTutorial) Get_().pData.rDefeat += c; }
    static void AddGreenDefeat(int c = 1) noexcept { if (!Get_().tData.isTutorial) Get_().pData.gDefeat += c; }
    static void AddBlueDefeat(int c = 1) noexcept  { if (!Get_().tData.isTutorial) Get_().pData.bDefeat += c; }

    static void ReportRedCombo(int v) noexcept
    {
        if (!Get_().tData.isTutorial) if (v >= 0) Get_().pData.rHighestCombo = std::max(Get_().pData.rHighestCombo, v);
    }
    static void ReportGreenCombo(int v) noexcept
    {
        if (!Get_().tData.isTutorial) if (v >= 0) Get_().pData.gHighestCombo = std::max(Get_().pData.gHighestCombo, v);
    }
    static void ReportBlueCombo(int v) noexcept
    {
        if (!Get_().tData.isTutorial) if (v >= 0) Get_().pData.bHighestCombo = std::max(Get_().pData.bHighestCombo, v);
    }

    static void AddTotalWeapon(int c = 1) noexcept  { if (!Get_().tData.isTutorial) Get_().pData.totalWeapon += c; }
    static void AddRedWeapon(int c = 1) noexcept    { if (!Get_().tData.isTutorial) Get_().pData.rWeapon += c; }
    static void AddGreenWeapon(int c = 1) noexcept  { if (!Get_().tData.isTutorial) Get_().pData.gWeapon += c; }
    static void AddBlueWeapon(int c = 1) noexcept   { if (!Get_().tData.isTutorial) Get_().pData.bWeapon += c; }

    static void AddOutputDamage(float d) noexcept { if (!Get_().tData.isTutorial) if (d > 0) Get_().pData.outputDamage += d; }
    static void AddInputDamage(float d) noexcept  { if (!Get_().tData.isTutorial) if (d > 0) Get_().pData.inputDamage += d; }

    /////////////////////////////////////////////////////////
    // CurrencyData (not gated by isTutorial)
    /////////////////////////////////////////////////////////
    [[nodiscard]] static int GetCurrency() noexcept
    {
        return Get_().cData.amount;
    }

    static void AddCurrency(int amount) noexcept
    {
        if (amount > 0)
        {
            Get_().cData.amount += amount;
        }
    }

    static bool TrySpendCurrency(int amount) noexcept
    {
        if (amount <= 0)
        {
            return false;
        }
        CurrencyData& wallet = Get_().cData;
        if (wallet.amount < amount)
        {
            return false;
        }
        wallet.amount -= amount;
        return true;
    }

    static void SpawnWindow()
    {
        const auto& s = GameStatsCodex::Get();

        ImGui::Begin("Game Stats", nullptr);

        if (ImGui::Button("Reset Stats"))
        {
            GameStatsCodex::Reset();
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Score & State", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Score: %d", s.score);
            ImGui::Text("Currency: %d", GameStatsCodex::GetCurrency());
            ImGui::Text("GameClear: %s", s.gameClear ? "true" : "false");
            ImGui::Text("LifeTime: %.2f sec", s.lifeTime);
        }

        if (ImGui::CollapsingHeader("Defeat", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Total: %d", s.totalDefeat);
            ImGui::BulletText("Red:   %d", s.rDefeat);
            ImGui::BulletText("Green: %d", s.gDefeat);
            ImGui::BulletText("Blue:  %d", s.bDefeat);
        }

        if (ImGui::CollapsingHeader("Highest Combo", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Red:   %d", s.rHighestCombo);
            ImGui::Text("Green: %d", s.gHighestCombo);
            ImGui::Text("Blue:  %d", s.bHighestCombo);
        }
            
        if (ImGui::CollapsingHeader("Weapon", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Total: %d", s.totalWeapon);
            ImGui::BulletText("Red:   %d", s.rWeapon);
            ImGui::BulletText("Green: %d", s.gWeapon);
            ImGui::BulletText("Blue:  %d", s.bWeapon);
        }

        if (ImGui::CollapsingHeader("Damage", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("OutputDamage: %.1f", s.outputDamage);
            ImGui::Text("InputDamage:  %.1f", s.inputDamage);

            float od = s.outputDamage;
            float id = s.inputDamage;

            const float scale = 1000.0f;
            ImGui::ProgressBar((od / scale) > 1.0f ? 1.0f : (od / scale), ImVec2(220, 0), "Output");
            ImGui::ProgressBar((id / scale) > 1.0f ? 1.0f : (id / scale), ImVec2(220, 0), "Input");
        }

        ImGui::End();
    }

private:
    static GameStatsCodex& Get_() noexcept
    {
        static GameStatsCodex inst;
        return inst;
    }

private:
    PerformanceData pData;
    TutorialData tData;
    CurrencyData cData;
};
