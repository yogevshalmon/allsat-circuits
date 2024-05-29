#include "AllSatAlgo/AllSatAlgoBase.hpp"

#include "Globals/AllSatAlgoGlobals.hpp"
#include "Utilities/StringUtilities.hpp"

using namespace std;
using namespace lorina;


AllSatAlgoBase::AllSatAlgoBase(const InputParser& inputParser):
// default is not printing
m_PrintEnumer(inputParser.getBoolCmdOption("/general/print_enumer", false)),
// if timeout was given
m_UseTimeOut(inputParser.cmdOptionExists("/general/timeout")),
// check if timeout is given in command
m_TimeOut(inputParser.getUintCmdOption("/general/timeout", DEF_TIMEOUT)),
m_NumberOfAssg(0), 
m_NumberOfModels(0), 
m_IsTimeOut(false), 
m_TimeOnGeneralization(0),
m_DontCarePrecSum(0)
{
    m_Clk = clock();
}

AllSatAlgoBase::~AllSatAlgoBase() 
{

}

void AllSatAlgoBase::PrintInitialInformation()
{
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

}

void AllSatAlgoBase::PrintResult(bool wasInterrupted)
{
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
    cout << "c Percentage of time spent on generalization: " << m_TimeOnGeneralization/Time;

    cout << endl;
    cout << "c Average Cardinality: " << (1 - m_DontCarePrecSum/(double)m_NumberOfAssg);

    cout << endl;
    cout << "c cpu time : " << Time <<" sec" << endl;
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