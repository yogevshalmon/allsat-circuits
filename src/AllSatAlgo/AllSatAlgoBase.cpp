#include "AllSatAlgo/AllSatAlgoBase.hpp"

#include "Utilities/StringUtilities.hpp"

using namespace std;
using namespace lorina;


AllSatAlgoBase::AllSatAlgoBase(const AllSatConfig& config):
// default is not printing
m_PrintEnumer(config.printEnumerations),
// default is printing info
m_PrintInfo(config.printInfo),
// if timeout was given
m_UseTimeOut(config.useTimeout),
// check if timeout is given in command
m_TimeOut(config.timeoutSeconds),
// projection is disabled by default
m_UseProjection(false),
m_AigView(nullptr),
m_NumberOfAssg(0), 
m_NumberOfModels(0), 
m_IsTimeOut(false), 
m_TimeOnGeneralization(0),
m_DontCarePrecSum(0),
m_ProjectionSize(0)
{
    m_Clk = clock();
}

AllSatAlgoBase::~AllSatAlgoBase() 
{

}

void AllSatAlgoBase::PrintInitialInformation()
{
    if (!m_PrintInfo)
    {
        return;
    }
    cout << "c Start enumerating AllSAT" << endl;
    #ifdef DEBUG
        cout << "c Tool is compiled in Debug" << endl;
    #endif

    #ifdef SAT_SOLVER_INDEX_64
        cout << "c Tool is compiled in 64 bit index mode" << endl;
    #endif

    #ifdef SAT_SOLVER_COMPRESS
        cout << "c Tool is compiled in compress mode" << endl;
    #endif
    if (m_UseTimeOut)
    {
        cout << "c Timeout: " << m_TimeOut << " seconds" << endl;
    }  
}


void AllSatAlgoBase::ParseAigFile(const string& filename)
{
    return_code result;
    
    if (stringEndsWith(filename, ".aag"))
    {
        result = read_ascii_aiger(filename, m_AigParser);
    }
    else if (stringEndsWith(filename, ".aig"))
    {
        result = read_aiger(filename, m_AigParser);
    }
    else
    {
        throw runtime_error("Unkonw aiger format, please provide either .aag or .aig file");
    }

    if ( result == return_code::parse_error )
    {
        throw runtime_error("Error parsing the file");
    }

    SetAigerView(m_AigParser);

}

void AllSatAlgoBase::SetAigerView(const IAigerView& aiger)
{
    m_AigView = &aiger;
}

const IAigerView& AllSatAlgoBase::GetAigerView() const
{
    if (m_AigView == nullptr)
    {
        throw runtime_error("AIG view is not initialized");
    }
    return *m_AigView;
}

void AllSatAlgoBase::PrintResult(bool wasInterrupted)
{
    if (!m_PrintInfo)
    {
        return;
    }
    bool isInterrupted = m_IsTimeOut || wasInterrupted;
    unsigned long cpu_time =  clock() - m_Clk;
    double Time = (double)(cpu_time)/(double)(CLOCKS_PER_SEC);
    if (isInterrupted)
    {
        cout << "c *** Interrupted *** " << endl;
    }
    cout << "c Number of assignments: " << m_NumberOfAssg;
    if (isInterrupted)
    {
        cout << "+";
    }
    cout << endl;
    cout << "c Number of models: " << m_NumberOfModels;
    if (isInterrupted)
    {
        cout << "+";
    }
    cout << endl;
    cout << "c Percentage of time spent on generalization: " << (Time > 0.0 ? (m_TimeOnGeneralization / Time) : 0.0);

    cout << endl;
    if (m_NumberOfAssg == 0)
    {
        cout << "c Average Cardinality: 0";
    }
    else
    {
        cout << "c Average Cardinality: " << (1 - m_DontCarePrecSum/(double)m_NumberOfAssg);
    }

    cout << endl;
    cout << "c cpu time : " << Time <<" sec" << endl;
}

AllSatAlgoBase::AllSatStats AllSatAlgoBase::GetStats() const
{
    unsigned long cpu_time = clock() - m_Clk;
    double timeSec = (double)(cpu_time) / (double)(CLOCKS_PER_SEC);
    double avgCardinality = 0.0;
    if (m_NumberOfAssg > 0)
    {
        avgCardinality = 1.0 - (m_DontCarePrecSum / (double)m_NumberOfAssg);
    }

    AllSatStats stats;
    stats.numberOfAssignments = m_NumberOfAssg;
    stats.numberOfModels = m_NumberOfModels;
    stats.timeOnGeneralization = m_TimeOnGeneralization;
    stats.avgCardinality = avgCardinality;
    stats.cpuTimeSec = timeSec;
    stats.isTimeout = m_IsTimeOut;
    return stats;
}

// print value of a single AIG index
// base function for all implementation
void AllSatAlgoBase::PrintIndexVal(const AIGINDEX litIndex, const TVal& currVal)
{
    if (currVal == TVal::True)
    {
        cout << litIndex << " ";
    }
    else if (currVal == TVal::False)
    {
        cout << "-" << litIndex << " ";
    }
    else if (currVal == TVal::DontCare) // dont care case
    {
        // Note: for now print nothing
        //cout << "x ";
    }
    else
    {
        throw runtime_error("Unkown value for input");
    }
}

void AllSatAlgoBase::PrintLitVal(const AIGLIT lit, const TVal& currVal)
{
    if (IsAIGLitNeg(lit))
    {
        PrintIndexVal(AIGLitToAIGIndex(lit), GetTValNeg(currVal));
    }
    else
    {
        PrintIndexVal(AIGLitToAIGIndex(lit), currVal);
    }
}

void AllSatAlgoBase::PrintEnumr(const INPUT_ASSIGNMENT& model)
{   
    // model can be partial assignment, we only print Bool values, aka no DC
    for (const pair<AIGLIT, TVal>& assign : model)
    {
        PrintLitVal(assign.first, assign.second);
    }
    
    cout << endl;
}


unsigned AllSatAlgoBase::GetNumOfDCFromInputAssignment(const INPUT_ASSIGNMENT& assignment) const
{
    // count number of 1/0 values
    int numOfBoolVal = count_if(assignment.begin(), assignment.end(), [](const pair<AIGLIT, TVal>& assign)
    { 
         return ((assign.second == TVal::True) || (assign.second == TVal::False)); 
    });
    assert(numOfBoolVal >= 0);
    return (unsigned)m_InputSize - (unsigned)numOfBoolVal;
}

// *** Projection Methods ***

bool AllSatAlgoBase::IsProjectionVar(AIGLIT lit) const
{
    return m_ProjectionSet.find(lit) != m_ProjectionSet.end();
}

bool AllSatAlgoBase::InitializeProjection(const vector<AIGINDEX>& projectionIndices)
{
    if (projectionIndices.empty())
    {
        m_UseProjection = false;
        return true;
    }
    
    m_ProjectionInputs.clear();
    m_ProjectionSet.clear();
    
    // Build a set of valid input AIGINDEXes for validation
    unordered_set<AIGINDEX> validInputIndices;
    for (AIGLIT lit : m_Inputs)
    {
        validInputIndices.insert(AIGLitToAIGIndex(lit));
    }
    
    for (AIGINDEX aigIndex : projectionIndices)
    {
        // Validate that this AIGINDEX corresponds to an input
        if (validInputIndices.find(aigIndex) == validInputIndices.end())
        {
            cerr << "Error: AIGINDEX " << aigIndex << " is not a valid input." << endl;
            cerr << "Valid input indices are: ";
            for (size_t i = 0; i < m_Inputs.size(); ++i)
            {
                if (i > 0) cerr << ", ";
                cerr << AIGLitToAIGIndex(m_Inputs[i]);
            }
            cerr << endl;
            return false;
        }

        AIGLIT lit = AIGIndexToAIGLit(aigIndex);

        // Avoid duplicates
        if (m_ProjectionSet.find(lit) == m_ProjectionSet.end())
        {
            m_ProjectionInputs.push_back(lit);
            m_ProjectionSet.insert(lit);
        }
    }
    
    if (m_ProjectionInputs.empty())
    {
        cerr << "Error: No valid projection indices provided" << endl;
        return false;
    }
    
    m_ProjectionSize = m_ProjectionInputs.size();
    m_UseProjection = true;
    
    return true;
}

INPUT_ASSIGNMENT AllSatAlgoBase::FilterToProjection(const INPUT_ASSIGNMENT& assignment) const
{
    if (!m_UseProjection)
    {
        return assignment;
    }
    
    INPUT_ASSIGNMENT filtered;
    filtered.reserve(m_ProjectionSize);
    
    for (const pair<AIGLIT, TVal>& assign : assignment)
    {
        if (IsProjectionVar(assign.first))
        {
            filtered.push_back(assign);
        }
    }
    
    return filtered;
}

unsigned AllSatAlgoBase::GetNumOfDCFromProjectedAssignment(const INPUT_ASSIGNMENT& assignment) const
{
    if (!m_UseProjection)
    {
        return GetNumOfDCFromInputAssignment(assignment);
    }
    
    // Count boolean values only among projection variables
    unsigned numOfBoolVal = 0;
    for (const pair<AIGLIT, TVal>& assign : assignment)
    {
        if (IsProjectionVar(assign.first))
        {
            if (assign.second == TVal::True || assign.second == TVal::False)
            {
                numOfBoolVal++;
            }
        }
    }
    
    return (unsigned)m_ProjectionSize - numOfBoolVal;
}

void AllSatAlgoBase::PrintEnumrProjected(const INPUT_ASSIGNMENT& model)
{
    if (!m_UseProjection)
    {
        PrintEnumr(model);
        return;
    }
    
    // Print only projection variables
    for (const pair<AIGLIT, TVal>& assign : model)
    {
        if (IsProjectionVar(assign.first))
        {
            PrintLitVal(assign.first, assign.second);
        }
    }
    
    cout << endl;
}