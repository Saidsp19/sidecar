#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "ClockSource.h"
#include "Configurator.h"
#include "ConfiguratorFile.h"
#include "Formatters.h"
#include "Log.h"
#include "Msg.h"
#include "Writers.h"

#include "Threading/Threading.h"
#include "UnitTest/UnitTest.h"

Logger::Log& top = Logger::Log::Find("top-dog");

using namespace Logger;

// Template function that attempts to process a configuration text, and expects a specific exception to get
// thrown.
//
bool
checkException(const char* txt)
{
    std::istringstream is(txt);
    try {
        Configurator cfg(is);
        std::clog << "*** checkException: no exception thrown for: ***\n" << txt << std::endl;
        return false; // expected an exception
    } catch (...) {
        return true;
    }
}

class MyClockSource : public ClockSource {
public:
    using Ref = boost::shared_ptr<MyClockSource>;

    static Ref Make()
    {
        Ref ref(new MyClockSource);
        Log::SetClockSource(ref);
        return ref;
    }

    MyClockSource() : ClockSource(), count_(0) {}

    void reset() { count_ = 0; }

    virtual void now(timeval& t);

private:
    int count_;
};

void
MyClockSource::now(timeval& t)
{
    t.tv_sec = count_++;
    t.tv_usec = 0;
}

MyClockSource::Ref testClock(MyClockSource::Make());

struct TestStartup : public UnitTest::TestObj {
    TestStartup();

    void test();

    struct Child : public Threading::Thread {
        Child(TestStartup& parent, const std::string& logName);

        virtual void run();

        TestStartup& parent_;
        std::string logName_;
    };

    int alive_;
    int dead_;
    Threading::Condition::Ref condition_;
};

TestStartup::TestStartup() : TestObj("Startup"), alive_(0), dead_(0), condition_(Threading::Condition::Make())
{
    ;
}

TestStartup::Child::Child(TestStartup& parent, const std::string& logName) : parent_(parent), logName_(logName)
{
    ++parent_.alive_;
}

void
TestStartup::Child::run()
{
    Sleep(::drand48());
    Logger::Log& log = Logger::Log::Find(logName_);
    log.fatal() << std::hex << &log << std::dec << " parent: " << std::hex << log.getParent() << std::dec << " "
                << parent_.alive_ << ' ' << parent_.dead_ << std::endl;
    Sleep(::drand48());
    Threading::Locker lock(parent_.condition_);
    ++parent_.dead_;
    parent_.condition_->signal();
}

void
TestStartup::test()
{
    testClock->reset();
    Threading::Locker lock(condition_);

    std::string full("a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z");
    for (int index = full.size(); index > 0; index -= 2) {
        (new Child(*this, full.substr(0, index)))->start();
        (new Child(*this, full.substr(0, index)))->start();
        (new Child(*this, full.substr(0, index)))->start();
        (new Child(*this, full.substr(0, index)))->start();
    }

    while (dead_ != 26 * 4) condition_->waitForSignal();

    dead_ = 0;
    alive_ = 0;

    for (size_t index = 1; index <= full.size(); index += 2) {
        (new Child(*this, full.substr(0, index)))->start();
        (new Child(*this, full.substr(0, index)))->start();
        (new Child(*this, full.substr(0, index)))->start();
        (new Child(*this, full.substr(0, index)))->start();
    }

    while (dead_ != 26 * 4) condition_->waitForSignal();
}

struct TestConfigurator : public UnitTest::TestObj {
    TestConfigurator() : TestObj("Configurator") {}
    void test();
};

void
TestConfigurator::test()
{
    testClock->reset();

    // Check to see that we throw appropriate exceptions for improper configuration texts
    //
    assertTrue(checkException("    zippy scabby   \n"));
    assertTrue(checkException("zippy writer\n"));
    assertTrue(checkException("zippy writer poobah\n"));
    assertTrue(checkException("zippy priority\n"));
    assertTrue(checkException("zippy priority blah\n"));
    assertTrue(checkException("zippy writer clog formatter\n"));
    assertTrue(checkException("zippy writer clog formatter gagme\n"));
    assertTrue(checkException("zippy writer clog formatter pattern\n"));
    assertTrue(checkException("zippy writer clog formatter pattern\n"));
    assertTrue(checkException("zippy writer file\n"));
    assertTrue(checkException("zippy writer file '/tmp/zippy mode\n"));
    assertTrue(checkException("zippy writer file '/tmp/zippy' mode\n"));
    assertTrue(checkException("zippy writer file /tmp/zippy mode\n"));
    assertTrue(checkException("zippy writer file /tmp/zippy hungry\n"));
    assertTrue(checkException("zippy writer rolling /tmp/zippy versions xyz\n"));
    assertTrue(checkException("zippy writer rolling /tmp/zippy size abc\n"));
    assertTrue(checkException("zippy writer syslog\n"));
    assertTrue(checkException("zippy writer syslog mommy facility xyz\n"));
    assertTrue(checkException("zippy writer syslog mommy bogus\n"));
    assertTrue(checkException("zippy writer remote\n"));
    assertTrue(checkException("zippy writer remote localhost port hi\n"));
    assertTrue(checkException("zippy writer remote localhost bumper\n"));

    // Now for a valid configuration...
    //
    std::istringstream is("#\n\
# This is a comment\n\
#\n\
root priority info\n\
root.foo1 writer clog formatter terse\n\
root.foo1.bar priority debug-1 writer clog formatter pattern 'hi mom %p - %m'\n\
\n\
#\n\
# Last line\n\
");
    Configurator cfg(is);
    Log& a(Log::Find("root"));
    assertEqual(Priority::kInfo, a.getPriorityLimit());
    a.debug1() << "root message" << std::endl;

    Log& b(Log::Find("foo1.bar"));
    assertEqual(Priority::kDebug1, b.getPriorityLimit());
    b.debug1() << "hello!" << std::endl;
    Log& c(Log::Find("foo1"));
    assertEqual(Priority::kError, c.getPriorityLimit());
    c.debug1() << "hidden" << std::endl;
    c.info() << "repetitive" << std::endl;
    a.setPriorityLimit(Priority::kError);
}

struct TestConfiguratorFile : public UnitTest::TestObj {
    TestConfiguratorFile() : TestObj("ConfiguratorFile") {}

    void test();
};

void
TestConfiguratorFile::test()
{
    testClock->reset();

    // Attempt to access a bogus file.
    //
    try {
        ConfiguratorFile c("/this/file/does not/exist.exe");
        assertTrue(0);
    } catch (...) {
        assertTrue(1);
    }

    ::unlink("foo.cfg");
    std::ofstream of("foo.cfg");
    of << "#\n\
# This is a comment\n\
#\n\
root priority d2\n\
root.foo2 priority info writer cerr formatter terse\n\
root.foo2.bar priority d2 writer cerr formatter pattern 'hi mom %p - %m'\n\
\n\
#\n\
# Last line\n\
";
    of.close();

    ConfiguratorFile cfg("foo.cfg");
    Log& a(Log::Find("root"));
    Log& b(Log::Find("foo2.bar"));
    Log& c(Log::Find("foo2"));

    assertEqual(Priority::kDebug2, a.getPriorityLimit());
    a.debug2() << "root message" << std::endl;

    assertEqual(Priority::kDebug2, b.getPriorityLimit());
    b.debug2() << "hello!" << std::endl;
    assertEqual(Priority::kInfo, c.getPriorityLimit());
    c.debug2() << "hidden" << std::endl;
    c.info() << "repetitive" << std::endl;

    // Change priority values and then reload. Should revert to previous values.
    //
    b.setPriorityLimit(Priority::kError);
    c.setPriorityLimit(Priority::kDebug3);
    assertEqual(false, cfg.isStale());
    cfg.reload();
    assertEqual(Priority::kDebug2, b.getPriorityLimit());
    assertEqual(Priority::kInfo, c.getPriorityLimit());
    assertEqual(1, b.numWriters());

    // Stall a bit so that we can generate a stale file.
    //
    Threading::Thread::Sleep(1.0);

    // Start separate thread to monitor the modification time of the configuration file.
    //
    cfg.startMonitor(0.1);

    // Change the priority levels of our configured Log objects
    //
    b.setPriorityLimit(Priority::kError);
    c.setPriorityLimit(Priority::kDebug1);
    assertEqual(false, cfg.isStale());

    // Now touch the config file and see if the monitor thread reloads.
    //
    of.open("foo.cfg", std::ios_base::out | std::ios_base::app);
    of << "# a new line" << std::endl;
    of.close();

    // Spin until the reload is finished.
    //
    while (cfg.isStale()) Threading::Thread::Sleep(0.5);

    // These should have reverted back to their configured values
    //
    assertEqual(Priority::kDebug2, b.getPriorityLimit());
    assertEqual(Priority::kInfo, c.getPriorityLimit());
    a.setPriorityLimit(Priority::kError);

    cfg.stopMonitor();

    ::unlink("foo.cfg");
}

struct TestClockSource : public UnitTest::TestObj {
    void test();
};

struct CleanUp {
    CleanUp(Log& l, const Writers::Writer::Ref& w) : l_(l), w_(w) {}
    ~CleanUp() { l_.removeWriter(w_); }

    Log& l_;
    Writers::Writer::Ref w_;
};

void
TestClockSource::test()
{
    testClock->reset();

    Log& a(Log::Find("testLog"));
    std::ostringstream os;
    Writers::Writer::Ref sw(Writers::Stream::Make(Formatters::Terse::Make(), os));
    a.addWriter(sw);
    CleanUp cl(a, sw);

    a.fatal() << "this is a bummer" << std::endl;
    assertEqual("1.00 FATAL - this is a bummer\n", os.str());

    os.str("");
    a.fatal() << "I can see clearly "
              << "now the rain has gone" << std::endl;
    assertEqual("2.00 DBG1 - I can see clearly now the rain has gone\n", std::string(os.str(), 0, os.tellp()));
}

struct TestFormatter : public UnitTest::TestObj {
    void testTerse()
    {
        testClock->reset();
        Msg a("category", "msg", Priority::Find("error"));
        std::ostringstream os("");
        Formatters::Formatter::Ref b(Formatters::Terse::Make());
        b->format(os, a);
        assertEqual("19700101 000000.00 E - msg", os.str());
    }

    void testVerbose()
    {
        testClock->reset();
        Msg a("category", "msg", Priority::Find("info"));
        a.when_.tv_sec = 123;
        a.when_.tv_usec = 995678;
        std::ostringstream os("");
        Formatters::Formatter::Ref b(Formatters::Verbose::Make());
        b->format(os, a);
        assertEqual("19700101 000203.99 INFO category: msg", os.str());
        a.when_.tv_usec = 994567;
        os.str("");
        b->format(os, a);
        assertEqual("19700101 000203.99 INFO category: msg", os.str());
    }

    void testPattern()
    {
        // Force ASCII times to be in UTC. We expect to be in C or US locale.
        //
        ::putenv(::strdup("TZ="));
        ::tzset();

        testClock->reset();
        Msg a("category", "msg", Priority::Find("INFO"));
        a.when_.tv_sec = 987650000;
        a.when_.tv_usec = 454321;
        Formatters::Pattern::Ref b(Formatters::Pattern::Make());
        std::ostringstream os("");
        b->format(os, a);
        assertEqual("20010419 031320.45 I category: msg", os.str());
        os.str("");
        b->setPattern("hubba %W %P bubba");
        b->format(os, a);
        assertEqual("hubba Thu Apr 19 03:13:20 UTC 2001 INFO bubba", os.str());
        os.str("");
        b->setPattern("%% %w %S %M %c %p %P %m");
        b->format(os, a);
        assertEqual("% 20010419 031320.45 987650000 454321 category I "
                    "INFO msg",
                    os.str());
        os.str("");
        b->setPattern("%'%A %B %e, %Y %H:%M:%S' %p %m%z");
        b->format(os, a);
        assertEqual("Thursday April 19, 2001 03:13:20 I msg\n", os.str());
    }

    static UnitTest::ProcSuite<TestFormatter>* Install(UnitTest::ProcSuite<TestFormatter>* ps)
    {
        ps->add("terse", &TestFormatter::testTerse);
        ps->add("verbose", &TestFormatter::testVerbose);
        ps->add("pattern", &TestFormatter::testPattern);
        return ps;
    }
};

struct TestLog : public UnitTest::TestObj {
    void test() { testLog(); }

    void testFind();
    void testLog();
    void testPropagation();
    void testPriorityLimit();
    void testProcLog();

    static UnitTest::ProcSuite<TestLog>* Install(UnitTest::ProcSuite<TestLog>* ps)
    {
        ps->add("find", &TestLog::testFind);
        ps->add("log", &TestLog::testLog);
        ps->add("propagation", &TestLog::testPropagation);
        ps->add("priorityLimit", &TestLog::testPriorityLimit);
        ps->add("procLog", &TestLog::testProcLog);
        return ps;
    }
};

void
TestLog::testFind()
{
    testClock->reset();

    // Should obtain the same `root' object
    //
    Log& a(Log::Root());
    Log& b(Log::Find("root"));
    assertEqual(&b, &a);

    // Create a new Log object with a fully-qualified name.
    //
    Log& c(Log::Find("root.bubba"));
    assertEqual("bubba", c.name());
    assertEqual("root.bubba", c.fullName());
    assertEqual("root", c.getParent()->name());
    assertEqual(&b, c.getParent());

    // Should get the same object if not fully-qualified.
    //
    Log& d(Log::Find("bubba"));
    assertEqual("bubba", d.name());
    assertEqual("root", d.getParent()->name());
    assertEqual(&b, d.getParent());
    assertEqual(&c, &d);
}

void
TestLog::testLog()
{
    testClock->reset();
    // Log::SetClockSource(new MyClockSource);

    Log& root(Log::Find("root"));

    assertEqual("root", root.name());
    assertEqual(static_cast<Log*>(0), root.getParent());
    assertEqual(&root, &Log::Root());
    assertEqual(Priority::kError, root.getPriorityLimit());

    Log& a(Log::Find("testLog"));
    assertEqual("root", a.getParent()->name());
    assertEqual(&root, a.getParent());
    assertEqual(Priority::kError, a.getPriorityLimit());
    a.setPriorityLimit(Priority::kWarning);
    assertEqual(false, a.isAccepting(Priority::kDebug1));
    assertEqual(true, a.isAccepting(Priority::kFatal));
    assertEqual(false, a.isAccepting(Priority::kInfo));

    std::stringstream oss("");
    Writers::Writer::Ref sw(Writers::Stream::Make(Formatters::Terse::Make(), oss));
    a.addWriter(sw);
    CleanUp cl(a, sw);
    a.error() << "this is a bummer" << std::endl;
    std::cerr << oss.str() << std::endl;
    assertEqual("19700101 000000.00 E - this is a bummer\n", oss.str());

    oss.str("");
    a.debug1() << "can't touch this" << std::endl;
    assertEqual("", std::string(oss.str(), 0, oss.tellp()));

    oss.str("");
    a.setPriorityLimit(Priority::kDebug1);
    assertEqual(Priority::kDebug1, a.getPriorityLimit());
    a.debug1() << "I can see clearly "
               << "now the rain has gone" << std::endl;
    assertEqual("19700101 000001.00 D1 - I can see clearly now the rain "
                "has gone\n",
                std::string(oss.str(), 0, oss.tellp()));
}

class Blah {
public:
    static Logger::Log& Log();
    void a();
    void b();
};

Log&
Blah::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("Blah");
    return log_;
}

void
Blah::a()
{
    static ProcLog log("a", Log());
    log.debug1() << "hi" << std::endl;
}

void
Blah::b()
{
    static ProcLog log("b", Log());
    log.debug2() << "there" << std::endl;
}

void
TestLog::testPriorityLimit()
{
    testClock->reset();
    Log& parent(Log::Find("root.testPriority"));
    assertEqual(0UL, top.getNumChildren());
    parent.setPriorityLimit(Priority::kError);
    assertEqual(Priority::kError, parent.getPriorityLimit());
    assertEqual(Priority::kError, parent.getMaxPriorityLimit());

    // Create child
    //
    Log& child(Log::Find("root.testPriority.one"));
    child.setPriorityLimit(Priority::kDebug1);
    assertEqual(Priority::kDebug1, child.getPriorityLimit());
    assertEqual(Priority::kDebug1, child.getMaxPriorityLimit());
    assertEqual(1UL, parent.getNumChildren());

    // Change parent's priority limit
    //
    parent.setPriorityLimit(Priority::kDebug2);
    assertEqual(Priority::kDebug2, parent.getPriorityLimit());
    assertEqual(Priority::kDebug2, parent.getMaxPriorityLimit());

    // Verify that child's maxPriorityLimit_ changed too.
    //
    assertEqual(Priority::kDebug1, child.getPriorityLimit());
    assertEqual(Priority::kDebug2, child.getMaxPriorityLimit());

    // Now revert parent's priority limit back to previous value.
    //
    parent.setPriorityLimit(Priority::kError);
    assertEqual(Priority::kError, parent.getPriorityLimit());
    assertEqual(Priority::kError, parent.getMaxPriorityLimit());

    // Verify that child's reverted to previous value.
    //
    assertEqual(Priority::kDebug1, child.getPriorityLimit());
    assertEqual(Priority::kDebug1, child.getMaxPriorityLimit());
}

void
TestLog::testProcLog()
{
    testClock->reset();
    std::stringstream oss("");
    Writers::Writer::Ref sw(Writers::Stream::Make(Formatters::Verbose::Make(), oss));
    Blah::Log().addWriter(sw);
    Blah::Log().setPriorityLimit(Priority::kDebug2);
    Blah z;
    z.a();
    z.b();
    assertEqual("19700101 000000.00 DEBUG-1 root.Blah.a: hi\n"
                "19700101 000001.00 DEBUG-2 root.Blah.b: there\n",
                oss.str());
}

void
TestLog::testPropagation()
{
    testClock->reset();
    Log& bar(Log::Find("root.foo3.bar"));
    Log& foo(Log::Find("root.foo3"));
    assertEqual(&foo, bar.getParent());

    std::stringstream fooBuf("");
    Writers::Writer::Ref fooSW(Writers::Stream::Make(Formatters::Terse::Make(), fooBuf));
    foo.addWriter(fooSW);
    CleanUp fooCl(foo, fooSW);

    std::stringstream barBuf("");
    Writers::Writer::Ref barSW(Writers::Stream::Make(Formatters::Terse::Make(), barBuf));
    bar.addWriter(barSW);
    CleanUp barCl(bar, barSW);

    bar.setPriorityLimit(Priority::kDebug1);
    foo.setPriorityLimit(Priority::kInfo);
    foo.info() << "only foo" << std::endl;

    assertEqual("", barBuf.str());
    assertEqual("19700101 000000.00 I - only foo\n", fooBuf.str());
    fooBuf.str("");

    bar.debug1() << "only bar" << std::endl;
    assertEqual("", std::string(fooBuf.str(), 0, fooBuf.tellp()));
    assertEqual("19700101 000001.00 D1 - only bar\n", barBuf.str());
    barBuf.str("");

    bar.setPropagate(true);
    bar.info() << "both foo and bar" << std::endl;
    assertEqual("19700101 000002.00 I - both foo and bar\n", barBuf.str());
    assertEqual("19700101 000002.00 I - both foo and bar\n", fooBuf.str());
}

struct TestPriority : public UnitTest::TestObj {
    TestPriority() : TestObj("Priority") {}
    void test();
};

void
TestPriority::test()
{
    std::string key("none");
    Priority::Level a(Priority::Find(key));
    assertEqual(Priority::kNone, a);
    assertEqual("none", key);
    assertEqual(Priority::kDebug1, Priority::Find("D1"));
    try {
        Priority::Find("bugger");
    } catch (...) {
        assertTrue(1);
    }

    assertEqual(std::string("D3"), Priority::GetShortName(Priority::kDebug3));
    assertEqual(std::string("FATAL"), Priority::GetLongName(Priority::kFatal));
}

struct TestWriter : public UnitTest::TestObj {
    void testFileWriter();
    void testStreamWriter();
    void testRollingWriter();
    void testSyslogWriter();
    void testRemoteSyslogWriter();

    static UnitTest::ProcSuite<TestWriter>* Install(UnitTest::ProcSuite<TestWriter>* ps)
    {
        ps->add("FileWriter", &TestWriter::testFileWriter);
        ps->add("StreamWriter", &TestWriter::testStreamWriter);
        ps->add("RollingWriter", &TestWriter::testRollingWriter);
        ps->add("SyslogWriter", &TestWriter::testSyslogWriter);
        ps->add("RemoteSyslogWriter", &TestWriter::testRemoteSyslogWriter);
        return ps;
    }
};

void
TestWriter::testFileWriter()
{
    std::clog << "testClock: " << testClock << std::endl;
    std::clog << "1" << std::endl;
    testClock->reset();
    std::clog << "2" << std::endl;
    ::unlink("foo");
    std::clog << "3" << std::endl;
    Writers::Writer::Ref fw(Writers::File::Make(Formatters::Terse::Make(), "foo"));
    std::clog << "4" << std::endl;
    fw->open();
    std::clog << "5" << std::endl;
    Msg msg("category", "msg\n", Priority::kDebug3);
    std::clog << "6" << std::endl;
    fw->write(msg);
    std::clog << "7" << std::endl;
    fw->write(msg);
    std::clog << "8" << std::endl;
    fw->close();
    std::clog << "9" << std::endl;

    std::ifstream ifs("foo");
    std::string line;
    std::getline(ifs, line);
    assertEqual("19700101 000000.00 D3 - msg", line);
    std::getline(ifs, line);
    assertEqual("19700101 000000.00 D3 - msg", line);
    ::unlink("foo");
}

void
TestWriter::testStreamWriter()
{
    testClock->reset();
    std::ostringstream os;
    Writers::Writer::Ref sw(Writers::Stream::Make(Formatters::Terse::Make(), os));
    sw->close();
    Msg msg("category", "msg\n", Priority::kDebug3);
    sw->write(msg);
    sw->open();
    sw->write(msg);
    assertEqual("19700101 000000.00 D3 - msg\n"
                "19700101 000000.00 D3 - msg\n",
                os.str());
}

void
TestWriter::testRollingWriter()
{
    testClock->reset();
    ::unlink("foo");
    ::unlink("foo.1");
    ::unlink("foo.2");
    Writers::Writer::Ref rw(Writers::RollingFile::Make(Formatters::Terse::Make(), "foo", 4, 2));
    Msg msg("category", "msg 1\n", Priority::kDebug1);
    rw->write(msg);
    msg.message_ = "msg 2\n";
    rw->write(msg);
    msg.message_ = "msg 3\n";
    rw->write(msg);
    rw->close();

    std::string line;
    std::ifstream ifs1("foo.2");
    std::getline(ifs1, line);
    assertEqual("19700101 000000.00 D1 - msg 1", line);

    std::ifstream ifs2("foo.1");
    std::getline(ifs2, line);
    assertEqual("19700101 000000.00 D1 - msg 2", line);

    std::ifstream ifs3("foo");
    std::getline(ifs3, line);
    assertEqual("19700101 000000.00 D1 - msg 3", line);

    ::unlink("foo");
    ::unlink("foo.1");
    ::unlink("foo.2");
}

void
TestWriter::testSyslogWriter()
{
    testClock->reset();
    // Open syslog message file, and move to the end.
    //
#ifdef linux
    std::ifstream ifs("/var/log/messages");
#endif

#ifdef solaris
    std::ifstream ifs("/var/adm/messages");
#endif

#ifdef darwin
    std::ifstream ifs("/var/log/system.log");
#endif
    ifs.seekg(0, std::ios_base::end);

    Writers::Writer::Ref slw(Writers::Syslog::Make(Formatters::Terse::Make(), "mommy", LOG_LOCAL0));
#ifdef darwin
    Msg msg("category", "syslog test", Priority::kInfo);
#else
    Msg msg("category", "syslog test", Priority::kFatal);
#endif

    // Send out a syslog message. Sleep a bit afterwards, just in case system is busy...
    //
    slw->write(msg);
    slw->close();
    ::sleep(2);

    // Read in a line from the syslog message file. Should contain the message data used above.
    //
    std::string line;
    std::getline(ifs, line);

    // assertNotEqual(std::string::npos, line.find(" - syslog test"));
}

void
TestWriter::testRemoteSyslogWriter()
{
    testClock->reset();
    // Open syslog message file, and move to the end.
    //
#ifdef linux
    std::ifstream ifs("/var/log/messages");
#endif

#ifdef solaris
    std::ifstream ifs("/var/adm/messages");
#endif

#ifdef darwin
    std::ifstream ifs("/var/log/system.log");
#endif
    ifs.seekg(0, std::ios_base::end);

    // Connect to ourselves.
    //
    Writers::Writer::Ref rslw(Writers::RemoteSyslog::Make(Formatters::Terse::Make(), "127.0.0.1"));
#ifdef darwin
    Msg msg("category", "remote test", Priority::kInfo);
#else
    Msg msg("category", "remote test", Priority::kFatal);
#endif

    // Send out a syslog message using datagrams. Sleep a bit afterwards, just in case system is busy...
    //
    rslw->write(msg);
    rslw->close();
    ::sleep(2);

    // Read in a line from the syslog message file. Should contain the message data used above.
    //
    std::string line;
    std::getline(ifs, line);
    // assertNotEqual(std::string::npos, line.find(" - remote test"));
}

int
main(int argc, const char* argv[])
{
    UnitTest::Suite st("Logger");
    st.add(new TestStartup);
    st.add(TestLog::Install(new UnitTest::ProcSuite<TestLog>("Log")));
    st.add(TestWriter::Install(new UnitTest::ProcSuite<TestWriter>("Writer")));
    st.add(new TestConfigurator);
    st.add(new TestConfiguratorFile);
    st.add(new TestPriority);
    st.add(TestFormatter::Install(new UnitTest::ProcSuite<TestFormatter>("Formatter")));
    return st.mainRun();
}
