#pragma once

#include <vector>

#include "Globals/AllSatGloblas.hpp"
#include "Globals/AllSatSolverGloblas.hpp"
#include "Aiger/AigerParser.hpp"
#include "Utilities/InputParser.hpp"

/*
    base class for allsat solver
    provide some general functonallity
*/
class AllSatSolverBase 
{
    public:

        AllSatSolverBase(const InputParser& inputParser, const CirEncoding& enc, const bool isDual);

        virtual ~AllSatSolverBase() 
        {
        }

        // Note: const not compiling because of intelSAT
        virtual void AddClause(std::vector<SATLIT>& cls)
        {
            throw std::runtime_error("Function not implemented");
        }

        void AddClause(SATLIT lit)
        {
            AddClause({lit});
        }

        void AddClause(std::initializer_list<SATLIT> lits) 
        { 
            std::vector<SATLIT> v(lits); return AddClause(v); 
        }

        // initialize solver from aig
        void InitializeSolver(const AigerParser& aigeParser);

        // return ipasir status
        virtual SOLVER_RET_STATUS Solve()
        {
            throw std::runtime_error("Function not implemented");
        }

        // return ipasir status
        virtual SOLVER_RET_STATUS SolveUnderAssump(std::vector<SATLIT>& assmp)
        {
            throw std::runtime_error("Function not implemented");
        }

        // Ignore asssmp of dont care values
        // return ipasir status
        SOLVER_RET_STATUS SolveUnderAssump(const INPUT_ASSIGNMENT& assmp);

        // if conflict_limit > 0 set the conflict limit for the next call
        virtual void SetConflictLimit(int conflict_limit)
        {
           throw std::runtime_error("Function not implemented");
        }

        // Note: does not work for ipasir solvers
        virtual void FixPolarity(SATLIT lit)
        {
           throw std::runtime_error("Function not implemented");
        }

        // Note: does not work for ipasir solvers
        virtual void BoostScore(SATLIT lit)
        {
            throw std::runtime_error("Function not implemented");
        }

        // get the circuit encoding for the current solver
        const CirEncoding& GetEnc() const;

        // value from AIG index, used only on the circuit inputs
        TVal GetTValFromAIGLit(AIGLIT aigLit) const;

        // used for getting assigment from solver for the circuit inputs
        INPUT_ASSIGNMENT GetAssignmentForAIGLits(const std::vector<AIGLIT>& aigLits) const;

        // get unsat core under the assumption of initialValues
        // return result assignment of the core
        // if timeout return initialValues
        // useLitDrop - if to use literal dropping startegy
        // dropt_lit_conflict_limit - limit the conflict limit for each check for drop lit
        // useRecurUnCore - if to use unsat core extraction recursivly with each drop lit check
        INPUT_ASSIGNMENT GetUnSATCore(const INPUT_ASSIGNMENT& initialValues, bool useLitDrop = false, int dropt_lit_conflict_limit = -1, bool useRecurUnCore = false);
        
        // block the current assignment
        // currently blockNoRep only work for dual-rail encoding
        void BlockAssignment(const INPUT_ASSIGNMENT& assignment, bool blockNoRep = false);

    protected:

        // check if the sat lit is satisfied, must work at any solver
        virtual bool IsSATLitSatisfied(SATLIT lit) const
        {
            throw std::runtime_error("Function not implemented");
        }

        // check if assumption at pos is required
        virtual bool IsAssumptionRequired(size_t pos)
        {
            throw std::runtime_error("Function not implemented");
        }

        // write the and operation l = r1 & r2
		void WriteAnd(SATLIT l, SATLIT r1, SATLIT r2);

		// write the and operation l = r1 | r2
		void WriteOr(SATLIT l, SATLIT r1, SATLIT r2);

        void HandleAndGate(const AigAndGate& gate);

        void HandleOutPutAssert(AIGLIT outLit);
        
        // *** Params ***

        // hold the encoding
        const CirEncoding m_CirEncoding;

        // hold if the current solver is dual represntation
        const bool m_IsDual;
		
        // *** Variables ***


		// *** Stats ***

};
