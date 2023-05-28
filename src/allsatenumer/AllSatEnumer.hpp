#pragma once

#include <vector>

#include "AllSatEnumerBase.hpp"

using namespace Topor;
using namespace std;

/*
    allsat class based on Tseitin encoding
*/
class AllSatEnumer : public AllSatEnumerBase
{
    public:
        AllSatEnumer(const InputParser& inputParser) : AllSatEnumerBase(inputParser)
        {

        }

        void InitializeSolver(string filename)
        {
            ParseAigFile(filename);

            // initilize tersim if needed
            if (m_UseTerSim)
            {
                m_TernarySimulation = new TernarySim(m_AigParser);
            }

            m_Solver->AddClause(CONST_LIT_TRUE);

            for(const AigAndGate& gate : m_AigParser.GetAndGated())
            {
                HandleAndGate(gate);
            }
            
            const vector<AIGLIT>& inputs = m_AigParser.GetInputs();

            m_Inputs.resize(inputs.size());

            transform(inputs.begin(), inputs.end(), m_Inputs.begin(), [&](const AIGLIT& aiglit) -> SATLIT
            {
                return AIGLitToSATLit(aiglit);
            });
            
            
            // Get all the the Output
            const vector<AIGLIT>& outputs = m_AigParser.GetOutputs();

            if (outputs.size() > 1)
            {
                throw runtime_error("Error, number of outputs should be 1"); 
            }

            for(AIGLIT aigLitOutput : outputs)
            {
                SATLIT out = AIGLitToSATLit(aigLitOutput);
                m_Solver->AddClause(out);
            }
            
        }

        virtual ~AllSatEnumer(){}

    protected:

        // return the sat lit for each AIG lit
        // defined as the correspond aig index + 1 
        // save the 1 lit for TRUE\FALSE const
        SATLIT AIGLitToSATLit(AIGLIT lit) const 
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
        AIGINDEX SATLitToAIGIndex(SATLIT lit) const 
        {
            if( lit == CONST_LIT_FALSE)
                return 0;
            if( lit == CONST_LIT_TRUE)
                return 1;

            return (abs(lit) - 1);
        }

		void HandleAndGate(const AigAndGate& gate)
		{
			AIGLIT l = gate.GetL();
			AIGLIT r0 = gate.GetR0();
			AIGLIT r1 = gate.GetR1();

            // write and gate using the index variables
			WriteAnd(AIGLitToSATLit(l), AIGLitToSATLit(r0), AIGLitToSATLit(r1));
		}
        
        // get value for current lit from the SAT solver
        TVal GetTValFromLit(const SATLIT lit)
        {
            auto isPos = m_Solver->GetLitValue(lit) == TToporLitVal::VAL_SATISFIED;
            if (isPos)
            {
                return TVal::True;
            }
            else
            {
                return TVal::False;
            }
        }

        // return the number of dont cares
        unsigned GetBlockingClause()
        {
            unsigned numOfDontCares = 0;
            m_BlockingClause.clear();

            // use ternary simulation
            if (m_UseTerSim)
            {
                vector<pair<AIGLIT, TVal>> inputValues(m_Inputs.size());

                transform(m_Inputs.begin(), m_Inputs.end(), inputValues.begin(), [&](const SATLIT inputSatLit) -> pair<AIGLIT, TVal>
                {
                    return {AIGIndexToAIGLit(SATLitToAIGIndex(inputSatLit)), m_Solver->GetLitValue(inputSatLit) == TToporLitVal::VAL_SATISFIED ? TVal::True : TVal::False};
                });

                // use ternary simulation for maximize dont-care values
                m_TernarySimulation->MaximizeDontCare(inputValues);
            }

            for (const SATLIT input : m_Inputs)
            {
                TVal currVal = m_UseTerSim ? m_TernarySimulation->GetValForIndex(SATLitToAIGIndex(input)) : GetTValFromLit(input);

                if (currVal == TVal::True)
                {
                    m_BlockingClause.push_back(-input);
                }
                else if (currVal == TVal::False)
                {
                    m_BlockingClause.push_back(input);
                }
                else if (currVal == TVal::DontCare) // dont care case
                {
                    numOfDontCares++;
                }
                else
                {
                    throw runtime_error("Unkown value for input");
                }
            }

            return numOfDontCares;
        }

        void printEnumr()
        {
            for (const SATLIT& input : m_Inputs)
            {
                TVal currVal = m_UseTerSim ? m_TernarySimulation->GetValForIndex(SATLitToAIGIndex(input)) : GetTValFromLit(input);
                
                AIGINDEX litIndex = SATLitToAIGIndex(input); 

                printIndexVal(litIndex, currVal);
            }
                
            cout << endl;
        }

    protected:

        vector<SATLIT> m_Inputs;

};
