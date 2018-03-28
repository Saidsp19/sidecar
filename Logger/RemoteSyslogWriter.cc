#include <sys/socket.h> // for ::socket
#include <sys/types.h>

#include <arpa/inet.h>
#include <errno.h>
#include <iostream>
#include <netdb.h> // for ::gethostbyname
#include <netinet/in.h>
#include <unistd.h> // for ::close

#include "Msg.h"
#include "Writers.h"

using namespace Logger;
using namespace Logger::Writers;

void
RemoteSyslog::open()
{
    // Assume remote host specifier is a DNS name, and try to look it up.
    //
    hostent* addr = ::gethostbyname(host_.c_str());
    if (!addr) {
        // Nope. Try as a IP address in 111.222.333.444 format.
        //
        in_addr_t ip = ::inet_addr(host_.c_str());
        if (ip != INADDR_NONE) { addr = ::gethostbyaddr(reinterpret_cast<char*>(&ip), sizeof(ip), AF_INET); }
    }

    if (!addr) {
        std::cerr << "*** RemoteSyslogWriter: invalid host: " << host_ << '\n';
        return;
    }

    // Obtain socket to remote host. This does not establish a connection since we are using datagrams. Must use
    // ::sendto function to ship data to the host, which requires an address so we fill in and keep around the
    // address to send to.
    //
    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ == -1) {
        std::cerr << "*** RemoteSyslogWriter: failed to create socket - " << errno << '\n';
        return;
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = *reinterpret_cast<unsigned long*>(addr->h_addr);
}

void
RemoteSyslog::close()
{
    if (socket_ != -1) {
        ::close(socket_);
        socket_ = -1;
    }
}

void
RemoteSyslog::write(const Msg& msg) throw(std::invalid_argument)
{
    if (socket_ == -1) open();
    if (socket_ == -1) {
        std::cerr << "RemoteSyslogWriter: connection not open\n";
        return;
    }

    // Reset buffer, and create message for remote syslog daemon with priority at the beginning of it.
    //
    buffer_.str("");
    buffer_ << '<' << Syslog::ConvertPriority(msg.level_) << '>';
    format(buffer_, msg);

    // Now obtain size and pointer to message data.
    //
    std::string s(buffer_.str());
    int size = buffer_.tellp();
    int left = size;
    const char* ptr = s.data();
    while (left > 0) {
        // Attempt to send the log message
        //
        errno = 0;
        int sent = ::sendto(socket_, ptr, size, 0, reinterpret_cast<struct sockaddr*>(&addr_), sizeof(addr_));
        if (sent == -1) {
            // Is this an error that we can handle?
            //
            switch (errno) {
            case EMSGSIZE: size >>= 1; // Try a message half the size. Fall thru to...
            case EINTR: continue;      // Try again
            default: std::cerr << "RemoteSyslogWriter: failed sendto call - " << errno << '\n'; return;
            }
        } else {
            // Update count and pointers and loop around until we've sent everything.
            //
            left -= sent;
            ptr += sent;
            size = left;
        }
    }
}
