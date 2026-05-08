#include "macro_action.h"
#include "config_loader.h"

void MacroAction::addStep(Step step) { steps_.push_back(std::move(step)); }

void MacroAction::getJsonProperties(JsonObject& json) const
{
    JsonArray stepsArr = json.createNestedArray("steps");
    for (const auto& step : steps_)
    {
        JsonArray stepArr = stepsArr.createNestedArray();
        for (const auto& action : step)
        {
            JsonObject actionObj = stepArr.createNestedObject();
            ConfigLoader::actionToJson(action.get(), actionObj);
        }
    }
}

void MacroAction::execute()
{
    if (steps_.empty() || running_)
    {
        return;
    }

    currentStep_ = 0;
    running_ = true;
    fireCurrentStep();
    // Advance synchronous steps immediately so callers see the final state
    // without needing an extra update() tick.
    while (running_ && currentStepDone())
    {
        ++currentStep_;
        if (currentStep_ >= steps_.size())
        {
            running_ = false;
            return;
        }
        fireCurrentStep();
    }
}

void MacroAction::executeRelease()
{
    if (! running_ || currentStep_ >= steps_.size())
    {
        return;
    }
    for (auto& action : steps_[currentStep_])
    {
        action->executeRelease();
    }
}

void MacroAction::update()
{
    if (! running_)
    {
        return;
    }

    // Poll in-progress actions in the current step (e.g. DelayedAction timer ticks).
    if (currentStep_ < steps_.size())
    {
        for (auto& action : steps_[currentStep_])
        {
            if (action->isInProgress())
            {
                action->execute();
            }
        }
    }

    while (currentStepDone())
    {
        ++currentStep_;
        if (currentStep_ >= steps_.size())
        {
            running_ = false;
            return;
        }
        fireCurrentStep();
    }
}

void MacroAction::fireCurrentStep()
{
    for (auto& action : steps_[currentStep_])
    {
        action->execute();
    }
}

bool MacroAction::currentStepDone() const
{
    if (currentStep_ >= steps_.size())
    {
        return true;
    }
    for (const auto& action : steps_[currentStep_])
    {
        if (action->isInProgress())
        {
            return false;
        }
    }
    return true;
}
