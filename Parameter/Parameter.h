#ifndef SIDECAR_PARAMETER_PARAMETER_H	// -*- C++ -*-
#define SIDECAR_PARAMETER_PARAMETER_H

#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "boost/signals2.hpp"
#include "boost/shared_ptr.hpp"

#include "IO/CDRStreamable.h"
#include "Utils/Exception.h"
#include "Utils/IO.h"
#include "XMLRPC/XmlRpcValue.h"

namespace Logger { class Log; }

namespace SideCar {
namespace Parameter {

/** Exception thrown if a value given to a parameter is invalid.
 */
class InvalidValue : public Utils::Exception, public Utils::ExceptionInserter<InvalidValue> {
public:

    /** Constructor.

        \param routine name of the routine that created the exception

        \param name name of the parameter that rejected the value
    */
    InvalidValue(const std::string& routine, const std::string& name);
};

/** Abstract base class for named parameter values. Defines the interface all derived classes must provide.
    Parameter values may change at any time due to external messages or signals.

    An object interested in knowning when the internal value of a Value instance changes may register for
    value-change notifications by calling connectChangedSignalTo().

    Currently, there are two kinds of parameters, normal and advanced. They operate the same in all ways except
    when viewed from within the Master application. There, advanced parameters are normally hidden from view,
    and are only revealed when the 'Advanced' checkbox is checked.
*/
class ValueBase : public IO::CDRStreamable<ValueBase> {
public:

    /** Constructor. Sets the XML name and GUI label for the parameter.

        \param name XML name of the parameter

        \param label GUI text label of the parameter
    */
    ValueBase(const std::string& name, const std::string& label)
	: name_(name), label_(label), advanced_(false), editable_(true) {}

    /** Destructor. Here to silence warnings about missing virtual destructors, though it is not needed.
     */
    virtual ~ValueBase() = default;

    /** Obtain the XML name of the parameter.

        \return the XML name
    */
    const std::string& getName() const { return name_; }

    /** Obtain the GUI label of the parameter. This is shown to the user when editing parameter values.

        \return the GUI label
    */
    const std::string& getLabel() const { return label_; }

    /** Determine if the parameter is 'advanced', and not shown to the user by default. Default value is false.

        \return true if so
    */
    bool isAdvanced() const { return advanced_; }

    /** Change the parameter's advanced designation.

        \param state true if advanced
    */
    void setAdvanced(bool state) { advanced_ = state; }

    /** Determine if the parameter is editable by the user. Default value is true.

        \return true if so
    */
    bool isEditable() const { return editable_; }

    /** Change the parameter's editable designation.

        \param state true if editable
    */
    void setEditable(bool state) { editable_ = state; }

    /** Acquire a new parameter value from an ACE CDR input stream. Derived classes must define.
        
	\param cdr stream to read from

	\return true if acquired value was valid and the parameter was set to it
    */
    virtual bool checkedLoad(ACE_InputCDR& cdr) = 0;

    /** Fetch the parameter value from a C++ input stream. Derived class must define.

        \param os stream to read

        \return stream read
    */
    virtual std::istream& load(std::istream& os) = 0;

    /** Write the parameter value to a C++ output stream. Derived classes must define.

        \param os stream to write to

        \return stream written to
    */
    virtual std::ostream& save(std::ostream& os) const = 0;

    /** Load a new parameter value using data from a CDR input stream. Derived classes must define.

        \param cdr stream to read from

        \return stream read from
    */
    virtual ACE_InputCDR& load(ACE_InputCDR& cdr) = 0;

    /** Write the parameter value to a CDR input stream. Derived classes must define.
        
        \param cdr stream to write to

        \return stream written to
    */
    virtual ACE_OutputCDR& write(ACE_OutputCDR& cdr) const = 0;

    /** Set the value using a value contained in an XML-RPC request. Derived classes must define.
        
        \param xml the XML-RPC value to use
    */
    virtual void setXMLValue(XmlRpc::XmlRpcValue& xml) = 0;

    /** Set the original value using a value contained in an XML-RPC request. Derived classes must define.
        
	\param xml the XML-RPC value to use
    */
    virtual void setXMLOriginal(XmlRpc::XmlRpcValue& xml) = 0;

    /** Describe the parameter name attributes using an XML-RPC struct. Derived classes must define.
        
        \param xml XML-RPC container to hold the attribute values.
    */
    virtual void describe(XmlRpc::XmlRpcValue& xml) const = 0;

    /** Determine if the parameter value is different than the value used to initialize the parameter. Derived
        classes must define.
     
        \return true if different
    */
    virtual bool isNotOriginal() const = 0;

private:
    std::string name_;		///< Parameter name
    std::string label_;		///< Text for GUI labels
    bool advanced_;		///< True if an 'advanced' parameter
    bool editable_;		///< True if editing of value is allowed
};

/** Type to use when sharing parameter objects.
 */
using Ref = boost::shared_ptr<ValueBase>;

/** Input stream extractor for object derived from Parameter::ValueBase.

    \param is stream to read from

    \param obj object to load

    \return stream read from
*/
inline std::istream&
operator>>(std::istream& is, ValueBase& obj) { return obj.load(is); }

/** Output stream inserter for objects derived from Parameter::Base.

    \param is stream to write to

    \param obj object to save

    \return stream written to
*/
inline std::ostream&
operator<<(std::ostream& is, const ValueBase& obj) { return obj.save(is); }

/** A template definition for a named parameter value. The sole template parameter defines the capabilities
    available to the template class. The definition object must contain the following items:

    - ValueType: declaration of the type used to hold a parameter value.
    - ReferenceType: declaration for a non-const reference to the value.
    - ConstReferenceType: declaration for a const reference to the value.
    - IsValid: method that should return true if the value it is given is OK.
    - Load: read the parameter value from a C++ input stream.
    - Save: write out the parameter name and value to a C++ output stream.
    - Load: read the parameter value from an ACE CDR input stream.
    - Write: write out the parameter name and value to an ACE CDR output stream.
    - GetXMLTypeName: obtain the XML-RPC type designator
    - FromXML: convert an XML-RPC value into a native value
    - DescribeXML: fill in an XML-RPC struct with a description of the parameter (name, type, value)
*/
template <typename ParamDef>
class TValue : public ValueBase, public ParamDef
{
public:

    /** Our own type.
     */
    using Self = TValue<ParamDef>;

    /** The type of the value we hold
     */
    using ValueType = typename ParamDef::ValueType;

    /** A read-write reference to a ValueType value
     */
    using ReferenceType = typename ParamDef::ReferenceType;

    /** A read-only reference to a ValueType value
     */
    using ConstReferenceType = typename ParamDef::ConstReferenceType;

    /** Type to use when sharing parameter objects.
     */
    using Ref = boost::shared_ptr<Self>;

    /** Method/function specification for notifcations. Each method receives a reference to ourselves.
     */
    using ChangedSignal = boost::signals2::signal<void (const Self&)>;

    /** Value type for observer objects which get called when a Parameter object changes.
     */
    using ChangedSignalProc = typename ChangedSignal::slot_function_type;

    /** Value type for signal connections.
     */
    using SignalConnection = boost::signals2::connection;

    /** Factory method to create shareable parameter objects. The parameter's initial value is the value of
        ValueType().

        \param name name of the parameter

        \param label the text string to use in a GUI showing this value

        \return shareable reference to new object
    */
    static Ref Make(const std::string& name, const std::string& label)
	{
	    Ref ref(new TValue(name, label));
	    return ref;
	}

    /** Factory method to create shareable parameter objects with an initial value. NOTE: validates the given
        value before assigning it to the parameter, throwing an InvalidValue exception if it is invalid.

        \param name name of the parameter

        \param label the text string to use in a GUI showing this value

        \param value initial parameter value

        \return shareable reference to new object
    */
    static Ref Make(const std::string& name, const std::string& label, ConstReferenceType value)
	{
	    Ref ref(new TValue(name, label, value));
	    if (! ref->isValid(value)) {
		ref.reset();
		InvalidValue ex("TValue::Make()", name);
		throw ex << value;
	    }
	    return ref;
	}

    /** Determines if the given value is valid for the parameter.

        \param value the value to check

        \return true if valid
    */
    bool isValid(ConstReferenceType value) const { return ParamDef::IsValid(value); }

    /** Obtain the parameter's current value.

        \return reference to currently-held value
    */
    ConstReferenceType getValue() const { return value_; }

    /** Obtain the parameter's original setting.

        \return reference to currently-held value
    */
    ConstReferenceType getOriginal() const { return original_; }

    /** Determine if the current value differs from the original setting.

        \return true if different
    */
    bool isNotOriginal() const override { return value_ != original_; }

    /** Give the parameter a new value. If the value is not valid for this parameter type, throw an InvalidValue
        exception. After setting the new value, notifies registered change observers of the value change. NOTE:
        currently, method emits a notification signal even when the values are the same. This may not hold in
        the future.

        \param value new value to use
    */
    void setValue(ConstReferenceType value)
	{
	    if (! isValid(value)) {
		InvalidValue ex("TParameter::setValue", getName());
		throw ex << value;
	    }
	    value_ = value;
	    changedSignal_(*this);
	}

    /** Give the parameter a new original (and current) value. If the value is not valid for this parameter
        type, throw an InvalidValue exception. After setting the new value, notifies registered change observers
        of the value change. NOTE: currently, method emits a notification signal even when the values are the
        same. This may not hold in the future.

        \param value new value to use
    */
    void setOriginal(ConstReferenceType value)
	{
	    if (! isValid(value)) {
		InvalidValue ex("TParameter::setOriginal", getName());
		throw ex << value;
	    }
	    original_ = value;
	    value_ = value;
	    changedSignal_(*this);
	}

    /** Give the parameter a new value taken from an XLM/RPC parameter. Relies on the template definition's
        FromXML function to perform the translation from XML entity to ValueType. Throws an InvalidValue
        exception if the XML value is not valid for this parameter type. NOTE: currently, method emits a
        notification signal even when the values are the same. This may not hold in the future.

        \param value XML representation of value to use
    */
    void setXMLValue(XmlRpc::XmlRpcValue& value) override { setValue(ParamDef::FromXML(value)); }

    /** Give the parameter a new original (and current) value taken from an XLM/RPC parameter. Relies on the
        template definition's FromXML function to perform the translation from XML entity to ValueType. Throws
        an InvalidValue exception if the XML value is not valid for this parameter type. NOTE: currently, method
        emits a notification signal even when the values are the same. This may not hold in the future.

	\param value XML representation of value to use
    */
    void setXMLOriginal(XmlRpc::XmlRpcValue& value) override { setOriginal(ParamDef::FromXML(value)); }

    /** Revert the parameter value to its original value. NOTE: currently, method emits a notification signal
        even when the values are the same. This may not hold in the future.
    */
    void revert()
        {
            value_ = original_;
            changedSignal_(*this);
        }

    /** Assignment operator. See setValue().

        \param rhs new value to use

        \return reference to self
    */
    Self& operator=(ConstReferenceType rhs)
	{
	    setValue(rhs);
	    return *this;
	}

    /** Assignment operator. Copies value from another parameter, but keeps the original name. Notifies
        registered change observers of the value change.

        \param rhs new value to use

        \return reference to self
    */
    Self& operator=(Self& rhs)
	{
            if (this != &rhs) {
                value_ = rhs.value_;
                changedSignal_(this);
            }
	    return *this;
	}

    /** Obtain a new parameter value from a C++ input stream. Relies on the template parameter's Load method to
	do the loading. Throws an InvalidValue exception if the loaded value is not valid for this parameter
	type.

	\param is stream to read from

	\return stream read from
    */
    std::istream& load(std::istream& is) override
	{
	    auto tmp = ValueType();
	    if (! ParamDef::Load(is, tmp)) {
		InvalidValue ex("TParameter::load", getName());
		throw ex << tmp;
	    }
	    setValue(tmp);
	    return is;
	}

    /** Obtain a new parameter value from an ACE CDR input stream. Relies on the template parameter's Load()
	method to do the loading. Throws an InvalidValue exception if the loaded value is not valid for this
	parameter type.

	\param cdr stream to read from

	\return stream read from
    */
    ACE_InputCDR& load(ACE_InputCDR& cdr) override
	{
	    auto tmp = ValueType();
	    if (! ParamDef::Load(cdr, tmp)) {
		InvalidValue ex("TParameter::load", getName());
		throw ex << tmp;
	    }
	    setValue(tmp);
	    return cdr;
	}

    /** Same as the load() method, but an exception is never thrown. The return value indicates whether the
	value taken from the input stream was OK

	\param cdr stream to read from

	\return true if successful, false otherwise
    */
    bool checkedLoad(ACE_InputCDR& cdr) override
	{
	    auto tmp = ValueType();
	    if (! ParamDef::Load(cdr, tmp)) return false;
	    if (! isValid(tmp)) return false;
	    value_ = tmp;
	    changedSignal_(*this);
	    return true;
	}

    /** Write out to a C++ output stream the parameter name and value. Relies on the template parameter's Save()
	method to do the work.
        
	\param os stream to write to

	\return stream written to
    */
    std::ostream& save(std::ostream& os) const override { return ParamDef::Save(os, getName(), value_); }

    /** Write out to an ACE CDR output stream the parameter name and value. Relies on the template parameter's
        Write() method to do the work.
        
        \param cdr stream to write to

        \return stream written to
    */
    ACE_OutputCDR& write(ACE_OutputCDR& cdr) const override { return ParamDef::Write(cdr, value_); }

    /** Add an observer to this property's value state. When the value changes, the observer will receive
        notification. To disconnect a connection, invoke the disconnect() method of the returned value.
        
        \param observer object to register

        \return connection identifier for the observer.
    */
    SignalConnection connectChangedSignalTo(ChangedSignalProc observer)
        { return changedSignal_.connect(observer); }

    /** Implementation of ValueBase interface. Write an XML description of the parameter into the given XML-RPC
        container.

        \param xml XML container to write into
    */
    void describe(XmlRpc::XmlRpcValue& xml) const override
	{ ParamDef::DescribeXML(xml, getName(), getLabel(), isAdvanced(), ParamDef::ToXML(value_),
                                ParamDef::ToXML(original_)); }

protected:

    /** Constructor. Intialize with name and default value of ValueType().

        \param name name of the parameter

        \param label the text string to use in a GUI showing this value
    */
    TValue(const std::string& name, const std::string& label)
	: ValueBase(name, label), ParamDef(), value_(), original_()
	{}

    /** Constructor. Intialize with name and initial value. NOTE: there is no validation of initial value.

        \param name name of the parameter

        \param label the text string to use in a GUI showing this value

        \param value initial parameter value
    */
    TValue(const std::string& name, const std::string& label, ConstReferenceType value)
	: ValueBase(name, label), ParamDef(), value_(value), original_(value)
	{}

private:
    ValueType value_;		  ///< Parameter value
    ValueType original_;	  ///< Original value from constructor
    ChangedSignal changedSignal_; ///< Changed signal and registered observers
};

/** Type traits and methods to use with the TValue template.
 */
namespace Defs {

/** Helper class that initializes an XML-RPC value object with a name and type. Isolates type-independent
    functionality from the template classes below, thus minimizing the amount of generated template code.
*/
struct XMLMixin
{
    /** Obtain the log device for XMLMixin log messages

        \return Log reference
    */
    static Logger::Log& Log();
    
    /** Initialize an XML-RPC container object with a parameter's name, type, GUI label, and 'advanced' flag.

        \param xml XML-RPC container to write into

        \param name parameter name to record

        \param typeName parameter type to record

        \param label parameter GUI label to record

        \param isAdvanced true if parameter is for advanced use
    */
    static void Init(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& typeName,
                     const std::string& label, bool isAdvanced);

    /** Add enumeration tags to an XML-RPC conainer. Only used by the Enum parameter class to store the names
        shown to the user in a GUI pop-up menu widget.

        \param xml XML-RPC container to write into

        \param names array of C strings to write

        \param size number of C strings in the array
    */
    static void AddEnumNames(XmlRpc::XmlRpcValue& xml, const char* const* names, size_t size);
    
    /** Add enumeration tags to an XML-RPC conainer. Only used by the Enum parameter class to store the names
        shown to the user in a GUI pop-up menu widget.
        
        \param xml XML-RPC container to write into

        \param names vector containing the name strings to add
    */
    static void AddEnumNames(XmlRpc::XmlRpcValue& xml, const std::vector<std::string>& names);
};

/** Basic template defining the interface for a type definition that can be used as the template parameter for
    TValue. This is suitable for all C++ types that have iostream insertion/extraction operators defined (eg.
    int, float, double)

    The TypeTraits parameter defines the underlying data type to use to hold the parmeter's value. The
    definition of the TypeTraits parameter must contain the following items:

    - ValueType: declaration of the type used to hold a parameter value.
    - XMLType: declaration of the XMLRPC library type to use
    - ReferenceType: declaration for a non-const reference to the value.
    - ConstReferenceType: declaration for a const reference to the value.
    - GetXMLTypeName: method that returns the XML-RPC type to use.
*/
template <typename TypeTraits>
struct BasicTypeDef : public TypeTraits, public XMLMixin
{
    using ValueType = typename TypeTraits::ValueType;
    using XMLType = typename TypeTraits::XMLType;
    using ReferenceType = typename TypeTraits::ValueType&;
    using ConstReferenceType = const typename TypeTraits::ValueType&;

    /** Determine if a given value is valid for this type. Always returns true.

        \return true
    */
    static bool IsValid(ConstReferenceType) { return true; }

    /** Obtain a value from a C++ input stream.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(std::istream& is, ReferenceType value) { return (is >> value).good(); }

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value) { return cdr >> value; }

    /** Write out a name and value to a C++ output stream. Separates the two with a space.

        \param os stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static std::ostream& Save(std::ostream& os, const std::string& name, ConstReferenceType value)
	{ return os << name << ' ' << value; }

    /** Write out a name and value to an ACE CDR output stream.

        \param cdr stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static ACE_OutputCDR& Write(ACE_OutputCDR& cdr, ConstReferenceType value)
	{
	    cdr << value;
	    return cdr;
	}

    /** Convert an XML-RPC value to the template's value type.

        \param value the XML value to convert

        \return the converted value
    */
    static ValueType FromXML(XmlRpc::XmlRpcValue& value)
	{ return typename TypeTraits::ValueType(typename TypeTraits::XMLType(value)); }

    /** Convert a held value into a compatible XML-RPC value.

        \param value the value to convert

        \return the converted value
    */
    static XMLType ToXML(ConstReferenceType value) { return typename TypeTraits::XMLType(value); }

    /** Obtain an XML description with the parameter's value.

        \param xml XML-RPC struct to add to

        \param name name of the parameter

	\param label the label to use in a GUI representation

	\param isAdvanced flag indicating that the parameter is 'advanced'

        \param value value to add

	\param original the original value of the parameter
    */
    static void DescribeXML(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& label,
                            bool isAdvanced, XMLType value, XMLType original)
	{
	    XMLMixin::Init(xml, name, label, TypeTraits::GetXMLTypeName(), isAdvanced);
	    xml["value"] = value;
	    xml["original"] = original;
	}
};

/** Variant of the BasicTypeDef template that defines parameter values with a valid range. The TypeTraits
    parameter must provide everything described in the BasicTypeDef commentary as well as the following:

    - GetMinValue: method that returns the lower bounds of the valid range
    - GetMaxValue: method that returns the upper bounds of the valid range
*/
template <typename TypeTraits>
struct BasicRangedTypeDef : public BasicTypeDef<TypeTraits>
{
    using Super = BasicTypeDef<TypeTraits>;
    using ValueType = typename TypeTraits::ValueType;
    using XMLType = typename TypeTraits::XMLType;
    using ConstReferenceType = const typename TypeTraits::ValueType&;

    /** Determine if a given value is valid for this type.
        
        \return true if value is within configured min/max range
    */
    static bool IsValid(ConstReferenceType value)
	{ return value >= TypeTraits::GetMinValue() && value <= TypeTraits::GetMaxValue(); }

    /** Obtain an XML description with the parameter's value, including bounds information.
        
        \param xml XML-RPC struct to add to

        \param name name of the parameter

	\param label the label to use in a GUI representation

	\param isAdvanced flag indicating that the parameter is 'advanced'

        \param value current value value of the parameter

	\param original the original value of the parameter
    */
    static void DescribeXML(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& label,
                            bool isAdvanced, XMLType value, XMLType original)
	{
	    Super::DescribeXML(xml, name, label, isAdvanced, value, original);
	    xml["min"] = Super::ToXML(TypeTraits::GetMinValue());
	    xml["max"] = Super::ToXML(TypeTraits::GetMaxValue());
	}
};

/** Variant of the BasicRangedTypeDef template that treats a parameter value as a C++ enumeration type plus a
    string representation of the enumeration values. Also supports the concept of a 'none' value which
    represents the case where no enumeration value is selected or active. The TypeTraits template parameter must
    provide everything described in the BasicRangedTypeDef commentary as well as the following:
    
    - GetEnumNames: method that returns an array of character strings that represent the enumerated values.
*/
template <typename TypeTraits>
struct BasicEnumTypeDef : public BasicRangedTypeDef<TypeTraits>
{
    using Super = BasicRangedTypeDef<TypeTraits>;
    using ValueType = typename TypeTraits::ValueType;
    using ReferenceType = typename TypeTraits::ValueType&;
    using ConstReferenceType = const typename TypeTraits::ValueType&;
    using XMLType = typename TypeTraits::XMLType;

    /** Obtain an enumeration representation for a 'none' value. NOTE: this assumes that the enumeration value
	being used does not contain an element with the value -1.
    */
    static constexpr ValueType None() { return ValueType(-1); }

    /** Determine if the given value is the 'none' value.
     */
    static bool IsNone(ConstReferenceType value) { return value == None(); }

    /** Determine if a given value is valid for this type. Adaptation of BasicRangedTypeDef class method that
        also allows -1, which means no selection.
        
        \return true if valid
    */
    static bool IsValid(ConstReferenceType value) { return IsNone(value) || Super::IsValid(value); }

    /** Determine if the given value is a valid enumeration value. Note that here, the 'none' type (-1) is not a
	valid value.
    */
    static bool IsValidEnumeration(ConstReferenceType value) { return Super::IsValid(value); }

    /** Obtain a value from a C++ input stream.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(std::istream& is, ReferenceType value)
	{
	    int tmp;
	    if (! (is >> tmp)) return false;
	    value = ValueType(tmp);
	    return true;
	}

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value)
	{
	    int tmp;
	    if (! (cdr >> tmp)) return false;
	    value = ValueType(tmp);

	    return true;
	}

    /** Obtain an XML description with the parameter's value, including bounds information and enumeration
        constant names.

        \param xml XML-RPC struct to add to

        \param name name of the parameter

	\param label the label to use in a GUI representation

	\param isAdvanced flag indicating that the parameter is 'advanced'

        \param value current value value of the parameter

	\param original the original value of the parameter
    */
    static void DescribeXML(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& label,
                            bool isAdvanced, XMLType value, XMLType original)
	{
	    Super::DescribeXML(xml, name, label, isAdvanced, value, original);

	    // Add to the XML-RPC struct the string equivalents for the
	    // enumeration values.
	    //
	    XMLMixin::AddEnumNames(xml, TypeTraits::GetEnumNames(),
                                   TypeTraits::GetMaxValue() - TypeTraits::GetMinValue() + 1);
	}
};

/** Variant of the BasicTypeDef template that treats a parameter value as having a dynamically determined range.
    The TypeTraits template parameter must provide everything described in the BasicTypeDef commentary as well
    as the following:
    
    - getMinValue: method that returns the minimum valid value
    - getMaxValue: method that returns the maximum valid value

    Unlike all previous type definitions, this one introduces member methods and attributes to the parameter
    object. In particular, the IsValid and DescribeXML methods now become member functions instead of class
    functions.
*/
template <typename TypeTraits>
struct BasicDynamicRangedTypeDef : public BasicTypeDef<TypeTraits>
{
    using Super = BasicTypeDef<TypeTraits>;
    using ValueType = typename TypeTraits::ValueType;
    using XMLType = typename TypeTraits::XMLType;
    using ConstReferenceType = const typename TypeTraits::ValueType&;

    /** Determine if a given value is valid for this type. Adaptation of BasicTypeDef class method that checks
        the given value against the current min/max values. NOTE: this method is a member function, not a class
        function.

        \return true if valid
    */
    bool IsValid(ConstReferenceType value) const { return value >= getMinValue() && value <= getMaxValue(); }

    /** Obtain an XML description with the parameter's value, including bounds information.
        
        \param xml XML-RPC struct to add to

        \param name name of the parameter

	\param label the label to use in a GUI representation

	\param isAdvanced flag indicating that the parameter is 'advanced'

        \param value current value value of the parameter

	\param original the original value of the parameter
    */
    void DescribeXML(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& label, bool isAdvanced,
                     XMLType value, XMLType original) const
	{
	    Super::DescribeXML(xml, name, label, isAdvanced, value, original);
	    xml["min"] = Super::ToXML(getMinValue());
	    xml["max"] = Super::ToXML(getMaxValue());
	}

    /** Set the parameter's minimum acceptable value.

        \param value the new minimum acceptable value
    */
    void setMinValue(const ValueType& value) { minValue_ = value; }

    /** Set the parameter's maximum acceptable value.

        \param value the new maximum acceptable value
    */
    void setMaxValue(const ValueType& value) { maxValue_ = value; }

    /** Get the parameter's minimum acceptable value.

        \return minimum acceptable value
    */
    const ValueType& getMinValue() const { return minValue_; }

    /** Get the parameter's maximum acceptable value.

        \return maximum acceptable value
    */
    const ValueType& getMaxValue() const { return maxValue_; }

private:
    ValueType minValue_;
    ValueType maxValue_;
};

/** Variant of the BasicDynamicRangedTypeDef template that treats a parameter value as having a dynamically
    determined range of values, yet notionally represented by a C++ enumeration type. Replicates the
    functionality found in BasicEnumTypeDef, allowing for dynamic definition of the enumerated values.
*/
template <typename TypeTraits>
struct BasicDynamicEnumTypeDef : public BasicDynamicRangedTypeDef<TypeTraits>
{
    using Super = BasicDynamicRangedTypeDef<TypeTraits>;
    using ValueType = typename TypeTraits::ValueType;
    using ReferenceType = typename TypeTraits::ValueType&;
    using ConstReferenceType = const typename TypeTraits::ValueType&;
    using XMLType = typename TypeTraits::XMLType;

    /** Obtain an enumeration representation for a 'none' value. NOTE: this assumes that the enumeration value
	being used does not contain an element with the value -1.
    */
    static constexpr ValueType None() { return ValueType(-1); }

    /** Determine if the given value is the 'none' value.
     */
    static bool IsNone(ConstReferenceType value) { return value == None(); }

    /** Determine if a given value is valid for this type. Adaptation of BasicRangedTypeDef class method that
        also allows -1, which means no selection.
        
        \return true if valid
    */
    bool IsValid(ConstReferenceType value) const { return IsNone(value) || Super::IsValid(value); }

    /** Determine if the given value is a valid enumeration value. Note that here, the 'none' type (-1) is not a
	valid value.
    */
    bool IsValidEnumeration(ConstReferenceType value) const { return Super::IsValid(value); }

    /** Obtain a value from a C++ input stream.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(std::istream& is, ReferenceType value)
	{
	    int tmp;
	    if (! (is >> tmp)) return false;
	    value = ValueType(tmp);
	    return true;
	}

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value)
	{
	    int tmp;
	    if (! (cdr >> tmp)) return false;
	    value = ValueType(tmp);
	    return true;
	}

    /** Obtain an XML description with the parameter's value, including bounds information and enumeration
        labels.

        \param xml XML-RPC struct to add to

        \param name name of the parameter

	\param label the label to use in a GUI representation

	\param isAdvanced flag indicating that the parameter is 'advanced'

        \param value current value value of the parameter

	\param original the original value of the parameter
    */
    void DescribeXML(XmlRpc::XmlRpcValue& xml, const std::string& name, const std::string& label, bool isAdvanced,
                     XMLType value, XMLType original) const
	{
	    // Translate the held and original values into a zero-based offset
	    // from the minimum acceptable value.
	    //
	    Super::DescribeXML(xml, name, label, isAdvanced,
                               value == None() ? value : (value - Super::getMinValue()),
                               original == None() ? original : (original - Super::getMinValue()));

	    // Add the enumeration labels
	    //
	    XMLMixin::AddEnumNames(xml, enumLabels_);
	}

    /** Obtain an enumeration label for the given value
     */
    const std::string& getEnumLabel(ValueType value) const { return enumLabels_[value]; }

    /** Add an enumeration label.
     */
    void addEnumLabel(const std::string& name) { enumLabels_.push_back(name); }

    /** Change an existing enumeration label.
     */
    void setEnumLabel(size_t index, const std::string& label) { enumLabels_[index] = label; }

private:
    std::vector<std::string> enumLabels_;
};

/** Basic type definition for 'bool' values.
 */
struct BoolTypeTraits
{
    using ValueType = bool;	///< The C++ type to use
    using XMLType = bool;	///< The XML-RPC type to use

    /** Obtain the XML parameter type to use.

        \return "bool"
    */
    static constexpr const char* GetXMLTypeName() { return "bool"; }
};

/** Complete type and class method definitions for C++ boolean values.
 */
struct Bool : public BasicTypeDef<BoolTypeTraits>
{
    /** Convert an XML-RPC value to the template's value type.

        \param value the XML value to convert

        \return the converted value
    */
    static bool FromXML(XmlRpc::XmlRpcValue& value) { return bool(value); }

    /** Obtain a value from a C++ input stream.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(std::istream& is, ReferenceType value)
	{
	    char tmp;
	    is >> tmp;
	    value = tmp;
	    return is.good();
	}

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value)
	{
	    char tmp;
	    bool ok(cdr >> tmp);
	    if (ok) value = tmp;
	    return ok;
	}

    /** Write out a name and value to a C++ output stream. Separates the two with a space.

        \param os stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static std::ostream& Save(std::ostream& os, const std::string& name, ConstReferenceType value)
	{ return os << name << ' ' << char(value); }

    /** Write out a name and value to an ACE CDR output stream.

        \param cdr stream to write to

        \param value parameter value

        \return stream written to
    */
    static ACE_OutputCDR& Write(ACE_OutputCDR& cdr, ConstReferenceType value)
	{
	    cdr << char(value);
	    return cdr;
	}
};

/** Basic type definition for 'short' integer values. This is 16-bits regardless of platform.
 */
struct ShortTypeTraits
{
    using ValueType = int16_t;	///< The C++ type to use
    using XMLType = int;	///< The XML-RPC type to use

    /** Obtain the XML parameter type to use.

        \return "int"
    */
    static constexpr const char* GetXMLTypeName() { return "int"; }
};

/** Complete type and class method definitions for 16-bit integer values.
 */
struct Short : public BasicTypeDef<ShortTypeTraits> {};

/** Basic type definition for normal integer values. This is 32-bits regardless of platform.
 */
struct IntTypeTraits
{
    using ValueType = int32_t;	///< The C++ type to use
    using XMLType = int;

    /** Obtain the XML parameter type to use.

        \return "int"
    */
    static constexpr const char* GetXMLTypeName() { return "int"; }
};

/** Complete type and class method definitions for 32-bit integer values.
 */
struct Int : public BasicTypeDef<IntTypeTraits> {};

/** Basic type definition for range of integer values, defined by a lower and upper bound given as the template
    parameters MinValue and MaxValue.
*/
template <int MinValue, int MaxValue>
struct RangedIntTypeTraits : public IntTypeTraits
{
    /** Obtain the minimum value for values of this type.

        \return minimum value
    */
    static constexpr ValueType GetMinValue() { return MinValue; }

    /** Obtain the maximum value for values of this type.

        \return minimum value
    */
    static constexpr ValueType GetMaxValue() { return MaxValue; }
};

/** Complete type and class method definitions for fixed-range integer values.
 */
template <int MinValue, int MaxValue>
struct RangedInt : public BasicRangedTypeDef<RangedIntTypeTraits<MinValue,MaxValue>> {};

/** Complete type and class method definitions for a parameter value with dynamically defined value ranges.
 */
template <typename T>
struct DynamicRangedTypeTraits : public BasicDynamicRangedTypeDef<T> {};

/** Variant of the int definition that describes a range of positive integer values. NOTE: ideally, this would
    be defined by

    RangedIntTypeTraits<1,std::numeric_limits<int>::max()>

    but the GCC compiler complains that 'max()' cannot be used.
*/
struct PositiveIntTypeTraits : public IntTypeTraits
{
    /** Obtain the minimum value for values of this type.

        \return minimum
    */
    static constexpr ValueType GetMinValue() { return 1; }

    /** Obtain the maximum value for values of this type.

        \return maximum
    */
    static constexpr ValueType GetMaxValue() { return std::numeric_limits<ValueType>::max(); }
};

/** Complete type and class method definitions for positive integer values.
 */
struct PositiveInt : public BasicRangedTypeDef<PositiveIntTypeTraits> {};

/** Variant of the integer definition that describes a range of non-negative integer values. NOTE: ideally, this
    would be defined by

    RangedIntTypeTraits<0,std::numeric_limits<int>::max()>

    but the GCC compiler complains that 'max()' cannot be used.
*/
struct NonNegativeIntTypeTraits : public IntTypeTraits
{
    /** Obtain the minimum value for values of this type.

        \return minimum
    */
    static constexpr ValueType GetMinValue() { return 0; }

    /** Obtain the maximum value for values of this type.

        \return maximum
    */
    static constexpr ValueType GetMaxValue() { return std::numeric_limits<ValueType>::max(); }
};

/** Complete type and class method definitions for non-negative integer values.
 */
struct NonNegativeInt : public BasicRangedTypeDef<NonNegativeIntTypeTraits> {};

/** Basic type definition for float-point C++ double values.
 */
struct DoubleTypeTraits
{
    using ValueType = double;	///< The C++ type to use
    using XMLType = double;	///< The XML-RPC type to use

    /** Obtain the XML parameter type to use.

        \return "double"
    */
    static constexpr const char* GetXMLTypeName() { return "double"; }
};

/** Complete type and class method definitions for C++ double values.
 */
struct Double : public BasicTypeDef<DoubleTypeTraits> {};

/** Type definition for range of double values, defined by a lower and upper bound. The template parameter must
    define two class methods called GetMinValue() and GetMaxValue().
*/
template <typename DoubleBounds>
struct RangedDoubleTypeTraits : public DoubleTypeTraits, public DoubleBounds {};

/** Complete type and class method definitions for a range of double values.
 */
template <typename DoubleBounds>
struct RangedDouble : public BasicRangedTypeDef<RangedDoubleTypeTraits<DoubleBounds> > {};

/** Base class for enumeration type traits defined by users. Uses the 'int' type for XML-RPC values.
 */
struct EnumTypeTraitsBase
{
    using XMLType = int;	///< The XML-RPC type to use

    /** Obtain the XML parameter type to use.

        \return "enum"
    */
    static constexpr const char* GetXMLTypeName() { return "enum"; }
};

/** Complete type and class method definitions for an enumerated value.
 */
template <typename EnumDefs>
struct Enum : public BasicEnumTypeDef<EnumDefs> {};

/** Complete type and class method definitions for an enumerated value that whose range values are provided at
    runtime.
*/
template <typename EnumDefs>
struct DynamicEnum : public BasicDynamicEnumTypeDef<EnumDefs> {};

/** Complete type and class method definitions for std::string values.
 */
struct StringBase
{
    /** Obtain a new string value from a C++ input stream. String values may be delimited by either single or
        double quotes.
        
        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful
    */
    static bool Load(std::istream& is, std::string& value);

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, std::string& value);

    /** Write out a name and value to a C++ output stream. Separates the two with a space. The value is written
        out delimited by single quotes.

        \param os stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static std::ostream& Save(std::ostream& os, const std::string& name, const std::string& value);
};

/** Basic type definition for std::string values.
 */
struct StringTypeTraits
{
    using ValueType = std::string; ///< The C++ type to use
    using XMLType = std::string;   ///< The XML-RPC type to use

    /** Obtain the XML parameter type to use.

        \return "string"
    */
    static constexpr const char* GetXMLTypeName() { return "string"; }
};

/** Complete type and class method definitions for std::string values.
 */
struct String : public BasicTypeDef<StringTypeTraits>
{
    /** Obtain a new string value from a C++ input stream. String values may be delimited by either single or
        double quotes.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful
    */
    static bool Load(std::istream& is, ReferenceType value) { return StringBase::Load(is, value); }

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value) { return StringBase::Load(cdr, value); }

    /** Write out a name and value to a C++ output stream. Separates the two with a space. The value is written
        out delimited by single quotes.

        \param os stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static std::ostream& Save(std::ostream& os, const std::string& name, ConstReferenceType value)
	{ return StringBase::Save(os, name, value); }
};

/** Basic type definition for a readable file path.
 */
struct ReadPathTypeTraits : public StringTypeTraits
{
    /** Obtain the XML parameter type to use.

        \return "readPath"
    */
    static constexpr const char* GetXMLTypeName() { return "readPath"; }
};

/** Complete type and class method definitions for file path that one should be able to read from. Essentially
    the same as a string, but with a special XML tag.
 */
struct ReadPath : public BasicTypeDef<ReadPathTypeTraits>
{
    /** Obtain a value from a C++ input stream. Values may be delimited by either single or double quotes.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful
    */
    static bool Load(std::istream& is, ReferenceType value) { return StringBase::Load(is, value); }

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value) { return StringBase::Load(cdr, value); }

    /** Write out a name and value to a C++ output stream. Separates the two with a space. The value is written
        out delimited by single quotes.

        \param os stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static std::ostream& Save(std::ostream& os, const std::string& name, ConstReferenceType value)
	{ return StringBase::Save(os, name, value); }
};

/** Basic type definition for a writable file path.
 */
struct WritePathTypeTraits : public StringTypeTraits
{
    /** Obtain the XML parameter type to use.

        \return "writePath"
    */
    static constexpr const char* GetXMLTypeName() { return "writePath"; }
};

/** Complete type and class method definitions for file path that one should be able to write to. Essentially
    the same as a string, but with a special XML tag.
 */
struct WritePath : public BasicTypeDef<WritePathTypeTraits>
{
    /** Obtain a value from a C++ input stream. Values may be delimited by either single or double quotes.

        \param is stream to read from

        \param value where to store the new value

        \return true if read was successful
    */
    static bool Load(std::istream& is, ReferenceType value) { return StringBase::Load(is, value); }

    /** Obtain a value from an ACE CDR input stream

        \param cdr stream to read from

        \param value where to store the new value

        \return true if read was successful and the new value is valid
    */
    static bool Load(ACE_InputCDR& cdr, ReferenceType value) { return StringBase::Load(cdr, value); }

    /** Write out a name and value to a C++ output stream. Separates the two with a space. The value is written
        out delimited by single quotes.

        \param os stream to write to

        \param name parameter name

        \param value parameter value

        \return stream written to
    */
    static std::ostream& Save(std::ostream& os, const std::string& name, ConstReferenceType value)
	{ return StringBase::Save(os, name, value); }
};

/** Basic type definition for a notification. A notification is not really a value, though it has one. It exists
    as a proxy for a push button in a parameter editor dialog box. When the user presses the push button, the
    notification object receives an update, causing the object to emit its changed signal.
*/
struct NotificationTypeTraits : public IntTypeTraits
{
    /** Obtain the XML parameter type to use.

        \return "notification"
    */
    static const char* GetXMLTypeName() { return "notification"; }
};

/** Complete type and class method definitions for 'notification' values.
 */
struct Notification : public BasicTypeDef<NotificationTypeTraits> {};

/** Min/Max definitions for a normalized value.
 */
struct NormalizedRange
{
    static constexpr double GetMinValue() { return 0.0; }
    static constexpr double GetMaxValue() { return 1.0; }
};

} // end namespace Defs

/** Definition of a boolean parameter.
 */
using BoolValue = TValue<Defs::Bool>;

/** Definition of a 16-bit integer parameter.
 */
using ShortValue = TValue<Defs::Short>;

/** Definition of a 32-bit integer parameter.
 */
using IntValue = TValue<Defs::Int>;

/** Definition of a positive integer parameter, one that only holds integer values greater than zero.
 */
using PositiveIntValue = TValue<Defs::PositiveInt>;

/** Definition of a non-negative integer parameter, one that only holds value greater than or equal to zero.
 */
using NonNegativeIntValue = TValue<Defs::NonNegativeInt>;

/** Definition of a floating-point parameter.
 */
using DoubleValue = TValue<Defs::Double>;

/** Definition of a normalized floating-point parameter, one that is within the range of [0,1]
 */
using NormalizedValue = TValue<Defs::RangedDouble<Defs::NormalizedRange>>;

/** Definition of an 8-bit character string parameter.
 */
using StringValue = TValue<Defs::String>;

/** Definition of a file path parameter for files that can be opened and read.
 */
using ReadPathValue = TValue<Defs::ReadPath>;

/** Definition of a file path parameter for files that can be written to.
 */
using WritePathValue = TValue<Defs::WritePath>;

/** Definition of a 'notification' parameter that simply emits a signal whenever its value changes. Useful for
    'reset' buttons and such.
*/
using NotificationValue = TValue<Defs::Notification>;

/** Definition of a 16-bit integer parameter with a valid range of values that is set at runtime.
 */
using DynamicShortValue = TValue<Defs::DynamicRangedTypeTraits<Defs::ShortTypeTraits>>;

/** Definition of a 32-bit integer parameter with a valid range of values that is set at runtime.
 */
using DynamicIntValue = TValue<Defs::DynamicRangedTypeTraits<Defs::IntTypeTraits>>;

/** Definition of a floating-point parameter with a valid range of values that is set at runtime.
 */
using DynamicDoubleValue = TValue<Defs::DynamicRangedTypeTraits<Defs::DoubleTypeTraits>>;

} // end namespace Parameter
} // end namespace SideCar

/** \file
 */

#endif
