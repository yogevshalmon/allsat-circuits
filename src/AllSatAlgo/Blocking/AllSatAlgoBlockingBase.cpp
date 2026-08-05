#include "AllSatAlgoBlockingBase.hpp"

using namespace std;


AllSatAlgoBlockingBase::AllSatAlgoBlockingBase(const AllSatConfig& config):
AllSatAlgoBase(config),
// defualt is false
m_UseCirSim(config.useCirSim),
// default is false
m_UseTopToBotSim(config.useTopToBottomSim),
// default is false
m_UseDualSolver(config.useUcore),
// default is true
m_UseLitDrop(config.useLitDrop),
// default is 0 i.e. none
m_LitDropConflictLimit(config.litDropConflictLimit),
// default is false
m_LitDropChekRecurCore(config.useLitDropRecur),
// projection variables indices (empty = no projection)
m_ProjectionIndices(config.projectionIndices),
m_Solver(nullptr), 
m_DualSolver(nullptr), 
m_CirSimulation(nullptr),
m_EnumerationStarted(false),
m_PendingSolveStatus(UNSAT_RET_STATUS)
{
    // no SAT solver backend implements SetConflictLimit, so a non-zero limit would only
    // surface as "Function not implemented" in the middle of the enumeration. Fail here
    // instead, where the message can say what is actually wrong.
    if (m_LitDropConflictLimit > 0)
    {
        throw runtime_error("Conflict limited literal dropping is not supported: "
                            "no SAT solver backend implements a conflict limit, "
                            "please leave lit_drop_conflict_limit at 0");
    }
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

    InitializeFromAiger(GetAigerView());
}

void AllSatAlgoBlockingBase::InitializeWithAIG(const IAigerView& aiger)
{
    SetAigerView(aiger);
    InitializeFromAiger(aiger);
}

void AllSatAlgoBlockingBase::InitializeFromAiger(const IAigerView& aiger)
{
    m_EnumerationStarted = false;
    m_PendingSolveStatus = UNSAT_RET_STATUS;

    m_Inputs = aiger.GetInputs();
    m_InputSize = m_Inputs.size();

    // Initialize projection if specified
    if (!m_ProjectionIndices.empty())
    {
        if (!InitializeProjection(m_ProjectionIndices))
        {
            throw runtime_error("Failed to initialize projection variables");
        }
    }

    // initilize tersim if needed
    if (m_UseCirSim)
    {
        m_CirSimulation = new CirSim(aiger, m_UseTopToBotSim ? SimStrat::TopToBot : SimStrat::BotToTop,
                                     m_UseProjection ? &m_ProjectionSet : nullptr);
    }

    m_Solver->InitializeSolver(aiger);

    if (m_UseDualSolver)
    {
        m_DualSolver->InitializeSolver(aiger);
    }
}

void AllSatAlgoBlockingBase::BeginEnumeration(bool printInitial)
{
    if (printInitial)
    {
        PrintInitialInformation();
    }

    m_EnumerationStarted = true;
    m_PendingSolveStatus = m_Solver->Solve();
}

AllSatAlgoBlockingBase::StepStatus AllSatAlgoBlockingBase::NextModel(INPUT_ASSIGNMENT& outModel)
{
    if (!m_EnumerationStarted)
    {
        throw runtime_error("Enumeration has not been started");
    }

    if (m_PendingSolveStatus == TIMEOUT_RET_STATUS || m_IsTimeOut)
    {
        m_IsTimeOut = true;
        return StepStatus::Timeout;
    }

    if (m_PendingSolveStatus == UNSAT_RET_STATUS)
    {
        return StepStatus::Exhausted;
    }

    if (m_PendingSolveStatus != SAT_RET_STATUS)
    {
        throw runtime_error("Solver returned unknown status");
    }

    INPUT_ASSIGNMENT initialAssignment = m_Solver->GetAssignmentForAIGLits(m_Inputs);

    clock_t beforeGen = clock();
    INPUT_ASSIGNMENT minAssignment = GeneralizeModel(initialAssignment);
    unsigned long genCpuTimeTaken = clock() - beforeGen;
    double genTime = (double)(genCpuTimeTaken) / (double)(CLOCKS_PER_SEC);

    m_TimeOnGeneralization += genTime;

    // if timeout exit skip check for tautology
    if (m_IsTimeOut)
    {
        m_PendingSolveStatus = TIMEOUT_RET_STATUS;
        return StepStatus::Timeout;
    }

    size_t effectiveInputSize = m_UseProjection ? m_ProjectionSize : m_InputSize;
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
        if (m_PrintInfo)
        {
            cout << "c Tautology found" << endl;
        }
        outModel.clear();
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
    m_NumberOfModels = m_NumberOfModels + (unsigned long long)pow(2, currNumOfDC);
    m_NumberOfAssg++;

    if (effectiveInputSize > 0)
    {
        m_DontCarePrecSum += (double)currNumOfDC / (double)effectiveInputSize;
    }

    // block with the blocking clause before calling next SAT
    // For projection: BlockModel should only block on projection variables
    BlockModel(minAssignment);

    m_PendingSolveStatus = m_Solver->Solve();

    if (currNumOfDC == effectiveInputSize)
    {
        return StepStatus::Tautology;
    }

    outModel = m_UseProjection ? FilterToProjection(minAssignment) : minAssignment;
    return StepStatus::Model;
}

void AllSatAlgoBlockingBase::FindAllEnumer()
{
    BeginEnumeration(true);

    INPUT_ASSIGNMENT model;
    StepStatus res = StepStatus::Model;
    while (true)
    {
        res = NextModel(model);
        if (res == StepStatus::Model || res == StepStatus::Tautology)
        {
            continue;
        }
        break;
    }

    if (res == StepStatus::Timeout || m_IsTimeOut)
    {
        if (m_PrintInfo)
        {
            cout << "c TIMEOUT reach" << endl;
        }
        m_IsTimeOut = true;
        return;
    }

    // not unsat at the end
    if (res != StepStatus::Exhausted)
    {
        throw runtime_error("Last call wasnt UNSAT as expected");
    }
};



void AllSatAlgoBlockingBase::PrintInitialInformation()
{
    if (!m_PrintInfo)
    {
        return;
    }
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
