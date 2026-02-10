#pragma once

#include <string>
#include <vector>

static const unsigned DEF_TIMEOUT = 3600;

// tseitin + ternary simulation 
static const std::string TERSIM_ALG = "tale";
// dual-rail + maxsat heuristics + disjoint blocking 
static const std::string DRMS_DISJOINT_ALG = "mars-dis";
// dual-rail + maxsat heuristics + non-disjoint blocking 
static const std::string DRMS_NON_DISJOINT_ALG = "mars-nondis";
// dual-rail + maxsat heuristics + disjoint blocking + ternary simulation
static const std::string COMB_DISJOINT_BLOCK_ALG = "duty";
// tseitin + ucore
static const std::string CORE_ALG = "core";
// mode:tale + ucore
static const std::string ROC_ALG = "roc";
// mode:mars-dis + + ucore
static const std::string CARMA_ALG = "carma";

// Note:
// for simplicty remove the comb with non-disjoint blocking, as we only use one duty configuration in the paper
//static const std::string COMB_NON_DISJOINT_BLOCK_ALG = "duty-nondis-blocking";


// pre-configured algorithms modes
static const std::vector<std::string> MODES = {
    TERSIM_ALG,
    DRMS_DISJOINT_ALG,
    DRMS_NON_DISJOINT_ALG,
    COMB_DISJOINT_BLOCK_ALG,
    CORE_ALG,
    ROC_ALG,
    CARMA_ALG
};
