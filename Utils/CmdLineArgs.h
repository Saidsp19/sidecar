#ifndef UTILS_CMDLINEARGS_H // -*- C++ -*-
#define UTILS_CMDLINEARGS_H

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "Utils/Exception.h"

namespace Utils {

/** Command-line processor. Visits data found in argc and argv (as given to main(), and creates entries in
    instances of Options and Arguments based on what was found on the command-line. Parsing of the command-line
    is governed by user-supplied arrays that define what options are supported, and what arguments are required.

    @code

    OptionDef opts[] = { { 'd', "debug", "turn on debugging", "" },
                         { 'o', "output", "write output to FILE", "FILE" } };
    ArgumentDef args[] = { { "FILE", "read input from FILE" } };

    int main(int argc, const char* argv[])
    {
      std::string inputFile(""), outputFile("");
      CmdLineArgs cla(argc, argv, opts, sizeof(opts), args, sizeof(args));
      if (cla.hasOpt("debug")) enableDebugging();
      if (cla.hasOpt("output", outputFile)) redirect(outputFile);
      inputFile = cla.arg(0);
      ...
      ...
    }
    @endcode
*/
class CmdLineArgs {
public:
    /** Collection of attributes used to define a command-line option. - shortName the short version (one
        character) of the option name. If 0, then the option does not have a short name. - longName the long
        version of the option name. If an empty string, then the option does not have a long name. - help text
        to print out for this option in `usage' messages - valueTag identifier for an option value. If an empty
        string, the option does not take a value.
    */
    struct OptionDef {
        char shortName;
        const char* longName;
        const char* help;
        const char* valueTag;
    };

    /** Define collection of OptionDef instances.
     */
    using OptionDefVector = std::vector<OptionDef>;

    /** Define macro to help create an OptionDefVector from an C++ array of OptionDefs.
     */
#define OPTION_DEF_VECTOR(A) A, sizeof(A)
#define NULL_OPTION_DEF_VECTOR 0, 0

    /** Collection of attributes used to define a command-line argument. A zero-length name signals the start of
        optional arguments. - name the name of the argument. Only used in `usage' texts. - help text to print
        out for this argument in `usage' messages.
    */
    struct ArgumentDef {
        const char* name;
        const char* help;
    };

    /** Define collection of ArgumentDef instances.
     */
    using ArgumentDefVector = std::vector<ArgumentDef>;

    /** Define macro to help create an ArgumentDefVector from an C++ array of ArgumentDefs.
     */
#define ARGUMENT_DEF_VECTOR(A) A, sizeof(A)
#define NULL_ARGUMENT_DEF_VECTOR 0, 0

    /** Map of options found on the command line. If an option was given a value, then its value is stored;
        otherwise, an empty string is stored for the value.
    */
    class Options {
    public:
        using ValueVector = std::vector<std::string>;
        using OptionMap = std::map<std::string, ValueVector>;

        /** Constructor.
         */
        Options() : values_() {}

        /** Add a new option under one or two names.

            \param name name of the option (long or short name)

            \param alt an alternative name for the option. If `name' is a long name, then this would be a short
            name.

            \param value the value to store under the given name and alt.
        */
        void add(const std::string& name, const std::string& alt, const std::string& value);

        /** Lookup an option to see if it was set by the user.

            \param key the name of the option to look for

            \return TRUE if the option was found
        */
        bool has(const std::string& key) const { return values_.find(key) != values_.end(); }

        /** Lookup an option to see if it was set by the user.

            \param key the name of the option to look for

            \return TRUE if the option was found
        */
        bool has(char key) const { return has(std::string(1, key)); }

        /** Lookup an option to see if it was set by the user. If so, return the first value found under the
            option name.

            \param key the name of the option to look for

            \param value reference to string that is updated with the first option value found. If option was
            not set, then this value is left alone

            \return TRUE if the option was found
        */
        bool has(const std::string& key, std::string& value) const;

        /** Lookup an option to see if it was set by the user. If so, return the first value found under the
            option name.

            \param key the name of the option to look for

            \param value reference to string that is updated with the first option value found. If option was
            not set, then this value is left alone

            \return TRUE if the option was found
        */
        bool has(char key, std::string& value) const { return has(std::string(1, key), value); }

        /** Fetch all of the values for a long option key.

            \param key the name of the option to look for.

            \return vector of values set for the option. If option was never set, size of vector will be zero.
        */
        ValueVector& operator[](const std::string& key) { return values_[key]; }

        /** Fetch all of the values for a short option key.

            \param key the name of the option to look for.

            \return vector of values set for the option. If option was never set, size of vector will be zero.
        */
        ValueVector& operator[](char key) { return operator[](std::string(1, key)); }

    private:
        OptionMap values_; ///< collection of key/value pairs
    };                     // class Options

    /**
       Collection arguments found on the command-line.
    */
    class Arguments : public std::vector<std::string> {
    };

    /** Constructor. Processes the given command-line argument values, argc and argv. These should be the values
        given to the main() routine by the system. The first value in argv is taken as the running program's
        name. After processing, the remaining argc and argv values may be obtained via the CmdLineArgs::argc()
        and CmdLineArgs::argv() methods.

        \param argc number of values in argv

        \param argv array of C strings representing the command line values

        \param description short description of the program

        \param optDefs vector of option definition values

        \param argDefs vector of argument definition values

        \param quitOnError control whether the program call ::exit or throws an exception on an error.
    */
    CmdLineArgs(size_t argc, char* const* argv, const std::string& description, const OptionDefVector& optDefs,
                const ArgumentDefVector& argDefs, bool quitOnError = true);

    /** Constructor. Processes the given command-line argument values, argc and argv. These should be the values
        given to the main() routine by the system. The first value in argv is taken as the running program's
        name. After processing, the remaining argc and argv values may be obtained via the CmdLineArgs::argc()
        and CmdLineArgs::argv() methods.

        \param argc number of values in argv

        \param argv array of C strings representing the command line values

        \param description short description of the program

        \param optDefs array of option definitions

        \param sizeofOptDefs value of the sizeof operator applied to the optDefs value. Note that this must be
        done against the array object itself, and not a pointer to it.

        \param argDefs array of argument definitions

        \param sizeofArgDefs value of the sizeof operator applied to the argDefs value. Note that this must be
        done against the array object itself, and not a pointer to it.

        \param quitOnError control whether the program call ::exit or throws an exception on an error.
    */
    CmdLineArgs(size_t argc, char* const* argv, const std::string& description, const OptionDef* optDefs,
                size_t sizeofOptDefs, const ArgumentDef* argDefs, size_t sizeofArgDefs, bool quitOnError = true);

    /** \return the full path of the executable
     */
    const std::string& fullPath() const { return fullPath_; }

    /** \return the program name as taken from the first value in argv, sans any path.
     */
    const std::string& progName() const { return progName_; }

    /** Write out an error message to std::cerr and exit the program with a non-zero value.

        \param txt error message text

        \param quit if true, call ::exit when done; otherwise throw std::runtime_error exception
    */
    void usage(const std::string& txt, bool quit = true) const;

    /** Show the usage text for the program, and exit with a zero value.
     */
    void help(bool quit = true) const;

    /** Obtain the options set on the command line.

        \return reference to Options instance
    */
    Options& opts() { return opts_; }

    /** Obtain the arguments provided on the command line.

        \return reference to Arguments instance
    */
    Arguments& args() { return args_; }

    /** Lookup an option to see if it was set by the user.

        \param key the name of the option to look for

        \return true if the option was found; false otherwise.
    */
    bool hasOpt(const std::string& key) const { return verifyOptionDef(key), opts_.has(key); }

    /** Lookup an option to see if it was set by the user.

        \param key the name of the option to look for

        \return true if the option was found; false otherwise.
    */
    bool hasOpt(char key) const { return verifyOptionDef(key), opts_.has(std::string(1, key)); }

    /** Lookup an option to see if it was set by the user. If so, return the first value found under the option
        name.

        \param key the name of the option to look for

        \param value reference to string that is updated with the first option value found. If option was not
        set, then this value is left alone

        \return true if the option was found; false otherwise.
    */
    bool hasOpt(const std::string& key, std::string& value) const
    {
        return verifyOptionDef(key), opts_.has(key, value);
    }

    /** Lookup an option to see if it was set by the user. If so, return the first value found under the option
        name.

        \param key the name of the option to look for

        \param value reference to string that is updated with the first option value found. If option was not
        set, then this value is left alone

        \return true if the option was found; false otherwise
    */
    bool hasOpt(char key, std::string& value) const
    {
        return verifyOptionDef(key), opts_.has(std::string(1, key), value);
    }

    /** Fetch all of the values for an option key.

        \param key the name of the option to look for.

        \return vector of values set for the option. If option was never set, size of vector will be zero.
    */
    Options::ValueVector& opt(const std::string& key) { return opts_[key]; }

    /** Fetch all of the values for an option key.

        \param key the name of the option to look for.

        \return vector of values set for the option. If option was never set, size of vector will be zero.
    */
    Options::ValueVector& opt(char key) { return opts_[std::string(1, key)]; }

    /** Check if a certain argument is available.

        \param index which argument to look for

        \return true if the argument is present; false otherwise
    */
    bool hasArg(size_t index) const { return index < args_.size(); }

    /** Check if a certain argument is available. If so, return its value.

        \param index which argument to look for

        \param value reference to string that is updated with the argument value, if the argument exists

        \return TRUE if the argument was found; false otherwise
    */
    bool hasArg(size_t index, std::string& value) const;

    /** Fetch a specific argument value.

        \param index which argument to obtain (zero-based)

        \return argument value
    */
    const std::string& arg(size_t index) const { return args_[index]; }

    /** Obtain the residual argc value after all option and argument processing.

        \return number of values remaining in argv array
    */
    int argc() const { return argc_; }

    /** Obtain the residual argv value after all option and argument processing.

        \return address of first unprocessed argument item
    */
    char* const* argv() const { return argv_; }

    /** Obtain the arguments representing all of the command-line arguments processed up to this point.

        \return string of processed arguments
    */
    const std::string& cmdline() const { return cmdline_; }

private:
    /** Perform common initialization tasks for constructors.
     */
    void initialize();

    /** Fetch the next value from the system-supplied argv value.

        \return next value or throw exception if none available
    */
    const char* getArg();

    /** Locate the OptionDef record that has a given short name. If the option is not found, cause an error
        message to display to the user.

        \param shortName the short option to look for

        \return reference to matching OptionDef. If none found, then an error is printed and the program will
        exit.
    */
    const OptionDef& findOptionDef(char shortName) const;

    /** Locate the OptionDef record that has a given long name. If the option is not found, cause an error
        message to display to the user.

        \param opt the name to look for

        \return reference to matching OptionDef. If none found, then an error is printed and the program will
        exit.
    */
    const OptionDef& findOptionDef(const std::string& opt) const;

    /** Verify that there is an OptionDef record that has a given short name. If the option does not exist,
        throws std::logic_error.

        \param shortName the short option to look for
    */
    void verifyOptionDef(char shortName) const;

    /** Verify that there is an OptionDef record that has a given long name. If the option does not exist,
        throws std::logic_error.

        \param opt the name to look for
    */
    void verifyOptionDef(const std::string& opt) const;

    /** Process the argument list and record which options were found, along with any values they require.
     */
    void processOptions();

    /** Process a long option.

        \param opt option name
    */
    void processLongOption(std::string opt);

    /** Process a short option.

        \param opt option name
    */
    void processShortOption(const std::string& opt);

    /** Process the argument list and record which arguments were provided by the user.
     */
    void processArguments();

    /** Create a `usage' text made up from the user-supplied option and argument definitions, and write to
        std::cerr.
    */
    void printUsage() const;

    /** Convenience method that generates an error message.

        \param txt error text

        \param opt short option that had the error
    */
    void error(const char* txt, char opt) const;

    /** Convenience method that generates an error message.

        \param txt error text

        \param opt long option that had the error
    */
    void error(const char* txt, const std::string& opt) const;

    size_t argc_;               ///< Number of command line arguments left
    char* const* argv_;         ///< Pointer to next command line argument
    std::string fullPath_;      ///< Full path of executable
    std::string progName_;      ///< Name of the running program
    std::string description_;   ///< Description to show in help
    OptionDefVector optDefs_;   ///< Option definitions from user
    ArgumentDefVector argDefs_; ///< Argument definitions from user
    Options opts_;              ///< Options found during processing
    Arguments args_;            ///< Arguments found during processin
    std::string cmdline_;       ///< Copy of the full command line
    bool quitOnError_;          ///< If true, call ::exit after reporting error
};                              // class CmdLineArgs

} // namespace Utils

/** \file
 */

#endif
