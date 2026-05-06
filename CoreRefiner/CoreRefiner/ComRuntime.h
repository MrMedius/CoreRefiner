#pragma once
#include "Win.h"

class ComRuntime
{
public:
    enum class Model
    {
        STA,
        MTA,
    };

    explicit ComRuntime(Model model = Model::STA);
    ComRuntime(const ComRuntime&) = delete;
    ComRuntime& operator=(const ComRuntime&) = delete;
    ~ComRuntime();

    bool IsInitialized() const noexcept { return initialized_; }
    Model GetModel() const noexcept { return model_; }

private:
    bool initialized_ = false;
    Model model_ = Model::STA;
};

