#pragma once

#include <vector>

#include "Topor.hpp"
#include "AllSatGloblas.hpp"
#include "AigerParser.hpp"
#include "InputParser.hpp"

using namespace Topor;
using namespace std;

static constexpr int ToporBadRetVal = -1;
static constexpr int ToporSatRetVal = 10;
static constexpr int ToporUnSatRetVal = 20;

class AllSatEnumerBase 
{
    public:

        AllSatEnumerBase(const InputParser& inputParser):
        // default is with rep
        m_WithRep(!inputParser.cmdOptionExists("--no_rep")),
        // default is not printing
        m_PrintEnumer(inputParser.cmdOptionExists("--print_model")),
        // default is mode 5
        m_ToporMode(inputParser.getUintCmdOption("-topor_mode", 5)),
		m_Solver(nullptr), m_NumberOfAssg(0), m_NumberOfModels(0)
        {
			m_Clk = clock();
            m_Solver = new CTopor();

            m_Solver->SetParam("/verbosity/level",(double)0);
            m_Solver->SetParam("/mode/value",(double)m_ToporMode);
        }

        virtual void InitializeSolver(string filename) { throw runtime_error("Function not implemented"); };

        virtual void FindAllEnumer() { throw runtime_error("Function not implemented"); };

        void PrintResult(bool wasInterrupted = false)
        {
			unsigned long cpu_time =  clock() - m_Clk;
            double Time = (double)(cpu_time)/(double)(CLOCKS_PER_SEC);
            if (wasInterrupted)
            {
                cout << "c *** Interrupted *** " << endl;
            }
            cout << "c Number of assignments: " << m_NumberOfAssg;
            if (wasInterrupted)
            {
                cout << "+";
            }
            cout << endl;
            cout << "c Number of models: " << m_NumberOfModels;
            if (wasInterrupted)
            {
                cout << "+";
            }
            cout << endl;
            cout << "c cpu time (solve)  : " << Time <<" sec" << endl;
        }

        virtual ~AllSatEnumerBase() { delete m_Solver;}

    protected:

        // return the hard clauses and initilize m_NumVars
        void ParseAigFile(string filename)
        {
            return_code result;
            result = read_ascii_aiger( filename, m_AigParser);

            if ( result == return_code::parse_error )
            {
                throw runtime_error("Error parsing the file");
            }

        }

		// write the and operation l = r1 & r2
		void WriteAnd(SATLIT l, SATLIT r1, SATLIT r2)
		{
			m_Solver->AddClause(l, -r1, -r2);
			m_Solver->AddClause(-l, r1);
			m_Solver->AddClause(-l, r2);
		}

		// write the and operation l = r1 | r2
		void WriteOr(SATLIT l, SATLIT r1, SATLIT r2)
		{
			WriteAnd(-l, -r1, -r2);
		}
        
        int SolveAndGetResult()
        {
            TToporReturnVal res = m_Solver->Solve();

            switch (res)
            {
            case Topor::TToporReturnVal::RET_SAT:
                //cout << "c Enumeration found" << endl;
                return ToporSatRetVal;
            case Topor::TToporReturnVal::RET_UNSAT:
                //cout << "UNSATISFIABLE" << endl << "c No more assignments found" << endl;
                return ToporUnSatRetVal;
            case Topor::TToporReturnVal::RET_TIMEOUT_LOCAL:
                cout << "s TIMEOUT_LOCAL" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_CONFLICT_OUT:
                cout << "s CONFLICT_OUT" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_MEM_OUT:
                cout << "s MEMORY_OUT" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_USER_INTERRUPT:
                cout << "s USER_INTERRUPT" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_INDEX_TOO_NARROW:
                cout << "s INDEX_TOO_NARROW" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_PARAM_ERROR:
                cout << "s PARAM_ERROR" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_TIMEOUT_GLOBAL:
                cout << "s TIMEOUT_GLOBAL" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_DRAT_FILE_PROBLEM:
                cout << "s DRAT_FILE_PROBLEM" << endl;
                return ToporBadRetVal;
            case Topor::TToporReturnVal::RET_EXOTIC_ERROR:
                cout << "s EXOTIC_ERROR" << endl;
                return ToporBadRetVal;
            default:
                cout << "s UNEXPECTED_ERROR" << endl;
                return ToporBadRetVal;
            }
        }

        virtual void printEnumr() { throw runtime_error("Function not implemented"); };
        
        // should return the number of dont cares
        virtual unsigned EnforceSatEnumr() { throw runtime_error("Function not implemented"); return 0;};
		
        // *** Variables ***

        bool m_WithRep;
        bool m_PrintEnumer;
        unsigned m_ToporMode;

        AigerParser m_AigParser;
        CTopor* m_Solver;

		// *** Stats ***

		clock_t m_Clk;
        // number of assignemnts found, assignment can contain dont cares
        unsigned long long m_NumberOfAssg;
        // number of models found - assignment with x dont cares -> 2^x models
        unsigned long long m_NumberOfModels;
};
