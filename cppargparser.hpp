#ifndef __CPP_ARG_PARSER_HPP
#define __CPP_ARG_PARSER_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <memory>
#include <sstream>

#include <stdio.h>

// #define DEBUG_CPP_ARG_PARSER

#ifdef DEBUG_CPP_ARG_PARSER
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

namespace argparse {

using String = std::string;
template<typename T>
using Vector = std::vector<T>;
template<typename K, typename V>
using Map = std::map<K, V>;

struct ArgDef {
    String mName;
    Vector<String> mAliases;
    String mDescription;
    bool mOptional;
    bool mIsFlag;
    static std::shared_ptr<ArgDef> Create() {
        return std::make_shared<ArgDef>();
    }
};
using ArgDefPtr = std::shared_ptr<ArgDef>;

struct ArgPair {
    ArgDefPtr mArg;
    String mValue;
    static std::shared_ptr<ArgPair> Create() {
        return std::make_shared<ArgPair>();
    }
public:
    String asString(String pDefault="") const {
        if(this->mValue.length() == 0) return pDefault;
        return this->mValue;
    }
    int asInt(int pDefault=0) const { 
        if(this->mValue.length() == 0) return pDefault;
        return std::stoi(this->mValue);
    }
    long asLong(long pDefault=0) const { 
        if(this->mValue.length() == 0) return pDefault;
        return std::stol(this->mValue);
    }
    float asFloat(float pDefault=0) const { 
        if(this->mValue.length() == 0) return pDefault;
        return std::stof(this->mValue);
    }
    double asDouble(long pDefault=0) const { 
        if(this->mValue.length() == 0) return pDefault;
        return std::stod(this->mValue);
    }
    bool asBool(bool pDefault=false) const { 
        if(this->mValue.length() == 0) return pDefault;
        return this->mValue == "true" || this->mValue == "yes";
    }
};
using ArgPairPtr = std::shared_ptr<ArgPair>;

struct ArgParserResult {
    Vector<ArgPairPtr> mResults;
    Map<String, ArgPairPtr> mResultMap;

    void addResult(ArgPairPtr result) {
        mResults.push_back(result);
        mResultMap.insert(std::make_pair(result->mArg->mName, result));
    }

    void clear() {
        mResults.clear();
        mResultMap.clear();
    }
};

class UnknownArgument : public std::exception {
    String message, key;
public:
    UnknownArgument(String key) {
        this->message = "Unknown argument: " + key;
        this->key = key;
    }

    const String& getKey() const {
        return this->key;
    }

	const char * what () const throw () {
    	return this->message.c_str();
    }
};
class MissingArgument : public std::exception {
    String message, key;
public:
    MissingArgument(String key) {
        this->message = "Missing mandatory argument: " + key;
        this->key = key;
    }

    const String& getKey() const {
        return this->key;
    }

	const char * what () const throw () {
    	return this->message.c_str();
    }
};
class MissingValue : public std::exception {
    String message, key;
public:
    MissingValue(String key) {
        this->message = "Found argument " + key + "with no value";
        this->key = key;
    }

    const String& getKey() const {
        return this->key;
    }

	const char * what () const throw () {
    	return this->message.c_str();
    }
};

/**
 * @brief C++ Argument Parser main class
 * Prepare the parser arguments with addArg, addFlag, setFinalArg,
 * and config parsing parameters with setProgramName, setCatchExceptions, setPrintHelpOnCaughtException
 */
class ArgParser {
private:
    // Stage 1 - Initialization and setup
    /// The name of the program
    String mProgramName;
    /// Config: Catch errors instead of throwing them up the call stack
    bool mCfgCatchExceptions;
    /// Config: On a caught error, print help info to the console
    bool mCfgPrintHelpOnCaughtException;

    /// The configured arguments
    Vector<ArgDefPtr> mArgs;
    /// A name->argument mapping
    Map<String, ArgDefPtr> mArgsMap;
    /// The final argument, if configured. NULL otherwise
    ArgDefPtr mArgFinal = NULL;
    /// True if the parser is configured with a final argument. Same as 'mArgFinal != null'
    bool mHasFinalArg = false;
    /// The character to split arguments by if possible. e.g. --config=./theConfig.cfg
    String mArgSplitChar = "=";

    /**
     * @brief Register the given argument definition in the parser.
     * Adds it to the list and map. Adds aliases to the map too
     */
    void registerArgDef(ArgDefPtr pArg) {
        this->mArgs.push_back(pArg);
        this->mArgsMap.insert(std::make_pair(pArg->mName, pArg));
        for(const String &str : pArg->mAliases) {
            this->mArgsMap.insert(std::make_pair(str, pArg));
        }
    };

    /**
     * @brief Get the argument definition given its name or alias.
     * If it doesn't exist, will throw an UnknownArgument exception
     * @throw UnknownArgument
     */
    ArgDefPtr getArgDef(String key) {
        if(this->mArgsMap.find(key) == this->mArgsMap.end()) {
            throw UnknownArgument(key);
        }
        return this->mArgsMap.at(key);
    }

    // Stage 2 - Results after parsing

    /// A struct for storing results in. Makes life a bit cleaner
    ArgParserResult mResult;

    /**
     * @brief Adds the found argument (and its optional value) to the result struct
     * Does some special case processing for flags
     */
    void handleArgPair(ArgDefPtr def, String value) {
        ArgPairPtr result = ArgPair::Create();
        result->mArg = def;
        if(result->mArg->mIsFlag) {
            result->mValue = "true";
        } else {
            result->mValue = value;
        }
        this->mResult.addResult(result);
    }

    /**
     * @brief Finds the first argument pair given an argument definition
     * @return NULL if not found
     */
    ArgPairPtr getArgPairFromArgDef(ArgDefPtr pArgDef) {
        for(ArgPairPtr &pair : this->mResult.mResults) {
            if(pair->mArg == pArgDef) {
                return pair;
            }
        }
        return NULL;
    }

    /**
     * @brief Finds the first argument pair given an argument name
     * @throw MissingArgument If the arg pair doesn't exist
     */
    ArgPairPtr getArgPairFromArgName(String key) {
        if(this->mResult.mResultMap.find(key) == this->mResult.mResultMap.end()) {
            throw MissingArgument(key);
        }
        return this->mResult.mResultMap.at(key);
    }

public:
    ArgParser() {
        this->mProgramName = "PROGRAM";
        this->mCfgCatchExceptions = false;
        this->mCfgPrintHelpOnCaughtException = true;
    };

    /**
     * @brief Creates a help string for the configured arg parser
     */
    String getHelp() {
        std::stringstream ss;
        ss << "Example Command: \n";
        ss << "  " << this->getExampleCommand() << "\n\n";
        
        for(ArgDefPtr &def : this->mArgs) {
            ss << "  " << def->mName;
            for(const String &alias : def->mAliases) {
                ss << ", " << alias;
            }
            if(!def->mIsFlag) {
                ss << " <value>";
            }
            ss << "\n    " << def->mDescription;
            if(!def->mIsFlag) {
                ss << (def->mOptional?": Optional":": Mandatory");
            };
            ss << "\n\n";
        }
        if(this->mHasFinalArg) {
            ss << "  " << this->mArgFinal->mName;
            ss << "\n    " << this->mArgFinal->mDescription << "\n\n";
        }
        return ss.str();
    }

    /**
     * @brief Creates an example command for the configured arg parser
     */
    String getExampleCommand() {
        std::stringstream ss;
        ss << this->mProgramName << " ";
        
        for(ArgDefPtr &def : this->mArgs) {
            if(def->mIsFlag) {
                ss << "[" << def->mName << "] "; 
            } else if(def->mOptional) {
                ss << "[" << def->mName << " <value>] "; 
            } else {
                ss << "<" << def->mName << " <value>> "; 
            }
        }
        if(this->mHasFinalArg) {
            ss << this->mArgFinal->mName;
        }
        return ss.str();
    }
    /**
     * @brief Get the argument pair given the argument name.
     * @throw MissingArgument if the pair doesn't exist
     */
    const ArgPair& operator[](const char* key) {
        ArgPairPtr def = this->getArgPairFromArgName(key);
        return *def;
    };

    // Parsing functions

    /**
     * @brief Parse the command line arguments, validating them too
     * Finds all arguments and flags. If a non-optional argument is missing,
     * will throw a MissingArgument exception. If an optional argument is missing,
     * will populate the result with an empty string for the argument.
     * If there is an argument without a value, will throw 'MissingValue' exception.
     * 
     * If configured, will catch the exceptions instead, and only return true/false
     * If configured, will print help / error messages to stdout and stderr.
     */
    bool parse(int argc, char const* const* argv) {
        try {
            this->mResult.clear();

            int firstArgIndex = 1; // Skip the first arg, usually the executable
            int lastArgIndex = argc;
            if(this->mHasFinalArg) {
                lastArgIndex--;
            }

            String key, val;
            ArgDefPtr def = NULL;
            bool isKey = true, isDone = false;
            // For every valid argument, find the key-value pair. Or just 'key' if it's a flag.
            for(int i = firstArgIndex; i < lastArgIndex; i++) {
                String component = String(argv[i]);
                
                LOG("Handling %s\n", component.c_str());

                if(isKey) {
                    if(SplitArg(component, key, val)) {
                        def = this->getArgDef(key);
                        isDone = true;
                    } else {
                        key = component;
                        def = this->getArgDef(key);
                        // If it's a flag. e.g. it's existence determines its value
                        if(def->mIsFlag) {
                            isDone = true;
                        } else {
                            isKey = false;
                        }
                    }
                } else {
                    val = component;
                    isDone = true;
                    isKey = true;
                }

                if(isDone) {
                    isDone = false;
                    LOG("Got: (%s) -> (%s)\n", key.c_str(), val.c_str());
                    this->handleArgPair(def, val);
                    key = "";
                    val = "";
                    def = NULL;
                }
            }
            // We found a key but not value for it. Must be missing something
            if(!isKey) {
                throw MissingValue(key);
            }
            // Configure the mandatory final argument if applicable
            if(this->mHasFinalArg) {
                String finalArg = String(argv[argc-1]);
                this->handleArgPair(this->mArgFinal, finalArg);
            }
            // Validate
            for(ArgDefPtr &def : this->mArgs) {
                if(!def->mOptional) {
                    // Not an optional arg, validate then that it has been found
                    ArgPairPtr found = this->getArgPairFromArgDef(def);
                    if(found == NULL) {
                        throw MissingArgument(def->mName);
                    }
                } else if(def->mIsFlag) {
                    ArgPairPtr found = this->getArgPairFromArgDef(def);
                    if(found == NULL) {
                        // Add missing flags (just fudge it mate) with value of false
                        ArgPairPtr fudge = ArgPair::Create();
                        fudge->mArg = def;
                        fudge->mValue = "false";
                        this->mResult.addResult(fudge);
                    }
                } else if(def->mOptional) {
                    ArgPairPtr found = this->getArgPairFromArgDef(def);
                    if(found == NULL) {
                        // Fudge an empty parameter
                        ArgPairPtr fudge = ArgPair::Create();
                        fudge->mArg = def;
                        fudge->mValue = "";
                        this->mResult.addResult(fudge);
                    }
                }
            }
            return true; // Success

            // Catch errors
        } catch(const UnknownArgument &except) {
            if(this->mCfgCatchExceptions) {
                if(this->mCfgPrintHelpOnCaughtException) {
                    fprintf(stderr, "Error: Unknown argument provided: %s\n", except.getKey().c_str());
                }
            } else {
                throw except;
            }
        } catch(const MissingArgument &except) {
            if(this->mCfgCatchExceptions) {
                if(this->mCfgPrintHelpOnCaughtException) {
                    fprintf(stderr, "Error: Required argument %s is missing\n", except.getKey().c_str());
                }
            } else {
                throw except;
            }
        } catch(const MissingValue &except) {
            if(this->mCfgCatchExceptions) {
                if(this->mCfgPrintHelpOnCaughtException) {
                    fprintf(stderr, "Error: Last argument %s is missing corresponding value\n", except.getKey().c_str());
                    if(this->mHasFinalArg) {
                        fprintf(stderr, "  Hint: Did you forget the final argument '%s'?\n", this->mArgFinal->mName.c_str());
                    }
                }
            } else {
                throw except;
            }
        }
        if(this->mCfgPrintHelpOnCaughtException) {
            printf("%s", this->getHelp().c_str());
        }

        return false; // Something went wrong
    }

    // Config functions

    /**
     * @brief Configures the name of the program. Only used in the help string
     */
    void setProgramName(String pName) {
        this->mProgramName = pName;
    }

    /**
     * @brief Will force the parse function to catch exceptions
     * You can check if the arguments were passed by looking at
     * the returned boolean from the parse function.
     * @param pCatchExceptions If true, will catch exceptions in 'parse'
     */
    void setCatchExceptions(bool pCatchExceptions) {
        this->mCfgCatchExceptions = pCatchExceptions;
    }

    /**
     * @brief If the exception is caught, will print help messages.
     * You can enable exception catching through 'setCatchExceptions', and then 
     * enable printed help messages with this function.
     * You can of course print the help manually with 'getHelp'
     * @param pPrintHelpOnCaughtException If true, will print on caught exceptions
     */
    void setPrintHelpOnCaughtException(bool pPrintHelpOnCaughtException) {
        this->mCfgPrintHelpOnCaughtException = pPrintHelpOnCaughtException;
    }

    /**
     * @brief Add an argument that requires a value.
     * e.g. Not a flag, but something like '--threads 12'
     * 
     * @param pName The full name of the argument, including an prefix. e.g. --thread
     * @param pCSAliases Comma-separated aliases, including prefixes. e.g. "-t,-h"
     * @param pDesc Description of the parameter
     * @param opt True if this parameter is optional
     */
    void addArg(String pName, String pCSAliases, String pDesc="", bool opt=true) {
        LOG("Adding Arg (%s) -> %s\n", pName.c_str(), pDesc.c_str());
        ArgDefPtr arg = ArgDef::Create();
        arg->mName = pName;
        SplitCsvStr(pCSAliases, arg->mAliases);
        arg->mDescription = pDesc;
        arg->mOptional = opt;
        arg->mIsFlag = false;

        this->registerArgDef(arg);
    };

    /**
     * @brief Add an argument that doesn't require a value
     * e.g. '--enablecolour'
     * 
     * @param pName The full name of the argument, including an prefix. e.g. --enablecolour
     * @param pCSAliases Comma-separated aliases, including prefixes. e.g. "-e,-c"
     * @param pDesc Description of the parameter
     */
    void addFlag(String pName, String pCSAliases, String pDesc="") {
        LOG("Adding Arg (%s) -> %s\n", pName.c_str(), pDesc.c_str());
        ArgDefPtr arg = ArgDef::Create();
        arg->mName = pName;
        SplitCsvStr(pCSAliases, arg->mAliases);
        arg->mDescription = pDesc;
        arg->mOptional = true; /// Flags are always optional. There presence indices they're true
        arg->mIsFlag = true;

        this->registerArgDef(arg);
    };

    /**
     * @brief Set the mandatory final argument
     * e.g. 'myProgram --arg1 val1 --arg2=val2 --flag1 FINAL_ARGUMENT'
     * 
     * @param pName The full name of the argument
     * @param pDesc Description of the argument
     */
    void setFinalArg(String pName, String pDesc="") {
        ArgDefPtr arg = ArgDef::Create();
        arg->mName = pName;
        arg->mDescription = pDesc;
        arg->mOptional = false;
        arg->mIsFlag = false;

        this->mHasFinalArg = true;
        this->mArgFinal = arg;
    }

    // String utils

private:
    /// @brief Split a comma-separated string into multiple
    void SplitCsvStr(String input, Vector<String> &output) {
        int inputLength = input.length();
        int start = 0;
        for(int i = 0; i < inputLength; i++) {
            if(input[i] == ',') {
                int end = i;
                int len = end - start;
                if(len > 0) {
                    String result = input.substr(start, len);
                    output.push_back(result);
                }
                start = i+1;
            }
        }
        {
            int end = inputLength;
            int len = end - start;
            if(len > 0) {
                String result = input.substr(start, len);
                output.push_back(result);
            }
        }
    }

    /**
     * @brief Split an argument pair, if possible
     * @param arg The input argument, e.g. '--myarg=true'
     * @param key The output key string, e.g. '--myarg'
     * @param val The output value string, e.g. 'true'
     * @return True if the argument was split, false otherwise
     */
    bool SplitArg(String arg, String &key, String &val) {
        String splitC = this->mArgSplitChar;
        String::iterator result = std::find_first_of(arg.begin(), arg.end(), splitC.begin(), splitC.end());
        if(result != arg.end()) {
            int foundAt = std::distance(arg.begin(), result);
            int foundLen = splitC.length();
            int valStart = foundAt+foundLen;
            key = arg.substr(0, foundAt);
            val = arg.substr(valStart, arg.length() - valStart);
            return true;
        }
        return false;
    }

};

};

#endif //__CPP_ARG_PARSER_HPP

/**
 * @mainpage CPP Argument Parser
 * The "C++ Argument Parser" is a header-only library for parsing
 * command line arguments. 
 * 
 * It makes very few assumptions about the style
 * of arguments. e.g. they can start with '\-\-' or '/'.
 * 
 * Requires C++ 11.
 * 
 * See the example.cpp file for an example.
 * 
 * See ArgParser for more documentation
 */
