#include <fstream>
#include <iostream>
#include <stdexcept>

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
    AIGLIT c   = builder.AddInput();
    AIGLIT ab  = builder.AddAnd(a, b);
    AIGLIT out = builder.AddAnd(ab, c);
    builder.SetOutput(out);
    builder.Validate();

    EnumerateOptions options;
    options.printInfo = false;
    options.printEnumerations = false;
    options.projectionIndices = {AIGLitToAIGIndex(a), AIGLitToAIGIndex(b)};

    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    EnumerateStatus status = enumerator.Next(model);
    Require(status == EnumerateStatus::Model, "Expected one projected model");
    Require(model.size() == 2, "Projected model must contain only a and b, not c");

    bool foundA = false, foundB = false;
    for (const auto& entry : model)
    {
        Require(entry.first != c, "c must not appear in projected model");
        if (entry.first == a)
        {
            foundA = true;
            Require(entry.second == TVal::True, "Expected a = True in projected model");
        }
        else if (entry.first == b)
        {
            foundB = true;
            Require(entry.second == TVal::True, "Expected b = True in projected model");
        }
        else
        {
            Require(false, "Unexpected literal in projected model");
        }
    }
    Require(foundA && foundB, "Expected both a and b in projected model");

    status = enumerator.Next(model);
    Require(status == EnumerateStatus::Exhausted, "Expected exhausted after one projected model");
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
