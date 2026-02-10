#pragma once

#include <vector>

#include "Globals/AllSatAlgoGlobals.hpp"
#include "Globals/AllSatGloblas.hpp"

struct AllSatConfig
{
    enum class Encoding
    {
        Tseitin,
        DualRail
    };

    Encoding encoding = Encoding::Tseitin;

    bool useCirSim = false;
    bool useTopToBottomSim = false;
    bool useUcore = false;
    bool useLitDrop = true;
    unsigned litDropConflictLimit = 0;
    bool useLitDropRecur = false;

    bool dualBlockNoRep = false;
    bool dualForcePolarity = false;
    bool dualBoostScore = false;
    bool dualUseTseitinForDual = false;

    bool useIpasirForPlain = false;
    bool useIpasirForDual = true;

    bool printEnumerations = false;
    bool printInfo = true;
    bool useTimeout = false;
    unsigned timeoutSeconds = DEF_TIMEOUT;

    std::vector<AIGINDEX> projectionIndices;

    unsigned intelSatMode = 5;
};
