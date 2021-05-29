#ifndef MANAGE_MALLOC_H
#define MANAGE_MALLOC_H
/**
 * \class managed_malloc Like new[], but it automatically gets freed when the object goes out of context.
 * Similar to alloca(), except that the buffer itself does not go on the stack.
 * throws std::bad_alloc if no memory.
 */
template < class TYPE > class managed_malloc {
    // default construction, copy construction and assignment are meaningless
    // and not implemented
    managed_malloc& operator=(const managed_malloc &);
    managed_malloc(const managed_malloc&);
    managed_malloc();
public:
    TYPE *buf;
    managed_malloc(size_t bytes):buf(new TYPE[bytes]){ }
    ~managed_malloc(){
        if(buf) delete []buf;
    }
};

#endif
