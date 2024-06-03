#pragma once

#include "AllSatAlgo/AllSatAlgoBase.hpp"
#include "AllSatSolver/Solvers.hpp"
#include "CirSimulation/CirSim.hpp"

/*
    base class for blocking allsat algoirthm
*/
class AllSatAlgoBlockingBase : public AllSatAlgoBase
{
    public:

        AllSatAlgoBlockingBase(const InputParser& inputParser);

        virtual ~AllSatAlgoBlockingBase();

        virtual void InitializeWithAIGFile(const std::string& filename);

        // find all enumeration with general blocking algo
        virtual void FindAllEnumer();

        void PrintResult(bool wasInterrupted = false);

    protected:

        // print initial information, timeout etc..
        virtual void PrintInitialInformation();
        
        INPUT_ASSIGNMENT GeneralizeWithCirSimulation(const INPUT_ASSIGNMENT& model);

        virtual INPUT_ASSIGNMENT GeneralizeModel(const INPUT_ASSIGNMENT& model)
        { 
            throw std::runtime_error("Function not implemented"); 
        };
        
        // implement how to block, dr encoding can have more than one option
        virtual void BlockModel(const INPUT_ASSIGNMENT& model)
        { 
            throw std::runtime_error("Function not implemented"); 
        };

        // *** Params ***

        // if to use simulation
        const bool m_UseCirSim;
        // if to use top to bottom simulation
        const bool m_UseTopToBotSim;
        // if to use dual solver for unsat core
        const bool m_UseDualSolver;
        // if to use literal dropping in unsat core
        const bool m_UseLitDrop;
        // if > 0 use conflict limit when use drop lit in unsat core
        const unsigned m_LitDropConflictLimit;
        // if to check unsat core with each drop lit check
        const bool m_LitDropChekRecurCore;

  
		
        // *** Variables ***
        
        // solver for the original circuit
        AllSatSolverBase* m_Solver;
        // solver for the dual circuit, used for ucore extraction
        AllSatSolverBase* m_DualSolver;
        // cir simulation component
        CirSim* m_CirSimulation;


		// *** Stats ***

};
