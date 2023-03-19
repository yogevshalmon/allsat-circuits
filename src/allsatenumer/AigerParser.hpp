#pragma once

#include <vector>

#include "lorina/aiger.hpp"
#include "AllSatGloblas.hpp"
#include "AigAndGate.hpp"

using namespace lorina;
using namespace std;

/*
    parser for aiger, based on lorina
*/
class AigerParser : public aiger_reader
{
public:
    AigerParser () {};

    void on_header( uint64_t m, uint64_t i, uint64_t l, uint64_t o, uint64_t a ) const override
    {
        if ( m == 0)
        {
            throw runtime_error("Error when parsing AIG, max variable index is 0"); 
        }

        if ( i == 0)
        {
            throw runtime_error("Error when parsing AIG, no inputs provided"); 
        }

         if ( o > 1)
        {
            throw runtime_error("Error when parsing AIG, please provide only 1 output"); 
        }


        m_IsVarRef.resize((size_t)m, false);

        // always refernce index 0 for true\false
        if (m > 0)
        {
            m_IsVarRef[0] = true;
        }
    }

    void on_header( uint64_t m, uint64_t i, uint64_t l, uint64_t o, uint64_t a,
                          uint64_t b, uint64_t c, uint64_t j, uint64_t f ) const override
    {
        // TODO it seems lorina always call this header, remove output for now
        // cout << "c Extended aiger format found, ignoring unsupported features." << endl;

        // just call normal header
        on_header(m, i, l, o, a);
    }

    void on_input( uint32_t index, uint32_t lit ) const override 
    {
        // TODO check input is even?

        // always ref input, if not used we still need to iterate
        m_IsVarRef[AIGLitToAIGIndex(lit)] = true;

        assert( index == m_Inputs.size() );
        m_Inputs.push_back(lit);
    }
    
    void on_output( uint32_t index, uint32_t lit ) const override
    {
        // output should be asserted in any case, no need to insert to m_IsVarRef

        assert( index == m_Outputs.size() );
        m_Outputs.push_back(lit);
    }

    void on_and( uint32_t g_index, uint32_t left_lit, uint32_t right_lit ) const override
    {   
        m_IsVarRef[g_index] = true;
        if ( m_IsVarRef[AIGLitToAIGIndex(left_lit)] == false || m_IsVarRef[AIGLitToAIGIndex(right_lit)] == false)
        {
           throw runtime_error("Error when parsing AIG, an AND gate reference unkown or not intorduce literals " + to_string(left_lit) + ", " + to_string(right_lit)); 
        }

        m_AndGates.push_back(AigAndGate(AIGIndexToAIGLit(g_index), left_lit, right_lit));
    }

    const vector<AIGLIT>& GetInputs() const {return m_Inputs;};

    const vector<AIGLIT>& GetOutputs() const {return m_Outputs;};

    const vector<AigAndGate>& GetAndGated() const {return m_AndGates;};

    const vector<bool>& GetIsIndexRef() const {return m_IsVarRef;};

    const AIGINDEX GetMaxIndex() const {return (AIGINDEX)m_IsVarRef.size();}

protected:

    mutable vector<AIGLIT> m_Inputs;
    mutable vector<AIGLIT> m_Outputs;
    mutable vector<AigAndGate> m_AndGates;
    // hold all the actuall used vars
    mutable vector<bool> m_IsVarRef;
};
