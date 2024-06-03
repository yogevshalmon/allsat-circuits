#include "AllSatAlgo/Blocking/TseitinEnc/AllSatAlgoTseitinEnc.hpp"

using namespace std;

AllSatAlgoTseitinEnc::AllSatAlgoTseitinEnc(const InputParser& inputParser):
AllSatAlgoBlockingBase(inputParser),
m_UseIpaisrAsPrimary(inputParser.getBoolCmdOption("/alg/blocking/use_ipasir_for_plain", false)),
m_UseIpaisrAsDual(inputParser.getBoolCmdOption("/alg/blocking/use_ipasir_for_dual", true))
{
    if (m_UseIpaisrAsPrimary)
    {
        m_Solver = new AllSatSolverIpasir(inputParser, CirEncoding::TSEITIN_ENC, false);
    }
    else
    {
        m_Solver = new AllSatSolverTopor(inputParser, CirEncoding::TSEITIN_ENC, false);
    }

    
    if (m_UseDualSolver)
    {
        if (m_UseIpaisrAsDual)
        {
            m_DualSolver = new AllSatSolverIpasir(inputParser, CirEncoding::TSEITIN_ENC, true);
        }
        else
        {
            m_DualSolver = new AllSatSolverTopor(inputParser, CirEncoding::TSEITIN_ENC, true);
        }  
    } 
}

AllSatAlgoTseitinEnc::~AllSatAlgoTseitinEnc()
{

}

void AllSatAlgoTseitinEnc::PrintInitialInformation()
{
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
        generalizeModel = m_DualSolver->GetUnSATCore(generalizeModel, m_UseLitDrop, m_LitDropConflictLimit, m_LitDropChekRecurCore);
    }
    return generalizeModel;
};

void AllSatAlgoTseitinEnc::BlockModel(const INPUT_ASSIGNMENT& model)
{ 
    m_Solver->BlockAssignment(model);
};