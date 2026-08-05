#include "AllSatAlgo/Blocking/DualRailEnc/AllSatAlgoDualRailEnc.hpp"

using namespace std;

AllSatAlgoDualRailEnc::AllSatAlgoDualRailEnc(const AllSatConfig& config):
AllSatAlgoBlockingBase(config),
// defualt is false
m_BlockNoRep(config.dualBlockNoRep),
// default is false
m_DoForcePol(config.dualForcePolarity),
// default is false
m_DoBoost(config.dualBoostScore),
// default is false
m_UseTseitinEncForDual(config.dualUseTseitinForDual),
m_UseIpaisrAsPrimary(config.useIpasirForPlain),
m_UseIpaisrAsDual(config.useIpasirForDual)
{
    if (m_UseIpaisrAsPrimary)
    {
        m_Solver = new AllSatSolverIpasir(config, CirEncoding::DUALRAIL_ENC, false);
    }
    else
    {
        m_Solver = new AllSatSolverTopor(config, CirEncoding::DUALRAIL_ENC, false);
    }

    if (m_UseDualSolver) 
    {
        if (m_UseIpaisrAsDual)
        {
            m_DualSolver = new AllSatSolverIpasir(config, m_UseTseitinEncForDual ? CirEncoding::TSEITIN_ENC : CirEncoding::DUALRAIL_ENC, true);
        }
        else
        {
            m_DualSolver = new AllSatSolverTopor(config, m_UseTseitinEncForDual ? CirEncoding::TSEITIN_ENC : CirEncoding::DUALRAIL_ENC, true);
        }
    }
}

AllSatAlgoDualRailEnc::~AllSatAlgoDualRailEnc()
{

}

void AllSatAlgoDualRailEnc::InitializeFromAiger(const IAigerView& aiger)
{
    AllSatAlgoBlockingBase::InitializeFromAiger(aiger);

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
    if (!m_PrintInfo)
    {
        return;
    }
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
        // pass the projection set so only the projection literals are dropped from the core
        generalizeModel = m_DualSolver->GetUnSATCore(generalizeModel, m_UseLitDrop, m_LitDropConflictLimit, m_LitDropChekRecurCore,
                                                     m_UseProjection ? &m_ProjectionSet : nullptr);
    }
    return generalizeModel;
};

void AllSatAlgoDualRailEnc::BlockModel(const INPUT_ASSIGNMENT& model)
{ 
    // For projected enumeration, only block on projection variables
    INPUT_ASSIGNMENT blockingModel = m_UseProjection ? FilterToProjection(model) : model;
    m_Solver->BlockAssignment(blockingModel, m_BlockNoRep);
};
