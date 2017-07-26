# Garbage Collector

Charly features a simple Mark and Sweep Garbage Collector. It was written completly from scratch.

The Garbage collector is the component of the machine that is responsible for memory management.

# Heaps

The GC keeps track of a list of heaps. A heap is a fixed-size region of memory capable of holding a fixed
amount of cells.

Heaps are of fixed size so their addresses don't change. When doing a call to `realloc`, the new address might
be different to the one before. Because the VM stores absolute addresses in its data types, it would be
inefficient and cumbersome to go and update all addresses everywhere.

# Cells

A cell is where the VM stores it's data in. It contains a little bit of metadata for the GC and a union
of all types the VM supports.

# Free list

The free list is a unidirectional linked-list of cells which the GC can lease out to the VM for usage.

Once there are no more cells inside the freelist, we do a collection phase. If the collection phase
didn't free any cells for the GC to lease out, we double the amount of heaps we have.
