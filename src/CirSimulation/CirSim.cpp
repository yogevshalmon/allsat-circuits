#include "CirSim.hpp"

using namespace std;

CirSim::CirSim(const AigerParser& aigerParser, SimStrat simStart):
m_Inputs(aigerParser.GetInputs()), m_Outputs(aigerParser.GetOutputs()), m_AndGates(aigerParser.GetAndGated()),
m_MaxIndex(aigerParser.GetMaxIndex()),
m_SimStart(simStart)
{
    m_IndexCurrVal.resize((size_t)GetNextAigIndex(), TVal::UnKown);

    m_IndexGatesWatch.resize((size_t)GetNextAigIndex());

    // intilize m_IndexGatesWatch, we assume the gates are in order from bottom-up
    for (size_t gIndex = 0; gIndex < m_AndGates.size(); ++gIndex)
    {
        const AigAndGate& gate = m_AndGates[gIndex];
        m_IndexGatesWatch[AIGLitToAIGIndex(gate.GetR0())].push_back(gIndex);
        m_IndexGatesWatch[AIGLitToAIGIndex(gate.GetR1())].push_back(gIndex);
    }
};


// initialVal contain the values to start simulate from
vector<pair<AIGLIT, TVal>> CirSim::MaximizeDontCare(const vector<pair<AIGLIT, TVal>>& initialValues, const bool onlySatOut)
{
    // TODO check initialValues really are values to the inputs
    vector<pair<AIGLIT, TVal>> minimizeValues(initialValues.size());

    // clear values before new simulation
    m_IndexCurrVal.assign(m_IndexCurrVal.size(), TVal::UnKown);

    for (const auto& initValPair : initialValues)
    {
        AssignValForLit(initValPair.first, initValPair.second);
    }

    // simulate all the gates value from initial values
    // should be a valid assigment
    SimulateAllGates();

    if (onlySatOut)
    {
        for (const AIGLIT outLit : m_Outputs)
        {
            // assume all outputs should be 1
            if (GetValForLit(outLit) != TVal::True)
            {
                throw runtime_error("Initial assigment values are not valid, output not asserted to 1");
            }  
        }
    }
    
    if (m_SimStart == SimStrat::BotToTop)
    {
        GenBotToTop();
    }
    else if (m_SimStart == SimStrat::TopToBot)
    {
        GenTopToBot();
    }
    else // unkown option
    {
       throw runtime_error("Unkown simulation startegy");
    }
    

    transform(initialValues.begin(), initialValues.end(), minimizeValues.begin(), [&](const pair<AIGLIT, TVal>& assg) -> pair<AIGLIT, TVal>
    {
       return make_pair(assg.first, GetValForLit(assg.first)); 
    });
    
    return minimizeValues;
}


void CirSim::GenBotToTop()
{
    // TODO for now assume all false or all true
    TVal outInitVal = GetValForLit(m_Outputs[0]);
    // save the last valid values stack
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
        // assign DC to the current input
        AssignValForLit(inputLit, TVal::DontCare);

        SimulateValuesWithQ(inputLit);
        
        // after simulation check if any output is DontCare
        // if so the revert to the last valid values
        for (const AIGLIT outLit : m_Outputs)
        {
            if (GetValForLit(outLit) != outInitVal)
            {
                m_IndexCurrVal = lastValidVal;
                break;
            }  
        }
    }
}


void CirSim::GenTopToBot()
{
    // for each aig lit represent if it need to stay constant meaning it cannot convert to DC
    vector<bool> isConstReq(GetNextAigIndex(), false);

    // assume all outputs should remain constant
    for (const AIGLIT outLit : m_Outputs)
    {
        isConstReq[AIGLitToAIGIndex(outLit)] = true;
    }

    for (auto gateRevIt = m_AndGates.rbegin(); gateRevIt != m_AndGates.rend(); ++gateRevIt)
    {
        const AigAndGate& currGate = *gateRevIt;

        AIGLIT gateOutLit = currGate.GetL();
        AIGINDEX gateOutInd = AIGLitToAIGIndex(gateOutLit);
        AIGLIT gateR0Lit = currGate.GetR0();
        AIGINDEX gateR0Ind = AIGLitToAIGIndex(gateR0Lit);
        AIGLIT gateR1Lit = currGate.GetR1();
        AIGINDEX gateR1Ind = AIGLitToAIGIndex(gateR1Lit);

        if (isConstReq[gateOutInd] && !(isConstReq[gateR0Ind] && isConstReq[gateR1Ind]))
        {
            TVal currGateVal = GetValForLit(gateOutLit);
            // if the output of and gate must be 1, then both input to gate must be constant
            if (currGateVal == TVal::True)
            {
                isConstReq[gateR0Ind] = isConstReq[gateR1Ind] = true;
            }
            else
            { // value should be TVal::False since isConstReq[gateOutInd] = true
              // so the value can not be DC  
                #ifdef DEBUG
                if (currGateVal != TVal::False)
                {
                    throw runtime_error("Top to bottom simulation error, expect constant value");
                }
                #endif
                TVal currR0Val = GetValForLit(gateR0Lit);
                TVal currR1Val = GetValForLit(gateR1Lit);
                if (currR0Val == TVal::False && currR1Val != TVal::False)
                {
                    isConstReq[gateR0Ind] = true;
                }
                else if (currR0Val != TVal::False && currR1Val == TVal::False)
                {
                    isConstReq[gateR1Ind] = true;
                }
                else if (currR0Val == TVal::False && currR1Val == TVal::False)
                {   // can decide which to mark, by default mark the "smaller" node
                    // the intuation is that higher node may have larger cone under and include more inputs
                    isConstReq[min(gateR0Ind, gateR1Ind)] = true;
                }
                else
                { // should not happen
                    #ifdef DEBUG
                        throw runtime_error("Top to bottom simulation error, value of gate is not valid");      
                    #endif
                }
            } 
        }

        if (!isConstReq[gateOutInd])
        {
            AssignValForLit(gateOutLit, TVal::DontCare);
        }
    }

    for (const AIGLIT inputLit : m_Inputs)
    {
        if (!isConstReq[AIGLitToAIGIndex(inputLit)])
        {
            AssignValForLit(inputLit, TVal::DontCare);
        }
    }
}

// get value the current value for lit
TVal CirSim::GetValForLit(const AIGLIT lit)
{
    // special cases for const true\false
    if (lit == 1)
        return TVal::True;

    if (lit == 0)
        return TVal::False;

    AIGINDEX index = AIGLitToAIGIndex(lit);

    #ifdef DEBUG
        if (index >= m_IndexCurrVal.size())
        {
            throw runtime_error("Accessing unkonw lit when getting val for lit " + to_string(lit));
        }
    #endif 

    if( (lit & 1) == 0)
        return m_IndexCurrVal[index];
    else // odd
        return GetValNeg(m_IndexCurrVal[index]);
}

 // get value the current value for index
TVal CirSim::GetValForIndex(const AIGINDEX index, bool neg)
{
    #ifdef DEBUG
        if (index >= m_IndexCurrVal.size())
        {
            throw runtime_error("Accessing unkonw index when getting val for index " + to_string(index));
        }
    #endif 

    if (!neg)
        return m_IndexCurrVal[index];
    else // odd
        return GetValNeg(m_IndexCurrVal[index]);
}


 // get the value of the output
TVal CirSim::GetValForOut(const size_t outIndex)
{
     #ifdef DEBUG
        if (outIndex >= m_Outputs.size())
        {
            throw runtime_error("Accessing unkonw out index when getting val for output index " + to_string(outIndex));
        }
    #endif 

    return GetValForLit(m_Outputs[outIndex]);
}

void CirSim::AssignValForLit(const AIGLIT lit, const TVal& val)
{
    AIGINDEX index = AIGLitToAIGIndex(lit);

    #ifdef DEBUG
        if (index >= m_IndexCurrVal.size())
        {
            throw runtime_error("Accessing unkonw lit when assigning lit " + to_string(lit));
        }
    #endif 

    if( (lit & 1) == 0)
        m_IndexCurrVal[index] = val;
    else // odd
        m_IndexCurrVal[index] = GetValNeg(val);
}

TVal CirSim::GetAndOfVal(const TVal& val0, const TVal& val1)
{
    return TableAndGateVal[val0][val1];
}

// define negation of ternary values
TVal CirSim::GetValNeg(const TVal& tval)
{
    return TableNegVal[tval];
};

AIGINDEX CirSim::GetNextAigIndex() const
{
    return m_MaxIndex + 1;
}

void CirSim::SimulateValuesWithQ(const AIGLIT inputLit)
{
    const vector<size_t>& inputGatesRef = m_IndexGatesWatch[AIGLitToAIGIndex(inputLit)];

    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> gatesToCheck;

    for (const size_t& inputRefGateIndex : inputGatesRef)
    {
        gatesToCheck.push(inputRefGateIndex);
    }

    // iterate until no more gates to check
    while (!gatesToCheck.empty())
    {
        const size_t currGateIndex = gatesToCheck.top();
        gatesToCheck.pop();

        // remove duplicate gates check
        while (currGateIndex == gatesToCheck.top() && !gatesToCheck.empty())
        {
            gatesToCheck.pop();
        }

        const AigAndGate& currGate = m_AndGates[currGateIndex];
        
        const AIGLIT& gateLit = currGate.GetL();

        TVal currGateVal = GetValForLit(gateLit);

        TVal newGateVal = GetAndOfVal(GetValForLit(currGate.GetR0()), GetValForLit(currGate.GetR1()));

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
}

void CirSim::SimulateAllGates()
{
    // simulate all the gates value from initial values
    // should be a valid assigment
    for (const AigAndGate& gate : m_AndGates)
    {
        AssignValForLit(gate.GetL(), GetAndOfVal(GetValForLit(gate.GetR0()), GetValForLit(gate.GetR1())));
    }
}