#pragma once

#include <vector>
#include <string>
#include <unordered_set>

#include "Globals/AllSatGloblas.hpp"
#include "Globals/AllSatSolverGloblas.hpp"
#include "Aiger/AigerParser.hpp"
#include "Aiger/IAigerView.hpp"
#include "Globals/AllSatConfig.hpp"

/*
    base class for allsat algoirthm
    provide some general functonallity
*/
class AllSatAlgoBase 
{
    public:

        AllSatAlgoBase(const AllSatConfig& config);

        virtual ~AllSatAlgoBase();

        // intilize with the aiger file
        virtual void InitializeWithAIGFile(const std::string& filename)
        { 
            throw std::runtime_error("Function not implemented"); 
        };

        // initialize with an in-memory AIG view
        virtual void InitializeWithAIG(const IAigerView& aiger)
        {
            throw std::runtime_error("Function not implemented");
        };

        // find all enumeration 
        virtual void FindAllEnumer()
        { 
            throw std::runtime_error("Function not implemented"); 
        };

        void PrintResult(bool wasInterrupted = false);

        struct AllSatStats
        {
            unsigned long long numberOfAssignments;
            unsigned long long numberOfModels;
            double timeOnGeneralization;
            double avgCardinality;
            double cpuTimeSec;
            bool isTimeout;
        };

        AllSatStats GetStats() const;

    protected:

        // print initial information, timeout etc..
        virtual void PrintInitialInformation();
        
        // parse aag or aig files
        // initilize m_AigParser
        void ParseAigFile(const std::string& filename);

        void SetAigerView(const IAigerView& aiger);

        const IAigerView& GetAigerView() const;

        // print single model enumeration
        void PrintEnumr(const INPUT_ASSIGNMENT& model);
        
        // print value of a single AIG index
        void PrintIndexVal(const AIGINDEX litIndex, const TVal& currVal);

        // print value of a single AIG lit
        void PrintLitVal(const AIGLIT lit, const TVal& currVal);

        unsigned GetNumOfDCFromInputAssignment(const INPUT_ASSIGNMENT& assignment) const;

        // *** Params ***

        // if to print the enumerated assignments
        const bool m_PrintEnumer;
        // if to print informational messages
        const bool m_PrintInfo;
        // if timeout was given
        const bool m_UseTimeOut;
        // timeout
        const double m_TimeOut;
        // if to use projection (enumerate only subset of inputs)
        bool m_UseProjection;
		
        // *** Variables ***
        
        // parser for Aiger 
        AigerParser m_AigParser;

        // view of the current AIG (file-based or in-memory)
        const IAigerView* m_AigView;

        // original inputs
        std::vector<AIGLIT> m_Inputs;
        // size of m_Inputs
        size_t m_InputSize;
        
        // *** Projection Variables ***
        
        // subset of inputs to project onto (only these are enumerated/blocked)
        std::vector<AIGLIT> m_ProjectionInputs;
        // size of m_ProjectionInputs
        size_t m_ProjectionSize;
        // set for O(1) lookup of projection variables
        std::unordered_set<AIGLIT> m_ProjectionSet;
        
        // *** Projection Helper Methods ***
        
        // check if a variable is in the projection set
        bool IsProjectionVar(AIGLIT lit) const;
        
        // initialize projection from a list of indices
        // throws with the offending index and the valid ones if any index is not an input
        void InitializeProjection(const std::vector<AIGINDEX>& projectionIndices);
        
        // filter assignment to only include projection variables
        INPUT_ASSIGNMENT FilterToProjection(const INPUT_ASSIGNMENT& assignment) const;
        
        // count don't-cares only among projection variables
        unsigned GetNumOfDCFromProjectedAssignment(const INPUT_ASSIGNMENT& assignment) const;
        
        // print only projection variable assignments
        void PrintEnumrProjected(const INPUT_ASSIGNMENT& model);

		// *** Stats ***

		clock_t m_Clk;
        // number of satisfiable assignemnts found, assignment can contain dont-cares
        unsigned long long m_NumberOfAssg;
        // number of models (full assignments) found - assignment with x dont cares -> 2^x models
        unsigned long long m_NumberOfModels;
        // if timeout happend
        bool m_IsTimeOut;
        // time spent on generalization
        double m_TimeOnGeneralization;
        // summing the total dont care precntage for total avg
        double m_DontCarePrecSum;
};
