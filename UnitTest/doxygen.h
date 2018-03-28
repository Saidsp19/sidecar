#ifndef SIDECAR_UNITTEST_DOXYGEN_H // -*- C++ -*-
#define SIDECAR_UNITTEST_DOXYGEN_H

/** \page unittest UnitTest Framework

    The UniTest framework contains a suite of classes that let developers write unit tests for their code that
    get executed during normal code compilation.

    - ProcSuite.h defines a template class that allows one to designate one or more methods of another class as
    tests to execute.

    - RunResults.h defines a collector of unit test execution results

    - Suite.h defines a container that holds one or more TestObj-derived objects. Executes each test in the
    sequence they were registered.

    - TestObj.h defines the base class for all unit tests. Derived tests must override its test() method to
    define an appropriate unit test.

    - UnitTest.h convenience header that includes all of the above

    Visit the documentation for the above files for addition information and examples on how to use the classes
    they define.
*/

/** Namespace for classes associated with the unit testing framework. Unit tests allow developers to guard
    against unexpected behavioral changes in their code due to editing changes.

    \copydoc unittest
*/
namespace UnitTest {
}

#endif
