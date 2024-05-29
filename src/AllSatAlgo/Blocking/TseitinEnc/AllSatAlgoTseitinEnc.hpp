#pragma once

#include "AllSatAlgo/Blocking/AllSatAlgoBlockingBase.hpp"

/*
    allsat algorithm based on blocking with Tseitin encoding
*/
class AllSatAlgoTseitinEnc : public AllSatAlgoBlockingBase
{
    public:

        AllSatAlgoTseitinEnc(const InputParser& inputParser);

        ~AllSatAlgoTseitinEnc();

    protected:

        // print initial information, timeout etc..
        virtual void PrintInitialInformation();
        
        INPUT_ASSIGNMENT GeneralizeModel(const INPUT_ASSIGNMENT& model) override ;

        void BlockModel(const INPUT_ASSIGNMENT& model) override ;
    
        // *** Params ***

        const bool m_UseIpaisrAsPrimary;
        const bool m_UseIpaisrAsDual;

        // *** Variables ***


		// *** Stats ***

};
