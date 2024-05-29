#pragma once

#include <vector>

#include "AllSatSolver/AllSatSolverBase.hpp"


/*
    base class for allsat solver
    provide some general functonallity
*/
class AllSatSolverIpasir : public AllSatSolverBase
{
    public:

        AllSatSolverIpasir(const InputParser& inputParser, const CirEncoding& enc, const bool isDual);

        virtual ~AllSatSolverIpasir();

        // add clause to solver
        void AddClause(std::vector<SATLIT>& cls);

        // return ipasir status
        virtual SOLVER_RET_STATUS Solve();

        // return ipasir status
        virtual SOLVER_RET_STATUS SolveUnderAssump(std::vector<SATLIT>& assmp);


    protected:

        // check if the sat lit is satisfied, must work at any solver
        virtual bool IsSATLitSatisfied(SATLIT lit) const;

        // check if assumption at pos is required
        virtual bool IsAssumptionRequired(size_t pos);
        
        // *** Params ***

        // if timeout was given
        const bool m_UseTimeOut;
        // timeout
        const double m_TimeOut;

        // *** Variables ***

        void* m_IpasirSolver;

        // this will hold the last assmp used for ucore extraction
        std::vector<SATLIT> lastAssmp;

		// *** Stats ***

};