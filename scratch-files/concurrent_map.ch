class Array {
    func concurrent_map(block) {
        // self = [1, 2, 3, ..., 18, 19, 20]
        const results = Array.of_size(@length)

        const partition_size = Math.floor(@length / Fiber.MaxParallelism)
        const last_error = @length - (Fiber.MaxParallelism * partition_size)

        const partition_counters = Array.of_size(Fiber.MaxParallelism).map(->(p_index) {
            const start = p_index * partition_size
            let limit = start + partition_size

            if p_index + 1 == Fiber.MaxParallelism {
                limit += last_error
            }

            AtomicCounter(start, limit)
        })

        Fiber.MaxParallelism.map(->(i) {
            spawn {

                // work through own partition first
                const own_counter = partition_counters[i]
                loop {
                    const index = own_counter.next()
                    if index == null break
                    results[index] = block(self[index], index, self)
                }

                // steal from other partitions
                loop {
                    let stolen_index
                    partition_counters.find(->(counter) {
                        stolen_index = counter.next()
                        return stolen_index != null
                    })

                    if stolen_index == null break
                    results[index] = block(self[index], index, self)
                }
            }
        }).each(->(worker) await worker)

        return results
    }
}
