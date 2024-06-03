#pragma once

#include <stdint.h>
#include <stdlib.h>

/*
    *** AIG lit/index ***
*/

// represet aig lit, where even is positive index, odd is negative index
// for example 3 -> !1, 2 -> 1
typedef uint32_t AIGLIT;

// represent aig index
typedef uint32_t AIGINDEX;

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

inline static bool IsAIGLitNeg(AIGLIT lit)
{
    // lit is odd
    return (lit & 1) != 0;
};


/*
    *** SAT lit ***
*/

// SAT lit, can be negative
typedef int32_t SATLIT;

// define TRUE and False for aig always 1,-1 regard the encoding
static constexpr SATLIT CONST_LIT_TRUE = 1;
static constexpr SATLIT CONST_LIT_FALSE = -1;


// return the sat lit for each AIG lit
// defined as the correspond aig index + 1 
// save the 1 lit for TRUE\FALSE const
inline static SATLIT AIGLitToSATLit(AIGLIT lit)
{
    if( lit == 0)
        return CONST_LIT_FALSE;
    if( lit == 1)
        return CONST_LIT_TRUE;
    // even
    if( (lit & 1) == 0)
        return ((SATLIT)AIGLitToAIGIndex(lit) + 1);
    else // odd
        return -((SATLIT)AIGLitToAIGIndex(lit) + 1);  
}

// return the sat lit for each AIG lit
// defined as the correspond aig index + 1 
// save the 1 lit for TRUE\FALSE const
inline static AIGINDEX SATLitToAIGIndex(SATLIT lit)
{
    if( lit == CONST_LIT_FALSE)
        return 0;
    if( lit == CONST_LIT_TRUE)
        return 1;

    return (abs(lit) - 1);   
}


/*
    *** Dual-Rail ***
*/

// dual-raul variable, pairs for SATLIT
typedef std::pair<SATLIT, SATLIT> DRVAR;

inline static SATLIT GetPos(const DRVAR& dvar)
{
    return dvar.first;
}

inline static SATLIT GetNeg(const DRVAR& dvar)
{
    return dvar.second;
}

// Get the two var represent the var_pos and var_neg from AIGLIT
// if even i.e. 2 return 2,3
// else odd i.e. 3 return 3,2
inline static DRVAR AIGLitToDR(AIGLIT var)
{
    // special case for constant TRUE\FALSE
    if( var == 0)
        return {CONST_LIT_FALSE, CONST_LIT_TRUE};
    if( var == 1)
        return {CONST_LIT_TRUE, CONST_LIT_FALSE};
    // even
    if( (var & 1) == 0)
        return {var, var + 1};
    else // odd
        return {var, var - 1};
}


inline static AIGLIT DRToAIGLit(const DRVAR& drvar)
{
    return (AIGLIT)GetPos(drvar);
}

inline static AIGINDEX DRToAIGIndex(const DRVAR& drvar)
{
    return AIGLitToAIGIndex(((AIGLIT)GetPos(drvar)));
}


