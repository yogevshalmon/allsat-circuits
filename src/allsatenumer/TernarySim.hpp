#pragma once

#include "AllSatGloblas.hpp"
#include "AigerParser.hpp"

using namespace std;

enum TVal : unsigned char
{
  True,
  False,
  DontCare,
  UnKown
};

inline static TVal GetTValNeg(const TVal& tval)
{
    if (tval == TVal::True)
        return TVal::False;

    if (tval == TVal::False)
        return TVal::True;

    if (tval == TVal::DontCare)
        return TVal::DontCare;

    return TVal::UnKown;
};

// class for ternary simulation
// get the aig from AigerParser class
class TernarySim
{
public:
    TernarySim(const AigerParser& aigerParser):
    m_Inputs(aigerParser.GetInputs()), m_Outputs(aigerParser.GetOutputs()), m_AndGates(aigerParser.GetAndGated()),
    m_MaxIndex(aigerParser.GetMaxIndex())
    {
        m_IndexCurrVal.resize((size_t)(m_MaxIndex + 1), TVal::UnKown);
    };

    // initialVal contain the values to start simulate from
    void MaximizeDontCare(const vector<pair<AIGLIT, bool>>& initialValues)
    {
        // clear values before new simulation
        m_IndexCurrVal.assign( m_IndexCurrVal.size(), TVal::UnKown);

        //cout << "initialValues size : " << initialValues.size() << endl;

        for (const auto& initValPair : initialValues)
        {
            //cout << "AssignValForLit : " << initValPair.first << endl;
            AssignValForLit(initValPair.first, initValPair.second ? TVal::True : TVal::False);
        }

        vector<TVal> lastValidVal;

        // now try to maximize the DC values
        for (const AIGLIT inputLit : m_Inputs)
        {
            // save the last valid values vec until now
            lastValidVal = m_IndexCurrVal;
            AssignValForLit(inputLit, TVal::DontCare);
            
            // simulate all the gates
            for (const AigAndGate& gate : m_AndGates)
            {
                AssignValForLit(gate.GetL(), GetAndOfTVal(GetValForLit(gate.GetR0()), GetValForLit(gate.GetR1())));
            }

            // after simulation check if any output is DontCare
            // if so the revert to the last valid values
            for (const AIGLIT outLit : m_Outputs)
            {
                if (GetValForLit(outLit) == TVal::DontCare)
                {
                    m_IndexCurrVal = lastValidVal;
                    break;
                }  
            }
        }
    }

    TVal GetValForLit(const AIGLIT lit)
    {
        AIGINDEX index = AIGLitToAIGIndex(lit);
        if (index >= m_IndexCurrVal.size())
        {
           throw runtime_error("Accessing unkonw lit when getting val for lit " + to_string(lit));
        }
        
        if( (lit & 1) == 0)
            return m_IndexCurrVal[index];
		else // odd
			return GetTValNeg(m_IndexCurrVal[index]);
    }

    TVal GetValForIndex(const AIGINDEX index, bool neg = false)
    {
        if (index >= m_IndexCurrVal.size())
        {
           throw runtime_error("Accessing unkonw index when getting val for index " + to_string(index));
        }
 
        if (!neg)
            return m_IndexCurrVal[index];
		else // odd
			return GetTValNeg(m_IndexCurrVal[index]);
    }


private:
    const vector<AIGLIT> m_Inputs;
    const vector<AIGLIT> m_Outputs;
    const vector<AigAndGate> m_AndGates;
    const AIGINDEX m_MaxIndex;

    // hold the curr value for the simulation
    vector<TVal> m_IndexCurrVal;

    void AssignValForLit(const AIGLIT lit, TVal val)
    {
        AIGINDEX index = AIGLitToAIGIndex(lit);
        if (index >= m_IndexCurrVal.size())
        {
           throw runtime_error("Accessing unkonw lit when assigning lit " + to_string(lit));
        }
        
        if( (lit & 1) == 0)
            m_IndexCurrVal[index] = val;
		else // odd
			m_IndexCurrVal[index] = GetTValNeg(val);
    }

    // True & True = True
    // False & True = False
    // False & False = False
    // False & DontCare = False
    // True & DontCare = DontCare
    // DontCare & DontCare = DontCare
    TVal GetAndOfTVal(const TVal& val0, const TVal& val1)
    {
        if (val0 == TVal::UnKown || val1 == TVal::UnKown)
            throw runtime_error("And gate not defined for unkown value");
        if (val0 == TVal::False || val1 == TVal::False)
            return TVal::False;
        if (val0 == TVal::DontCare || val1 == TVal::DontCare)
            return TVal::DontCare;
        
        return TVal::True;
    }

};