#pragma once

#include <vector>

#include "AllSatSolver/AllSatSolverBase.hpp"
#include "Globals/ToporGlobal.hpp"


/*
    base class for allsat solver
    provide some general functonallity
*/
class AllSatSolverTopor : public AllSatSolverBase
{
    public:

        AllSatSolverTopor(const InputParser& inputParser, const CirEncoding& enc, const bool isDual);

        virtual ~AllSatSolverTopor();

        // add clause to solver
        void AddClause(std::vector<SATLIT>& cls);

        // return ipasir status
        virtual SOLVER_RET_STATUS Solve();

        // return ipasir status
        virtual SOLVER_RET_STATUS SolveUnderAssump(std::vector<SATLIT>& assmp);

        // fix ploratiy of lit
        virtual void FixPolarity(SATLIT lit);
        // boost score of lit
        virtual void BoostScore(SATLIT lit);

    protected:

        // check if the sat lit is satisfied, must work at any solver
        virtual bool IsSATLitSatisfied(SATLIT lit) const;

        // check if assumption at pos is required
        virtual bool IsAssumptionRequired(size_t pos);
        
        // *** Params ***

		// sat solver mode
        const unsigned m_SatSolverMode;
        // if timeout was given
        const bool m_UseTimeOut;
        // timeout
        const double m_TimeOut;

        // *** Variables ***

        Topor::CTopor<SOLVER_LIT_SIZE, SOLVER_INDEX_SIZE, SOLVER_COMPRESS>* m_ToporSolver;

		// *** Stats ***

};