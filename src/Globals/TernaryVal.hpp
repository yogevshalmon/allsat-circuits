#pragma once

#include <array>
/*
    TVal
*/

// ternary values enum, used as general values instead of 0\1
enum TVal : unsigned char
{
  False,
  True,
  DontCare,
  UnKown
};

constexpr std::array<std::array<TVal, 4>, 4> TableAndGateVal = []
{
    std::array<std::array<TVal, 4>, 4> table = {{
    //  False  True  DontCare Unknown
        {False,False,False,UnKown},
        {False,True,DontCare,UnKown},
        {False,DontCare,DontCare,UnKown},
        {UnKown,UnKown,UnKown,UnKown}
    }};

    
    return table;
}();

constexpr std::array<TVal, 4> TableNegVal = []
{
    std::array<TVal, 4> table = {
    //  False  True  DontCare Unknown
        {True,False,DontCare,UnKown}
    };

    
    return table;
}();

inline static TVal GetTValNeg(const TVal& tval)
{
    return TableNegVal[tval];
};