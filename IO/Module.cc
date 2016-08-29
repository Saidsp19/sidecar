#include "Logger/Log.h"

#include "Module.h"

using namespace SideCar::IO;

Logger::Log&
Module::Log()
{
    Logger::Log& log_ = Logger::Log::Find("SideCar.IO.Module");
    return log_;
}

Module::Module(const Task::Ref& task, const boost::shared_ptr<Stream>& stream)
    : ACE_Module<ACE_MT_SYNCH, ACE_System_Time_Policy>(), task_(task)
{
    Logger::ProcLog log("Module", Log());
    task_->setStream(stream);
    std::string moduleName("Module ");
    moduleName += task_->getTaskName();
    open(moduleName.c_str(),
         task.get(),		// writer task (downstream)
         0,			// reader task (upstream)
         0,			// argument
         ACE_Module_Base::M_DELETE_NONE // Let Boost delete via ref counts
	);

    LOGDEBUG << "reader: " << reader() << " writer: " << writer() << std::endl;
}
