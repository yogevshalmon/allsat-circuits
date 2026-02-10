#include "AllSatAlgo/Blocking/TseitinEnc/AllSatAlgoTseitinEnc.hpp"

using namespace std;

AllSatAlgoTseitinEnc::AllSatAlgoTseitinEnc(const AllSatConfig& config):
AllSatAlgoBlockingBase(config),
m_UseIpaisrAsPrimary(config.useIpasirForPlain),
m_UseIpaisrAsDual(config.useIpasirForDual)
{
    if (m_UseIpaisrAsPrimary)
    {
        m_Solver = new AllSatSolverIpasir(config, CirEncoding::TSEITIN_ENC, false);
    }
    else
    {
        m_Solver = new AllSatSolverTopor(config, CirEncoding::TSEITIN_ENC, false);
    }

    
    if (m_UseDualSolver)
    {
        if (m_UseIpaisrAsDual)
        {
            m_DualSolver = new AllSatSolverIpasir(config, CirEncoding::TSEITIN_ENC, true);
        }
        else
        {
            m_DualSolver = new AllSatSolverTopor(config, CirEncoding::TSEITIN_ENC, true);
        }  
    } 
}

AllSatAlgoTseitinEnc::~AllSatAlgoTseitinEnc()
{

}

void AllSatAlgoTseitinEnc::PrintInitialInformation()
{
    if (!m_PrintInfo)
    {
        return;
    }
    AllSatAlgoBlockingBase::PrintInitialInformation();

    cout << "c Use Tseitin encoding" << endl;   
}

INPUT_ASSIGNMENT AllSatAlgoTseitinEnc::GeneralizeModel(const INPUT_ASSIGNMENT& model)
{ 
    INPUT_ASSIGNMENT generalizeModel = model;
    if (m_UseCirSim)
    {
        generalizeModel = GeneralizeWithCirSimulation(generalizeModel);
    }
    if (m_UseDualSolver)
    {
        // Pass projection set to prioritize keeping projection vars in core
        generalizeModel = m_DualSolver->GetUnSATCore(generalizeModel, m_UseLitDrop, m_LitDropConflictLimit, m_LitDropChekRecurCore,
                                                     m_UseProjection ? &m_ProjectionSet : nullptr);
    }
    return generalizeModel;
};

void AllSatAlgoTseitinEnc::BlockModel(const INPUT_ASSIGNMENT& model)
{ 
    // For projected enumeration, only block on projection variables
    INPUT_ASSIGNMENT blockingModel = m_UseProjection ? FilterToProjection(model) : model;
    m_Solver->BlockAssignment(blockingModel);
};