#ifndef SIDECAR_CONFIGURATION_LOADER_H // -*- C++ -*-
#define SIDECAR_CONFIGURATION_LOADER_H

#include "QtCore/QList"
#include "QtCore/QMap"
#include "QtCore/QString"
#include "QtCore/QStringList"

#include "boost/scoped_ptr.hpp"

namespace SideCar {
namespace Configuration {

class RunnerConfig;

/** Configuration file loader. Reads in a SideCar configuration file, and parses <radar> and <runner> entity
    tags. For the former, it parses the <radar> children and initializes a Messages::RadarConfig instance with
    the children values. For the <runner> entities, it creates an instance of the RunnerConfig class to store
    the <runner> attributes and to hold the XML DOM node that contains one or more <stream> entities.

    Uses the Qt XML library (QtXml) to parse the whole configuration file.
*/
class Loader
{
public:

    /** Container type for the list of RunnerConfig objects
     */
    using RunnerConfigList = QList<RunnerConfig*>;

    /** Result codes from the last load() operation. Obtained from getLastLoadResult().
     */
    enum LoadResult {
	kOK = 0,		///< The load succeeded
	kFailedFileOpen,	///< Unable to open the configuration file
	kFailedXMLParse,	///< Failed to parse the configuration file
	kMissingEntityNode,	///< Missing a required/expected entity node
	kMissingSidecarNode,	///< Missing a <sidecar> entity node
	kMissingRadarNode,	///< Missing a <radar> entity node
	kInvalidRadarNode,	///< Invalid <radar> entity node
	kMissingDPNode,		///< Missing a <dp> entity node
	kMissingStreams,	///< Missing a <stream> entity node
	kMissingRunners,	///< Missing a <runner> entity node
	kNotLoaded		///< The load() method has not been called
    };

    /** Constructor. Intializes instance but does nothing else.
     */
    Loader();

    /** Destructor. Necessary to allow incomplete boost::scoped_ptr below.
     */
    ~Loader();

    /** Attempt to load a SideCar configuration file.

        \param configurationPath location of the file

        \return true if successful
    */
    bool load(const QString& configurationPath);

    /** Determine if a configuration file has been successfully loaded

        \return true if so
    */
    bool isLoaded() const { return getLastLoadResult() == kOK; }

    /** Obtain a list of file paths that were given in 'file' attributes in the <radar> and/or <runner>
        entities.

        \return list of file paths (may be empty)
    */
    QStringList getIncludePaths() const;

    /** Obtain a list of host names found in <runner> entity 'host' attributes. This is the list of hosts that
        will run remote runner processes.

        \return list of host names (must not be empty)
    */
    QStringList getHostNames() const;

    /** Obtain the result of the last load operation.

        \return LoadResult code
    */
    LoadResult getLastLoadResult() const;

    /** Obtain a RunnerConfig with a given name. Each RunnerConfig has a name that was found in the 'name'
        attribute of a <runner> entity.

        \param name the name of the RunnerConfig to locate

        \return found RunnerConfig object, ur NULL
    */
    RunnerConfig* getRunnerConfig(const QString& name) const;

    /** Obtain a RunnerConfig by its position in the configuration file.

        \param index position of the RunnerConfig to fetch. Must be less than getNumRunnerConfigs().

        \return found RunnerConfig object, or NULL
    */
    RunnerConfig* getRunnerConfig(size_t index) const;

    /** Obtain the number of RunnerConfig objects defined by the loaded configuration file. Only valid if
        isLoaded() returns true.

        \return number defined RunnerConfig objects
    */
    size_t getNumRunnerConfigs() const;

    /** Obtain a read-only reference to the list of loaded RunnerConfig objects.

        \return reference to RunnerConfig list
    */
    const RunnerConfigList& getRunnerConfigs() const;

    /** Obtain the full path of the file that the Qt XML library failed to parse. This may not be the
        configuration file; it could be an include file found in a <radar> or <runner> entity.

        \return file path
    */
    QString getParseFilePath() const;

    /** Obtain parse error information returned by the Qt XML library. Only valid if getLastLoadResult() is
        kFailedXMLParse.

        \param line the number of the line where the parse failed

        \param column the column of line where the parse failed

        \return error text from the XML library
    */
    QString getParseErrorInfo(int& line, int& column) const;

    /** Obtain the full path of the file given in the last call to load().

        \return the configuration file path
    */
    QString getConfigurationPath() const;

    /** Obtain the name of the configuration represented by the configuration file.

        \return base name of the configuration file without extension
    */
    QString getConfigurationName() const;

    /** Obtain the log directory value found in 'logDirectory

        \return log directory path
    */
    QString getLogsDirectory() const;

    /** Obtain the directory where new recordings will reside

        \return recording directory path
    */
    QString getRecordingsDirectory() const;

    /** Obtain the file to give to Logger::ConfiguratorFile to configure Logger::Log devices.

        \return file path
    */
    QString getLoggerConfiguration() const;

private:
    struct Private;
    boost::scoped_ptr<Private> p_;
};

} // end namespace Config
} // end namespace SideCar

#endif
