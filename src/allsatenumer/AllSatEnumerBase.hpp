#pragma once

#include <vector>

#include "Topor.hpp"
#include "AllSatGloblas.hpp"
#include "AigerParser.hpp"
#include "InputParser.hpp"
#include "TernarySim.hpp"
#include "Utilities.hpp"

using namespace Topor;
using namespace std;

static constexpr int ToporBadRetVal = -1;
static constexpr int ToporSatRetVal = 10;
static constexpr int ToporUnSatRetVal = 20;

static constexpr SATLIT CONST_LIT_TRUE = 1;
static constexpr SATLIT CONST_LIT_FALSE = -1;

static bool CheckTersimDefFromCurrMode(const string& mode)
{
    bool defVal = true;
    if (mode == TERSIM_ALG || mode == COMB_DISJOINT_BLOCK_ALG || mode == COMB_NON_DISJOINT_BLOCK_ALG)
    {
        return true;
    }
    else if (mode == DRMS_DISJOINT_ALG || mode == DRMS_NON_DISJOINT_ALG)
    {
        return false;
    }

    // return default
    return defVal;
}

/*
    base class for allsat
    provide some general functonallity
*/
class AllSatEnumerBase 
{
    public:

        AllSatEnumerBase(const InputParser& inputParser):
        // defualt is true
        m_UseTerSim(inputParser.cmdOptionExists("--no_tersim") ? false : CheckTersimDefFromCurrMode(inputParser.getCmdOption("-mode"))),
        // default is not printing
        m_PrintEnumer(inputParser.cmdOptionExists("--print_models")),
        // default is mode 5
        m_ToporMode(inputParser.getUintCmdOption("-satsolver_mode", 5)),
		m_Solver(nullptr), m_TernarySimulation(nullptr), m_NumberOfAssg(0), m_NumberOfModels(0)
        {
			m_Clk = clock();
            m_Solver = new CTopor();

            m_Solver->SetParam("/verbosity/level",(double)0);
            m_Solver->SetParam("/mode/value",(double)m_ToporMode);
        }

        virtual void InitializeSolver(string filename) { throw runtime_error("Function not implemented"); };

        void FindAllEnumer()
        {
            int res = ToporBadRetVal;
            res = SolveAndGetResult();
   
            while( res == ToporSatRetVal)
            {
                m_NumberOfAssg++; 
                unsigned numOfDontCares = GetBlockingClause();         
                if (m_PrintEnumer)
                {
                    printEnumr();
                }
                m_NumberOfModels = m_NumberOfModels + (unsigned long long)pow(2,numOfDontCares);

                // block with the blocking clause before calling next SAT
                m_Solver->AddClause(m_BlockingClause);

                res = SolveAndGetResult();      
            }

            // not unsat at the end
            if (res != ToporUnSatRetVal)
            {
                throw runtime_error("Last call wasnt UNSAT as expected");
            }
        };

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

        virtual ~AllSatEnumerBase() 
        {
            delete m_Solver;
            delete m_TernarySimulation;
        }

    protected:
        
        // parse aag or aig files
        // initilize m_AigParser
        void ParseAigFile(string filename)
        {
            return_code result;
            
            if (stringEndsWith(filename, ".aag"))
            {
                result = read_ascii_aiger( filename, m_AigParser);
            }
            else if (stringEndsWith(filename, ".aig"))
            {
                result = read_aiger( filename, m_AigParser);
            }
            else
            {
                throw runtime_error("Unkonw aiger format, please provide either .aag or .aig file");
            }

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
        // and initilize m_BlockingClause with the current blocking clause
        virtual unsigned GetBlockingClause() { throw runtime_error("Function not implemented"); return 0;};
		
        // *** Variables ***

        const bool m_UseTerSim;
        const bool m_PrintEnumer;
        const unsigned m_ToporMode;

        AigerParser m_AigParser;
        CTopor* m_Solver;
        TernarySim* m_TernarySimulation;

        vector<SATLIT> m_BlockingClause;

		// *** Stats ***

		clock_t m_Clk;
        // number of satisfiable assignemnts found, assignment can contain dont-cares
        unsigned long long m_NumberOfAssg;
        // number of models (full assignments) found - assignment with x dont cares -> 2^x models
        unsigned long long m_NumberOfModels;
};
