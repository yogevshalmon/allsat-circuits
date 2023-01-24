#pragma once

#include <stdint.h>

typedef uint32_t AIGLIT;
typedef uint32_t AIGINDEX;

typedef int32_t SATLIT;
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