#pragma once

#include "Globals/AllSatGloblas.hpp"
#include "Globals/TernaryVal.hpp"

// encodings values enum
enum CirEncoding : unsigned char
{
    TSEITIN_ENC = 0,
    DUALRAIL_ENC = 1
};

// return ipasir status
// 10 : SAT
// 20 : UnSAT
// 0 : Timeout
// -1 : Error 
typedef int SOLVER_RET_STATUS;

// assignment for input use the AIGLIT of the input as index and TVal as value
// can be partial assignment for the inputs where DC is allowed, if input not exist consider as DC
// all variables reprsent some original input
typedef std::vector<std::pair<AIGLIT, TVal>> INPUT_ASSIGNMENT;

static constexpr SOLVER_RET_STATUS SAT_RET_STATUS = 10;
static constexpr SOLVER_RET_STATUS UNSAT_RET_STATUS = 20;
static constexpr SOLVER_RET_STATUS TIMEOUT_RET_STATUS = 0;
