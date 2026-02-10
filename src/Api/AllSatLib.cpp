#include "allsat/AllSatLib.hpp"

#include <sstream>

#include "AllSatAlgo/Blocking/TseitinEnc/AllSatAlgoTseitinEnc.hpp"
#include "AllSatAlgo/Blocking/DualRailEnc/AllSatAlgoDualRailEnc.hpp"

namespace allsat
{

static void AddBoolOption(std::vector<std::string>& tokens, const std::string& key, bool value, bool defaultValue)
{
    if (value != defaultValue)
    {
        tokens.push_back(key);
        tokens.push_back(value ? "1" : "0");
    }
}

static void AddUIntOption(std::vector<std::string>& tokens, const std::string& key, unsigned value, unsigned defaultValue)
{
    if (value != defaultValue)
    {
        tokens.push_back(key);
        tokens.push_back(std::to_string(value));
    }
}

static void AddStringOption(std::vector<std::string>& tokens, const std::string& key, const std::string& value)
{
    if (!value.empty())
    {
        tokens.push_back(key);
        tokens.push_back(value);
    }
}

static std::string JoinProjectionIndices(const std::vector<AIGINDEX>& indices)
{
    if (indices.empty())
    {
        return std::string();
    }

    std::ostringstream oss;
    for (size_t i = 0; i < indices.size(); ++i)
    {
        if (i != 0)
        {
            oss << ",";
        }
        oss << indices[i];
    }
    return oss.str();
}

struct ResolvedOptions
{
    EnumerateOptions::Encoding encoding = EnumerateOptions::Encoding::Tseitin;

    bool useCirSim = false;
    bool useTopToBottomSim = false;
    bool useUcore = false;
    bool useLitDrop = true;
    unsigned litDropConflictLimit = 0;
    bool useLitDropRecur = false;

    bool dualBlockNoRep = false;
    bool dualForcePolarity = false;
    bool dualBoostScore = false;
    bool dualUseTseitinForDual = false;

    bool useIpasirForPlain = false;
    bool useIpasirForDual = true;

    bool printEnumerations = false;
    bool printInfo = false;
    bool useTimeout = false;
    unsigned timeoutSeconds = 3600;

    std::vector<AIGINDEX> projectionIndices;
};

static void ApplyPreset(ResolvedOptions& options, EnumerateOptions::Preset preset)
{
    switch (preset)
    {
        case EnumerateOptions::Preset::None:
            break;
        case EnumerateOptions::Preset::Tale:
            options.useCirSim = true;
            break;
        case EnumerateOptions::Preset::MarsDisjoint:
            options.encoding = EnumerateOptions::Encoding::DualRail;
            options.dualBoostScore = true;
            options.dualForcePolarity = true;
            options.dualBlockNoRep = true;
            break;
        case EnumerateOptions::Preset::MarsNonDisjoint:
            options.encoding = EnumerateOptions::Encoding::DualRail;
            options.dualBoostScore = true;
            options.dualForcePolarity = true;
            options.dualBlockNoRep = false;
            break;
        case EnumerateOptions::Preset::Duty:
            options.encoding = EnumerateOptions::Encoding::DualRail;
            options.dualBoostScore = true;
            options.dualForcePolarity = true;
            options.dualBlockNoRep = true;
            options.useCirSim = true;
            break;
        case EnumerateOptions::Preset::Core:
            options.useUcore = true;
            break;
        case EnumerateOptions::Preset::Roc:
            options.useCirSim = true;
            options.useUcore = true;
            break;
        case EnumerateOptions::Preset::Carma:
            options.encoding = EnumerateOptions::Encoding::DualRail;
            options.dualBoostScore = true;
            options.dualForcePolarity = true;
            options.dualBlockNoRep = true;
            options.useUcore = true;
            options.dualUseTseitinForDual = true;
            break;
    }
}

static ResolvedOptions ResolveOptions(const EnumerateOptions& options)
{
    ResolvedOptions resolved;
    ApplyPreset(resolved, options.preset);

    if (options.encoding.has_value())
    {
        resolved.encoding = *options.encoding;
    }
    if (options.useCirSim.has_value())
    {
        resolved.useCirSim = *options.useCirSim;
    }
    if (options.useTopToBottomSim.has_value())
    {
        resolved.useTopToBottomSim = *options.useTopToBottomSim;
    }
    if (options.useUcore.has_value())
    {
        resolved.useUcore = *options.useUcore;
    }
    if (options.useLitDrop.has_value())
    {
        resolved.useLitDrop = *options.useLitDrop;
    }
    if (options.litDropConflictLimit.has_value())
    {
        resolved.litDropConflictLimit = *options.litDropConflictLimit;
    }
    if (options.useLitDropRecur.has_value())
    {
        resolved.useLitDropRecur = *options.useLitDropRecur;
    }

    if (options.dualBlockNoRep.has_value())
    {
        resolved.dualBlockNoRep = *options.dualBlockNoRep;
    }
    if (options.dualForcePolarity.has_value())
    {
        resolved.dualForcePolarity = *options.dualForcePolarity;
    }
    if (options.dualBoostScore.has_value())
    {
        resolved.dualBoostScore = *options.dualBoostScore;
    }
    if (options.dualUseTseitinForDual.has_value())
    {
        resolved.dualUseTseitinForDual = *options.dualUseTseitinForDual;
    }

    if (options.useIpasirForPlain.has_value())
    {
        resolved.useIpasirForPlain = *options.useIpasirForPlain;
    }
    if (options.useIpasirForDual.has_value())
    {
        resolved.useIpasirForDual = *options.useIpasirForDual;
    }

    resolved.printEnumerations = options.printEnumerations;
    resolved.printInfo = options.printInfo;
    resolved.useTimeout = options.useTimeout;
    resolved.timeoutSeconds = options.timeoutSeconds;
    resolved.projectionIndices = options.projectionIndices;

    return resolved;
}

static std::vector<std::string> BuildTokens(const EnumerateOptions& options)
{
    ResolvedOptions resolved = ResolveOptions(options);
    std::vector<std::string> tokens;

    tokens.push_back("/alg");
    tokens.push_back("blocking");

    tokens.push_back("/alg/blocking/enc");
    tokens.push_back(resolved.encoding == EnumerateOptions::Encoding::DualRail ? "dual_rail" : "tseitin");

    AddBoolOption(tokens, "/alg/blocking/use_cirsim", resolved.useCirSim, false);
    AddBoolOption(tokens, "/alg/blocking/use_top_to_bot_sim", resolved.useTopToBottomSim, false);
    AddBoolOption(tokens, "/alg/blocking/use_ucore", resolved.useUcore, false);
    AddBoolOption(tokens, "/alg/blocking/use_lit_drop", resolved.useLitDrop, true);
    AddUIntOption(tokens, "/alg/blocking/lit_drop_conflict_limit", resolved.litDropConflictLimit, 0);
    AddBoolOption(tokens, "/alg/blocking/lit_drop_recur_ucore", resolved.useLitDropRecur, false);

    AddBoolOption(tokens, "/alg/blocking/use_ipasir_for_plain", resolved.useIpasirForPlain, false);
    AddBoolOption(tokens, "/alg/blocking/use_ipasir_for_dual", resolved.useIpasirForDual, true);

    AddBoolOption(tokens, "/alg/blocking/dual_rail/block_no_rep", resolved.dualBlockNoRep, false);
    AddBoolOption(tokens, "/alg/blocking/dual_rail/force_pol", resolved.dualForcePolarity, false);
    AddBoolOption(tokens, "/alg/blocking/dual_rail/boost_score", resolved.dualBoostScore, false);
    AddBoolOption(tokens, "/alg/blocking/dual_rail/use_tseitin_for_dual", resolved.dualUseTseitinForDual, false);

    if (resolved.useTimeout)
    {
        AddUIntOption(tokens, "/general/timeout", resolved.timeoutSeconds, 3600);
    }

    AddBoolOption(tokens, "/general/print_enumer", resolved.printEnumerations, false);
    AddBoolOption(tokens, "/general/print_info", resolved.printInfo, true);

    AddStringOption(tokens, "/general/projection_vars", JoinProjectionIndices(resolved.projectionIndices));

    return tokens;
}

Enumerator::Enumerator(const EnumerateOptions& options)
    : m_Options(options)
{
}

void Enumerator::Initialize(const IAigerView& aiger)
{
    const auto* memAiger = dynamic_cast<const AigerMemory*>(&aiger);
    if (memAiger != nullptr)
    {
        memAiger->Validate();
    }

    std::vector<std::string> tokens = BuildTokens(m_Options);
    m_InputParser = std::make_unique<InputParser>(tokens);

    ResolvedOptions resolved = ResolveOptions(m_Options);

    if (resolved.encoding == EnumerateOptions::Encoding::DualRail)
    {
        m_Algo = std::make_unique<AllSatAlgoDualRailEnc>(*m_InputParser);
    }
    else
    {
        m_Algo = std::make_unique<AllSatAlgoTseitinEnc>(*m_InputParser);
    }

    m_Algo->InitializeWithAIG(aiger);
    m_Algo->BeginEnumeration(false);
}

EnumerateStatus Enumerator::Next(Assignment& outModel)
{
    if (!m_Algo)
    {
        throw std::runtime_error("Enumerator not initialized");
    }

    INPUT_ASSIGNMENT model;
    AllSatAlgoBlockingBase::StepStatus res = m_Algo->NextModel(model);
    if (res == AllSatAlgoBlockingBase::StepStatus::Model)
    {
        outModel = model;
        return EnumerateStatus::Model;
    }
    if (res == AllSatAlgoBlockingBase::StepStatus::Tautology)
    {
        outModel = model;
        return EnumerateStatus::Tautology;
    }
    if (res == AllSatAlgoBlockingBase::StepStatus::Timeout)
    {
        return EnumerateStatus::Timeout;
    }
    if (res == AllSatAlgoBlockingBase::StepStatus::Exhausted)
    {
        return EnumerateStatus::Exhausted;
    }

    throw std::runtime_error("Solver returned unknown status");
}

EnumerateStats Enumerator::GetStats() const
{
    if (!m_Algo)
    {
        throw std::runtime_error("Enumerator not initialized");
    }

    AllSatAlgoBase::AllSatStats stats = m_Algo->GetStats();
    EnumerateStats out;
    out.numberOfAssignments = stats.numberOfAssignments;
    out.numberOfModels = stats.numberOfModels;
    out.timeOnGeneralization = stats.timeOnGeneralization;
    out.avgCardinality = stats.avgCardinality;
    out.cpuTimeSec = stats.cpuTimeSec;
    out.isTimeout = stats.isTimeout;
    return out;
}

} // namespace allsat
