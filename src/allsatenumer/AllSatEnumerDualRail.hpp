#pragma once

#include <vector>

#include "AllSatEnumerBase.hpp"

using namespace Topor;
using namespace std;

class AllSatEnumerDualRail : public AllSatEnumerBase
{
    public:
        AllSatEnumerDualRail(bool useRep, double toporMode, bool printEnumer) : AllSatEnumerBase(useRep, toporMode, printEnumer)
        {

        }

        void InitializeSolver(string filename)
        {
            ParseAigFile(filename);

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
                m_Solver->FixPolarity(-GetPos(drvar));
                m_Solver->FixPolarity(-GetNeg(drvar));

                // and bump score
                m_Solver->BoostScore(abs(GetPos(drvar)));
                m_Solver->BoostScore(abs(GetNeg(drvar)));
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
            
            // SATLIT maxLit = (SATLIT)isIndexRef.size() + 1;
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
                if (m_PrintEnumer)
                {
                    printEnumr();
                }
                unsigned numOfDontCares = EnforceSatEnumr();
                m_NumberOfModels = m_NumberOfModels + (unsigned long long)pow(2,numOfDontCares);
                res = SolveAndGetResult();      
            }
            // not unsat at the end
            if (res != ToporUnSatRetVal)
            {
                throw runtime_error("Last call wasnt Unsat as expected");
            }
        };

        virtual ~AllSatEnumerDualRail(){}

    protected:

        vector<DRVAR> m_InputsDR;

        // Get the two var represent the var_pos and var_neg from AIGLIT
        // if even i.e. 2 return 2,3
		// else odd i.e. 3 return 3,2
        DRVAR GetDualFromAigLit(const AIGLIT& var)
        {
			// even
			if( (var & 1) == 0)
            	return {var, var + 1};
			else // odd
				return {var, var - 1};
        }

		static constexpr SATLIT GetPos(const DRVAR& dvar)
		{
			return dvar.first;
		}

		static constexpr SATLIT GetNeg(const DRVAR& dvar)
		{
			return dvar.second;
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
        unsigned EnforceSatEnumr()
        {
            unsigned numOfDontCares = 0;
            vector<SATLIT> blockingClause = {};

            for (const DRVAR& inputdr : m_InputsDR)
            {
                auto const &[var_pos, var_neg] = inputdr;
                auto isPos = m_Solver->GetLitValue(var_pos) == TToporLitVal::VAL_SATISFIED;
                auto isNeg = m_Solver->GetLitValue(var_neg) == TToporLitVal::VAL_SATISFIED;

                if (isPos)
                {
                    blockingClause.push_back( m_WithRep ? -var_pos : var_neg);
                }
                else if (isNeg)
                {
                    blockingClause.push_back( m_WithRep ? -var_neg : var_pos);
                }
                else // dont care case
                {
                    numOfDontCares++;
                }

            }

            m_Solver->AddClause(blockingClause);

            return numOfDontCares;
        }

        void printEnumr()
        {
            for (const DRVAR& inputdr : m_InputsDR)
            {
                auto const &[var_pos, var_neg] = inputdr;
                auto is_pos = m_Solver->GetLitValue(var_pos) == TToporLitVal::VAL_SATISFIED;
                auto is_neg = m_Solver->GetLitValue(var_neg) == TToporLitVal::VAL_SATISFIED;

                // TODO here handle the case inputdr is inserted negated? -> shouldnt happen
                // var_pos is the aig lit 

                auto lit_index = var_pos/2;

                if (is_pos)
                {
                    cout << lit_index << " ";
                }
                else if (is_neg)
                {
                    cout << -lit_index << " ";
                }
                else
                { // Dont care case
                    cout << "x ";
                }    
            }
                
            cout << endl;
        }

};
