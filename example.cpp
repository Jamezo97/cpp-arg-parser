#include "cppargparser.hpp"
#include <iostream>

using namespace argparse;

int main(int argc, char** argv) {
    ArgParser argParser;
    argParser.setProgramName("cpp-arg-parser-test");
    argParser.addFlag("--colour", "-c", "Enable colour");
    argParser.addArg("--output", "-o", "Output folder. Default to working directory");
    argParser.addArg("--threads", "-t", "Number of threads to use", false);
    argParser.setFinalArg("file", "The input file or folder to process");

    std::cout << argParser.getHelp() << std::endl;
    
    argParser.parse(argc, argv);

    bool doColour = argParser["--colour"].asBool(false);
    String file = argParser["file"].asString();
    String output = argParser["--output"].asString();
    int threads = argParser["--threads"].asInt();

    std::cout << "Found params: " << std::endl;

    std::cout << "doColour: " << doColour << std::endl;
    std::cout << "output: " << output << std::endl;
    std::cout << "threads: " << threads << std::endl;
    std::cout << "file: " << file << std::endl;

}