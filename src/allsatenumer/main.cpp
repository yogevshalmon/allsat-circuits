#include <iostream>
#include <vector>
#include <signal.h>

#include "AllSatGloblas.hpp"
#include "AllSatEnumerDualRail.hpp"

using namespace std;

// define global algo for sigHandling
AllSatEnumerBase* allSatAlgo = nullptr;

// function for handling sig
// just print the result with wasInterrupted = true
void sigHandler(int s){
    printf("Caught signal %d\n",s);
    if(allSatAlgo != nullptr)
    {
        allSatAlgo->PrintResult(true);
        delete allSatAlgo;
    }
    exit(1); 
}

int main(int argc, char **argv) 
{
    if (argc < 2) {
        cout << "USAGE: ./allsatenumr-aig <input_file_name> <with_rep> <print_enumer> <topor_mode>" << endl;
        cout << "where <input_file_name> is the path to a aag instance in Aiger format" << endl;
        cout << "where <with_rep> represent if to use repetition" << endl;
        cout << "\t Accepeted Values: [{0/false},{1/true}] \n\t defualt value: 1" << endl;
        cout << "where <print_enumer> represent if to print the enumerations found" << endl;
        cout << "\t Accepeted Values: [{0/false},{1/true}] \n\t defualt value: 0" << endl;
        cout << "where <topor_mode> represent the topor mode" << endl;
        cout << "\t Accepeted Values: [0,1,2,3,4,5,6,7] \n\t defualt value: 5" << endl;
        cout << "Runnig example: \n\t allsatenumr-aig inputs/test1.aag 0 1" << endl;
        return 1;
    }

    bool w_rep = true;
    if (argc >=3)
    {   
        string w_rep_arg_val = argv[2];
        if (w_rep_arg_val == "true" || w_rep_arg_val == "1")
            w_rep = true;
        else if (w_rep_arg_val == "false" || w_rep_arg_val == "0")
            w_rep = false;
        else
        {
            cout << "ERROR: parsing <with_rep> argument\n";
            return -1;
        }
    }

    bool printEnumer = false;
    if (argc >=4)
    {
        string printEnumerArgVal = argv[3];
        if (printEnumerArgVal == "true" || printEnumerArgVal == "1")
            printEnumer = true;
        else if (printEnumerArgVal == "false" || printEnumerArgVal == "0")
            printEnumer = false;
        else
        {
            cout << "ERROR: parsing <print_enumer> argument\n";
            return -1;
        }
              
    }

    int topor_mode = 5;

    if (argc >=5)
    {
        try
        {
            topor_mode = std::stoi(argv[3]); 
        }
        catch(const exception& e)
        {
            cout << "ERROR: parsing <topor_mode> argument\n";
            return -1;
        }
              
    }

	allSatAlgo = new AllSatEnumerDualRail(w_rep, topor_mode, printEnumer);

    // define sigaction for catchin ctr+c etc..
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);


    try
    { 
        allSatAlgo->InitializeSolver(argv[1]);
        allSatAlgo->FindAllEnumer();
        allSatAlgo->PrintResult();
    }
    catch (exception& ex)
    {
        delete allSatAlgo;
        cout << "Error acord: " << ex.what() << endl;
        return -1;
    }

    delete allSatAlgo;

    return 0;
}
