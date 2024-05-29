#pragma once

#include <string>
#include <unordered_map>
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


static const std::unordered_map<std::string, std::vector<std::string>> MODE_PARAMS
{
    {TERSIM_ALG, {"/alg/blocking/use_cirsim","1"}},

    {DRMS_DISJOINT_ALG, {"/alg/blocking/enc","dual_rail","/alg/blocking/dual_rail/boost_score","1","/alg/blocking/dual_rail/force_pol","1",
    "/alg/blocking/dual_rail/block_no_rep","1"}},

    {DRMS_NON_DISJOINT_ALG, {"/alg/blocking/enc","dual_rail","/alg/blocking/dual_rail/boost_score","1","/alg/blocking/dual_rail/force_pol","1",
    "/alg/blocking/dual_rail/block_no_rep","0"}},

    {COMB_DISJOINT_BLOCK_ALG, {"/alg/blocking/enc","dual_rail","/alg/blocking/dual_rail/boost_score","1","/alg/blocking/dual_rail/force_pol","1",
    "/alg/blocking/dual_rail/block_no_rep","1","/alg/blocking/use_cirsim","1"}},

    {CORE_ALG, {"/alg/blocking/use_ucore","1"}},

    {ROC_ALG, {"/alg/blocking/use_cirsim","1","/alg/blocking/use_ucore","1"}},

    {CARMA_ALG, {"/alg/blocking/enc","dual_rail","/alg/blocking/dual_rail/boost_score","1","/alg/blocking/dual_rail/force_pol","1",
    "/alg/blocking/dual_rail/block_no_rep","1","/alg/blocking/use_ucore","1","/alg/blocking/dual_rail/use_tseitin_for_dual","1"}}    
};
