#ifndef SIDECAR_IO_AUTOCLOSEFILEDESCRIPTOR_H // -*- C++ -*-
#define SIDECAR_IO_AUTOCLOSEFILEDESCRIPTOR_H

#include "Utils/Utils.h"

namespace SideCar {
namespace IO {

/** Simple utility class that closes a held file descriptor when the object is destroyed.
 */
class AutoCloseFileDescriptor : public Utils::Uncopyable
{
public:
    
    /** Constructor. Assumes ownership of the given file descriptor.

        \param fd file descriptor to manage
    */
    explicit AutoCloseFileDescriptor(int fd) : fd_(fd) {}

    /** Destructor. Closes the file descriptor.
     */
    ~AutoCloseFileDescriptor() { if (fd_ != -1) ::close(fd_); }

    /** Manually close the file descriptor.

        \return result of ::close() system call.
    */
    int close()
        {
            int rc = -1;
            do {
                errno = 0;
                rc = ::close(fd_);
            } while (rc == -1 && errno == EINTR);

            fd_ = -1;
            return rc;
        }

    /** Obtain the file descriptor value.

        \return file descriptor
    */
    operator int() const { return fd_; }

    /** Determine if the file descriptor is valid (not -1)

        \return true if so
    */
    operator bool() const { return fd_ != -1; }

private:
    
    int fd_;
};

} // end namespace IO
} // end namespace SideCar

/** \file
 */

#endif
