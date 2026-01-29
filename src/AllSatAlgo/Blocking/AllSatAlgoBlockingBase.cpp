#include "AllSatAlgoBlockingBase.hpp"

using namespace std;


AllSatAlgoBlockingBase::AllSatAlgoBlockingBase(const InputParser& inputParser):
AllSatAlgoBase(inputParser),
// defualt is false
m_UseCirSim(inputParser.getBoolCmdOption("/alg/blocking/use_cirsim", false)),
// default is false
m_UseTopToBotSim(inputParser.getBoolCmdOption("/alg/blocking/use_top_to_bot_sim", false)),
// default is false
m_UseDualSolver(inputParser.getBoolCmdOption("/alg/blocking/use_ucore", false)),
// default is true
m_UseLitDrop(inputParser.getBoolCmdOption("/alg/blocking/use_lit_drop", true)),
// default is 0 i.e. none
m_LitDropConflictLimit(inputParser.getUintCmdOption("/alg/blocking/lit_drop_conflict_limit", 0)),
// default is false
m_LitDropChekRecurCore(inputParser.getBoolCmdOption("/alg/blocking/lit_drop_recur_ucore", false)),
// projection variables as comma-separated indices (empty = no projection)
m_ProjectionVarsStr(inputParser.getCmdOption("/general/projection_vars")),
m_Solver(nullptr), 
m_DualSolver(nullptr), 
m_CirSimulation(nullptr)
{
}

AllSatAlgoBlockingBase::~AllSatAlgoBlockingBase() 
{
    delete m_Solver;
    delete m_DualSolver;
    delete m_CirSimulation;
}

void AllSatAlgoBlockingBase::InitializeWithAIGFile(const string& filename)
{
    ParseAigFile(filename);

    m_Inputs = m_AigParser.GetInputs();
    m_InputSize = m_Inputs.size();

    // Initialize projection if specified
    if (!m_ProjectionVarsStr.empty())
    {
        if (!InitializeProjection(m_ProjectionVarsStr))
        {
            throw runtime_error("Failed to initialize projection variables");
        }
    }

    // initilize tersim if needed
    if (m_UseCirSim)
    {
        m_CirSimulation = new CirSim(m_AigParser, m_UseTopToBotSim ? SimStrat::TopToBot : SimStrat::BotToTop, 
                                     m_UseProjection ? &m_ProjectionSet : nullptr);
    }

    m_Solver->InitializeSolver(m_AigParser);

    if (m_UseDualSolver) m_DualSolver->InitializeSolver(m_AigParser);

}

void AllSatAlgoBlockingBase::FindAllEnumer()
{
    PrintInitialInformation();

    int res = m_Solver->Solve();

    // Determine the effective input size for statistics (projection or all inputs)
    size_t effectiveInputSize = m_UseProjection ? m_ProjectionSize : m_InputSize;

    while( res == SAT_RET_STATUS)
    {
        INPUT_ASSIGNMENT initialAssignment = m_Solver->GetAssignmentForAIGLits(m_Inputs);
        
        clock_t beforeGen = clock();
        INPUT_ASSIGNMENT minAssignment = GeneralizeModel(initialAssignment);
        unsigned long genCpuTimeTaken =  clock() - beforeGen;
        double genTime = (double)(genCpuTimeTaken)/(double)(CLOCKS_PER_SEC);

        m_TimeOnGeneralization += genTime;

        // if timeout exit skip check for tautology
        if (m_IsTimeOut)
        {
            break;
        }

        // Get don't-care count based on projection mode
        unsigned currNumOfDC = m_UseProjection ? 
            GetNumOfDCFromProjectedAssignment(minAssignment) : 
            GetNumOfDCFromInputAssignment(minAssignment); 

        // no blocking clause, all (projected) inputs are DC -> tautology
        if (currNumOfDC == effectiveInputSize)
        {
            if (m_PrintEnumer)
            {
                cout << "s tautology" << endl;
            }
            cout << "c Tautology found" << endl;
        }
        else
        {
            if (m_PrintEnumer)
            {
                if (m_UseProjection)
                {
                    PrintEnumrProjected(minAssignment);
                }
                else
                {
                    PrintEnumr(minAssignment);
                }
            }
        }

        // TODO handle overflow - currently not supported
        m_NumberOfModels = m_NumberOfModels + (unsigned long long)pow(2,currNumOfDC);
        m_NumberOfAssg++;

        m_DontCarePrecSum += (double)currNumOfDC/(double)effectiveInputSize;

  
        // block with the blocking clause before calling next SAT
        // For projection: BlockModel should only block on projection variables
        BlockModel(minAssignment);

        res = m_Solver->Solve();      
    }

    if (res == TIMEOUT_RET_STATUS || m_IsTimeOut)
    {
        cout << "c TIMEOUT reach" << endl;
        m_IsTimeOut = true;
        return;
    }

    // not unsat at the end
    if (res != UNSAT_RET_STATUS)
    {
        throw runtime_error("Last call wasnt UNSAT as expected");
    }
};



void AllSatAlgoBlockingBase::PrintInitialInformation()
{
    AllSatAlgoBase::PrintInitialInformation();

    cout << "c Use Blocking based algorithm" << endl;

    if (m_UseProjection)
    {
        cout << "c Use Projected enumeration with " << m_ProjectionSize << " projection variables" << endl;
    }

    if (m_UseCirSim)
    {
        cout << "c Use Ternary simulation" << endl;
        if (m_UseTopToBotSim)
        {
            cout << "c Use Top to Bottom simulation" << endl;
        }
    }
    if (m_UseDualSolver)
    {
        cout << "c Use dual solver for unSAT-core" << endl;
        if (m_UseLitDrop)
        {
            cout << "c Use literal dropping for unSAT-core" << endl;
            if (m_LitDropConflictLimit > 0)
            {
                cout << "c Limit conflict in literal dropping to " << m_LitDropConflictLimit << endl;
            }
            if (m_LitDropChekRecurCore)
            {
                cout << "c Use recursive unSAT-core check in literal dropping" << endl;
            } 
        }
    }
      
}


INPUT_ASSIGNMENT AllSatAlgoBlockingBase::GeneralizeWithCirSimulation(const INPUT_ASSIGNMENT& model)
{
    // use simulation for maximize dont-care values
    return m_CirSimulation->MaximizeDontCare(model);
}
