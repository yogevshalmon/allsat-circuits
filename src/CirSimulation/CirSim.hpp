#pragma once

#include <queue>
#include <vector>
#include <unordered_set>

#include "Globals/AllSatGloblas.hpp"
#include "Globals/AllSatSolverGloblas.hpp"
#include "Globals/TernaryVal.hpp"
#include "Aiger/IAigerView.hpp"

// decide on the order of the simulation start
enum SimStrat : unsigned char
{
    BotToTop, // bot to top
    TopToBot  // top to bot
};


/*
    class for ternary simulation
    get the aig from AigerParser class
*/
class CirSim
{
public:
    CirSim(const IAigerView& aigerParser, SimStrat simStart = SimStrat::BotToTop, 
           const std::unordered_set<AIGLIT>* projectionSet = nullptr);

    // initialVal contain the values to start simulate from
    INPUT_ASSIGNMENT MaximizeDontCare(const INPUT_ASSIGNMENT& initialValues, const bool onlySatOut = true);

    // get value the current value for lit
    TVal GetValForLit(const AIGLIT lit);

    // get value the current value for index
    TVal GetValForIndex(const AIGINDEX index, bool neg = false);

    // get the value of the output
    TVal GetValForOut(const size_t outIndex = 0);


protected:

    // generalize by simulation from bottom to top
    void GenBotToTop();

    // generalize by simulation from top to bot
    void GenTopToBot();


    void AssignValForLit(const AIGLIT lit, const TVal& val);

    TVal GetAndOfVal(const TVal& val0, const TVal& val1);

    // define negation of ternary values
    TVal GetValNeg(const TVal& tval);

    // return the next aig index (max+1)
    AIGINDEX GetNextAigIndex() const;

    void SimulateValuesWithQ(const AIGLIT inputLit);

    void SimulateAllGates();

protected:

    // hold the inputs
    const std::vector<AIGLIT> m_Inputs;

    // for every AIGINDEX hold all the gates indexes it is refernce in from m_AndGates 
    std::vector<std::vector<size_t>> m_IndexGatesWatch;

    // hold all the outputs
    const std::vector<AIGLIT> m_Outputs;
    // hold all the gates
    const std::vector<AigAndGate> m_AndGates;

    const AIGINDEX m_MaxIndex;

    // hold the startegy
    const SimStrat m_SimStart;
    
    // pointer to projection set (nullptr if no projection)
    // only projection variables will be considered for don't-care conversion
    const std::unordered_set<AIGLIT>* m_ProjectionSet;
    
    // check if a variable is in the projection set (or no projection is used)
    bool IsProjectionVar(AIGLIT lit) const;
    
    // for every AIGINDEX hold the curr value for the simulation
    std::vector<TVal> m_IndexCurrVal;

};