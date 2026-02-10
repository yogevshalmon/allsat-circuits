#include "AllSatSolver/Ipasir/AllSatSolverIpasir.hpp"

#include "Globals/ipasir.h"

using namespace std;

AllSatSolverIpasir::AllSatSolverIpasir(const AllSatConfig& config, const CirEncoding& enc, const bool isDual):
AllSatSolverBase(config, enc, isDual),
// if timeout was given
m_UseTimeOut(config.useTimeout),
// check if timeout is given in command
m_TimeOut(config.timeoutSeconds),
m_IpasirSolver(nullptr)
{
    m_IpasirSolver = ipasir_init();
}

AllSatSolverIpasir::~AllSatSolverIpasir() 
{
    ipasir_release (m_IpasirSolver);
}

void AllSatSolverIpasir::AddClause(vector<SATLIT>& cls)
{
    for (SATLIT lit : cls)
    {
        ipasir_add(m_IpasirSolver, lit);
    }

    ipasir_add(m_IpasirSolver, 0);
}

SOLVER_RET_STATUS AllSatSolverIpasir::Solve()
{
    return ipasir_solve(m_IpasirSolver);
}

SOLVER_RET_STATUS AllSatSolverIpasir::SolveUnderAssump(std::vector<SATLIT>& assmp)
{
    lastAssmp = assmp;
    for (SATLIT lit : assmp)
    {
        ipasir_assume(m_IpasirSolver, lit);
    }

    return ipasir_solve(m_IpasirSolver);
}


bool AllSatSolverIpasir::IsSATLitSatisfied(SATLIT lit) const
{
    return ipasir_val(m_IpasirSolver, lit) > 0;
}

// check if assumption at pos is required
bool AllSatSolverIpasir::IsAssumptionRequired(size_t pos)
{   
    return ipasir_failed(m_IpasirSolver, lastAssmp[pos]) == 1;
}