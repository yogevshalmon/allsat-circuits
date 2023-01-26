#pragma once

#include <vector>

#include "AllSatEnumerBase.hpp"

using namespace Topor;
using namespace std;

class AllSatEnumerDualRail : public AllSatEnumerBase
{
    public:
        AllSatEnumerDualRail(const InputParser& inputParser) : AllSatEnumerBase(inputParser) ,
        // defualt is false
        m_BlockNoRep(inputParser.cmdOptionExists("--dr_block_no_rep")),
        // default is true
        m_DoForcePol(!inputParser.cmdOptionExists("--dr_no_force_pol")),
        m_DoBoost(!inputParser.cmdOptionExists("--dr_no_boost"))
        {

        }

        void InitializeSolver(string filename)
        {
            ParseAigFile(filename);

            if (m_UseTerSim)
            {
                m_TernarySimulation = new TernarySim(m_AigParser);
            }

            m_Solver->AddClause(CONST_LIT_TRUE);

            for(const AigAndGate& gate : m_AigParser.GetAndGated())
            {
                HandleAndGate(gate);
            }
            
            // go over the ref indexes and create a blocking clause for 1,1 case
            const vector<bool>& isIndexRef = m_AigParser.GetIsIndexRef();
            for(size_t i = 1; i < isIndexRef.size(); i++)
            {
               if (isIndexRef[i])
               {
                   // each index represent the aig var meaning we need to *2
                   DRVAR dvar = GetDualFromAigLit((AIGLIT)i * 2);
                   m_Solver->AddClause({-GetPos(dvar),-GetNeg(dvar)});
               }
            }

            const vector<AIGLIT>& inputs = m_AigParser.GetInputs();

            m_InputsDR.resize(inputs.size());

            transform(inputs.begin(), inputs.end(), m_InputsDR.begin(), [&](const AIGLIT& aiglit) -> DRVAR
            {
                return GetDualFromAigLit(aiglit);
            });

            for(const DRVAR& drvar: m_InputsDR)
            {
                // the fix polarity make max-sat approximation
                if (m_DoForcePol)
                {                  
                    m_Solver->FixPolarity(-GetPos(drvar));
                    m_Solver->FixPolarity(-GetNeg(drvar));
                }

                // and bump score
                if (m_DoBoost)
                {
                    m_Solver->BoostScore(abs(GetPos(drvar)));
                    m_Solver->BoostScore(abs(GetNeg(drvar)));
                }
            }
            
             //TODO outputs should be size 1 ?
            // Get all the the Output
            const vector<AIGLIT>& outputs = m_AigParser.GetOutputs();
            // TODO can assert different things on the output

            for(AIGLIT aigOutput : outputs)
            {
                DRVAR outdr = GetDualFromAigLit(aigOutput);
                // assert that the dr encoding is pos = 1, neg = 0
                m_Solver->AddClause(GetPos(outdr));
                m_Solver->AddClause(-GetNeg(outdr));
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

        virtual ~AllSatEnumerDualRail(){}

    protected:

        // Get the two var represent the var_pos and var_neg from AIGLIT
        // if even i.e. 2 return 2,3
		// else odd i.e. 3 return 3,2
        DRVAR GetDualFromAigLit(const AIGLIT& var)
        {
            // special case for constant TRUE\FALSE
            if( var == 0)
                return {CONST_LIT_FALSE, CONST_LIT_TRUE};
            if( var == 1)
                return {CONST_LIT_TRUE, CONST_LIT_FALSE};
			// even
			if( (var & 1) == 0)
            	return {var, var + 1};
			else // odd
				return {var, var - 1};
        }

        static AIGLIT DRToAIGLit(const DRVAR& drvar)
        {
            return (AIGLIT)GetPos(drvar);
        }

        static AIGINDEX DRToAIGIndex(const DRVAR& drvar)
        {
            return AIGLitToAIGIndex(((AIGLIT)GetPos(drvar)));
        }

		static constexpr SATLIT GetPos(const DRVAR& dvar)
		{
			return dvar.first;
		}

		static constexpr SATLIT GetNeg(const DRVAR& dvar)
		{
			return dvar.second;
		}

        TVal GetTValFromDR(const DRVAR& dvar)
        {
            bool isPos = m_Solver->GetLitValue(dvar.first) == TToporLitVal::VAL_SATISFIED;
            if (isPos)
                return TVal::True;
            bool isNeg = m_Solver->GetLitValue(dvar.second) == TToporLitVal::VAL_SATISFIED;
            if (isNeg)
                return TVal::False;
            
            // no true no false -> Dont care
            // we block the true & false case
            return TVal::DontCare;
        }

		void HandleAndGate(AigAndGate gate)
		{
			DRVAR l = GetDualFromAigLit(gate.GetL());
			DRVAR r0 = GetDualFromAigLit(gate.GetR0());
			DRVAR r1 = GetDualFromAigLit(gate.GetR1());

			WriteAnd(GetPos(l), GetPos(r0), GetPos(r1) );
			WriteOr(GetNeg(l), GetNeg(r0), GetNeg(r1) );
		}

        // return the number of dont cares
        unsigned GetBlockingClause()
        {
            unsigned numOfDontCares = 0;
            m_BlockingClause.clear();

            if (m_UseTerSim)
            {
                vector<pair<AIGLIT, TVal>> inputValues(m_InputsDR.size());

                transform(m_InputsDR.begin(), m_InputsDR.end(), inputValues.begin(), [&](const DRVAR& inputDR) -> pair<AIGLIT, TVal>
                {
                    return {DRToAIGLit(inputDR), GetTValFromDR(inputDR)};
                });

                //cout << "Before MaximizeDontCare" << endl;
                m_TernarySimulation->MaximizeDontCare(inputValues);
            }

            for (const DRVAR& inputdr : m_InputsDR)
            {
                auto const &[var_pos, var_neg] = inputdr;

                TVal currVal = m_UseTerSim ? m_TernarySimulation->GetValForLit(DRToAIGLit(inputdr)) : GetTValFromDR(inputdr);

                if (currVal == TVal::True)
                {
                    m_BlockingClause.push_back( !m_BlockNoRep ? -var_pos : var_neg);
                }
                else if (currVal == TVal::False)
                {
                    m_BlockingClause.push_back( !m_BlockNoRep ? -var_neg : var_pos);
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
            for (const DRVAR& inputdr : m_InputsDR)
            {          
                TVal currVal = m_UseTerSim ? m_TernarySimulation->GetValForLit(DRToAIGLit(inputdr)) : GetTValFromDR(inputdr);

                // TODO here handle the case inputdr is inserted negated? -> shouldnt happen
                AIGINDEX litIndex = DRToAIGIndex(inputdr);

                if (currVal == TVal::True)
                {
                    cout << litIndex << " ";
                }
                else if (currVal == TVal::False)
                {
                    cout << "-" << litIndex << " ";
                }
                else if (currVal == TVal::DontCare) // dont care case
                {
                    cout << "x ";
                }
                else
                {
                    throw runtime_error("Unkown value for input");
                }   
            }
                
            cout << endl;
        }


    
    const bool m_BlockNoRep;
    const bool m_DoBoost;
    const bool m_DoForcePol;

    vector<DRVAR> m_InputsDR;
};
