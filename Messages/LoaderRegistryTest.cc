#include "LoaderRegistry.h"
#include "Logger/Log.h"
#include "UnitTest/UnitTest.h"

using namespace SideCar::Messages;

struct Test : public UnitTest::TestObj {
    Test() : TestObj("LoaderRegistry") {}
    void test();

    static int called_;

    static ACE_InputCDR& loaderV1(Test*, ACE_InputCDR& cdr);
    static ACE_InputCDR& loaderV3(Test*, ACE_InputCDR& cdr);
    static ACE_InputCDR& loaderV4(Test*, ACE_InputCDR& cdr);
    static ACE_InputCDR& loaderV8(Test*, ACE_InputCDR& cdr);
};

int Test::called_;

ACE_InputCDR&
Test::loaderV1(Test*, ACE_InputCDR& cdr)
{
    called_ = 1;
    return cdr;
}

ACE_InputCDR&
Test::loaderV3(Test*, ACE_InputCDR& cdr)
{
    called_ = 3;
    return cdr;
}

ACE_InputCDR&
Test::loaderV4(Test*, ACE_InputCDR& cdr)
{
    called_ = 4;
    return cdr;
}

ACE_InputCDR&
Test::loaderV8(Test*, ACE_InputCDR& cdr)
{
    called_ = 8;
    return cdr;
}

void
Test::test()
{
    Logger::Log& log(Logger::Log::Root());
    log.setPriorityLimit(Logger::Priority::kDebug);

    {
        // Register out-of-order to check for proper inserion in addLoader()
        //
        TLoaderRegistry<Test>::VersionedLoader loaders[] = {TLoaderRegistry<Test>::VersionedLoader(8, &Test::loaderV8),
                                                            TLoaderRegistry<Test>::VersionedLoader(1, &Test::loaderV1),
                                                            TLoaderRegistry<Test>::VersionedLoader(4, &Test::loaderV4),
                                                            TLoaderRegistry<Test>::VersionedLoader(3, &Test::loaderV3)};

        {
            TLoaderRegistry<Test> lr(loaders, 4);
            assertEqual(8, lr.getCurrentVersion());
            ACE_OutputCDR os;
            {
                // Create a stream of version indicators
                //
                using VT = VoidLoaderRegistry::VersionType;
                os << VT(1);
                os << VT(2);
                os << VT(3);
                os << VT(4);
                os << VT(5);
                os << VT(6);
                os << VT(7);
                os << VT(8);
                os << VT(9);
            }

            ACE_InputCDR is(os);
            called_ = -1;
            {
                // Create input stream from output stream, and check that the correct loader procedure is
                // invoked.
                //
                lr.load(this, is); // 1
                assertEqual(1, called_);
                called_ = -1;
                lr.load(this, is); // 2
                assertEqual(3, called_);
                called_ = -1;
                lr.load(this, is); // 3
                assertEqual(3, called_);
                called_ = -1;
                lr.load(this, is); // 4
                assertEqual(4, called_);
                called_ = -1;
                lr.load(this, is); // 5
                assertEqual(8, called_);
                called_ = -1;
                lr.load(this, is); // 6
                assertEqual(8, called_);
                called_ = -1;
                lr.load(this, is); // 7
                assertEqual(8, called_);
                called_ = -1;
                lr.load(this, is); // 8
                assertEqual(8, called_);
                called_ = -1;
                lr.load(this, is); // 9
                assertEqual(8, called_);
                called_ = -1;
            }
        }
    }

    {
        // Check for duplicate checking
        //
        TLoaderRegistry<Test>::VersionedLoader loaders[] = {
            TLoaderRegistry<Test>::VersionedLoader(1, &Test::loaderV1),
            TLoaderRegistry<Test>::VersionedLoader(1, &Test::loaderV1),
        };

        // Check for empty loaders set.
        //
        try {
            TLoaderRegistry<Test> lr(loaders, 0);
            assertFalse(true);
        } catch (Utils::Exception& ex) {
            ;
        }

        // Check for duplicates.
        //
        try {
            TLoaderRegistry<Test> lr(loaders, 2);
            assertFalse(true);
        } catch (Utils::Exception& ex) {
            ;
        }
    }
}

int
main(int, const char**)
{
    return Test().mainRun();
}
