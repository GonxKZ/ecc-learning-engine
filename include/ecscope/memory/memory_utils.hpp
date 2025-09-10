#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>
#include <immintrin.h>
#include <algorithm>
#include <bit>

#ifdef _WIN32
    #include <windows.h>
    #include <memoryapi.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

namespace ecscope::memory {

// ==== SIMD-OPTIMIZED MEMORY OPERATIONS ====
class SIMDMemoryOps {
public:
    // Check CPU capabilities
    static bool has_sse2() noexcept {
        static bool checked = false;
        static bool result = false;
        
        if (!checked) {
            uint32_t cpuid[4];
            __cpuid(reinterpret_cast<int*>(cpuid), 1);
            result = (cpuid[3] & (1 << 26)) != 0;
            checked = true;
        }
        
        return result;
    }
    
    static bool has_avx2() noexcept {
        static bool checked = false;
        static bool result = false;
        
        if (!checked) {
            uint32_t cpuid[4];
            __cpuid(reinterpret_cast<int*>(cpuid), 7);
            result = (cpuid[1] & (1 << 5)) != 0;
            checked = true;
        }
        
        return result;
    }
    
    static bool has_avx512() noexcept {
        static bool checked = false;
        static bool result = false;
        
        if (!checked) {
            uint32_t cpuid[4];
            __cpuid(reinterpret_cast<int*>(cpuid), 7);
            result = (cpuid[1] & (1 << 16)) != 0;
            checked = true;
        }
        
        return result;
    }
    
    // Ultra-fast memory copy using best available SIMD instructions
    static void fast_copy(void* dest, const void* src, size_t size) noexcept {
        if (size == 0) return;
        
        auto dst = static_cast<uint8_t*>(dest);
        auto src_ptr = static_cast<const uint8_t*>(src);
        
        // Handle unaligned start
        while (size > 0 && (reinterpret_cast<uintptr_t>(dst) & 0x1F) != 0) {
            *dst++ = *src_ptr++;
            --size;
        }
        
        // Choose optimal copy strategy based on size and CPU capabilities
        if (size >= 512 && has_avx512()) {
            fast_copy_avx512(dst, src_ptr, size);
        } else if (size >= 64 && has_avx2()) {
            fast_copy_avx2(dst, src_ptr, size);
        } else if (size >= 16 && has_sse2()) {
            fast_copy_sse2(dst, src_ptr, size);
        } else {
            std::memcpy(dst, src_ptr, size);
        }
    }
    
    // Ultra-fast memory set using SIMD
    static void fast_set(void* dest, uint8_t value, size_t size) noexcept {
        if (size == 0) return;
        
        auto dst = static_cast<uint8_t*>(dest);
        
        // Handle unaligned start
        while (size > 0 && (reinterpret_cast<uintptr_t>(dst) & 0x1F) != 0) {
            *dst++ = value;
            --size;
        }
        
        if (size >= 512 && has_avx512()) {
            fast_set_avx512(dst, value, size);
        } else if (size >= 64 && has_avx2()) {
            fast_set_avx2(dst, value, size);
        } else if (size >= 16 && has_sse2()) {
            fast_set_sse2(dst, value, size);
        } else {
            std::memset(dst, value, size);
        }
    }
    
    // Compare memory blocks using SIMD
    static int fast_compare(const void* ptr1, const void* ptr2, size_t size) noexcept {
        if (size == 0) return 0;
        if (ptr1 == ptr2) return 0;
        
        auto p1 = static_cast<const uint8_t*>(ptr1);
        auto p2 = static_cast<const uint8_t*>(ptr2);
        
        // For large blocks, use SIMD comparison
        if (size >= 64 && has_avx2()) {
            return fast_compare_avx2(p1, p2, size);
        } else if (size >= 16 && has_sse2()) {
            return fast_compare_sse2(p1, p2, size);
        }
        
        return std::memcmp(p1, p2, size);
    }
    
    // Zero memory using fastest available method
    static void fast_zero(void* dest, size_t size) noexcept {
        fast_set(dest, 0, size);
    }

private:
    static void fast_copy_avx512(uint8_t* dst, const uint8_t* src, size_t& size) noexcept {
        while (size >= 64) {
            __m512i chunk = _mm512_loadu_si512(src);
            _mm512_storeu_si512(dst, chunk);
            dst += 64;
            src += 64;
            size -= 64;
        }
        
        // Handle remaining bytes
        if (size > 0) {
            std::memcpy(dst, src, size);
        }
    }
    
    static void fast_copy_avx2(uint8_t* dst, const uint8_t* src, size_t& size) noexcept {
        while (size >= 32) {
            __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), chunk);
            dst += 32;
            src += 32;
            size -= 32;
        }
        
        if (size > 0) {
            std::memcpy(dst, src, size);
        }
    }
    
    static void fast_copy_sse2(uint8_t* dst, const uint8_t* src, size_t& size) noexcept {
        while (size >= 16) {
            __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), chunk);
            dst += 16;
            src += 16;
            size -= 16;
        }
        
        if (size > 0) {
            std::memcpy(dst, src, size);
        }
    }
    
    static void fast_set_avx512(uint8_t* dst, uint8_t value, size_t& size) noexcept {
        __m512i pattern = _mm512_set1_epi8(value);
        
        while (size >= 64) {
            _mm512_storeu_si512(dst, pattern);
            dst += 64;
            size -= 64;
        }
        
        if (size > 0) {
            std::memset(dst, value, size);
        }
    }
    
    static void fast_set_avx2(uint8_t* dst, uint8_t value, size_t& size) noexcept {
        __m256i pattern = _mm256_set1_epi8(value);
        
        while (size >= 32) {
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), pattern);
            dst += 32;
            size -= 32;
        }
        
        if (size > 0) {
            std::memset(dst, value, size);
        }
    }
    
    static void fast_set_sse2(uint8_t* dst, uint8_t value, size_t& size) noexcept {
        __m128i pattern = _mm_set1_epi8(value);
        
        while (size >= 16) {
            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), pattern);
            dst += 16;
            size -= 16;
        }
        
        if (size > 0) {
            std::memset(dst, value, size);
        }
    }
    
    static int fast_compare_avx2(const uint8_t* p1, const uint8_t* p2, size_t& size) noexcept {
        while (size >= 32) {
            __m256i chunk1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p1));
            __m256i chunk2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p2));
            __m256i diff = _mm256_xor_si256(chunk1, chunk2);
            
            if (!_mm256_testz_si256(diff, diff)) {
                // Found difference, fall back to byte-by-byte comparison
                return std::memcmp(p1, p2, std::min(size_t{32}, size));
            }
            
            p1 += 32;
            p2 += 32;
            size -= 32;
        }
        
        return size > 0 ? std::memcmp(p1, p2, size) : 0;
    }
    
    static int fast_compare_sse2(const uint8_t* p1, const uint8_t* p2, size_t& size) noexcept {
        while (size >= 16) {
            __m128i chunk1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1));
            __m128i chunk2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2));
            __m128i diff = _mm_xor_si128(chunk1, chunk2);
            
            if (!_mm_test_all_zeros(diff, diff)) {
                return std::memcmp(p1, p2, std::min(size_t{16}, size));
            }
            
            p1 += 16;
            p2 += 16;
            size -= 16;
        }
        
        return size > 0 ? std::memcmp(p1, p2, size) : 0;
    }
};

// ==== ADVANCED MEMORY ALIGNMENT ====
class AlignmentUtils {
public:
    // Get optimal alignment for given size and access pattern
    static constexpr size_t get_optimal_alignment(size_t size) noexcept {
        if (size >= 64) return 64;      // Cache line alignment
        if (size >= 32) return 32;      // AVX2 alignment
        if (size >= 16) return 16;      // SSE2 alignment
        if (size >= 8) return 8;        // 64-bit alignment
        return alignof(std::max_align_t);
    }
    
    // Allocate aligned memory with custom alignment
    template<size_t Alignment>
    static void* aligned_alloc(size_t size) {
        static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be power of 2");
        static_assert(Alignment >= alignof(void*), "Alignment too small");
        
#ifdef _WIN32
        return _aligned_malloc(size, Alignment);
#else
        return std::aligned_alloc(Alignment, align_up(size, Alignment));
#endif
    }
    
    // Free aligned memory
    static void aligned_free(void* ptr) noexcept {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }
    
    // Check if pointer is aligned
    template<size_t Alignment>
    static constexpr bool is_aligned(const void* ptr) noexcept {
        return (reinterpret_cast<uintptr_t>(ptr) & (Alignment - 1)) == 0;
    }
    
    // RAII aligned memory wrapper
    template<size_t Alignment>
    class AlignedMemory {
        static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be power of 2");
        
    public:
        explicit AlignedMemory(size_t size) : size_(size) {
            ptr_ = aligned_alloc<Alignment>(size);
            if (!ptr_) {
                throw std::bad_alloc{};
            }
        }
        
        ~AlignedMemory() {
            if (ptr_) {
                aligned_free(ptr_);
            }
        }
        
        // Non-copyable, movable
        AlignedMemory(const AlignedMemory&) = delete;
        AlignedMemory& operator=(const AlignedMemory&) = delete;
        
        AlignedMemory(AlignedMemory&& other) noexcept 
            : ptr_(std::exchange(other.ptr_, nullptr))
            , size_(std::exchange(other.size_, 0)) {}
        
        AlignedMemory& operator=(AlignedMemory&& other) noexcept {
            if (this != &other) {
                if (ptr_) aligned_free(ptr_);
                ptr_ = std::exchange(other.ptr_, nullptr);
                size_ = std::exchange(other.size_, 0);
            }
            return *this;
        }
        
        [[nodiscard]] void* get() noexcept { return ptr_; }
        [[nodiscard]] const void* get() const noexcept { return ptr_; }
        [[nodiscard]] size_t size() const noexcept { return size_; }
        
        template<typename T>
        [[nodiscard]] T* as() noexcept {
            return static_cast<T*>(ptr_);
        }
        
        template<typename T>
        [[nodiscard]] const T* as() const noexcept {
            return static_cast<const T*>(ptr_);
        }

private:
        void* ptr_;
        size_t size_;
    };
};

// ==== MEMORY PROTECTION ====
class MemoryProtection {
public:
    enum class Protection {
        NONE = 0,
        READ = 1,
        WRITE = 2,
        EXECUTE = 4,
        READ_WRITE = READ | WRITE,
        READ_EXECUTE = READ | EXECUTE,
        READ_WRITE_EXECUTE = READ | WRITE | EXECUTE
    };
    
    // Set memory protection on a region
    static bool protect_memory(void* ptr, size_t size, Protection protection) noexcept {
        if (!ptr || size == 0) return false;
        
#ifdef _WIN32
        DWORD old_protect;
        DWORD new_protect = 0;
        
        switch (protection) {
            case Protection::NONE:
                new_protect = PAGE_NOACCESS;
                break;
            case Protection::READ:
                new_protect = PAGE_READONLY;
                break;
            case Protection::READ_WRITE:
                new_protect = PAGE_READWRITE;
                break;
            case Protection::READ_EXECUTE:
                new_protect = PAGE_EXECUTE_READ;
                break;
            case Protection::READ_WRITE_EXECUTE:
                new_protect = PAGE_EXECUTE_READWRITE;
                break;
            default:
                return false;
        }
        
        return VirtualProtect(ptr, size, new_protect, &old_protect) != 0;
#else
        int prot = 0;
        
        if (static_cast<int>(protection) & static_cast<int>(Protection::READ)) {
            prot |= PROT_READ;
        }
        if (static_cast<int>(protection) & static_cast<int>(Protection::WRITE)) {
            prot |= PROT_WRITE;
        }
        if (static_cast<int>(protection) & static_cast<int>(Protection::EXECUTE)) {
            prot |= PROT_EXEC;
        }
        
        return mprotect(ptr, size, prot) == 0;
#endif
    }
    
    // Create guard pages around memory region
    class GuardedMemory {
        static constexpr size_t PAGE_SIZE = 4096; // Assume 4KB pages
        
    public:
        explicit GuardedMemory(size_t user_size) {
            // Allocate extra pages for guards
            total_size_ = align_up(user_size + 2 * PAGE_SIZE, PAGE_SIZE);
            user_size_ = user_size;
            
#ifdef _WIN32
            base_ptr_ = VirtualAlloc(nullptr, total_size_, 
                                   MEM_COMMIT | MEM_RESERVE, 
                                   PAGE_READWRITE);
#else
            base_ptr_ = mmap(nullptr, total_size_, 
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, 
                           -1, 0);
            if (base_ptr_ == MAP_FAILED) base_ptr_ = nullptr;
#endif
            
            if (!base_ptr_) {
                throw std::bad_alloc{};
            }
            
            // Set up guard pages
            user_ptr_ = static_cast<uint8_t*>(base_ptr_) + PAGE_SIZE;
            
            protect_memory(base_ptr_, PAGE_SIZE, Protection::NONE);  // Front guard
            protect_memory(static_cast<uint8_t*>(base_ptr_) + PAGE_SIZE + user_size_, 
                         PAGE_SIZE, Protection::NONE);  // Back guard
        }
        
        ~GuardedMemory() {
            if (base_ptr_) {
#ifdef _WIN32
                VirtualFree(base_ptr_, 0, MEM_RELEASE);
#else
                munmap(base_ptr_, total_size_);
#endif
            }
        }
        
        // Non-copyable, movable
        GuardedMemory(const GuardedMemory&) = delete;
        GuardedMemory& operator=(const GuardedMemory&) = delete;
        
        GuardedMemory(GuardedMemory&& other) noexcept 
            : base_ptr_(std::exchange(other.base_ptr_, nullptr))
            , user_ptr_(std::exchange(other.user_ptr_, nullptr))
            , user_size_(std::exchange(other.user_size_, 0))
            , total_size_(std::exchange(other.total_size_, 0)) {}
        
        [[nodiscard]] void* get() noexcept { return user_ptr_; }
        [[nodiscard]] const void* get() const noexcept { return user_ptr_; }
        [[nodiscard]] size_t size() const noexcept { return user_size_; }

private:
        void* base_ptr_;
        void* user_ptr_;
        size_t user_size_;
        size_t total_size_;
        
        static constexpr size_t align_up(size_t size, size_t alignment) noexcept {
            return (size + alignment - 1) & ~(alignment - 1);
        }
    };
};

// ==== MEMORY ENCRYPTION ====
class MemoryEncryption {
    static constexpr size_t KEY_SIZE = 32; // 256-bit key
    static constexpr size_t BLOCK_SIZE = 16; // 128-bit blocks
    
public:
    using Key = std::array<uint8_t, KEY_SIZE>;
    
    // Simple XOR-based encryption (for demonstration)
    // In production, use proper cryptographic algorithms like AES
    static void encrypt_inplace(void* data, size_t size, const Key& key) noexcept {
        auto bytes = static_cast<uint8_t*>(data);
        
        for (size_t i = 0; i < size; ++i) {
            bytes[i] ^= key[i % key.size()];
        }
    }
    
    static void decrypt_inplace(void* data, size_t size, const Key& key) noexcept {
        // XOR encryption is symmetric
        encrypt_inplace(data, size, key);
    }
    
    // Generate cryptographically secure key
    static Key generate_key() {
        Key key;
        
#ifdef _WIN32
        // Use Windows CNG for random key generation
        // This is a simplified version
        for (auto& byte : key) {
            byte = static_cast<uint8_t>(rand()); // NOT secure - use proper crypto API
        }
#else
        // Use /dev/urandom on Unix systems
        std::ifstream urandom("/dev/urandom", std::ios::binary);
        if (urandom) {
            urandom.read(reinterpret_cast<char*>(key.data()), key.size());
        } else {
            // Fallback to weak random
            for (auto& byte : key) {
                byte = static_cast<uint8_t>(rand());
            }
        }
#endif
        
        return key;
    }
    
    // Encrypted memory region
    class EncryptedMemory {
    public:
        explicit EncryptedMemory(size_t size) 
            : size_(size), key_(generate_key()) {
            
            memory_ = std::make_unique<AlignmentUtils::AlignedMemory<32>>(size);
        }
        
        // Decrypt for access, re-encrypt after use
        template<typename Func>
        void access(Func&& func) {
            decrypt_inplace(memory_->get(), size_, key_);
            
            // Call user function with decrypted memory
            func(memory_->get(), size_);
            
            // Re-encrypt after use
            encrypt_inplace(memory_->get(), size_, key_);
        }
        
        [[nodiscard]] size_t size() const noexcept { return size_; }

private:
        size_t size_;
        Key key_;
        std::unique_ptr<AlignmentUtils::AlignedMemory<32>> memory_;
    };
};

// ==== COPY-ON-WRITE MEMORY ====
class CopyOnWriteMemory {
public:
    explicit CopyOnWriteMemory(size_t size) : size_(size) {
        // Allocate read-only shared memory initially
#ifdef _WIN32
        // Windows implementation would use CreateFileMapping
        memory_ = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READONLY);
#else
        memory_ = mmap(nullptr, size, PROT_READ, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (memory_ == MAP_FAILED) memory_ = nullptr;
#endif
        
        if (!memory_) {
            throw std::bad_alloc{};
        }
    }
    
    ~CopyOnWriteMemory() {
        if (memory_) {
#ifdef _WIN32
            VirtualFree(memory_, 0, MEM_RELEASE);
#else
            munmap(memory_, size_);
#endif
        }
    }
    
    // Get read-only access
    [[nodiscard]] const void* get_read_ptr() const noexcept {
        return memory_;
    }
    
    // Get write access (triggers copy-on-write)
    [[nodiscard]] void* get_write_ptr() {
        if (!is_writable_) {
            make_writable();
        }
        return memory_;
    }
    
    [[nodiscard]] size_t size() const noexcept { return size_; }

private:
    void make_writable() {
        // Change protection to read-write (triggers COW)
        bool success = MemoryProtection::protect_memory(
            memory_, size_, MemoryProtection::Protection::READ_WRITE);
        
        if (success) {
            is_writable_ = true;
        }
    }
    
    void* memory_;
    size_t size_;
    bool is_writable_ = false;
};

// ==== MEMORY COMPRESSION ====
class MemoryCompression {
public:
    // Simple run-length encoding compression
    struct CompressedData {
        std::unique_ptr<uint8_t[]> data;
        size_t compressed_size;
        size_t original_size;
        
        CompressedData(size_t comp_size, size_t orig_size) 
            : data(std::make_unique<uint8_t[]>(comp_size))
            , compressed_size(comp_size)
            , original_size(orig_size) {}
    };
    
    static std::unique_ptr<CompressedData> compress(const void* input, size_t size) {
        const auto* src = static_cast<const uint8_t*>(input);
        
        // Estimate worst-case size (no compression)
        auto result = std::make_unique<CompressedData>(size * 2, size);
        auto* dst = result->data.get();
        size_t dst_pos = 0;
        size_t src_pos = 0;
        
        while (src_pos < size) {
            uint8_t current_byte = src[src_pos];
            size_t count = 1;
            
            // Count consecutive bytes
            while (src_pos + count < size && src[src_pos + count] == current_byte && count < 255) {
                ++count;
            }
            
            if (count >= 3) {
                // Use run-length encoding
                dst[dst_pos++] = 0xFF; // Escape byte
                dst[dst_pos++] = current_byte;
                dst[dst_pos++] = static_cast<uint8_t>(count);
            } else {
                // Copy literally
                for (size_t i = 0; i < count; ++i) {
                    if (src[src_pos + i] == 0xFF) {
                        // Escape the escape byte
                        dst[dst_pos++] = 0xFF;
                        dst[dst_pos++] = 0x00;
                    } else {
                        dst[dst_pos++] = src[src_pos + i];
                    }
                }
            }
            
            src_pos += count;
        }
        
        result->compressed_size = dst_pos;
        return result;
    }
    
    static bool decompress(const CompressedData& compressed, void* output) {
        const auto* src = compressed.data.get();
        auto* dst = static_cast<uint8_t*>(output);
        size_t src_pos = 0;
        size_t dst_pos = 0;
        
        while (src_pos < compressed.compressed_size && dst_pos < compressed.original_size) {
            uint8_t byte = src[src_pos++];
            
            if (byte == 0xFF && src_pos < compressed.compressed_size) {
                uint8_t next = src[src_pos++];
                
                if (next == 0x00) {
                    // Escaped 0xFF
                    dst[dst_pos++] = 0xFF;
                } else {
                    // Run-length encoded data
                    if (src_pos >= compressed.compressed_size) return false;
                    uint8_t count = src[src_pos++];
                    
                    for (uint8_t i = 0; i < count && dst_pos < compressed.original_size; ++i) {
                        dst[dst_pos++] = next;
                    }
                }
            } else {
                dst[dst_pos++] = byte;
            }
        }
        
        return dst_pos == compressed.original_size;
    }
    
    // Compressed memory region that decompresses on access
    class CompressedMemory {
    public:
        explicit CompressedMemory(const void* data, size_t size) 
            : compressed_(MemoryCompression::compress(data, size)) {}
        
        template<typename Func>
        void access(Func&& func) {
            // Decompress into temporary buffer
            auto temp = std::make_unique<uint8_t[]>(compressed_->original_size);
            
            if (decompress(*compressed_, temp.get())) {
                func(temp.get(), compressed_->original_size);
            }
        }
        
        [[nodiscard]] size_t compressed_size() const noexcept {
            return compressed_->compressed_size;
        }
        
        [[nodiscard]] size_t original_size() const noexcept {
            return compressed_->original_size;
        }
        
        [[nodiscard]] double compression_ratio() const noexcept {
            return static_cast<double>(compressed_->original_size) / compressed_->compressed_size;
        }

private:
        std::unique_ptr<CompressedData> compressed_;
    };
};

} // namespace ecscope::memory