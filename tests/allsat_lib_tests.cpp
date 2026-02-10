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
