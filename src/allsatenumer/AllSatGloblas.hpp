#pragma once

#include <stdint.h>
#include <vector>


static const std::string TERSIM_ALG = "tale";
static const std::string DRMS_DISJOINT_ALG = "mars-dis";
static const std::string DRMS_NON_DISJOINT_ALG = "mars-nondis";
static const std::string COMB_DISJOINT_BLOCK_ALG = "duty";
// for simplicty remove the comb with disjoint blocking, as we only one duty configuration in the paper
//static const std::string COMB_NON_DISJOINT_BLOCK_ALG = "duty-nondis-blocking";

static const std::string DEF_ALG = COMB_DISJOINT_BLOCK_ALG;

// pre-configured algorithms modes
static const std::vector<std::string> MODES = {
    TERSIM_ALG,
    DRMS_DISJOINT_ALG,
    DRMS_NON_DISJOINT_ALG,
    COMB_DISJOINT_BLOCK_ALG
};

// represet aig lit, where even is positive index, odd is negative index
// for example 3 -> !1, 2 -> 1
typedef uint32_t AIGLIT;

// represent aig index
typedef uint32_t AIGINDEX;

// SAT lit, can be negative
typedef int32_t SATLIT;
// dual-raul variable, pairs for SATLIT
typedef std::pair<SATLIT, SATLIT> DRVAR;

// return lit index i.e. 
// 3 -> 1
// 2 -> 1
inline static AIGINDEX AIGLitToAIGIndex(AIGLIT lit)
{
    return (AIGINDEX)(lit >> 1);
};

// return index to lit i.e. 
// 2 -> 4
// 5 -> 10
inline static AIGLIT AIGIndexToAIGLit(AIGINDEX index)
{
    return (AIGLIT)(index << 1);
};
