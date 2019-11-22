#include "cppargparser.hpp"
#include <iostream>

using namespace argparse;

int main(int argc, char** argv) {
    ArgParser argParser;
    argParser.addArg("--input", "-i", "Input File", false);
    argParser.addFlag("--colour", "-c", "Enable colour");

    std::cout << argParser.getHelp() << std::endl;

    argParser.parse(argc, argv);

    String file = argParser["--input"].asString();
    bool doColour = argParser["--colour"].asBool();

    std::cout << "Found param: " << file << "  DoColour: " << doColour << std::endl;

}