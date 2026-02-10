#pragma once

#include "allsat/AllSatLib.hpp"
#include "Globals/AllSatConfig.hpp"

inline void ApplyPreset(AllSatConfig& config, allsat::EnumerateOptions::Preset preset)
{
    switch (preset)
    {
        case allsat::EnumerateOptions::Preset::None:
            break;
        case allsat::EnumerateOptions::Preset::Tale:
            config.useCirSim = true;
            break;
        case allsat::EnumerateOptions::Preset::MarsDisjoint:
            config.encoding = AllSatConfig::Encoding::DualRail;
            config.dualBoostScore = true;
            config.dualForcePolarity = true;
            config.dualBlockNoRep = true;
            break;
        case allsat::EnumerateOptions::Preset::MarsNonDisjoint:
            config.encoding = AllSatConfig::Encoding::DualRail;
            config.dualBoostScore = true;
            config.dualForcePolarity = true;
            config.dualBlockNoRep = false;
            break;
        case allsat::EnumerateOptions::Preset::Duty:
            config.encoding = AllSatConfig::Encoding::DualRail;
            config.dualBoostScore = true;
            config.dualForcePolarity = true;
            config.dualBlockNoRep = true;
            config.useCirSim = true;
            break;
        case allsat::EnumerateOptions::Preset::Core:
            config.useUcore = true;
            break;
        case allsat::EnumerateOptions::Preset::Roc:
            config.useCirSim = true;
            config.useUcore = true;
            break;
        case allsat::EnumerateOptions::Preset::Carma:
            config.encoding = AllSatConfig::Encoding::DualRail;
            config.dualBoostScore = true;
            config.dualForcePolarity = true;
            config.dualBlockNoRep = true;
            config.useUcore = true;
            config.dualUseTseitinForDual = true;
            break;
    }
}
