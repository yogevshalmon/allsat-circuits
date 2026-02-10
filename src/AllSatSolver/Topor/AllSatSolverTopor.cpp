#include "AllSatSolver/Topor/AllSatSolverTopor.hpp"

using namespace std;
using namespace Topor;

AllSatSolverTopor::AllSatSolverTopor(const AllSatConfig& config, const CirEncoding& enc, const bool isDual):
AllSatSolverBase(config, enc, isDual),
// default is mode 5
m_SatSolverMode(config.intelSatMode),
// if timeout was given
m_UseTimeOut(config.useTimeout),
// check if timeout is given in command
m_TimeOut(config.timeoutSeconds),
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