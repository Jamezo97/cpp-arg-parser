# CPP Arg Parser

A C++ 11 header-only argument parser, with no fore-thought and made in 2 hours.

Simply include the header, setup the parser, parse, and grab the results:

```cpp
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
```