#include "boost/bind.hpp"
#include "boost/enable_shared_from_this.hpp"

#include "Logger/Log.h"

#include "ASIOMonitor.h"
#include "Transaction.h"

using namespace SideCar::Zeroconf;

/** Internal implementation class for ASIOMonitor.
 */
struct ASIOMonitor::Private : public boost::enable_shared_from_this<ASIOMonitor::Private>
{
    static Logger::Log& Log() { return ASIOMonitor::Log(); }

    Private(boost::asio::io_service& ios) : socket_(ios), transaction_(0), readStarted_(false) {}

    void serviceStarted(Transaction* transaction)
	{
	    Logger::ProcLog log("serviceStarted", Log());
	    LOGDEBUG << this << std::endl;;

	    transaction_ = transaction; 

	    auto fd = ::dup(transaction->getConnection());
	    LOGDEBUG << "fd: " << fd << std::endl;
	    socket_.assign(boost::asio::local::stream_protocol(), fd);
			    
	    boost::asio::local::stream_protocol::socket::non_blocking_io nbio(true);
	    socket_.io_control(nbio);
	    startAsyncRead();
	}

    void dataAvailable(boost::system::error_code err)
	{
	    static Logger::ProcLog log("dataAvailable", Log());
	    LOGDEBUG << this << " err: " << err << std::endl;;
	    readStarted_ = false;
	    if (! err) {
		if (transaction_ && transaction_->processConnection()) {
		    startAsyncRead();
		}
	    }
	}

    void startAsyncRead()
	{
	    static Logger::ProcLog log("startAsyncRead", Log());
	    LOGDEBUG << this << std::endl;;
	    readStarted_ = true;
	    socket_.async_read_some(boost::asio::null_buffers(),
                                    boost::bind(&ASIOMonitor::Private::dataAvailable, shared_from_this(),
                                                boost::asio::placeholders::error));
	    LOGDEBUG << this << " end" << std::endl;
	}

    void serviceStopping()
	{
	    static Logger::ProcLog log("serviceStopping", Log());
	    LOGDEBUG << this << std::endl;;
	    transaction_ = 0;
	    socket_.cancel();
	}

    Transaction* transaction_;
    boost::asio::local::stream_protocol::socket socket_;
    bool readStarted_;
};

Logger::Log&
ASIOMonitor::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("SideCar.Zeroconf.ASIOMonitor");
    return log_;
}

ASIOMonitor::ASIOMonitor(boost::asio::io_service& ios)
    : p_(new Private(ios))
{
    Logger::ProcLog log("ASIOMonitor", Log());
    LOGINFO << this << std::endl;;
}

ASIOMonitor::~ASIOMonitor()
{
    Logger::ProcLog log("~ASIOMonitor", Log());
    LOGINFO << this << std::endl;;
}

void
ASIOMonitor::serviceStarted()
{
    Logger::ProcLog log("serviceStarted", Log());
    LOGINFO << this << std::endl;;
    p_->serviceStarted(getMonitored());
}

void
ASIOMonitor::serviceStopping()
{
    Logger::ProcLog log("serviceStopping", Log());
    LOGINFO << this << std::endl;;
    p_->serviceStopping();
}

ASIOMonitorFactory::Ref
ASIOMonitorFactory::Make(boost::asio::io_service& ios)
{
    Ref ref(new ASIOMonitorFactory(ios));
    return ref;
}
