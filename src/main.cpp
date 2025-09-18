#include <iostream>
#include <string>
#include <vector>
#include <set>
#include "segyscanner.h"

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] <input_path>" << std::endl;
    std::cout << "  input_path: Path to SEG-Y file or directory containing SEG-Y files" << std::endl;
    std::cout << "              Supported extensions: .sgy, .segy" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -sou        Generate source tables and maps" << std::endl;
    std::cout << "  -rec        Generate receiver tables and maps" << std::endl;
    std::cout << "  -cdp        Generate CDP tables and maps" << std::endl;
    std::cout << "  -h, --help  Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "  If no domain options are specified, all domains are generated." << std::endl;
    std::cout << "  Options can be combined: -sou -rec (sources and receivers only)" << std::endl;
    std::cout << std::endl;
    std::cout << "Output:" << std::endl;
    std::cout << "  Creates 'segyscan' directory with:" << std::endl;
    std::cout << "    - tables/: Statistical tables for each file" << std::endl;
    std::cout << "    - maps/: Scatter plots of selected domains" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Invalid number of arguments" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    std::set<std::string> domains;
    std::string input_path;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-sou") {
            domains.insert("sou");
        } else if (arg == "-rec") {
            domains.insert("rec");
        } else if (arg == "-cdp") {
            domains.insert("cdp");
        } else if (arg[0] != '-') {
            // This is the input path
            input_path = arg;
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (input_path.empty()) {
        std::cerr << "Error: Input path is required" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    // If no domains specified, use all
    if (domains.empty()) {
        domains.insert("sou");
        domains.insert("rec");
        domains.insert("cdp");
    }
    
    try {
        SegyScanner scanner;
        return scanner.process(input_path, domains);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
