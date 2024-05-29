#include "AllSatAlgo/Blocking/DualRailEnc/AllSatAlgoDualRailEnc.hpp"

using namespace std;

AllSatAlgoDualRailEnc::AllSatAlgoDualRailEnc(const InputParser& inputParser):
AllSatAlgoBlockingBase(inputParser),
// defualt is false
m_BlockNoRep(inputParser.getBoolCmdOption("/alg/blocking/dual_rail/block_no_rep", false)),
// default is false
m_DoForcePol(inputParser.getBoolCmdOption("/alg/blocking/dual_rail/force_pol", false)),
// default is false
m_DoBoost(inputParser.getBoolCmdOption("/alg/blocking/dual_rail/boost_score", false)),
// default is false
m_UseTseitinEncForDual(inputParser.getBoolCmdOption("/alg/blocking/dual_rail/use_tseitin_for_dual", false)),
m_UseIpaisrAsPrimary(inputParser.getBoolCmdOption("/alg/blocking/use_ipasir_for_plain", false)),
m_UseIpaisrAsDual(inputParser.getBoolCmdOption("/alg/blocking/use_ipasir_for_dual", true))
{
    if (m_UseIpaisrAsPrimary)
    {
        m_Solver = new AllSatSolverIpasir(inputParser, CirEncoding::DUALRAIL_ENC, false);
    }
    else
    {
        m_Solver = new AllSatSolverTopor(inputParser, CirEncoding::DUALRAIL_ENC, false);
    }

    if (m_UseDualSolver) 
    {
        if (m_UseIpaisrAsDual)
        {
            m_DualSolver = new AllSatSolverIpasir(inputParser, m_UseTseitinEncForDual ? CirEncoding::TSEITIN_ENC : CirEncoding::DUALRAIL_ENC, true);
        }
        else
        {
            m_DualSolver = new AllSatSolverTopor(inputParser, m_UseTseitinEncForDual ? CirEncoding::TSEITIN_ENC : CirEncoding::DUALRAIL_ENC, true);
        }
    }
}

AllSatAlgoDualRailEnc::~AllSatAlgoDualRailEnc()
{

}

void AllSatAlgoDualRailEnc::InitializeWithAIGFile(const string& filename)
{
    AllSatAlgoBlockingBase::InitializeWithAIGFile(filename);

    for (AIGLIT inputLit: m_Inputs)
    {
        DRVAR inpurDr =  AIGLitToDR(inputLit);
        // the fix polarity make max-sat approximation
        if (m_DoForcePol)
        {                  
            m_Solver->FixPolarity(-GetPos(inpurDr));
            m_Solver->FixPolarity(-GetNeg(inpurDr));
        }

        // and bump score
        if (m_DoBoost)
        {
            m_Solver->BoostScore(abs(GetPos(inpurDr)));
            m_Solver->BoostScore(abs(GetNeg(inpurDr)));
        }
    }

}

void AllSatAlgoDualRailEnc::PrintInitialInformation()
{
    AllSatAlgoBlockingBase::PrintInitialInformation();

    cout << "c Use Dual-Rail encoding" << endl;

    if (m_BlockNoRep)
    {
        cout << "c Block with no reptition" << endl;   
    }
    if (m_DoForcePol)
    {
        cout << "c Use force polarity" << endl; 
    }
    if (m_DoBoost)
    {
        cout << "c Use boost score" << endl; 
    }
    if (m_UseDualSolver && m_UseTseitinEncForDual)
    {
        cout << "c Use Tseitin encoding for dual solver" << endl; 
    }
}

INPUT_ASSIGNMENT AllSatAlgoDualRailEnc::GeneralizeModel(const INPUT_ASSIGNMENT& model)
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

void AllSatAlgoDualRailEnc::BlockModel(const INPUT_ASSIGNMENT& model)
{ 
    m_Solver->BlockAssignment(model, m_BlockNoRep);
};