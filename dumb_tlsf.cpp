
#include <stdlib.h>

void* (& _dumb_tlsf_malloc ) (size_t) = malloc;
void (& _dumb_tlsf_free ) (void*) = free;

namespace dumb_tlsf_private {

size_t alloc_size = 1<<20;

constexpr uint segregate1 (uint esize, uint l1) {
    return (l1 << 2 | (esize >> (l1 - 2) & 3)) - 8;
}
constexpr uint segregate (uint esize) {
    return esize == 0 ? 0 : segregate1(esize, 31 - __builtin_clz(esize));
}
constexpr uint esizeof (uint size) {
    return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
}

 // It'll probably be better to have fewer columns in this table...
void* table [] = {  // Some of these near the beginning will be unnused.
    NULL, NULL, NULL, NULL,  // 4
    NULL, NULL, NULL, NULL,  // 8
    NULL, NULL, NULL, NULL,  // 16
    NULL, NULL, NULL, NULL,  // 32
    NULL, NULL, NULL, NULL,  // 64
    NULL, NULL, NULL, NULL,  // 128
    NULL, NULL, NULL, NULL,  // 256
    NULL, NULL, NULL, NULL,  // 512
    NULL, NULL, NULL, NULL,  // 1024
    NULL, NULL, NULL, NULL,  // 2048
    NULL, NULL, NULL, NULL   // 4096
};
const size_t max_size = 4096 + 3*1024;

size_t reserved_memory = 0;
char* limit = NULL;
char* end = NULL;

void* alloc_e (size_t esize, uint index) {
    void*& fl = table[index];
    if (fl) {
        void* r = fl;
        fl = *(void**)fl;
        return r;
    }
    else {
        if (limit + esize > end) {
            limit = (char*)_dumb_tlsf_malloc(alloc_size);
            reserved_memory += alloc_size;
            end = limit + alloc_size;
        }
        void* r = limit;
        limit += esize;
        return r;
    }
}

void free_e (void* p, size_t esize, uint index) {
    *(void**)p = table[index];
    table[index] = p;
}

}

namespace dumb_tlsf {

void* dalloc (size_t size) {
    using namespace dumb_tlsf_private;
    return size > max_size ? _dumb_tlsf_malloc(size) : alloc_e(esizeof(size), segregate(esizeof(size)));
}

void dfree (void* p, size_t size) {
    using namespace dumb_tlsf_private;
    return size > max_size ? _dumb_tlsf_free(p) : free_e(p, esizeof(size), segregate(esizeof(size)));
}

void dreserve (size_t size) {
    using namespace dumb_tlsf_private;
    if (!limit) {
        limit = (char*)_dumb_tlsf_malloc(size);
        end = limit + size;
    }
}

void dset_alloc_size (size_t size) {
    dumb_tlsf_private::alloc_size = size;
}

size_t dused_memory () {
    using namespace dumb_tlsf_private;
    return reserved_memory - (end - limit);
}
size_t dreserved_memory () {
    return dumb_tlsf_private::reserved_memory;
}

template <class T>
T* dalloc () {
    return (T*)dalloc(sizeof(T));
}
template <class T, class... Args>
T* dnew (Args... args) {
    T* r = dalloc<T>();
    r->T(args...);
    return r;
}
template <class T>
void dfree (T* p) {
    dfree(p, sizeof(T));
}
template <class T>
void ddelete (T* p) {
    p->~T();
    dfree(p);
}

}
