#pragma once
#include <algorithm>
#include "imgui/imgui.h"
#include "JsonTextCopy.h"

struct GlobalData
{
    Language language{ Language::Zh };

    // ALL Volume 0–1
    float masterVolume{ 0.5f };
    // BGM Volume 0–1
    float bgmVolume{ 0.5f };
    // SE Volume 0–1
    float seVolume{ 0.5f };
    bool muted{ false };
    bool fullscreen{ false };
    // 0=1280x720，1=1600x900，2=1920x1080
    int windowSizeIndex{ 0 };
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

// InGame
namespace
{
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

    struct CurrencyData
    {
        int amount{ 0 };

        void Reset() noexcept
        {
            amount = 0;
        }
    };

    struct ExpData
    {
        int exp{ 0 };
        int level{ 0 };

        static constexpr int kBase = 3;
        static constexpr int kStep = 2;
        static constexpr int kLevelsPerTier = 5;
        static constexpr int kCurrencyPerLevel = 2;

        void Reset() noexcept
        {
            exp = 0;
            level = 0;
        }

        // exp require = 3 + (level / 5) * 2
        [[nodiscard]] int ExpToNext() const noexcept
        {
            const int lv = (std::max)(level, 0);
            return kBase + (lv / kLevelsPerTier) * kStep;
        }

        int Add(int amount) noexcept
        {
            if (amount <= 0)
            {
                return 0;
            }
            exp += amount;
            int grants = 0;
            while (exp >= ExpToNext())
            {
                exp -= ExpToNext();
                ++level;
                ++grants;
            }
            return grants;
        }
    };
}
struct InGameData
{
    PerformanceData performance;
    CurrencyData currency;
    ExpData exp;

    void Reset() noexcept
    {
        performance.Reset();
        currency.Reset();
        exp.Reset();
    }
};

struct CareerData
{

};

class GameStatsCodex
{
public:
    // write
    static void Reset() noexcept 
    { 
        Get_().igData.Reset();
        Get_().tData.Reset();
    }

    /////////////////////////////////////////////////////////
    // GlobalData (not cleared by Reset)
    /////////////////////////////////////////////////////////
    [[nodiscard]] static Language GetLanguage() noexcept
    {
        return Get_().gData.language;
    }

    static void SetLanguage(Language lang) noexcept
    {
        Get_().gData.language = lang;
    }

    [[nodiscard]] static float GetMasterVolume() noexcept
    {
        return Get_().gData.masterVolume;
    }

    static void SetMasterVolume(float vol) noexcept
    {
        Get_().gData.masterVolume = std::clamp(vol, 0.0f, 1.0f);
    }

    [[nodiscard]] static float GetBgmVolume() noexcept
    {
        return Get_().gData.bgmVolume;
    }

    static void SetBgmVolume(float vol) noexcept
    {
        Get_().gData.bgmVolume = std::clamp(vol, 0.0f, 1.0f);
    }

    [[nodiscard]] static float GetSeVolume() noexcept
    {
        return Get_().gData.seVolume;
    }

    static void SetSeVolume(float vol) noexcept
    {
        Get_().gData.seVolume = std::clamp(vol, 0.0f, 1.0f);
    }

    [[nodiscard]] static bool GetMuted() noexcept
    {
        return Get_().gData.muted;
    }

    static void SetMuted(bool muted) noexcept
    {
        Get_().gData.muted = muted;
    }

    [[nodiscard]] static bool GetFullscreen() noexcept
    {
        return Get_().gData.fullscreen;
    }

    static void SetFullscreen(bool fullscreen) noexcept
    {
        Get_().gData.fullscreen = fullscreen;
    }

    static constexpr int kWindowSizeCount = 3;

    // 0=1280x720，1=1600x900，2=1920x1080。
    [[nodiscard]] static int GetWindowSizeIndex() noexcept
    {
        return Get_().gData.windowSizeIndex;
    }

    static void SetWindowSizeIndex(int index) noexcept
    {
        Get_().gData.windowSizeIndex = std::clamp(index, 0, kWindowSizeCount - 1);
    }


    /////////////////////////////////////////////////////////
    // PerformanceData
    /////////////////////////////////////////////////////////
    // read only
    static const PerformanceData& Get() noexcept { return Get_().igData.performance; }

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
            Get_().igData.performance.lifeTime += dt;
    }

    static void AddScore(int s) noexcept { if (!Get_().tData.isTutorial) Get_().igData.performance.score += s; }
    static void SetGameClear() noexcept  { if (!Get_().tData.isTutorial) Get_().igData.performance.gameClear = true; }

    static void AddTotalDefeat(int c = 1) noexcept { if (!Get_().tData.isTutorial) Get_().igData.performance.totalDefeat += c; }
    static void AddRedDefeat(int c = 1) noexcept   { if (!Get_().tData.isTutorial) Get_().igData.performance.rDefeat += c; }
    static void AddGreenDefeat(int c = 1) noexcept { if (!Get_().tData.isTutorial) Get_().igData.performance.gDefeat += c; }
    static void AddBlueDefeat(int c = 1) noexcept  { if (!Get_().tData.isTutorial) Get_().igData.performance.bDefeat += c; }

    static void ReportRedCombo(int v) noexcept
    {
        if (!Get_().tData.isTutorial) if (v >= 0) Get_().igData.performance.rHighestCombo = std::max(Get_().igData.performance.rHighestCombo, v);
    }
    static void ReportGreenCombo(int v) noexcept
    {
        if (!Get_().tData.isTutorial) if (v >= 0) Get_().igData.performance.gHighestCombo = std::max(Get_().igData.performance.gHighestCombo, v);
    }
    static void ReportBlueCombo(int v) noexcept
    {
        if (!Get_().tData.isTutorial) if (v >= 0) Get_().igData.performance.bHighestCombo = std::max(Get_().igData.performance.bHighestCombo, v);
    }

    static void AddTotalWeapon(int c = 1) noexcept  { if (!Get_().tData.isTutorial) Get_().igData.performance.totalWeapon += c; }
    static void AddRedWeapon(int c = 1) noexcept    { if (!Get_().tData.isTutorial) Get_().igData.performance.rWeapon += c; }
    static void AddGreenWeapon(int c = 1) noexcept  { if (!Get_().tData.isTutorial) Get_().igData.performance.gWeapon += c; }
    static void AddBlueWeapon(int c = 1) noexcept   { if (!Get_().tData.isTutorial) Get_().igData.performance.bWeapon += c; }

    static void AddOutputDamage(float d) noexcept { if (!Get_().tData.isTutorial) if (d > 0) Get_().igData.performance.outputDamage += d; }
    static void AddInputDamage(float d) noexcept  { if (!Get_().tData.isTutorial) if (d > 0) Get_().igData.performance.inputDamage += d; }


    /////////////////////////////////////////////////////////
    // CurrencyData (not gated by isTutorial)
    /////////////////////////////////////////////////////////
    [[nodiscard]] static int GetCurrency() noexcept
    {
        return Get_().igData.currency.amount;
    }

    static void AddCurrency(int amount) noexcept
    {
        if (amount > 0)
        {
            Get_().igData.currency.amount += amount;
        }
    }

    static bool TrySpendCurrency(int amount) noexcept
    {
        if (amount <= 0)
        {
            return false;
        }
        CurrencyData& wallet = Get_().igData.currency;
        if (wallet.amount < amount)
        {
            return false;
        }
        wallet.amount -= amount;
        return true;
    }


    /////////////////////////////////////////////////////////
    // ExpData (not gated by isTutorial)
    /////////////////////////////////////////////////////////
    [[nodiscard]] static int GetExp() noexcept
    {
        return Get_().igData.exp.exp;
    }

    [[nodiscard]] static int GetLevel() noexcept
    {
        return Get_().igData.exp.level;
    }

    [[nodiscard]] static int GetExpToNext() noexcept
    {
        return Get_().igData.exp.ExpToNext();
    }

    static void AddExp(int amount) noexcept
    {
        const int grants = Get_().igData.exp.Add(amount);
        if (grants > 0)
        {
            AddCurrency(grants * ExpData::kCurrencyPerLevel);
        }
    }

    static void SpawnWindow()
    {
        const auto& s = GameStatsCodex::Get();

        ImGui::Begin("Game Stats", nullptr);

        if (ImGui::Button("Reset Stats"))
        {
            GameStatsCodex::Reset();
        }
        ImGui::SameLine();
        if (ImGui::Button("AddExp(3)"))
        {
            GameStatsCodex::AddExp(ExpData::kBase);
        }

        ImGui::Separator();

        if (ImGui::CollapsingHeader("Score & State", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Score: %d", s.score);
            ImGui::Text("Currency: %d", GameStatsCodex::GetCurrency());
            ImGui::Text("Exp: %d / %d  (Lv %d)", GameStatsCodex::GetExp(), GameStatsCodex::GetExpToNext(), GameStatsCodex::GetLevel());
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
    GlobalData gData;
    TutorialData tData;
    InGameData igData;
    CareerData cData;
};
