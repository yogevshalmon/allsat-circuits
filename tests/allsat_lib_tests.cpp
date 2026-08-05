#include <fstream>
#include <iostream>
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
using allsat::EnumerateStatus;
using allsat::Enumerator;

static void Require(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

static void TestAndGate()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT out = builder.AddAnd(a, b);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;

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

    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;
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

static void TestPresetOverride()
{
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT out = builder.AddAnd(a, b);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options;
    options.preset = EnumerateOptions::Preset::Roc;
    options.useUcore = false;
    options.printInfo = false;
    options.printEnumerations = false;

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Model, "Expected one model for AND gate (preset override)");
    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected enumeration exhausted after one model (preset override)");
}

static std::string ResolveTimeoutBenchmarkPath()
{
    const std::vector<std::string> candidates = {
        "benchmarks/islis_benchmarks/arithmetic/xor/log2/log2.aag",
        "../benchmarks/islis_benchmarks/arithmetic/xor/log2/log2.aag",
        "../../benchmarks/islis_benchmarks/arithmetic/xor/log2/log2.aag"
    };

    for (const std::string& path : candidates)
    {
        std::ifstream stream(path);
        if (stream.good())
        {
            return path;
        }
    }

    throw std::runtime_error("Failed to locate log2.aag benchmark file");
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

    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;

    Enumerator enumerator(options);
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
    // Projection onto {a, b}: c must not appear in model output.
    // The only satisfying complete assignment (a=T, b=T, c=T) projects to (a=T, b=T).
    AigBuilder builder;
    AIGLIT a   = builder.AddInput();
    AIGLIT b   = builder.AddInput();
    AIGLIT ab  = builder.AddAnd(a, b);
    AIGLIT c   = builder.AddInput();
    AIGLIT out = builder.AddAnd(ab, c);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;
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

// Evaluate the regression circuit under a total assignment to its inputs.
// values[index] holds the value of input index (1 based), other indices are computed.
static bool EvaluateRegressionCircuit(const std::vector<bool>& inputValues)
{
    const size_t numGates = sizeof(kRegressionGates) / sizeof(kRegressionGates[0]);
    std::vector<bool> indexValue(kRegressionInputCount + numGates + 1, false);
    for (size_t i = 0; i < kRegressionInputCount; ++i)
    {
        indexValue[i + 1] = inputValues[i];
    }

    auto litValue = [&indexValue](unsigned lit) -> bool
    {
        if (lit == 0) return false;
        if (lit == 1) return true;
        bool value = indexValue[lit >> 1];
        return (lit & 1) ? !value : value;
    };

    for (size_t i = 0; i < numGates; ++i)
    {
        indexValue[kRegressionInputCount + 1 + i] =
            litValue(kRegressionGates[i][0]) && litValue(kRegressionGates[i][1]);
    }

    return litValue(kRegressionOutputLit);
}

// Regression test for unsat-core literal dropping with recursive core extraction.
// The recursive check asks the solver which assumptions were required, by position in
// the vector that was last solved under. Indexing a *different* vector there drops
// literals that are actually required, and the resulting cubes no longer entail the
// output. On this circuit that produced 128 assignments that do not satisfy it.
//
// Every reported cube is checked exactly, by completing the don't-care inputs in all
// possible ways and evaluating the circuit on each completion.
static void TestRecursiveUnsatCoreKeepsRequiredLiterals()
{
    AigBuilder builder;
    std::vector<AIGLIT> inputs;
    for (unsigned i = 0; i < kRegressionInputCount; ++i)
    {
        inputs.push_back(builder.AddInput());
    }
    for (const auto& gate : kRegressionGates)
    {
        builder.AddAnd(gate[0], gate[1]);
    }
    builder.SetOutput(kRegressionOutputLit);
    builder.Validate();

    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;
    options.useUcore = true;
    options.useLitDrop = true;
    options.useLitDropRecur = true;

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    size_t numCubes = 0;
    while (enumerator.Next(model) == EnumerateStatus::Model)
    {
        ++numCubes;

        // positions of the inputs left as don't-care by this cube
        std::vector<bool> assigned(kRegressionInputCount, false);
        std::vector<bool> value(kRegressionInputCount, false);
        for (const auto& [lit, val] : model)
        {
            AIGINDEX index = AIGLitToAIGIndex(lit);
            Require(index >= 1 && index <= kRegressionInputCount, "Model contains a non-input literal");
            assigned[index - 1] = true;
            value[index - 1] = (val == TVal::True);
        }

        std::vector<size_t> freePositions;
        for (size_t i = 0; i < kRegressionInputCount; ++i)
        {
            if (!assigned[i]) freePositions.push_back(i);
        }

        // the cube must entail the output, that is, every completion satisfies the circuit
        for (unsigned long long mask = 0; mask < (1ull << freePositions.size()); ++mask)
        {
            std::vector<bool> completion = value;
            for (size_t b = 0; b < freePositions.size(); ++b)
            {
                completion[freePositions[b]] = ((mask >> b) & 1ull) != 0;
            }
            Require(EvaluateRegressionCircuit(completion),
                    "Reported cube does not entail the output, a required literal was dropped from the unSAT core");
        }
    }

    Require(numCubes > 0, "Expected at least one model for the regression circuit");
}

static void TestTimeoutStatus()
{
    std::string path = ResolveTimeoutBenchmarkPath();
    AigerParser parser = LoadAigerFromFile(path);

    EnumerateOptions options;
    options.preset = EnumerateOptions::Preset::Roc;
    options.printInfo = false;
    options.printEnumerations = false;
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
}

int main()
{
    try
    {
        TestAndGate();
        TestTautology();
        TestPresetOverride();
        TestNonConsecutiveInputs();
        TestPartialModelEnumeration();
        TestRecursiveUnsatCoreKeepsRequiredLiterals();
        TestTimeoutStatus();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Test failed: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
