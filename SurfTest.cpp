// SurfTest.cpp
//

#include <iostream>
#include <string>
#include <cstring>  // for strcmp
#include "VoxelSurfTest.h"

void printHelp()
{
    std::cout << "Usage: program [options]\n";
    std::cout << "Options:\n";
    std::cout << "  -f, --file FILENAME     CSV filename (default: VarSurf.csv)\n";
    std::cout << "  -t, --thresh THRESHOLD  Distance threshold (default: -1.0)\n";
    std::cout << "  -p, --prune             Enable pruning (default: true)\n";
    std::cout << "  -s, --save              Enable saving (default: true)\n";
    std::cout << "  -m, --simplify          Enable simplification (default: true)\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "\nExample:\n";
    std::cout << "  ./program -f data.csv -t 0.5 -p 1 -s 0 -m 1\n";
}

int main(int argc, char* argv[])
{
    // default param.
    std::string filename = "VarSurf.csv";
    double dThresh = -1.0;
    bool bPrune = true;
    bool bSave = true;
    bool bSimpl = true;

    //VoxelSurfTest::ReadCsvVarBlockVTK(filename);

    // param.
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                filename = argv[++i];
            }
            else {
                std::cerr << "Error: Missing filename after " << arg << std::endl;
                return 1;
            }
        }
        else if (arg == "-t" || arg == "--thresh") {
            if (i + 1 < argc) {
                try {
                    dThresh = std::stod(argv[++i]);
                }
                catch (const std::exception& e) {
                    std::cerr << "Error: Invalid threshold value" << std::endl;
                    return 1;
                }
            }
            else {
                std::cerr << "Error: Missing threshold value after " << arg << std::endl;
                return 1;
            }
        }
        else if (arg == "-p" || arg == "--prune") {
            if (i + 1 < argc) {
                std::string val = argv[++i];
                bPrune = (val == "1" || val == "true" || val == "yes");
            }
            else {
                bPrune = false;
            }
        }
        else if (arg == "-s" || arg == "--save") {
            if (i + 1 < argc) {
                std::string val = argv[++i];
                bSave = (val == "1" || val == "true" || val == "yes");
            }
            else {
                bSave = false;
            }
        }
        else if (arg == "-m" || arg == "--simplify") {
            if (i + 1 < argc) {
                std::string val = argv[++i];
                bSimpl = (val == "1" || val == "true" || val == "yes");
            }
            else {
                bSimpl = false;
            }
        }
        else {
            std::cerr << "Error: Unknown argument '" << arg << "'" << std::endl;
            printHelp();
            return 1;
        }
    }

    std::cout << "Running with parameters:\n";
    std::cout << "  Filename: " << filename << std::endl;
    std::cout << "  Threshold: " << dThresh << std::endl;
    std::cout << "  Prune: " << (bPrune ? "true" : "false") << std::endl;
    std::cout << "  Save: " << (bSave ? "true" : "false") << std::endl;
    std::cout << "  Simplify: " << (bSimpl ? "true" : "false") << std::endl;
    std::cout << std::endl;

    VoxelSurfTest::ReadCsvVarBlock(filename, dThresh, bPrune, bSave, bSimpl);

    system("pause");

    return 1;
}

