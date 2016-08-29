#include <errno.h>
#include <netdb.h>

#include <iostream>
#include <sstream>
#include <string>

#include "ace/INET_Addr.h"
#include "ace/SOCK_Dgram_Mcast.h"

int
main(int argc, const char* argv[])
{
    // ACE_INET_Addr addr("237.1.2.100:4465");
    std::string address;

    if (argc != 3) {
	std::cerr << "usage: mcast IP PORT\n";
	return 1;
    }

#if 0
    struct hostent* hostInfo = ::gethostbyname(argv[1]);
    if (! hostInfo) {
	std::cerr << "no host info for " << argv[1] << " - " << h_errno
		  << std::endl;
	return 1;
    }

    std::clog << hostInfo->h_name << ' ' << hostInfo->h_length << std::endl;
    const uint8_t* ptr = (uint8_t*)hostInfo->h_addr_list[0];
    std::ostringstream os;
    os << uint16_t(ptr[0]) << '.' << uint16_t(ptr[1]) << '.'
       << uint16_t(ptr[2]) << '.' << uint16_t(ptr[3]);
    os << ':' << argv[2];
    endhostent();

    address = os.str();
#endif

    address = argv[1];
    address += ':';
    address += argv[2];
    
    std::clog << "address: " << address << std::endl;
    ACE_INET_Addr addr;
    if (addr.set(address.c_str()) == -1) {
	std::cerr << "invalid address - " << errno << std::endl;
	return 1;
    }

    ACE_SOCK_Dgram_Mcast udp;
    char buffer[16 * 1024];
    std::clog << "udp.join: " << udp.join(addr) << std::endl;
    while (1) {
	int rc = udp.recv(buffer, sizeof(buffer), addr);
	std::clog << "udp.recv: " << rc << std::endl;
    }

    udp.leave(addr);

    return 0;
}
