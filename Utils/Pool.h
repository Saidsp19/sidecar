#ifndef UTILS_POOL_H // -*- C++ -*-
#define UTILS_POOL_H

#include <cstddef> // for size_t and sizeof
#include <exception>
#include <new>       // for std::bad_alloc
#include <stdexcept> //
#include <vector>

namespace Utils {

/** An allocator for same-size memory chunks. Basic idea is to acquire a large block of memory using C++ new
    operator, and then handle chunk requests from the large block. Available chunks are kept on a linked-list,
    which makes chunk allocation and deallocation very fast.

    To use as a class allocator, one needs to override the C++ new and delete operators for the class, and
    create a class attribute for the pool allocator. Here is an example:

    @code
    class Foo
    {
    public:
      static void* operator new(size_t) { return allocator_.allocate(); }
      static void operator delete(void* ptr, size_t) { allocator_.release(ptr); }
    private:
      static Utils::Pool allocator_;
    };

    // Create allocator for all Foo objects. Allocate 1000 at a time.
    Utils::Pool Foo::allocator_(sizeof(Foo), 1000);
    @endcode
*/
class Pool {
public:
    struct AllocationStats {
        size_t numBlocks;
        size_t numAllocated;
        size_t numInUse;
        size_t numFree;
    };

    /** Control whether release() calls have their address argument validated before being used.

        \param flag if true, validate release parameter

        \return previous setting
    */
    static bool SetValidateRelease(bool flag = true);

    /** Control whether allocate and release calls issue a warning message when invoked with an object size not
        equal to the pool size.

        \param flag if true, issue warnings

        \return previous setting
    */
    static bool SetObjectSizeWarning(bool flag = true);

    /** Constructor.

        \param objectSize size of pooled object

        \param objectsPerBlock number of objects to allocate per block of raw memory allocated.
    */
    Pool(std::size_t objectSize, std::size_t objectsPerBlock) throw(std::invalid_argument);

    /** Destructor. Frees allocated memory block.
     */
    ~Pool() throw();

    AllocationStats getAllocationStats() const;

    /** Allocate chunk for object.

        \return address of memory allocated
    */
    void* allocate(std::size_t size) throw(std::bad_alloc);

    /** Reclaim chunk allocated for an object.

        \param ptr address of memory to add back
    */
    void release(void* ptr, std::size_t size) throw();

    /** \return number of memory blocks currently in use.
     */
    size_t numBlocks() const throw() { return blocks_.size(); }

    /** \return number of created chunks
     */
    size_t numAllocated() const throw() { return numBlocks() * chunksPerBlock_; }

    /** \return number of chunks in use
     */
    size_t numInUse() const throw() { return numInUse_; }

    /** \return number of available chunks
     */
    size_t numFree() const throw() { return numAllocated() - numInUse_; }

private:
    /** Allocate a new Block object and initialize it.
     */
    void newBlock() throw(std::bad_alloc);

    /** When debugging, verify that a given pointer came from our allocator.

        \param ptr address to verify

        \return true if our pointer; false otherwise
    */
    bool isPoolObject(void* ptr) const;

    /** Simple structure used to link chunks in the free store.
     */
    struct FreeStore {
        FreeStore* next; ///< Pointer to next item on the free store
    };

    FreeStore* free_;           ///< Address of next chunk of memory to return
    std::vector<char*> blocks_; ///< Array of allocated memory blocks
    size_t objectSize_;         ///< Size of object being managed
    size_t chunkSize_;          ///< Byte count for one item
    size_t chunksPerBlock_;     ///< Number of items per memory block
    size_t blockSize_;          ///< Size of memory block allocated by newBlock
    size_t numInUse_;           ///< Number of chunks in use

    static bool validateRelease_;
    static bool objectSizeWarning_;
}; // class Pool

} // namespace Utils

/** \file
 */

#endif
