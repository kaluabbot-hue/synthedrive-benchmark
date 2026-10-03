#include <iostream>
#include <chrono>
#include <sys/mman.h>

// Simulating the standard sequential workload environment
void run_baseline_pass(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i += 64) {
        buffer[i] = buffer[i] ^ 0xAA; 
    }
}

// Simulating the optimized SyntheDrive memory pipeline loop
void run_synthedrive_optimized_pass(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i += 64) {
        // 1. Explicitly prefetching data into L3 cache lines
        __builtin_prefetch(&buffer[i + 256], 1, 3);
        
        buffer[i] = buffer[i] ^ 0xAA;
        
        // 2. Hardware memory fence barrier to flatten timing signatures
        #if defined(__x86_64__)
            asm volatile("lfence" ::: "memory");
        #elif defined(__aarch64__)
            asm volatile("isb" ::: "memory");
        #endif
    }
}

int main() {
    size_t mem_size = 1024 * 1024 * 512; // 512MB payload
    
    // Using MAP_ANONYMOUS to pin address spaces cleanly
    uint8_t* target_ram = (uint8_t*)mmap(NULL, mem_size, PROT_READ | PROT_WRITE, 
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (target_ram == MAP_FAILED) {
        std::cerr << "Memory allocation failed." << std::endl;
        return 1;
    }
    
    // Lock memory cells to eliminate page faults
    mlockall(MCL_CURRENT | MCL_FUTURE);

    std::cout << "[SyntheDrive Harness] Memory pinned and locked. Running micro-benchmarks..." << std::endl;
    
    // Execution timing logic
    auto start = std::chrono::high_resolution_clock::now();
    run_synthedrive_optimized_pass(target_ram, mem_size);
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "[Result] SyntheDrive Pipeline Latency: " << duration.count() << " ms" << std::endl;
    
    munmap(target_ram, mem_size);
    return 0;
}
