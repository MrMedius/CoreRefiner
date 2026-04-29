#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <filesystem>
#include "Timer.h"

class PerfLog
{
private:
    struct Entry
    {
        enum class Kind
        {
            Timing,
            Info,
            Warn
        };
        // Timing entry
        Entry(std::string s, float t)
            :
            kind(Kind::Timing),
            label(std::move(s)),
            time(t)
        {}
        // Info/Warn entry
        Entry(Kind k, std::string msg)
            :
            kind(k),
            message(std::move(msg)),
            time(0.0f)
        {}
        void WriteTo(std::ostream& out) const noexcept
        {
            switch (kind)
            {
            case Kind::Timing:
            {
                if (label.empty())
                    out << time * 1000.0f << "ms\n";
                else
                    out << "[" << label << "] " << time * 1000.0f << "ms\n";
                break;
            }
            case Kind::Info:
            {
                out << "[INFO] " << message << "\n";
                break;
            }
            case Kind::Warn:
            {
                out << "[WARN] " << message << "\n";
                break;
            }
            default:
                break;
            }
        }

        Kind kind = Kind::Timing;
        std::string label;
        std::string message;
        float time = 0.0f;
    };

    static PerfLog& Get_() noexcept
    {
        static PerfLog log;
        return log;
    }

    PerfLog() noexcept
    {
        entries.reserve(3000);
    }

    ~PerfLog()
    {
        Flush_();
    }

    void Start_(const std::string& label = "") noexcept
    {
        entries.emplace_back(label, 0.0f);
        timer.Mark();
    }

    void Mark_(const std::string& label = "") noexcept
    {
        float t = timer.Peek();
        entries.emplace_back(label, t);
    }

    void Warn_(const std::string& msg) noexcept
    {
        entries.emplace_back(Entry::Kind::Warn, msg);
    }

    void Info_(const std::string& msg) noexcept
    {
        entries.emplace_back(Entry::Kind::Info, msg);
    }

    void Flush_() noexcept
    {
        namespace fs = std::filesystem;
        fs::create_directories("Log");

        std::ofstream perfFile("Log\\perf.txt");
        std::ofstream infoFile("Log\\info.txt");
        std::ofstream warnFile("Log\\warn.txt");

        perfFile << std::setprecision(3) << std::fixed;

        for (const auto& e : entries)
        {
            switch (e.kind)
            {
            case Entry::Kind::Timing:
                e.WriteTo(perfFile);
                break;
            case Entry::Kind::Info:
                e.WriteTo(infoFile);
                break;
            case Entry::Kind::Warn:
                e.WriteTo(warnFile);
                break;
            default:
                break;
            }
        }

        entries.clear();
    }

public:
    static void Start(const std::string& label = "") noexcept
    {
        Get_().Start_(label);
    }

    static void Mark(const std::string& label = "") noexcept
    {
        Get_().Mark_(label);
    }

    static void Warn(const std::string& msg) noexcept
    {
        Get_().Warn_(msg);
    }

    static void Info(const std::string& msg) noexcept
    {
        Get_().Info_(msg);
    }

    static void Flush() noexcept
    {
        Get_().Flush_();
    }

private:
    Timer timer;
    std::vector<Entry> entries;
};