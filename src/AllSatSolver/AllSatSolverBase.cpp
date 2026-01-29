#include "AllSatSolver/AllSatSolverBase.hpp"

using namespace std;


AllSatSolverBase::AllSatSolverBase(const InputParser& inputParser, const CirEncoding& enc, const bool isDual):
// the desire encoding
m_CirEncoding(enc),
m_IsDual(isDual)
{		
}

const CirEncoding& AllSatSolverBase::GetEnc() const
{
    return m_CirEncoding;
}

void AllSatSolverBase::InitializeSolver(const AigerParser& aigeParser)
{ 
    AddClause(CONST_LIT_TRUE);

    for(const AigAndGate& gate : aigeParser.GetAndGated())
    {
        HandleAndGate(gate);
    }

    if (GetEnc() == DUALRAIL_ENC)
    {
        // go over the ref indexes and create a blocking clause for 1,1 case
        const vector<bool>& isIndexRef = aigeParser.GetIsIndexRef();
        for(size_t i = 1; i < isIndexRef.size(); i++)
        {
            if (isIndexRef[i])
            {
                // each index represent the aig var meaning we need to *2
                DRVAR dvar = AIGLitToDR((AIGLIT)i * 2);
                AddClause({-GetPos(dvar),-GetNeg(dvar)});            
            }
        }
    }

    // Get all the the Output
    const vector<AIGLIT>& outputs = aigeParser.GetOutputs();

    if (outputs.size() > 1)
    {
        throw runtime_error("Error, number of outputs should be 1"); 
    }

    for (AIGLIT aigLitOutput : outputs)
    {
        HandleOutPutAssert(aigLitOutput);
    }
}

SOLVER_RET_STATUS AllSatSolverBase::SolveUnderAssump(const INPUT_ASSIGNMENT& assmp)
{
    vector<SATLIT> satLitAssmp;

    switch (m_CirEncoding)
    {
        case TSEITIN_ENC:
        {
            for (const pair<AIGLIT, TVal>& assign : assmp)
            {
                if (assign.second == TVal::True)
                {
                    satLitAssmp.push_back(AIGLitToSATLit(assign.first));
                }
                else if (assign.second == TVal::False)
                {
                    satLitAssmp.push_back(-AIGLitToSATLit(assign.first));
                }
            }

        break;
        }
        case DUALRAIL_ENC:
        {
            for (const pair<AIGLIT, TVal>& assign : assmp)
            {
                DRVAR drVar = AIGLitToDR(assign.first);
                if (assign.second == TVal::True)
                {
                    satLitAssmp.push_back(GetPos(drVar));
                }
                else if (assign.second == TVal::False)
                {
                    satLitAssmp.push_back(GetNeg(drVar));
                }
            }

        break;
        }
        default:
        {
            throw runtime_error("Unkown circuit encoding");

        break;
        }
    }

    return SolveUnderAssump(satLitAssmp);
}


TVal AllSatSolverBase::GetTValFromAIGLit(AIGLIT aigLit) const
{
    TVal val = TVal::UnKown;
    switch (m_CirEncoding)
    {
        case TSEITIN_ENC:
        {
            if (IsSATLitSatisfied(AIGLitToSATLit(aigLit)))
            {
                val = TVal::True;
            }
            else
            {
                val = TVal::False;
            }

        break;
        }
        case DUALRAIL_ENC:
        {
            DRVAR drVar = AIGLitToDR(aigLit);
            bool isPos = IsSATLitSatisfied(GetPos(drVar));
            if (isPos)
            {
                val = TVal::True;
                break;
            }
            bool isNeg = IsSATLitSatisfied(GetNeg(drVar));
            if (isNeg)
            {
                val = TVal::False;
                break;
            }
         
            // no true no false -> Dont care
            // we block the true & false case
            val = TVal::DontCare;

        break;
        }
        default:
        {
            throw runtime_error("Unkown circuit encoding");

        break;
        }
    }

    return val;
}

// used for getting assigment from solver for the circuit inputs
INPUT_ASSIGNMENT AllSatSolverBase::GetAssignmentForAIGLits(const vector<AIGLIT>& aigLits) const
{
    INPUT_ASSIGNMENT assignment(aigLits.size());

    transform(aigLits.begin(), aigLits.end(), assignment.begin(), [&](AIGLIT aigLit) -> pair<AIGLIT, TVal>
    {
        return make_pair(aigLit, GetTValFromAIGLit(aigLit));
    });

    return assignment;
}


INPUT_ASSIGNMENT AllSatSolverBase::GetUnSATCore(const INPUT_ASSIGNMENT& initialValues, bool useLitDrop, int dropt_lit_conflict_limit, bool useRecurUnCore,
                                                const std::unordered_set<AIGLIT>* projectionSet)
{
    // valid return status should be unsat
    SOLVER_RET_STATUS resStatus = UNSAT_RET_STATUS;
    vector<SATLIT> assumpForSolver;
    INPUT_ASSIGNMENT coreValues = {};

    // in case no assumption mean Tautology
    if (initialValues.empty())
    {
        cout << "c Tautology found, no need for dual check." << endl;
        return initialValues;
    }

    
    // copy to another vec
    INPUT_ASSIGNMENT initValuesNoDC = initialValues;
    // remove all dc
    initValuesNoDC.erase(std::remove_if(initValuesNoDC.begin(), initValuesNoDC.end(), [](const pair<AIGLIT, TVal>& assign)
    {
        return assign.second == TVal::DontCare;
    }), initValuesNoDC.end());
        
    switch (m_CirEncoding)
    {
        case TSEITIN_ENC:
        {
            for (const pair<AIGLIT, TVal>& assign : initValuesNoDC)
            {
                if (assign.second == TVal::True)
                {
                    assumpForSolver.push_back(AIGLitToSATLit(assign.first));
                }
                else if (assign.second == TVal::False)
                {
                    assumpForSolver.push_back(-AIGLitToSATLit(assign.first));
                }
            }

        break;
        }
        case DUALRAIL_ENC:
        {
            for (const pair<AIGLIT, TVal>& assign : initValuesNoDC)
            {
                DRVAR drVar = AIGLitToDR(assign.first);
                if (assign.second == TVal::True)
                {
                    assumpForSolver.push_back(GetPos(drVar));
                }
                else if (assign.second == TVal::False)
                {
                    assumpForSolver.push_back(GetNeg(drVar));
                }
            }

        break;
        }
        default:
        {
            throw runtime_error("Unkown circuit encoding");

        break;
        }
    }

    // assumpForSolver corresponds to initValuesNoDC
    resStatus = SolveUnderAssump(assumpForSolver);

    // in case of timeout just return initialValues
    if (resStatus == TIMEOUT_RET_STATUS)
    {
        return initialValues;
    }
    if (resStatus == SAT_RET_STATUS)
    {
        throw runtime_error("UnSAT core call return SAT status");
    }

    // used only if useLitDrop = true
    vector<SATLIT> litDropAsmpForSolver;

    for (size_t assumPos = 0; assumPos < assumpForSolver.size(); assumPos++)
    {
        if (IsAssumptionRequired(assumPos))
        {
            // assumpForSolver corresponds to initValuesNoDC
            coreValues.push_back(initValuesNoDC[assumPos]);
            litDropAsmpForSolver.push_back(assumpForSolver[assumPos]);
        }
    }

    // try to drop literals from the unSAT core and check if still Unsat
    if (useLitDrop)
    {
        // Helper lambda to check if a variable is a projection variable
        auto isProjectionVar = [&projectionSet](AIGLIT lit) -> bool {
            if (projectionSet == nullptr) return true; // no projection, treat all as projection
            return projectionSet->find(lit) != projectionSet->end();
        };

        // Build iteration order: only projection vars (we only care about making these DC)
        // Non-projection vars are kept in core but we don't try to drop them
        vector<int> iterationOrder;
        for (int i = (int)coreValues.size() - 1; i >= 0; --i)
        {
            // Only try to drop projection variables
            // Non-projection vars don't affect blocking, so no need to try dropping them
            if (isProjectionVar(coreValues[i].first))
            {
                iterationOrder.push_back(i);
            }
        }

        // Track which indices have been removed
        vector<bool> removed(coreValues.size(), false);

        for (int assumpIndex : iterationOrder) 
        {
            // Skip if already removed
            if (removed[assumpIndex])
            {
                continue;
            }

            // Build assumption vector without this literal
            vector<SATLIT> testAssump;
            for (size_t i = 0; i < litDropAsmpForSolver.size(); ++i)
            {
                if (i != (size_t)assumpIndex && !removed[i])
                {
                    testAssump.push_back(litDropAsmpForSolver[i]);
                }
            }

            if (dropt_lit_conflict_limit > 0)
            {
                SetConflictLimit(dropt_lit_conflict_limit);
            }

            resStatus = SolveUnderAssump(testAssump);

            // in case of timeout, exit and then return the current core
            if (resStatus == TIMEOUT_RET_STATUS)
            {
                break;
            }
           
            if (resStatus == UNSAT_RET_STATUS) 
            {
                // still unsat we can remove the correspond lit assignment from the core
                removed[assumpIndex] = true;

                if (useRecurUnCore)
                {
                    // check recursive the UnsatCore - mark removed any not in new core
                    for (size_t newCoreassumPos = 0; newCoreassumPos < litDropAsmpForSolver.size(); ++newCoreassumPos)
                    {
                        if (!removed[newCoreassumPos] && !IsAssumptionRequired(newCoreassumPos))
                        {
                            removed[newCoreassumPos] = true;
                        }
                    }
                }
            } 
            else if (resStatus != SAT_RET_STATUS) 
            {
                throw runtime_error("UnSAT core drop literal strategy return unkown status");
            }
            // SAT_RET_STATUS: we can not remove the lit from the core, keep it
        }

        // Build final coreValues from non-removed entries
        INPUT_ASSIGNMENT finalCoreValues;
        for (size_t i = 0; i < coreValues.size(); ++i)
        {
            if (!removed[i])
            {
                finalCoreValues.push_back(coreValues[i]);
            }
        }
        coreValues = finalCoreValues;
    }
    
    return coreValues;
}

void AllSatSolverBase::BlockAssignment(const INPUT_ASSIGNMENT& assignment, bool blockNoRep)
{
    if (m_CirEncoding == TSEITIN_ENC && blockNoRep)
    {
        throw runtime_error("Tseitin encoding not support no reptition blocking");
    }

    vector<SATLIT> blockingClause;

    switch (m_CirEncoding)
    {
        case TSEITIN_ENC:
        {
            for (const pair<AIGLIT, TVal>& assign : assignment)
            {
                if (assign.second == TVal::True)
                {
                    blockingClause.push_back(-AIGLitToSATLit(assign.first));
                }
                else if (assign.second == TVal::False)
                {
                    blockingClause.push_back(AIGLitToSATLit(assign.first));
                }
            }

        break;
        }
        case DUALRAIL_ENC:
        {
            for (const pair<AIGLIT, TVal>& assign : assignment)
            {
                DRVAR drVar = AIGLitToDR(assign.first);
                if (assign.second == TVal::True)
                {
                    blockingClause.push_back( !blockNoRep ? -GetPos(drVar) : GetNeg(drVar));
                }
                else if (assign.second == TVal::False)
                {
                    blockingClause.push_back( !blockNoRep ? -GetNeg(drVar) : GetPos(drVar));
                }
            }

        break;
        }
        default:
        {
            throw runtime_error("Unkown circuit encoding");

        break;
        }
    }

    // if assignment is empty add empty clause
    AddClause(blockingClause);
}


void AllSatSolverBase::WriteAnd(SATLIT l, SATLIT r1, SATLIT r2)
{
    AddClause({l, -r1, -r2});
    AddClause({-l, r1});
    AddClause({-l, r2}); 
}


void AllSatSolverBase::WriteOr(SATLIT l, SATLIT r1, SATLIT r2)
{
    WriteAnd(-l, -r1, -r2);
}

void AllSatSolverBase::HandleAndGate(const AigAndGate& gate)
{
    AIGLIT l = gate.GetL();
    AIGLIT r0 = gate.GetR0();
    AIGLIT r1 = gate.GetR1();

    switch (m_CirEncoding)
    {
        case TSEITIN_ENC:
        {
            // write and gate using the index variables
            WriteAnd(AIGLitToSATLit(l), AIGLitToSATLit(r0), AIGLitToSATLit(r1));

        break;
        }
        case DUALRAIL_ENC:
        {
            DRVAR drL = AIGLitToDR(l);
            DRVAR drR0 = AIGLitToDR(r0);
            DRVAR drR1 = AIGLitToDR(r1);

            WriteAnd(GetPos(drL), GetPos(drR0), GetPos(drR1) );
            WriteOr(GetNeg(drL), GetNeg(drR0), GetNeg(drR1) );

        break;
        }
        default:
        {
            throw runtime_error("Unkown circuit encoding");

        break;
        }
    }
}

void AllSatSolverBase::HandleOutPutAssert(AIGLIT outLit)
{
    switch (m_CirEncoding)
    {
        case TSEITIN_ENC:
        {
            SATLIT out = AIGLitToSATLit(outLit);
            if (!m_IsDual)
            {
                AddClause(out);
            }
            else
            {
                AddClause(-out);
            }
            
        break;
        }
        case DUALRAIL_ENC:
        {
            DRVAR outdr = AIGLitToDR(outLit);
            if (!m_IsDual)
            {
                AddClause(GetPos(outdr));
                AddClause(-GetNeg(outdr));
            }
            else
            {
                AddClause(-GetPos(outdr));
                AddClause(GetNeg(outdr));
            }

        break;
        }
        default:
        {
            throw runtime_error("Unkown cir encoding");

        break;
        }
    }
}