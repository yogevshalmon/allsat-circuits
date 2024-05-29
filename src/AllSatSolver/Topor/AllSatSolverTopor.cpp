#include "AllSatSolver/Topor/AllSatSolverTopor.hpp"

#include "Globals/AllSatAlgoGlobals.hpp"

using namespace std;
using namespace Topor;

AllSatSolverTopor::AllSatSolverTopor(const InputParser& inputParser, const CirEncoding& enc, const bool isDual):
AllSatSolverBase(inputParser, enc, isDual),
// default is mode 5
m_SatSolverMode(inputParser.getUintCmdOption("/sat_solver/intel_sat/mode", 5)),
// if timeout was given
m_UseTimeOut(inputParser.cmdOptionExists("/general/timeout")),
// check if timeout is given in command
m_TimeOut(inputParser.getUintCmdOption("/general/timeout", DEF_TIMEOUT)),
m_ToporSolver(nullptr)
{
    m_ToporSolver = new CTopor<SOLVER_LIT_SIZE, SOLVER_INDEX_SIZE, SOLVER_COMPRESS>();

    m_ToporSolver->SetParam("/verbosity/level",(double)0);
    m_ToporSolver->SetParam("/mode/value",(double)m_SatSolverMode);

    if (m_UseTimeOut)
    {
        m_ToporSolver->SetParam("/timeout/global",(double)m_TimeOut);
    }
}

AllSatSolverTopor::~AllSatSolverTopor() 
{
    delete m_ToporSolver;
}

void AllSatSolverTopor::AddClause(vector<SATLIT>& cls)
{
    m_ToporSolver->AddClause(cls);
}

SOLVER_RET_STATUS AllSatSolverTopor::Solve()
{
    return GetToporResult(m_ToporSolver->Solve());
}

SOLVER_RET_STATUS AllSatSolverTopor::SolveUnderAssump(std::vector<SATLIT>& assmp)
{
    return GetToporResult(m_ToporSolver->Solve(assmp));
}

void AllSatSolverTopor::FixPolarity(SATLIT lit)
{
    m_ToporSolver->FixPolarity(lit);
}

void AllSatSolverTopor::BoostScore(SATLIT lit)
{
    m_ToporSolver->BoostScore(lit);
}

bool AllSatSolverTopor::IsSATLitSatisfied(SATLIT lit) const
{
    return m_ToporSolver->GetLitValue(lit) == TToporLitVal::VAL_SATISFIED;
}

// check if assumption at pos is required
bool AllSatSolverTopor::IsAssumptionRequired(size_t pos)
{   
    return m_ToporSolver->IsAssumptionRequired(pos);
}