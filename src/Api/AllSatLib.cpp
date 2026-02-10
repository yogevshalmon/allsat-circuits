#include "allsat/AllSatLib.hpp"

#include "AllSatAlgo/Blocking/TseitinEnc/AllSatAlgoTseitinEnc.hpp"
#include "AllSatAlgo/Blocking/DualRailEnc/AllSatAlgoDualRailEnc.hpp"
#include "Globals/AllSatConfig.hpp"
#include "Globals/AllSatPresets.hpp"

namespace allsat
{

static AllSatConfig ResolveOptions(const EnumerateOptions& options)
{
    AllSatConfig resolved;
    ApplyPreset(resolved, options.preset);

    if (options.encoding.has_value())
    {
        resolved.encoding = (*options.encoding == EnumerateOptions::Encoding::DualRail) ?
            AllSatConfig::Encoding::DualRail :
            AllSatConfig::Encoding::Tseitin;
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

Enumerator::Enumerator(const EnumerateOptions& options)
    : m_Options(options)
{
}

Enumerator::~Enumerator() = default;

void Enumerator::Initialize(const IAigerView& aiger)
{
    const auto* memAiger = dynamic_cast<const AigerMemory*>(&aiger);
    if (memAiger != nullptr)
    {
        memAiger->Validate();
    }

    AllSatConfig resolved = ResolveOptions(m_Options);

    if (resolved.encoding == AllSatConfig::Encoding::DualRail)
    {
        m_Algo = std::make_unique<AllSatAlgoDualRailEnc>(resolved);
    }
    else
    {
        m_Algo = std::make_unique<AllSatAlgoTseitinEnc>(resolved);
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
