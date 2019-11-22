#ifndef __CPP_ARG_PARSER_HPP
#define __CPP_ARG_PARSER_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <memory>
#include <sstream>

// #define DEBUG_CPP_ARG_PARSER

#ifdef DEBUG_CPP_ARG_PARSER
#include <stdio.h>
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
    String asString() const {
        return this->mValue;
    }
    int asInt() const { 
        return std::stoi(this->mValue);
    }
    int asLong() const { 
        return std::stol(this->mValue);
    }
    float asFloat() const { 
        return std::stof(this->mValue);
    }
    float asDouble() const { 
        return std::stof(this->mValue);
    }
    float asBool() const { 
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

    String& getKey() {
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

    String& getKey() {
        return this->key;
    }

	const char * what () const throw () {
    	return this->message.c_str();
    }
};


class ArgParser {
private:
    // Stage 1 - Initialization and setup
    Vector<ArgDefPtr> mArgs;
    Map<String, ArgDefPtr> mArgsMap;
    ArgDefPtr mArgFinal = NULL;
    bool mHasFinalArg = false;

    String mArgSplitChar = "=";

    void registerArgDef(ArgDefPtr pArg) {
        this->mArgs.push_back(pArg);
        this->mArgsMap.insert(std::make_pair(pArg->mName, pArg));
        for(const String &str : pArg->mAliases) {
            this->mArgsMap.insert(std::make_pair(str, pArg));
        }
    };

    ArgDefPtr getArgDef(String key) {
        if(this->mArgsMap.find(key) == this->mArgsMap.end()) {
            throw UnknownArgument(key);
        }
        return this->mArgsMap.at(key);
    }

    // Stage 2 - Results after parsing
    ArgParserResult mResult;

    void handleArgPair(String key, String value) {
        ArgPairPtr result = ArgPair::Create();
        result->mArg = this->getArgDef(key);
        if(result->mArg->mIsFlag) {
            result->mValue = "true";
        } else {
            result->mValue = value;
        }
        this->mResult.addResult(result);
    }

    ArgPairPtr getArgPairFromArgDef(ArgDefPtr pArgDef) {
        for(ArgPairPtr &pair : this->mResult.mResults) {
            if(pair->mArg == pArgDef) {
                return pair;
            }
        }
        return NULL;
    }

    ArgPairPtr getArgResult(String key) {
        if(this->mResult.mResultMap.find(key) == this->mResult.mResultMap.end()) {
            throw MissingArgument(key);
        }
        return this->mResult.mResultMap.at(key);
    }

public:
    ArgParser() {

    };

    String getHelp() {
        std::stringstream ss;
        ss << "Help:\n";
        
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

        return ss.str();
    }

    const ArgPair& operator[](const char* key) {
        ArgPairPtr def = this->getArgResult(key);
        return *def;
    };

    // Parsing functions

    void parse(int argc, char const* const* argv) {
        this->mResult.clear();

        int firstArgIndex = 1; // Skip the first arg, usually the executable
        int lastArgIndex = argc;
        if(this->mHasFinalArg) {
            lastArgIndex--;
        }

        LOG("Parsing\n");

        bool isKey = true, isDone = false;
        String key, val;

        for(int i = firstArgIndex; i < lastArgIndex; i++) {
            String component = String(argv[i]);
            
            LOG("Handling %s\n", component.c_str());

            if(isKey) {
                if(SplitArg(component, key, val)) {
                    isDone = true;
                } else {
                    key = component;
                    ArgDefPtr def = this->getArgDef(key);
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
                this->handleArgPair(key, val);
                key = "";
                val = "";
            }
        }
        if(!isKey) {
            LOG("Left over argument!\n");
        }
        if(this->mHasFinalArg) {
            String finalArg = String(argv[argc-1]);
            this->handleArgPair(this->mArgFinal->mName, finalArg);
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
    }

    // Config functions

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

    void addFlag(String pName, String pCSAliases, String pDesc="") {
        LOG("Adding Arg (%s) -> %s\n", pName.c_str(), pDesc.c_str());
        ArgDefPtr arg = ArgDef::Create();
        arg->mName = pName;
        SplitCsvStr(pCSAliases, arg->mAliases);
        arg->mDescription = pDesc;
        arg->mOptional = true;
        arg->mIsFlag = true;

        this->registerArgDef(arg);
    };

    void setFinalArg(String pName, String pDesc="") {
        ArgDefPtr arg = ArgDef::Create();
        arg->mName = pName;
        arg->mDescription = pDesc;
        arg->mOptional = false;
        this->mHasFinalArg = true;
        arg->mIsFlag = false;

        this->mArgFinal = arg;
    }

    // String utils

private:
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