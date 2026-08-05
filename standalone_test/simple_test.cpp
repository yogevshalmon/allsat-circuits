#include "allsat/AllSatLib.hpp"
#include <iostream>

using allsat::AigBuilder;
using allsat::EnumerateOptions;
using allsat::Enumerator;
using allsat::Assignment;
using allsat::EnumerateStatus;

int main()
{
    // Build a simple AND gate
    AigBuilder builder;
    AIGLIT a = builder.AddInput();
    AIGLIT b = builder.AddInput();
    AIGLIT out = builder.AddAnd(a, b);
    builder.SetOutput(out);
    builder.Validate();

    // Configure enumerator
    EnumerateOptions options;
    options.preset = EnumerateOptions::Preset::Roc;
    options.printInfo = false;
    options.printEnumerations = true;

    // Enumerate solutions
    Enumerator enumerator(options);
    enumerator.Initialize(builder.GetView());

    Assignment model;
    int count = 0;
    while (enumerator.Next(model) == EnumerateStatus::Model)
    {
        count++;
        std::cout << "Solution " << count << ": ";
        for (const auto& [lit, val] : model)
        {
            std::cout << (val == TVal::True ? "" : "-") << (lit / 2) << " ";
        }
        std::cout << std::endl;
    }


    std::cout << "Total solutions: " << count << std::endl;
    return 0;
}
