module;

#include <dwhbll/collections/ring.h>
#include <dwhbll/collections/cache.h>
#include <dwhbll/collections/memory_buffer.h>
#include <dwhbll/collections/sorted_linked_list.h>
#include <dwhbll/collections/streams.h>

export module dwhbll.collections;

export namespace dwhbll::collections {
    using dwhbll::collections::Ring;
    using dwhbll::collections::generic_cache;
    using dwhbll::collections::cache;
    using dwhbll::collections::MemBuf;
    using dwhbll::collections::SortedLinkedList;
}
