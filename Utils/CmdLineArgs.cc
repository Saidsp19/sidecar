#include <algorithm> // for std::find_if
#include <iostream>

#include "Utils/CmdLineArgs.h"
#include "Utils/Wrapper.h" // for Utils::wrap

using namespace Utils;

void
CmdLineArgs::Options::add(const std::string& name, const std::string& alt, const std::string& value)
{
    values_[name].push_back(value);
    if (!alt.empty()) values_[alt].push_back(value);
}

bool
CmdLineArgs::Options::has(const std::string& key, std::string& value) const
{
    OptionMap::const_iterator pos = values_.find(key);
    auto rc = pos != values_.end();
    if (rc) value = pos->second[0];
    return rc;
}

CmdLineArgs::CmdLineArgs(size_t argc, char* const* argv, const std::string& description, const OptionDefVector& optDefs,
                         const ArgumentDefVector& argDefs, bool quitOnError) :
    argc_(argc),
    argv_(argv), fullPath_(*argv), progName_(), description_(description), optDefs_(optDefs), argDefs_(argDefs),
    opts_(), args_(), cmdline_(""), quitOnError_(quitOnError)
{
    initialize();
}

CmdLineArgs::CmdLineArgs(size_t argc, char* const* argv, const std::string& description, const OptionDef* optDefs,
                         size_t sizeofOptDefs, const ArgumentDef* argDefs, size_t sizeofArgDefs, bool quitOnError) :
    argc_(argc),
    argv_(argv), fullPath_(*argv), progName_(), description_(description), optDefs_(), argDefs_(), opts_(), args_(),
    cmdline_(""), quitOnError_(quitOnError)
{
    if (optDefs) {
        auto count = sizeofOptDefs / sizeof(OptionDef);
        while (count-- > 0) optDefs_.push_back(*optDefs++);
    }

    if (argDefs) {
        auto count = sizeofArgDefs / sizeof(ArgumentDef);
        while (count-- > 0) argDefs_.push_back(*argDefs++);
    }

    initialize();
}

void
CmdLineArgs::initialize()
{
    // Remember first argument as the program name, but strip off any path
    //
    progName_ = (--argc_, *argv_++);
    auto pos = progName_.rfind('/');
    if (pos != std::string::npos) progName_.erase(0, pos + 1);
    processOptions();
    processArguments();
}

void
CmdLineArgs::help(bool quit) const
{
    printUsage();
    if (quit) ::exit(0);
}

void
CmdLineArgs::usage(const std::string& err, bool quit) const
{
    std::cerr << "*** ERR: " << err << " ***\n";
    help(quit);
    throw std::runtime_error("CmdLineArgs::usage");
}

bool
CmdLineArgs::hasArg(size_t index, std::string& value) const
{
    auto rc = index < args_.size();
    if (rc) value = args_[index];
    return rc;
}

const char*
CmdLineArgs::getArg()
{
    if (argc_ == 0) throw std::logic_error("CmdLineArgs::getArg");
    auto arg = (--argc_, *argv_++);
    if (*arg) {
        if (cmdline_.size()) cmdline_ += ' ';
        cmdline_ += arg;
    }
    return arg;
}

/** Functor used to print out a usage summary for an OptionDef instance.
 */
struct PrintSummaryOpt {
    /** Constructor.

        \param line reference to line buffer use for output
    */
    PrintSummaryOpt(std::string& line) : line_(line) {}

    /** Functor method that adds option flags to the line buffer.

        \param opt OptionDef instance to work with
    */
    void operator()(const CmdLineArgs::OptionDef& opt) const
    {
        bool hasLong = opt.longName && opt.longName[0];
        line_ += " [";
        if (opt.shortName) {
            line_ += '-';
            line_ += opt.shortName;
            if (hasLong) line_ += '|';
        }
        if (hasLong) {
            line_ += "--";
            line_ += opt.longName;
        }
        if (opt.valueTag && opt.valueTag[0]) {
            line_ += ' ';
            line_ += opt.valueTag;
        }
        line_ += ']';
    }

private:
    std::string& line_;
};

/** Functor used to print out a usage summary for an ArgumentDef instance.
 */
struct PrintSummaryArg {
    /** Constructor.

        \param l prefix value for the line
    */
    PrintSummaryArg(std::string& l) : line_(l), optional_(false) {}

    /** Functor method that add argument names to the internal buffer.

        \param arg ArgumentDef instance to work with
    */
    void operator()(const CmdLineArgs::ArgumentDef& arg)
    {
        if (arg.name == 0 || arg.name[0] == 0) {
            optional_ = true;
            line_ += " [";
        } else {
            line_ += ' ';
            line_ += arg.name;
        }
    }

    /** Conversion operator. Obtain the flag indicating the start of optional arguments.

        \return true if saw start of optional arguments
    */
    operator bool() const { return optional_; }

private:
    std::string& line_;
    bool optional_;
};

/** Functor used to print out usage detail for an OptionDef instance.
 */
struct PrintDetailOpt {
    /** Functor method that writes out a formatted representation of an OptionDef instance to the C++ standard
        output stream.

        \param opt OptionDef reference to write out
    */
    void operator()(const CmdLineArgs::OptionDef& opt) const
    {
        auto hasLong = opt.longName && opt.longName[0];
        std::string line("");
        if (opt.shortName) {
            line += '-';
            line += opt.shortName;
            if (hasLong) line += '|';
        }
        if (hasLong) {
            line += "--";
            line += opt.longName;
        }
        line += ": ";
        line += opt.help;
        std::cerr << wrap(line) << '\n';
    }
};

/** Functor used to print out usage detail for an ArgumentDef instance.
 */
struct PrintDetailArg {
    /** Functor method that writes out a formatted representation of an ArgumentDef instance to the C++ standard
        error stream.

        \param arg ArgumentDef reference to write out
    */
    void operator()(const CmdLineArgs::ArgumentDef& arg) const
    {
        if (arg.name && arg.name[0]) {
            std::string line("");
            line += arg.name;
            line += ": ";
            line += arg.help;
            std::cerr << wrap(line) << '\n';
        } else {
            std::cerr << "-- Optional Arguments --\n";
        }
    }
};

void
CmdLineArgs::printUsage() const
{
    std::string line("Usage: ");
    line += progName_;
    line += " [-?|--help]";
    std::for_each(optDefs_.begin(), optDefs_.end(), PrintSummaryOpt(line));
    if (std::for_each(argDefs_.begin(), argDefs_.end(), PrintSummaryArg(line))) { line += "]"; }

    std::cerr << wrap(line, "  ") << '\n' << wrap(description_, "  ") << "\n-?|--help: print out this message\n";
    std::for_each(optDefs_.begin(), optDefs_.end(), PrintDetailOpt());
    std::for_each(argDefs_.begin(), argDefs_.end(), PrintDetailArg());
}

const CmdLineArgs::OptionDef&
CmdLineArgs::findOptionDef(char opt) const
{
    auto pos = std::find_if(optDefs_.begin(), optDefs_.end(), [opt](auto& def) { return opt == def.shortName; });
    if (pos == optDefs_.end()) error("invalid option", opt);
    return *pos;
}

void
CmdLineArgs::verifyOptionDef(char opt) const
{
    auto pos = std::find_if(optDefs_.begin(), optDefs_.end(), [opt](auto& def) { return opt == def.shortName; });
    if (pos == optDefs_.end()) { throw std::logic_error(std::string("verifyOptionDef: invalid option - ") + opt); }
}

/** Functor used to locate an OptionDef with a certain longName value.
 */
struct FindLongName {
    /** Constructor.

        \param name option to look for
    */
    FindLongName(const std::string& name) : name_(name) {}

    /** Functor method that checks if an OptionDef entry has the long name we are looking for.

        \param def OptionDef entry to compare

        \return true if matched
    */
    bool operator()(const CmdLineArgs::OptionDef& def) const { return def.longName == name_; }

private:
    const std::string& name_;
};

const CmdLineArgs::OptionDef&
CmdLineArgs::findOptionDef(const std::string& opt) const
{
    auto pos = std::find_if(optDefs_.begin(), optDefs_.end(), [opt](auto& def) { return opt == def.longName; });
    if (pos == optDefs_.end()) error("invalid option", opt);
    return *pos;
}

void
CmdLineArgs::verifyOptionDef(const std::string& opt) const
{
    auto pos = std::find_if(optDefs_.begin(), optDefs_.end(), [opt](auto& def) { return opt == def.longName; });
    if (pos == optDefs_.end()) { throw std::logic_error(std::string("verifyOptionDef: invalid option - ") + opt); }
}

void
CmdLineArgs::processOptions()
{
    // Keep looping while we have options to process
    //
    while (argc_ > 0 && **argv_ == '-') {
        std::string opt(getArg());

        // Explicitly finished option processing?
        //
        if (opt == "-" || opt == "--") break;

        // Looking for help?
        //
        if (opt == "-?" || opt == "--help") help();

        if (opt[1] == '-') {
            processLongOption(opt);
        } else {
            processShortOption(opt);
        }
    }
}

void
CmdLineArgs::processShortOption(const std::string& opt)
{
    // Do we have an option of the form `-a=value'?
    //
    std::string value("");
    if (opt.size() > 2 && opt[2] == '=') {
        auto c = opt[1];
        auto def = findOptionDef(c);
        if (def.valueTag == 0 || def.valueTag[0] == 0) { error("cannot assign value to option", c); }
        opts_.add(std::string(1, c), def.longName, opt.substr(3));
    } else {
        // Process single-letter options. Those without required values can occur grouped together.
        //
        for (auto pos = 1; pos < opt.size(); ++pos) {
            auto c = opt[pos];
            auto def = findOptionDef(c);

            // Requires a value?
            //
            if (def.valueTag && def.valueTag[0]) {
                // Is this the only option present in the command-line argument?
                //
                if (opt.size() == 2) {
                    // Do we have another argument to work with?
                    //
                    if (argc_ == 0) {
                        std::string err("missing ");
                        err += def.valueTag;
                        err += " for option";
                        error(err.c_str(), c);
                    }
                    value = getArg();
                } else {
                    error("cannot assign value to option within arg", opt);
                }
            }

            opts_.add(std::string(1, c), def.longName, value);
            value = "";
        }
    }
}

void
CmdLineArgs::processLongOption(std::string opt)
{
    // Strip off `--' prefix
    //
    opt.erase(0, 2);

    // See if option has an attached value. If so, save the value and strip from the option name.
    //
    std::string value("");
    auto equalPos = opt.find('=');
    if (equalPos != std::string::npos) {
        value = opt.substr(equalPos + 1);
        opt.erase(equalPos);
    }

    // Does the option require an argument?
    //
    auto def = findOptionDef(opt);
    if (def.valueTag && def.valueTag[0]) {
        // Yes. If none was provided via `=' then take the next argv value for it.
        //
        if (equalPos == std::string::npos) {
            if (argc_ == 0) {
                std::string err("missing ");
                err += def.valueTag;
                err += " for option";
                error(err.c_str(), opt);
            }
            value = getArg();
        }
    }

    // Nope. So there should not have been an `=' found.
    //
    else if (equalPos != std::string::npos) {
        error("cannot assign value to option", opt);
    }

    opts_.add(opt, std::string(1, def.shortName), value);
}

void
CmdLineArgs::processArguments()
{
    // Process the remaining command-line arguments per the argument definition supplied by the user. This
    // details required arguments. Any optional arguments can be seen via the argc and argv methods.
    //
    auto pos = argDefs_.begin();
    auto end = argDefs_.end();
    auto optional = false;

    while (pos != end) {
        if (pos->name == 0 || pos->name[0] == 0) {
            optional = true;
        } else if (argc_ == 0) {
            if (optional) break;
            error("missing required argument", pos->name);
        } else {
            args_.push_back(getArg());
        }
        ++pos;
    }

    if (optional) {
        while (argc_) args_.push_back(getArg());
    }
}

void
CmdLineArgs::error(const char* txt, char opt) const
{
    std::string err(txt);
    err += " - `";
    err += opt;
    err += "'";
    usage(err, quitOnError_);
}

void
CmdLineArgs::error(const char* txt, const std::string& opt) const
{
    std::string err(txt);
    err += " - `";
    err += opt;
    err += "'";
    usage(err, quitOnError_);
}
