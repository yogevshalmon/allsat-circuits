#pragma once

#include <vector>

#include "AllSatEnumerBase.hpp"
#include "TernarySim.hpp"

using namespace Topor;
using namespace std;

class AllSatEnumerTernarySim : public AllSatEnumerBase
{
    public:
        AllSatEnumerTernarySim(const InputParser& inputParser) : AllSatEnumerBase(inputParser)
        {
            m_TernarySimulation = nullptr;
        }

        void InitializeSolver(string filename)
        {
            ParseAigFile(filename);

            m_TernarySimulation = new TernarySim(m_AigParser);

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
            
             //TODO outputs should be size 1 ?
            // Get all the the Output
            const vector<AIGLIT>& outputs = m_AigParser.GetOutputs();
            // TODO can assert different things on the output

            for(AIGLIT aigLitOutput : outputs)
            {
                SATLIT out = AIGLitToSATLit(aigLitOutput);
                m_Solver->AddClause(out);
            }
            
            // SATLIT maxLit = (SATLIT)((isIndexRef.size()+1)*2);
            // vector<SATLIT> outputsIsPos;

            // for(AIGLIT aigOutput : outputs)
            // {
            //     // create and for output is pos
            //     DRVAR outdr = GetDualFromAigLit(aigOutput);
            //     WriteAnd(maxLit, GetPos(outdr), -GetNeg(outdr));
            //     outputsIsPos.push_back(maxLit);
            //     maxLit++;
            // }

            // // now assert clause of outputsIsPos meaning one of the output must be 1

            // m_Solver->AddClause(outputsIsPos);
        }

        void FindAllEnumer()
        {
            int res = ToporBadRetVal;
            res = SolveAndGetResult();
   
            while( res == ToporSatRetVal)
            {
                m_NumberOfAssg++;          
                unsigned numOfDontCares = EnforceSatEnumr();
                if (m_PrintEnumer)
                {
                    printEnumr();
                }
                m_NumberOfModels = m_NumberOfModels + (unsigned long long)pow(2,numOfDontCares);
                res = SolveAndGetResult();      
            }
            // not unsat at the end
            if (res != ToporUnSatRetVal)
            {
                throw runtime_error("Last call wasnt Unsat as expected");
            }
        };

        virtual ~AllSatEnumerTernarySim()
        {
            delete m_TernarySimulation;
        }

    protected:

        vector<SATLIT> m_Inputs;
        TernarySim* m_TernarySimulation;

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
            // TODO check != 0 ?
            if( lit == CONST_LIT_FALSE)
                return 0;
            if( lit == CONST_LIT_TRUE)
                return 1;

            return (abs(lit) - 1);
        }

		void HandleAndGate(AigAndGate gate)
		{
			AIGLIT l = gate.GetL();
			AIGLIT r0 = gate.GetR0();
			AIGLIT r1 = gate.GetR1();

            // write and gate using the index variables
			WriteAnd(AIGLitToSATLit(l), AIGLitToSATLit(r0), AIGLitToSATLit(r1) );
		}

        // return the number of dont cares
        unsigned EnforceSatEnumr()
        {
            unsigned numOfDontCares = 0;
            vector<SATLIT> blockingClause = {};

            // use ternary simulation
            if (m_WithRep)
            {
                vector<pair<AIGLIT, bool>> inputValues(m_Inputs.size());

                transform(m_Inputs.begin(), m_Inputs.end(), inputValues.begin(), [&](const SATLIT inputSatLit) -> pair<AIGLIT, bool>
                {
                    return {AIGIndexToAIGLit(SATLitToAIGIndex(inputSatLit)), m_Solver->GetLitValue(inputSatLit) == TToporLitVal::VAL_SATISFIED};
                });

                //cout << "Before MaximizeDontCare" << endl;
                m_TernarySimulation->MaximizeDontCare(inputValues);
            }

            for (const SATLIT input : m_Inputs)
            {
                if (m_WithRep)
                {
                    TVal val = m_TernarySimulation->GetValForIndex(SATLitToAIGIndex(input));
                    if (val == TVal::True)
                    {
                        blockingClause.push_back(-input);
                    }
                    else if (val == TVal::False)
                    {
                        blockingClause.push_back(input);
                    }
                    else if (val == TVal::DontCare) // dont care case
                    {
                        numOfDontCares++;
                    }
                    else
                    {
                        throw runtime_error("Unkown value for input after maximize dont cares");
                    }
                }
                else // TODO check?
                {
                    // TODO check value is TRUE\FALSE?
                    auto isPos = m_Solver->GetLitValue(input) == TToporLitVal::VAL_SATISFIED;
                    if (isPos)
                    {
                        blockingClause.push_back(-input);
                    }
                    else
                    {
                        blockingClause.push_back(input);
                    }
                }

            }

            m_Solver->AddClause(blockingClause);

            return numOfDontCares;
        }

        void printEnumr()
        {
            for (const SATLIT& input : m_Inputs)
            {
                AIGINDEX litIndex = SATLitToAIGIndex(input); 
                if (m_WithRep)
                {
                    TVal val = m_TernarySimulation->GetValForIndex(litIndex);
                    if (val == TVal::True)
                    {
                        cout << litIndex << " ";
                    }
                    else if (val == TVal::False)
                    {
                        cout << "-" << litIndex << " ";
                    }
                    else if (val == TVal::DontCare) // dont care case
                    {
                        cout << "x ";
                    }
                    else
                    {
                        throw runtime_error("Unkown value for input after maximize dont cares");
                    }
                }
                else // TODO check?
                {
                    // TODO check value is TRUE\FALSE?
                    auto isPos = m_Solver->GetLitValue(input) == TToporLitVal::VAL_SATISFIED;
                    if (isPos)
                    {
                        cout << litIndex << " ";
                    }
                    else
                    {
                        cout << "-" << litIndex << " ";
                    }
                }  
            }
                
            cout << endl;
        }

};
