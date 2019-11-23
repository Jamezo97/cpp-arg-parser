#include "cppargparser.hpp"
#include <iostream>
#include <string>

/**
 * @brief Entry point for example
 */
int main(int argc, char** argv) {
    argparse::ArgParser argParser;
    argParser.setProgramName("cpp-arg-parser-test");
    argParser.addFlag("--colour", "-c", "Enable colour");
    argParser.addArg("--output", "-o", "Output folder. Default to working directory");
    argParser.addArg("--threads", "-t", "Number of threads to use", false);
    argParser.setFinalArg("file", "The input file or folder to process");

    argParser.setCatchExceptions(true);
    argParser.setPrintHelpOnCaughtException(true);
    
    if(false == argParser.parse(argc, argv)) {
        return 1;
    }

    bool doColour = argParser["--colour"].asBool(false);
    std::string file = argParser["file"].asString();
    std::string output = argParser["--output"].asString();
    int threads = argParser["--threads"].asInt();

    std::cout << "Found params: " << std::endl;

    std::cout << "doColour: " << doColour << std::endl;
    std::cout << "  output: " << output << std::endl;
    std::cout << " threads: " << threads << std::endl;
    std::cout << "    file: " << file << std::endl;

}