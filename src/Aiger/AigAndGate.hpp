#pragma once

#include "Globals/AllSatGloblas.hpp"

/*
    Simple class for an aiger and gate
*/
class AigAndGate
{
public:
    AigAndGate(AIGLIT l, AIGLIT r0, AIGLIT r1):
    m_L(l), m_R0(r0), m_R1(r1)
    {

    };
    AIGLIT GetL() const {return m_L;};
    AIGLIT GetR0() const {return m_R0;};
    AIGLIT GetR1() const {return m_R1;};
    
private:
    AIGLIT m_L;
    AIGLIT m_R0;
    AIGLIT m_R1;
};