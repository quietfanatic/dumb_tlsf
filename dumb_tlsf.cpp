
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
void* table [] = {  // Some of these near the beginning will be unused.
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

struct Header {
    Header* prev;
    size_t size;
    char data [0];
};

Header* res = NULL;
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
            Header* new_res = (Header*)_dumb_tlsf_malloc(alloc_size);
            new_res->prev = res;
            new_res->size = alloc_size;
            limit = new_res->data;
            res = new_res;
            end = (char*)res + alloc_size;
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
        res = (Header*)_dumb_tlsf_malloc(size);
        res->prev = NULL;
        res->size = size;
        limit = res->data;
        end = (char*)res + size;
    }
}

void dset_alloc_size (size_t size) {
    dumb_tlsf_private::alloc_size = size;
}

size_t dreserved_memory () {
    using namespace dumb_tlsf_private;
    size_t total = 0;
    for (Header* blk = res; blk; blk = blk->prev)
        total += blk->size;
    return total;
}
size_t dused_memory () {
    using namespace dumb_tlsf_private;
    return dreserved_memory() - (end - limit);
}

void dapocalypse () {
    using namespace dumb_tlsf_private;
    Header* pres;
    for (; res; res = pres) {
        pres = res->prev;
        _dumb_tlsf_free(res);
    }
    limit = NULL;
    end = NULL;
    for (uint i = 0; i < sizeof(table)/sizeof(void*); i++) {
        table[i] = NULL;
    }
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
