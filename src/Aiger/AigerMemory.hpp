#pragma once

#include <stdexcept>
#include <vector>

#include "Aiger/IAigerView.hpp"

class AigerMemory : public IAigerView
{
public:
    AigerMemory()
        : m_NextIndex(1)
    {
        m_IsVarRef.resize(1, false);
        m_IsVarRef[0] = true;
    }

    AIGLIT AddInput()
    {
        AIGINDEX index = m_NextIndex++;
        EnsureIndexCapacity(index);

        AIGLIT lit = AIGIndexToAIGLit(index);
        m_Inputs.push_back(lit);
        m_IsVarRef[index] = true;
        return lit;
    }

    AIGLIT AddAnd(AIGLIT leftLit, AIGLIT rightLit)
    {
        ValidateKnownLit(leftLit);
        ValidateKnownLit(rightLit);

        AIGINDEX index = m_NextIndex++;
        EnsureIndexCapacity(index);

        AIGLIT outLit = AIGIndexToAIGLit(index);
        m_AndGates.push_back(AigAndGate(outLit, leftLit, rightLit));
        m_IsVarRef[index] = true;
        return outLit;
    }

    void SetOutput(AIGLIT outLit)
    {
        ValidateKnownLit(outLit);
        if (!m_Outputs.empty())
        {
            throw std::runtime_error("Only a single output is supported");
        }
        m_Outputs.push_back(outLit);
    }

    void ClearOutput()
    {
        m_Outputs.clear();
    }

    bool HasOutput() const
    {
        return !m_Outputs.empty();
    }

    void Validate() const
    {
        if (m_Inputs.empty())
        {
            throw std::runtime_error("At least one input is required");
        }
        if (m_Outputs.size() != 1)
        {
            throw std::runtime_error("Exactly one output is required");
        }
    }

    const std::vector<AIGLIT>& GetInputs() const override { return m_Inputs; }
    const std::vector<AIGLIT>& GetOutputs() const override { return m_Outputs; }
    const std::vector<AigAndGate>& GetAndGated() const override { return m_AndGates; }
    const std::vector<bool>& GetIsIndexRef() const override { return m_IsVarRef; }
    const AIGINDEX GetMaxIndex() const override { return (AIGINDEX)m_IsVarRef.size(); }

private:
    void EnsureIndexCapacity(AIGINDEX index)
    {
        if (index >= m_IsVarRef.size())
        {
            m_IsVarRef.resize(index + 1, false);
        }
    }

    void ValidateKnownLit(AIGLIT lit) const
    {
        AIGINDEX index = AIGLitToAIGIndex(lit);
        if (index >= m_IsVarRef.size())
        {
            throw std::runtime_error("AIG literal references an unknown index");
        }
    }

    AIGINDEX m_NextIndex;
    std::vector<AIGLIT> m_Inputs;
    std::vector<AIGLIT> m_Outputs;
    std::vector<AigAndGate> m_AndGates;
    std::vector<bool> m_IsVarRef;
};
