#pragma once

#include <queue>

#include "AllSatGloblas.hpp"
#include "AigerParser.hpp"

using namespace std;

// ternary values enum
enum TVal : unsigned char
{
  True,
  False,
  DontCare,
  UnKown
};

// define negation of ternary values
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

/*
    class for ternary simulation
    get the aig from AigerParser class
*/
class TernarySim
{
public:
    TernarySim(const AigerParser& aigerParser):
    m_Inputs(aigerParser.GetInputs()), m_Outputs(aigerParser.GetOutputs()), m_AndGates(aigerParser.GetAndGated()),
    m_MaxIndex(aigerParser.GetMaxIndex())
    {
        m_IndexCurrVal.resize((size_t)(m_MaxIndex + 1), TVal::UnKown);

        m_IndexGatesWatch.resize((size_t)(m_MaxIndex + 1));

        // intilize m_IndexGatesWatch, we assume the gates are in order from bottom-up
        for (size_t gIndex = 0; gIndex < m_AndGates.size(); ++gIndex)
        {
            const AigAndGate& gate = m_AndGates[gIndex];
            m_IndexGatesWatch[AIGLitToAIGIndex(gate.GetR0())].push_back(gIndex);
            m_IndexGatesWatch[AIGLitToAIGIndex(gate.GetR1())].push_back(gIndex);
        }
    };

    // initialVal contain the values to start simulate from
    void MaximizeDontCare(const vector<pair<AIGLIT, TVal>>& initialValues)
    {
        // clear values before new simulation
        m_IndexCurrVal.assign( m_IndexCurrVal.size(), TVal::UnKown);

        for (const auto& initValPair : initialValues)
        {
            AssignValForLit(initValPair.first, initValPair.second);
        }

        // simulate all the gates value from initial values
        // should be a valid assigment
        for (const AigAndGate& gate : m_AndGates)
        {
            AssignValForLit(gate.GetL(), GetAndOfTVal(GetValForLit(gate.GetR0()), GetValForLit(gate.GetR1())));
        }

        for (const AIGLIT outLit : m_Outputs)
        {
            // assume all outputs should be 1
            if (GetValForLit(outLit) != TVal::True)
            {
                throw runtime_error("Initial assigment values are not valid, output not asserted to 1");
            }  
        }

        vector<TVal> lastValidVal;

        // now try to maximize the DC values
        for (const AIGLIT inputLit : m_Inputs)
        {
            // in case already Dont care case, can come from AllSatEnumerDualRail
            if (GetValForLit(inputLit) == TVal::DontCare)
            {
                continue;
            }

            // save the last valid values vec until now
            lastValidVal = m_IndexCurrVal;
            AssignValForLit(inputLit, TVal::DontCare);

            const vector<size_t>& inputGatesRef = m_IndexGatesWatch[AIGLitToAIGIndex(inputLit)];

            std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> gatesToCheck;

            for (const size_t& inputRefGateIndex : inputGatesRef)
            {
                gatesToCheck.push(inputRefGateIndex);
            }

            // iterate until no more gates to check
            while(!gatesToCheck.empty())
            {
                const size_t currGateIndex = gatesToCheck.top();
                gatesToCheck.pop();

                // remove duplicate gates check
                while(currGateIndex == gatesToCheck.top() && !gatesToCheck.empty())
                {
                    gatesToCheck.pop();
                }

                const AigAndGate& currGate = m_AndGates[currGateIndex];
                
                const AIGLIT& gateLit = currGate.GetL();

                TVal currGateVal = GetValForLit(gateLit);

                TVal newGateVal = GetAndOfTVal(GetValForLit(currGate.GetR0()), GetValForLit(currGate.GetR1()));

                // gate didnt change value, dont need to assign all the watched gates
                if (currGateVal == newGateVal)
                {
                    continue;
                }

                AssignValForLit(gateLit, newGateVal);

                for (const size_t& newGateIndex : m_IndexGatesWatch[AIGLitToAIGIndex(gateLit)])
                {
                    gatesToCheck.push(newGateIndex);
                }

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


    // get value the current value for lit
    TVal GetValForLit(const AIGLIT lit)
    {
        // special cases for const true\false
        if (lit == 1)
            return TVal::True;

        if (lit == 0)
            return TVal::False;

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

    // get value the current value for index
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
    // hold the inputs
    const vector<AIGLIT> m_Inputs;

    // for every AIGINDEX hold all the gates indexes it is refernce in from m_AndGates 
    vector<vector<size_t>> m_IndexGatesWatch;

    const vector<AIGLIT> m_Outputs;
    const vector<AigAndGate> m_AndGates;

    const AIGINDEX m_MaxIndex;

    // for every AIGINDEX hold the curr value for the simulation
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
