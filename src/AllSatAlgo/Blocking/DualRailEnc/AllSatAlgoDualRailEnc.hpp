#pragma once

#include "AllSatAlgo/Blocking/AllSatAlgoBlockingBase.hpp"

/*
    allsat algorithm based on blocking with Tseitin encoding
*/
class AllSatAlgoDualRailEnc : public AllSatAlgoBlockingBase
{
    public:

        AllSatAlgoDualRailEnc(const InputParser& inputParser);

        ~AllSatAlgoDualRailEnc();

        // override since we maybe want to use force polarity and boost score
        virtual void InitializeWithAIGFile(const std::string& filename);

    protected:

        // print initial information, timeout etc..
        virtual void PrintInitialInformation();
        
        INPUT_ASSIGNMENT GeneralizeModel(const INPUT_ASSIGNMENT& model) override ;

        void BlockModel(const INPUT_ASSIGNMENT& model) override ;
    
        // *** Params ***

        // if to block with no rep
        const bool m_BlockNoRep;
        // if to use boost score for the input dr variables
        const bool m_DoBoost;
        // if to use force polarity for the input dr variables
        const bool m_DoForcePol;
        // if to use tseitin encoding for the dual solver
        const bool m_UseTseitinEncForDual;
        // if to use ipasir solver as primary
        const bool m_UseIpaisrAsPrimary;
        // if to use ipasir as dual solver
        const bool m_UseIpaisrAsDual;
		
        // *** Variables ***


		// *** Stats ***

};
