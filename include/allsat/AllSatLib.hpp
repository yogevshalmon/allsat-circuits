#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Aiger/AigerMemory.hpp"
#include "Globals/AllSatGloblas.hpp"
#include "Globals/TernaryVal.hpp"

class AllSatAlgoBlockingBase;

namespace allsat
{

using Assignment = std::vector<std::pair<AIGLIT, TVal>>;

enum class EnumerateStatus
{
    Model,
    Tautology,
    Exhausted,
    Timeout
};

struct EnumerateStats
{
    unsigned long long numberOfAssignments = 0;
    unsigned long long numberOfModels = 0;
    double timeOnGeneralization = 0.0;
    double avgCardinality = 0.0;
    double cpuTimeSec = 0.0;
    bool isTimeout = false;
};

struct EnumerateOptions
{
    enum class Preset
    {
        None,
        Tale,
        MarsDisjoint,
        MarsNonDisjoint,
        Duty,
        Core,
        Roc,
        Carma
    };

    enum class Encoding
    {
        Tseitin,
        DualRail
    };

    Preset preset = Preset::None;

    std::optional<Encoding> encoding;

    std::optional<bool> useCirSim;
    std::optional<bool> useTopToBottomSim;
    std::optional<bool> useUcore;
    std::optional<bool> useLitDrop;
    std::optional<unsigned> litDropConflictLimit;
    std::optional<bool> useLitDropRecur;

    std::optional<bool> dualBlockNoRep;
    std::optional<bool> dualForcePolarity;
    std::optional<bool> dualBoostScore;
    std::optional<bool> dualUseTseitinForDual;

    std::optional<bool> useIpasirForPlain;
    std::optional<bool> useIpasirForDual;

    bool printEnumerations = false;
    bool printInfo = false;
    bool useTimeout = false;
    unsigned timeoutSeconds = 3600;

    std::vector<AIGINDEX> projectionIndices;
};

class AigBuilder
{
public:
    static constexpr AIGLIT kFalse = 0;
    static constexpr AIGLIT kTrue = 1;

    static AIGLIT Neg(AIGLIT lit)
    {
        return (lit ^ 1u);
    }

    AIGLIT AddInput()
    {
        return m_Aiger.AddInput();
    }

    AIGLIT AddAnd(AIGLIT leftLit, AIGLIT rightLit)
    {
        return m_Aiger.AddAnd(leftLit, rightLit);
    }

    void SetOutput(AIGLIT outLit)
    {
        m_Aiger.SetOutput(outLit);
    }

    void Validate() const
    {
        m_Aiger.Validate();
    }

    const IAigerView& GetView() const
    {
        return m_Aiger;
    }

private:
    AigerMemory m_Aiger;
};

class Enumerator
{
public:
    explicit Enumerator(const EnumerateOptions& options);

    ~Enumerator();

    void Initialize(const IAigerView& aiger);

    EnumerateStatus Next(Assignment& outModel);

    EnumerateStats GetStats() const;

private:
    EnumerateOptions m_Options;
    std::unique_ptr<AllSatAlgoBlockingBase> m_Algo;
};

} // namespace allsat
