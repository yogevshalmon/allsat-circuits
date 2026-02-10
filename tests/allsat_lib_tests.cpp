#include <iostream>
#include <stdexcept>

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

int main()
{
    try
    {
        TestAndGate();
        TestTautology();
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Test failed: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
