#pragma once

#include <vector>
#include <string>

#include "Globals/AllSatGloblas.hpp"
#include "Globals/AllSatSolverGloblas.hpp"
#include "Aiger/AigerParser.hpp"
#include "Utilities/InputParser.hpp"

/*
    base class for allsat algoirthm
    provide some general functonallity
*/
class AllSatAlgoBase 
{
    public:

        AllSatAlgoBase(const InputParser& inputParser);

        virtual ~AllSatAlgoBase();

        // intilize with the aiger file
        virtual void InitializeWithAIGFile(const std::string& filename)
        { 
            throw std::runtime_error("Function not implemented"); 
        };

        // find all enumeration 
        virtual void FindAllEnumer()
        { 
            throw std::runtime_error("Function not implemented"); 
        };

        void PrintResult(bool wasInterrupted = false);

    protected:

        // print initial information, timeout etc..
        virtual void PrintInitialInformation();
        
        // parse aag or aig files
        // initilize m_AigParser
        void ParseAigFile(const std::string& filename);

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
        // if timeout was given
        const bool m_UseTimeOut;
        // timeout
        const double m_TimeOut;
		
        // *** Variables ***
        
        // parser for Aiger 
        AigerParser m_AigParser;

        // original inputs
        std::vector<AIGLIT> m_Inputs;
        // size of m_Inputs
        size_t m_InputSize;

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
