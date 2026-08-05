// Library level tests for HALL.
//
// No test framework on purpose, each test is a function that throws on failure.
// main() runs them all and reports every failure rather than stopping at the first.
//
// The most valuable tests here are the brute force ones: for a small circuit the
// complete solution set can be computed exhaustively, so the enumeration can be
// checked exactly instead of against hand-written expectations. That is what
// catches unsoundness, which is the failure mode that matters for an AllSAT tool.

#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "lorina/aiger.hpp"

#include "Aiger/AigerParser.hpp"
#include "Utilities/StringUtilities.hpp"

#include "allsat/AllSatLib.hpp"

using allsat::AigBuilder;
using allsat::Assignment;
using allsat::EnumerateOptions;
using allsat::EnumerateStats;
using allsat::EnumerateStatus;
using allsat::Enumerator;

static void Require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

// *** Shared helpers ***

// every preset, so that a new one has to be added here consciously
static const EnumerateOptions::Preset kAllPresets[] = {
    EnumerateOptions::Preset::None,
    EnumerateOptions::Preset::Tale,
    EnumerateOptions::Preset::MarsDisjoint,
    EnumerateOptions::Preset::MarsNonDisjoint,
    EnumerateOptions::Preset::Duty,
    EnumerateOptions::Preset::Core,
    EnumerateOptions::Preset::Roc,
    EnumerateOptions::Preset::Carma
};

static const char* PresetName(EnumerateOptions::Preset preset)
{
    switch (preset)
    {
        case EnumerateOptions::Preset::None: return "none";
        case EnumerateOptions::Preset::Tale: return "tale";
        case EnumerateOptions::Preset::MarsDisjoint: return "mars-dis";
        case EnumerateOptions::Preset::MarsNonDisjoint: return "mars-nondis";
        case EnumerateOptions::Preset::Duty: return "duty";
        case EnumerateOptions::Preset::Core: return "core";
        case EnumerateOptions::Preset::Roc: return "roc";
        case EnumerateOptions::Preset::Carma: return "carma";
    }
    return "unknown";
}

static EnumerateOptions QuietOptions()
{
    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;
    return options;
}

// Evaluate an AIG under a total assignment to its inputs.
// inputValues[i] is the value of aig.GetInputs()[i].
static bool EvaluateCircuit(const IAigerView& aig, const std::vector<char>& inputValues)
{
    std::vector<char> indexValue(aig.GetMaxIndex() + 2, 0);

    const std::vector<AIGLIT>& inputs = aig.GetInputs();
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        indexValue[AIGLitToAIGIndex(inputs[i])] = inputValues[i];
    }

    auto litValue = [&indexValue](AIGLIT lit) -> bool
    {
        if (lit == 0) return false;
        if (lit == 1) return true;
        bool value = indexValue[AIGLitToAIGIndex(lit)] != 0;
        return IsAIGLitNeg(lit) ? !value : value;
    };

    // and gates are in topological order, both in the parser and in AigerMemory
    for (const AigAndGate& gate : aig.GetAndGated())
    {
        indexValue[AIGLitToAIGIndex(gate.GetL())] =
            (litValue(gate.GetR0()) && litValue(gate.GetR1())) ? 1 : 0;
    }

    return litValue(aig.GetOutputs()[0]);
}

// A cube over the input positions: -1 is don't care, 0 and 1 are the assigned values.
using Cube = std::vector<int>;

// Map an input literal to its position in GetInputs(), or throw if it is not an input.
static size_t InputPosition(const IAigerView& aig, AIGLIT lit)
{
    const std::vector<AIGLIT>& inputs = aig.GetInputs();
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        if (inputs[i] == lit) return i;
    }
    throw std::runtime_error("Model contains a literal that is not a circuit input");
}

struct EnumerationResult
{
    std::vector<Cube> cubes;
    bool sawTautology = false;
    EnumerateStats stats;
};

static EnumerationResult Enumerate(const IAigerView& aig, const EnumerateOptions& options)
{
    EnumerationResult result;
    const size_t numInputs = aig.GetInputs().size();

    Enumerator enumerator(options);
    enumerator.Initialize(aig);

    Assignment model;
    while (true)
    {
        EnumerateStatus status = enumerator.Next(model);
        if (status == EnumerateStatus::Exhausted) break;
        Require(status != EnumerateStatus::Timeout, "Unexpected timeout in a test enumeration");

        if (status == EnumerateStatus::Tautology)
        {
            result.sawTautology = true;
            result.cubes.push_back(Cube(numInputs, -1));
            continue;
        }

        Cube cube(numInputs, -1);
        for (const auto& [lit, val] : model)
        {
            Require(val == TVal::True || val == TVal::False,
                    "Don't-care values must not be reported in a model");
            cube[InputPosition(aig, lit)] = (val == TVal::True) ? 1 : 0;
        }
        result.cubes.push_back(cube);
    }

    result.stats = enumerator.GetStats();
    return result;
}

// The positions the enumeration ranges over: the projection, or all inputs.
static std::vector<size_t> RangedPositions(const IAigerView& aig, const std::vector<AIGINDEX>& projection)
{
    std::vector<size_t> positions;
    if (projection.empty())
    {
        for (size_t i = 0; i < aig.GetInputs().size(); ++i) positions.push_back(i);
        return positions;
    }

    for (AIGINDEX index : projection)
    {
        positions.push_back(InputPosition(aig, AIGIndexToAIGLit(index)));
    }
    std::sort(positions.begin(), positions.end());
    positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
    return positions;
}

// Ground truth: every assignment to the ranged positions that has at least one
// witness on the remaining inputs. With no projection this is just the solution set.
static std::set<std::vector<char>> BruteForceSolutions(const IAigerView& aig,
                                                       const std::vector<size_t>& positions)
{
    const size_t numInputs = aig.GetInputs().size();
    Require(numInputs <= 20, "Brute force is only meant for small circuits");

    std::set<std::vector<char>> covered;
    for (unsigned long long mask = 0; mask < (1ull << numInputs); ++mask)
    {
        std::vector<char> values(numInputs, 0);
        for (size_t i = 0; i < numInputs; ++i)
        {
            values[i] = ((mask >> i) & 1ull) ? 1 : 0;
        }
        if (EvaluateCircuit(aig, values))
        {
            std::vector<char> projected;
            for (size_t p : positions) projected.push_back(values[p]);
            covered.insert(projected);
        }
    }
    return covered;
}

// Expand the cubes into the set of ranged assignments they cover.
// Reports whether any two cubes overlapped, which the disjoint mode must not do.
static std::set<std::vector<char>> ExpandCubes(const std::vector<Cube>& cubes,
                                              const std::vector<size_t>& positions,
                                              bool* outOverlap = nullptr)
{
    std::set<std::vector<char>> covered;
    if (outOverlap) *outOverlap = false;

    for (const Cube& cube : cubes)
    {
        std::vector<size_t> freePositions;
        for (size_t p : positions)
        {
            if (cube[p] < 0) freePositions.push_back(p);
        }
        Require(freePositions.size() <= 20, "Cube expansion is only meant for small circuits");

        for (unsigned long long mask = 0; mask < (1ull << freePositions.size()); ++mask)
        {
            std::vector<char> completion;
            size_t freeIndex = 0;
            for (size_t p : positions)
            {
                if (cube[p] < 0)
                {
                    completion.push_back(((mask >> freeIndex) & 1ull) ? 1 : 0);
                    ++freeIndex;
                }
                else
                {
                    completion.push_back((char)cube[p]);
                }
            }
            const bool inserted = covered.insert(completion).second;
            if (!inserted && outOverlap)
            {
                *outOverlap = true;
            }
        }
    }
    return covered;
}

// The core assertion of this suite: enumeration must cover exactly the solution set.
static void CheckAgainstBruteForce(const IAigerView& aig,
                                   EnumerateOptions::Preset preset,
                                   const std::vector<AIGINDEX>& projection,
                                   const std::string& context)
{
    EnumerateOptions options = QuietOptions();
    options.preset = preset;
    options.projectionIndices = projection;

    const std::vector<size_t> positions = RangedPositions(aig, projection);
    const std::set<std::vector<char>> expected = BruteForceSolutions(aig, positions);

    EnumerationResult result = Enumerate(aig, options);

    // with projection, no literal outside the projection may be reported
    if (!projection.empty())
    {
        std::set<size_t> ranged(positions.begin(), positions.end());
        for (const Cube& cube : result.cubes)
        {
            for (size_t i = 0; i < cube.size(); ++i)
            {
                Require(cube[i] < 0 || ranged.count(i) != 0,
                        context + " [" + PresetName(preset) + "]: a non-projection input was reported");
            }
        }
    }

    const bool checkOverlap = (preset == EnumerateOptions::Preset::MarsDisjoint);
    bool overlap = false;
    const std::set<std::vector<char>> covered =
        ExpandCubes(result.cubes, positions, checkOverlap ? &overlap : nullptr);

    const std::string where = context + " [" + PresetName(preset) + "]";
    Require(covered == expected,
            where + ": enumeration does not match the brute forced solution set (covered " +
            std::to_string(covered.size()) + ", expected " + std::to_string(expected.size()) + ")");
    Require(!overlap, where + ": the disjoint preset produced overlapping cubes");
}

// *** Hand written tests ***

static void TestAndGate()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT out = builder.AddAnd(a, b);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options = QuietOptions();

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Model, "Expected one model for AND gate");
    Require(model.size() == 2, "Expected two literals in model");
    Require(model[0].first == a && model[0].second == TVal::True, "Expected a = True");
    Require(model[1].first == b && model[1].second == TVal::True, "Expected b = True");

    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected enumeration exhausted after one model");
}

static void TestTautology()
{
    AigBuilder builder;
    builder.AddInput();
    builder.SetOutput(AigBuilder::kTrue);
    builder.Validate();

    EnumerateOptions options = QuietOptions();
    options.useCirSim = true;

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Tautology, "Expected tautology status");
    Require(model.empty(), "Expected empty model for tautology");

    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected exhausted after tautology");
}

// A circuit whose output is unsatisfiable must report Exhausted right away.
static void TestUnsatisfiableCircuit()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    builder.AddInput();
    // a AND NOT a
    AIGLIT out = builder.AddAnd(a, AigBuilder::Neg(a));
    builder.SetOutput(out);
    builder.Validate();

    Enumerator enumerator(QuietOptions());
    enumerator.Initialize(builder.GetView());

    Assignment model;
    Require(enumerator.Next(model) == EnumerateStatus::Exhausted,
            "Expected no models for an unsatisfiable circuit");

    EnumerateStats stats = enumerator.GetStats();
    Require(stats.numberOfAssignments == 0, "Expected zero assignments for an unsatisfiable circuit");
    Require(stats.numberOfModels == 0, "Expected zero models for an unsatisfiable circuit");
}

// The output literal may be negated, which is a separate code path in the encoding.
static void TestNegatedOutput()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT ab = builder.AddAnd(a, b);
    // out = NOT (a AND b), satisfied by everything except a = b = 1
    builder.SetOutput(AigBuilder::Neg(ab));
    builder.Validate();

    for (EnumerateOptions::Preset preset : kAllPresets)
    {
        CheckAgainstBruteForce(builder.GetView(), preset, {}, "negated output");
    }
}

static void TestPresetOverride()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT out = builder.AddAnd(a, b);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options = QuietOptions();
    options.preset = EnumerateOptions::Preset::Roc;
    options.useUcore = false;

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Model, "Expected one model for AND gate (preset override)");
    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected enumeration exhausted after one model (preset override)");
}

// The encoding can be selected explicitly, independently of the preset.
static void TestEncodingOverride()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT c = builder.AddInput();
    AIGLIT ab = builder.AddAnd(a, b);
    // out = (a AND b) OR c, via De Morgan
    AIGLIT out = AigBuilder::Neg(builder.AddAnd(AigBuilder::Neg(ab), AigBuilder::Neg(c)));
    builder.SetOutput(out);
    builder.Validate();

    const std::vector<size_t> positions = RangedPositions(builder.GetView(), {});
    const std::set<std::vector<char>> expected = BruteForceSolutions(builder.GetView(), positions);

    for (EnumerateOptions::Encoding encoding :
         {EnumerateOptions::Encoding::Tseitin, EnumerateOptions::Encoding::DualRail})
    {
        EnumerateOptions options = QuietOptions();
        options.encoding = encoding;

        EnumerationResult result = Enumerate(builder.GetView(), options);
        Require(ExpandCubes(result.cubes, positions) == expected,
                "Explicit encoding override changed the solution set");
    }
}

static void TestNonConsecutiveInputs()
{
    // Reproduces the pattern: input, input, and, input, and
    // The AND gate at index 3 shifts the third input to index 4 (lit 8),
    // not the naive sequential index 3 (lit 6). The model must use the
    // correct input literals, not gate literals.
    AigBuilder builder;
    AIGLIT a  = builder.AddInput();       // index 1, lit 2
    AIGLIT b  = builder.AddInput();       // index 2, lit 4
    AIGLIT g1 = builder.AddAnd(a, b);    // index 3, lit 6  (AND gate, NOT an input)
    AIGLIT c  = builder.AddInput();       // index 4, lit 8  (non-consecutive!)
    AIGLIT out = builder.AddAnd(g1, c);  // index 5, lit 10
    builder.SetOutput(out);
    builder.Validate();

    Enumerator enumerator(QuietOptions());
    enumerator.Initialize(builder.GetView());

    // The only satisfying assignment is a=T, b=T, c=T
    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Model, "Expected one model for non-consecutive inputs");
    Require(model.size() == 3, "Expected three literals in model");

    // Each entry must be an input literal (2, 4, or 8), not the AND gate literal (6 or 10)
    bool foundA = false, foundB = false, foundC = false;
    for (const auto& entry : model)
    {
        if (entry.first == a)
        {
            foundA = true;
            Require(entry.second == TVal::True, "Expected a = True");
        }
        else if (entry.first == b)
        {
            foundB = true;
            Require(entry.second == TVal::True, "Expected b = True");
        }
        else if (entry.first == c)
        {
            foundC = true;
            Require(entry.second == TVal::True, "Expected c = True");
        }
        else
        {
            Require(false, "AND gate literal returned in model instead of input literal");
        }
    }
    Require(foundA && foundB && foundC, "Expected all three inputs in model");

    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected exactly one model for non-consecutive inputs");
}

static void TestPartialModelEnumeration()
{
    // Circuit: out = a AND b AND c
    // Projection onto {a, c}: b must not appear in the model output.
    // The only satisfying complete assignment (a=T, b=T, c=T) projects to (a=T, c=T).
    AigBuilder builder;
    AIGLIT a   = builder.AddInput();
    AIGLIT b   = builder.AddInput();
    AIGLIT ab  = builder.AddAnd(a, b);
    AIGLIT c   = builder.AddInput();
    AIGLIT out = builder.AddAnd(ab, c);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options = QuietOptions();
    options.projectionIndices = {AIGLitToAIGIndex(a), AIGLitToAIGIndex(c)};

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Model, "Expected one projected model");
    Require(model.size() == 2, "Projected model must contain only a and c, not b");

    bool foundA = false, foundC = false;
    for (const auto& entry : model)
    {
        Require(entry.first != b, "b must not appear in projected model");
        if (entry.first == a)
        {
            foundA = true;
            Require(entry.second == TVal::True, "Expected a = True in projected model");
        }
        else if (entry.first == c)
        {
            foundC = true;
            Require(entry.second == TVal::True, "Expected c = True in projected model");
        }
        else
        {
            Require(false, "Unexpected literal in projected model");
        }
    }
    Require(foundA && foundC, "Expected both a and c in projected model");

    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected exhausted after one projected model");
}

// Projecting onto every input must behave exactly like not projecting at all,
// and repeating an index in the projection list must be harmless.
static void TestProjectionEdgeCases()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT c = builder.AddInput();
    AIGLIT ab = builder.AddAnd(a, b);
    AIGLIT out = builder.AddAnd(ab, AigBuilder::Neg(c));
    builder.SetOutput(out);
    builder.Validate();

    const std::vector<AIGINDEX> all = {AIGLitToAIGIndex(a), AIGLitToAIGIndex(b), AIGLitToAIGIndex(c)};
    const std::vector<size_t> positions = RangedPositions(builder.GetView(), {});

    EnumerationResult none = Enumerate(builder.GetView(), QuietOptions());

    EnumerateOptions projectAll = QuietOptions();
    projectAll.projectionIndices = all;
    EnumerationResult full = Enumerate(builder.GetView(), projectAll);

    Require(ExpandCubes(none.cubes, positions) == ExpandCubes(full.cubes, positions),
            "Projecting onto all inputs differs from not projecting");

    EnumerateOptions duplicated = QuietOptions();
    duplicated.projectionIndices = {AIGLitToAIGIndex(a), AIGLitToAIGIndex(a), AIGLitToAIGIndex(b),
                                    AIGLitToAIGIndex(a), AIGLitToAIGIndex(b)};
    EnumerationResult dedup = Enumerate(builder.GetView(), duplicated);

    EnumerateOptions once = QuietOptions();
    once.projectionIndices = {AIGLitToAIGIndex(a), AIGLitToAIGIndex(b)};
    EnumerationResult single = Enumerate(builder.GetView(), once);

    const std::vector<size_t> abPositions = RangedPositions(builder.GetView(), once.projectionIndices);
    Require(ExpandCubes(dedup.cubes, abPositions) == ExpandCubes(single.cubes, abPositions),
            "Duplicated projection indices changed the result");
}

// An index that is not a circuit input must be rejected, not silently ignored.
static void TestInvalidProjectionIndexIsRejected()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    builder.SetOutput(builder.AddAnd(a, b));
    builder.Validate();

    EnumerateOptions options = QuietOptions();
    // index 3 is the AND gate, not an input
    options.projectionIndices = {3};

    Enumerator enumerator(options);
    std::string message;
    try
    {
        enumerator.Initialize(builder.GetView());
    }
    catch (const std::exception& ex)
    {
        message = ex.what();
    }

    Require(!message.empty(), "Projecting onto a non-input index must be rejected");
    // the diagnostic has to travel with the exception, a library user has no other
    // way to find out what was wrong
    Require(message.find("3") != std::string::npos,
            "The rejection must name the offending index, got: " + message);
    Require(message.find("1, 2") != std::string::npos,
            "The rejection must list the valid input indices, got: " + message);
}

// AigBuilder is the entry point library users touch first, its guard rails must hold.
static void TestBuilderValidation()
{
    {
        AigBuilder builder;
        builder.AddInput();
        bool threw = false;
        try { builder.Validate(); } catch (const std::exception&) { threw = true; }
        Require(threw, "A circuit without an output must be rejected");
    }
    {
        AigBuilder builder;
        AIGLIT a = builder.AddInput();
        builder.SetOutput(a);
        bool threw = false;
        try { builder.SetOutput(a); } catch (const std::exception&) { threw = true; }
        Require(threw, "A second output must be rejected");
    }
    {
        AigBuilder builder;
        builder.AddInput();
        bool threw = false;
        // literal 100 was never created
        try { builder.AddAnd(2, 100); } catch (const std::exception&) { threw = true; }
        Require(threw, "An AND over an unknown literal must be rejected");
    }
    {
        AigBuilder builder;
        builder.SetOutput(AigBuilder::kTrue);
        bool threw = false;
        try { builder.Validate(); } catch (const std::exception&) { threw = true; }
        Require(threw, "A circuit without inputs must be rejected");
    }
}

// No SAT backend implements a conflict limit. That has to fail with a message that
// says so, up front, instead of "Function not implemented" mid-enumeration.
static void TestConflictLimitIsRejected()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    builder.SetOutput(builder.AddAnd(a, b));
    builder.Validate();

    EnumerateOptions options = QuietOptions();
    options.useUcore = true;
    options.useLitDrop = true;
    options.litDropConflictLimit = 1;

    std::string message;
    try
    {
        Enumerator enumerator(options);
        enumerator.Initialize(builder.GetView());
        Assignment model;
        while (enumerator.Next(model) == EnumerateStatus::Model) {}
    }
    catch (const std::exception& ex)
    {
        message = ex.what();
    }

    Require(!message.empty(), "A non-zero conflict limit must be rejected");
    Require(message.find("conflict limit") != std::string::npos,
            "The rejection must explain that the conflict limit is unsupported, got: " + message);
}

// Only IntelSAT implements the polarity and score hints the dual-rail presets rely on,
// so pairing one of them with the IPASIR plain solver has to be rejected clearly.
static void TestDualRailHintsWithIpasirPlainRejected()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    builder.SetOutput(builder.AddAnd(a, b));
    builder.Validate();

    static const EnumerateOptions::Preset kDualRailPresets[] = {
        EnumerateOptions::Preset::MarsDisjoint,
        EnumerateOptions::Preset::MarsNonDisjoint,
        EnumerateOptions::Preset::Duty,
        EnumerateOptions::Preset::Carma
    };

    for (EnumerateOptions::Preset preset : kDualRailPresets)
    {
        EnumerateOptions options = QuietOptions();
        options.preset = preset;
        options.useIpasirForPlain = true;

        std::string message;
        try
        {
            Enumerator enumerator(options);
            enumerator.Initialize(builder.GetView());
        }
        catch (const std::exception& ex)
        {
            message = ex.what();
        }

        const std::string where = std::string("preset ") + PresetName(preset);
        Require(!message.empty(), where + ": expected the combination to be rejected");
        Require(message.find("IntelSAT") != std::string::npos,
                where + ": the rejection must name the unsupported backend, got: " + message);
    }

    // with the hints turned off the same combination is fine
    EnumerateOptions options = QuietOptions();
    options.preset = EnumerateOptions::Preset::MarsNonDisjoint;
    options.useIpasirForPlain = true;
    options.dualForcePolarity = false;
    options.dualBoostScore = false;

    const std::vector<size_t> positions = RangedPositions(builder.GetView(), {});
    Require(ExpandCubes(Enumerate(builder.GetView(), options).cubes, positions) ==
            BruteForceSolutions(builder.GetView(), positions),
            "Dual-rail with the IPASIR plain solver and no hints must still enumerate correctly");
}

// Don't-care inputs are simply absent from a model. The encodings disagree internally
// about whether they emit an explicit don't-care entry, the library must not.
static void TestModelsNeverContainDontCare()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT c = builder.AddInput();
    AIGLIT ab = builder.AddAnd(a, b);
    // out = (a AND b) OR c, which has cubes that leave an input don't-care
    AIGLIT out = AigBuilder::Neg(builder.AddAnd(AigBuilder::Neg(ab), AigBuilder::Neg(c)));
    builder.SetOutput(out);
    builder.Validate();

    for (EnumerateOptions::Preset preset : kAllPresets)
    {
        EnumerateOptions options = QuietOptions();
        options.preset = preset;

        Enumerator enumerator(options);
        enumerator.Initialize(builder.GetView());

        Assignment model;
        EnumerateStatus status = EnumerateStatus::Model;
        while (status == EnumerateStatus::Model || status == EnumerateStatus::Tautology)
        {
            status = enumerator.Next(model);
            for (const auto& [lit, val] : model)
            {
                (void)lit;
                Require(val == TVal::True || val == TVal::False,
                        std::string("Preset ") + PresetName(preset) +
                        " reported an explicit don't-care entry in a model");
            }
        }
    }
}

static void TestNextBeforeInitializeThrows()
{
    Enumerator enumerator(QuietOptions());
    Assignment model;
    bool threw = false;
    try { enumerator.Next(model); } catch (const std::exception&) { threw = true; }
    Require(threw, "Next before Initialize must throw");
}

// The statistics the tool prints are also part of the library contract.
static void TestStatsAreConsistent()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT c = builder.AddInput();
    AIGLIT ab = builder.AddAnd(a, b);
    // out = (a AND b) OR c
    AIGLIT out = AigBuilder::Neg(builder.AddAnd(AigBuilder::Neg(ab), AigBuilder::Neg(c)));
    builder.SetOutput(out);
    builder.Validate();

    const std::vector<size_t> positions = RangedPositions(builder.GetView(), {});
    const size_t trueSolutions = BruteForceSolutions(builder.GetView(), positions).size();

    EnumerateOptions options = QuietOptions();
    options.preset = EnumerateOptions::Preset::MarsDisjoint;
    EnumerationResult result = Enumerate(builder.GetView(), options);

    Require(result.stats.numberOfAssignments == result.cubes.size(),
            "numberOfAssignments must match the number of reported cubes");
    // the disjoint preset does not overlap, so the model count is exact
    Require(result.stats.numberOfModels == trueSolutions,
            "numberOfModels must equal the number of satisfying assignments for a disjoint enumeration");
    Require(result.stats.avgCardinality >= 0.0 && result.stats.avgCardinality <= 1.0,
            "avgCardinality must be a fraction");
    Require(!result.stats.isTimeout, "Enumeration must not report a timeout");
}

// *** Brute force cross validation ***

// Build a pseudo random AIG. Deterministic for a given seed so failures reproduce.
static void BuildRandomCircuit(AigBuilder& builder, std::mt19937& rng,
                               unsigned numInputs, unsigned numGates)
{
    std::vector<AIGLIT> pool;
    pool.push_back(AigBuilder::kFalse);
    pool.push_back(AigBuilder::kTrue);

    for (unsigned i = 0; i < numInputs; ++i)
    {
        AIGLIT input = builder.AddInput();
        pool.push_back(input);
        pool.push_back(AigBuilder::Neg(input));
    }

    AIGLIT last = pool.back();
    for (unsigned i = 0; i < numGates; ++i)
    {
        std::uniform_int_distribution<size_t> pick(0, pool.size() - 1);
        AIGLIT gate = builder.AddAnd(pool[pick(rng)], pool[pick(rng)]);
        pool.push_back(gate);
        pool.push_back(AigBuilder::Neg(gate));
        last = gate;
    }

    // sometimes negate the output, it is a different encoding path
    std::uniform_int_distribution<int> coin(0, 2);
    builder.SetOutput(coin(rng) == 0 ? AigBuilder::Neg(last) : last);
    builder.Validate();
}

// The heart of the suite: random circuits, every preset, checked exactly against
// exhaustive evaluation, with and without projection.
static void TestRandomCircuitsAgainstBruteForce()
{
    const unsigned kNumCircuits = 12;
    std::mt19937 rng(20240813u);

    for (unsigned c = 0; c < kNumCircuits; ++c)
    {
        std::uniform_int_distribution<unsigned> inputDist(3, 9);
        std::uniform_int_distribution<unsigned> gateDist(4, 30);
        const unsigned numInputs = inputDist(rng);

        AigBuilder builder;
        BuildRandomCircuit(builder, rng, numInputs, gateDist(rng));

        const std::string context = "random circuit " + std::to_string(c);

        // a random non-empty subset of the inputs to project onto
        std::vector<AIGINDEX> allIndices;
        for (AIGLIT lit : builder.GetView().GetInputs())
        {
            allIndices.push_back(AIGLitToAIGIndex(lit));
        }
        std::vector<AIGINDEX> projection = allIndices;
        std::shuffle(projection.begin(), projection.end(), rng);
        std::uniform_int_distribution<size_t> sizeDist(1, projection.size());
        projection.resize(sizeDist(rng));
        std::sort(projection.begin(), projection.end());

        for (EnumerateOptions::Preset preset : kAllPresets)
        {
            CheckAgainstBruteForce(builder.GetView(), preset, {}, context);
            CheckAgainstBruteForce(builder.GetView(), preset, projection, context + " projected");
        }
    }
}

// The same, with the non-default generalization knobs turned on. These are the
// options a user can reach through the library but that no preset enables.
static void TestNonDefaultOptionsAgainstBruteForce()
{
    std::mt19937 rng(99001122u);

    struct Variant
    {
        const char* name;
        void (*apply)(EnumerateOptions&);
    };

    static const Variant kVariants[] = {
        {"recursive unsat core", [](EnumerateOptions& o) {
            o.useUcore = true; o.useLitDrop = true; o.useLitDropRecur = true; }},
        {"no literal dropping", [](EnumerateOptions& o) {
            o.useUcore = true; o.useLitDrop = false; }},
        {"top to bottom simulation", [](EnumerateOptions& o) {
            o.useCirSim = true; o.useTopToBottomSim = true; }},
        {"ipasir for the plain instance", [](EnumerateOptions& o) {
            o.useIpasirForPlain = true; }},
        {"intel sat for the dual instance", [](EnumerateOptions& o) {
            o.useUcore = true; o.useIpasirForDual = false; }},
        {"tseitin for the dual instance", [](EnumerateOptions& o) {
            o.encoding = EnumerateOptions::Encoding::DualRail;
            o.useUcore = true; o.dualUseTseitinForDual = true; }},
    };

    for (unsigned c = 0; c < 6; ++c)
    {
        std::uniform_int_distribution<unsigned> inputDist(3, 8);
        std::uniform_int_distribution<unsigned> gateDist(4, 25);

        AigBuilder builder;
        BuildRandomCircuit(builder, rng, inputDist(rng), gateDist(rng));

        std::vector<AIGINDEX> projection;
        for (AIGLIT lit : builder.GetView().GetInputs())
        {
            projection.push_back(AIGLitToAIGIndex(lit));
        }
        projection.resize((projection.size() + 1) / 2);

        const std::vector<size_t> allPositions = RangedPositions(builder.GetView(), {});
        const std::set<std::vector<char>> expectedAll =
            BruteForceSolutions(builder.GetView(), allPositions);
        const std::vector<size_t> projPositions = RangedPositions(builder.GetView(), projection);
        const std::set<std::vector<char>> expectedProj =
            BruteForceSolutions(builder.GetView(), projPositions);

        for (const Variant& variant : kVariants)
        {
            const std::string where =
                "circuit " + std::to_string(c) + " [" + variant.name + "]";

            EnumerateOptions plain = QuietOptions();
            variant.apply(plain);
            Require(ExpandCubes(Enumerate(builder.GetView(), plain).cubes, allPositions) == expectedAll,
                    where + ": enumeration does not match the brute forced solution set");

            EnumerateOptions projected = QuietOptions();
            variant.apply(projected);
            projected.projectionIndices = projection;
            Require(ExpandCubes(Enumerate(builder.GetView(), projected).cubes, projPositions) == expectedProj,
                    where + " projected: enumeration does not match the brute forced solution set");
        }
    }
}

// *** File based tests ***

static std::string ResolveBenchmarkPath(const std::string& relative)
{
    const std::vector<std::string> prefixes = {"", "../", "../../"};

    for (const std::string& prefix : prefixes)
    {
        std::string path = prefix + relative;
        std::ifstream stream(path);
        if (stream.good())
        {
            return path;
        }
    }

    throw std::runtime_error("Failed to locate benchmark file " + relative +
                             " (run the tests from the repository root)");
}

static AigerParser LoadAigerFromFile(const std::string& path)
{
    AigerParser parser;
    lorina::return_code result;

    if (stringEndsWith(path, ".aag"))
    {
        result = lorina::read_ascii_aiger(path, parser);
    }
    else if (stringEndsWith(path, ".aig"))
    {
        result = lorina::read_aiger(path, parser);
    }
    else
    {
        throw std::runtime_error("Unknown aiger format, please provide either .aag or .aig file");
    }

    if (result == lorina::return_code::parse_error)
    {
        throw std::runtime_error("Error parsing the AIGER file");
    }

    return parser;
}

// A parsed circuit and an equivalent in-memory one must enumerate identically,
// which is the contract IAigerView exists to provide.
static void TestParsedFileMatchesInMemory()
{
    AigerParser parser = LoadAigerFromFile(ResolveBenchmarkPath("benchmarks/XOR.aag"));

    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT bothTrue = builder.AddAnd(a, b);
    AIGLIT bothFalse = builder.AddAnd(AigBuilder::Neg(a), AigBuilder::Neg(b));
    // xor = NOT(a AND b) AND NOT(NOT a AND NOT b)
    AIGLIT out = builder.AddAnd(AigBuilder::Neg(bothTrue), AigBuilder::Neg(bothFalse));
    builder.SetOutput(out);
    builder.Validate();

    const std::vector<size_t> positions = RangedPositions(parser, {});

    for (EnumerateOptions::Preset preset : kAllPresets)
    {
        EnumerateOptions options = QuietOptions();
        options.preset = preset;

        std::set<std::vector<char>> fromFile = ExpandCubes(Enumerate(parser, options).cubes, positions);
        std::set<std::vector<char>> fromMemory = ExpandCubes(Enumerate(builder.GetView(), options).cubes, positions);

        Require(fromFile == fromMemory,
                std::string("Parsed and in-memory circuits disagree for preset ") + PresetName(preset));
        Require(fromFile.size() == 2, "XOR must have exactly two solutions");
    }
}

// A real 60 input benchmark, for the scale the toy circuits above cannot reach.
// benchmarks/iscas85/or/c880 ORs all of c880's outputs together, which makes it a
// tautology, so the whole 2^60 input space is one solution. The unSAT-core modes
// must recognize that with a single empty cube, and no mode may report less than
// the full space. mars-dis is left out on purpose, disjoint enumeration does not
// finish on this circuit.
static void TestRealBenchmarkTautology()
{
    AigerParser parser = LoadAigerFromFile(ResolveBenchmarkPath("benchmarks/iscas85/or/c880/c880.aag"));

    const size_t numInputs = parser.GetInputs().size();
    Require(numInputs == 60, "Expected c880 to have 60 inputs");
    const unsigned long long totalSpace = 1ull << numInputs;

    static const EnumerateOptions::Preset kCompletingPresets[] = {
        EnumerateOptions::Preset::Tale,
        EnumerateOptions::Preset::MarsNonDisjoint,
        EnumerateOptions::Preset::Duty,
        EnumerateOptions::Preset::Core,
        EnumerateOptions::Preset::Roc,
        EnumerateOptions::Preset::Carma
    };

    for (EnumerateOptions::Preset preset : kCompletingPresets)
    {
        EnumerateOptions options = QuietOptions();
        options.preset = preset;
        options.useTimeout = true;
        options.timeoutSeconds = 120;

        Enumerator enumerator(options);
        enumerator.Initialize(parser);

        const std::string where = std::string("c880/or [") + PresetName(preset) + "]";

        Assignment model;
        bool sawTautology = false;
        EnumerateStatus status = EnumerateStatus::Model;
        while (status == EnumerateStatus::Model || status == EnumerateStatus::Tautology)
        {
            status = enumerator.Next(model);
            if (status == EnumerateStatus::Tautology) sawTautology = true;
        }
        Require(status == EnumerateStatus::Exhausted, where + ": did not finish");

        EnumerateStats stats = enumerator.GetStats();
        Require(stats.numberOfModels >= totalSpace,
                where + ": the reported cubes do not cover the whole input space");

        const bool usesUnsatCore = (preset == EnumerateOptions::Preset::Core ||
                                    preset == EnumerateOptions::Preset::Roc ||
                                    preset == EnumerateOptions::Preset::Carma);
        if (usesUnsatCore)
        {
            Require(sawTautology, where + ": expected the tautology to be detected");
            Require(stats.numberOfAssignments == 1, where + ": expected a single empty cube");
            Require(stats.numberOfModels == totalSpace,
                    where + ": expected the model count to be exactly the input space");
        }
    }
}

// *** Regression tests ***

// The 13 inputs of the regression circuit below, followed by its 50 AND gates given as
// (left literal, right literal). Gates are added in order, so gate i gets AIG index
// 13 + 1 + i, which is what the literals below refer to.
static const unsigned kRegressionInputCount = 13;
static const unsigned kRegressionOutputLit = 126;
static const unsigned kRegressionGates[][2] = {
    {27, 24}, {14, 14}, {12, 11}, {32, 30}, {11, 6}, {28, 19},
    {9, 5}, {34, 40}, {2, 38}, {25, 28}, {41, 47}, {39, 41},
    {10, 39}, {0, 53}, {33, 4}, {3, 2}, {12, 56}, {15, 38},
    {3, 59}, {41, 56}, {25, 66}, {29, 37}, {63, 0}, {10, 58},
    {35, 52}, {70, 10}, {32, 40}, {29, 65}, {36, 3}, {8, 72},
    {13, 51}, {13, 37}, {49, 8}, {2, 87}, {0, 27}, {26, 6},
    {60, 48}, {90, 50}, {53, 9}, {72, 80}, {25, 99}, {86, 34},
    {43, 11}, {39, 42}, {1, 52}, {97, 15}, {17, 31}, {90, 12},
    {1, 7}, {59, 102}
};

static void BuildRegressionCircuit(AigBuilder& builder)
{
    for (unsigned i = 0; i < kRegressionInputCount; ++i)
    {
        builder.AddInput();
    }
    for (const auto& gate : kRegressionGates)
    {
        builder.AddAnd(gate[0], gate[1]);
    }
    builder.SetOutput(kRegressionOutputLit);
    builder.Validate();
}

// Regression test for unsat-core literal dropping with recursive core extraction.
// The recursive check asks the solver which assumptions were required, by position in
// the vector that was last solved under. Indexing a *different* vector there drops
// literals that are actually required, and the resulting cubes no longer entail the
// output. On this circuit that produced cubes covering 2144 assignments where only
// 2016 satisfy it.
static void TestRecursiveUnsatCoreKeepsRequiredLiterals()
{
    AigBuilder builder;
    BuildRegressionCircuit(builder);

    EnumerateOptions options = QuietOptions();
    options.useUcore = true;
    options.useLitDrop = true;
    options.useLitDropRecur = true;

    const std::vector<size_t> positions = RangedPositions(builder.GetView(), {});
    const std::set<std::vector<char>> expected = BruteForceSolutions(builder.GetView(), positions);
    Require(expected.size() == 2016, "The regression circuit is expected to have 2016 solutions");

    EnumerationResult result = Enumerate(builder.GetView(), options);
    const std::set<std::vector<char>> covered = ExpandCubes(result.cubes, positions);

    Require(covered == expected,
            "Recursive unSAT-core dropped a required literal, the reported cubes cover " +
            std::to_string(covered.size()) + " assignments instead of " +
            std::to_string(expected.size()));
}

static void TestTimeoutStatus()
{
    std::string path = ResolveBenchmarkPath("benchmarks/islis_benchmarks/arithmetic/xor/log2/log2.aag");
    AigerParser parser = LoadAigerFromFile(path);

    EnumerateOptions options = QuietOptions();
    options.preset = EnumerateOptions::Preset::Roc;
    options.useTimeout = true;
    options.timeoutSeconds = 1;

    Enumerator enumerator(options);
    enumerator.Initialize(parser);

    Assignment model;
    EnumerateStatus status = EnumerateStatus::Model;
    while (status != EnumerateStatus::Timeout && status != EnumerateStatus::Exhausted)
    {
        status = enumerator.Next(model);
    }

    Require(status == EnumerateStatus::Timeout, "Expected timeout status from enumeration");

    EnumerateStats stats = enumerator.GetStats();
    Require(stats.isTimeout, "Stats must report the timeout");
}

// *** Runner ***

struct TestCase
{
    const char* name;
    void (*run)();
};

static const TestCase kTests[] = {
    {"and gate", TestAndGate},
    {"tautology", TestTautology},
    {"unsatisfiable circuit", TestUnsatisfiableCircuit},
    {"negated output", TestNegatedOutput},
    {"preset override", TestPresetOverride},
    {"encoding override", TestEncodingOverride},
    {"non consecutive inputs", TestNonConsecutiveInputs},
    {"partial model enumeration", TestPartialModelEnumeration},
    {"projection edge cases", TestProjectionEdgeCases},
    {"invalid projection index is rejected", TestInvalidProjectionIndexIsRejected},
    {"builder validation", TestBuilderValidation},
    {"conflict limit is rejected", TestConflictLimitIsRejected},
    {"dual-rail hints with ipasir plain rejected", TestDualRailHintsWithIpasirPlainRejected},
    {"models never contain don't care", TestModelsNeverContainDontCare},
    {"next before initialize throws", TestNextBeforeInitializeThrows},
    {"stats are consistent", TestStatsAreConsistent},
    {"random circuits vs brute force", TestRandomCircuitsAgainstBruteForce},
    {"non default options vs brute force", TestNonDefaultOptionsAgainstBruteForce},
    {"parsed file matches in memory", TestParsedFileMatchesInMemory},
    {"real benchmark tautology", TestRealBenchmarkTautology},
    {"recursive unsat core keeps required literals", TestRecursiveUnsatCoreKeepsRequiredLiterals},
    {"timeout status", TestTimeoutStatus}
};

int main()
{
    size_t failures = 0;
    const size_t numTests = sizeof(kTests) / sizeof(kTests[0]);

    for (const TestCase& test : kTests)
    {
        try
        {
            test.run();
            std::cout << "  ok    " << test.name << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cout << "  FAIL  " << test.name << ": " << ex.what() << std::endl;
            ++failures;
        }
    }

    std::cout << std::endl;
    if (failures != 0)
    {
        std::cout << failures << " of " << numTests << " tests failed." << std::endl;
        return 1;
    }

    std::cout << "All " << numTests << " tests passed." << std::endl;
    return 0;
}
