/**
 * @file cmake/detect_cpu_features.cpp
 * @brief CPU Feature Detection for Build-Time Optimization
 * 
 * This utility is used by CMake during configuration to detect available
 * CPU features on the build system. It enables compile-time optimization
 * flags to be set based on the actual capabilities of the build machine.
 */

#include <iostream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif

struct CPUFeatures {
    bool sse = false;
    bool sse2 = false;
    bool sse3 = false;
    bool ssse3 = false;
    bool sse4_1 = false;
    bool sse4_2 = false;
    bool avx = false;
    bool avx2 = false;
    bool avx512f = false;
    bool avx512vl = false;
    bool avx512dq = false;
    bool fma = false;
    bool aes = false;
    bool popcnt = false;
};

CPUFeatures detect_cpu_features() {
    CPUFeatures features;
    
#ifdef _MSC_VER
    int cpui[4];
    __cpuid(cpui, 0);
    int max_id = cpui[0];
    
    if (max_id >= 1) {
        __cpuid(cpui, 1);
        features.sse = (cpui[3] & (1 << 25)) != 0;
        features.sse2 = (cpui[3] & (1 << 26)) != 0;
        features.sse3 = (cpui[2] & (1 << 0)) != 0;
        features.ssse3 = (cpui[2] & (1 << 9)) != 0;
        features.sse4_1 = (cpui[2] & (1 << 19)) != 0;
        features.sse4_2 = (cpui[2] & (1 << 20)) != 0;
        features.avx = (cpui[2] & (1 << 28)) != 0;
        features.fma = (cpui[2] & (1 << 12)) != 0;
        features.aes = (cpui[2] & (1 << 25)) != 0;
        features.popcnt = (cpui[2] & (1 << 23)) != 0;
    }
    
    if (max_id >= 7) {
        __cpuidex(cpui, 7, 0);
        features.avx2 = (cpui[1] & (1 << 5)) != 0;
        features.avx512f = (cpui[1] & (1 << 16)) != 0;
        features.avx512vl = (cpui[1] & (1 << 31)) != 0;
        features.avx512dq = (cpui[1] & (1 << 17)) != 0;
    }
#else
    unsigned int eax, ebx, ecx, edx;
    
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        features.sse = (edx & (1 << 25)) != 0;
        features.sse2 = (edx & (1 << 26)) != 0;
        features.sse3 = (ecx & (1 << 0)) != 0;
        features.ssse3 = (ecx & (1 << 9)) != 0;
        features.sse4_1 = (ecx & (1 << 19)) != 0;
        features.sse4_2 = (ecx & (1 << 20)) != 0;
        features.avx = (ecx & (1 << 28)) != 0;
        features.fma = (ecx & (1 << 12)) != 0;
        features.aes = (ecx & (1 << 25)) != 0;
        features.popcnt = (ecx & (1 << 23)) != 0;
    }
    
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        features.avx2 = (ebx & (1 << 5)) != 0;
        features.avx512f = (ebx & (1 << 16)) != 0;
        features.avx512vl = (ebx & (1 << 31)) != 0;
        features.avx512dq = (ebx & (1 << 17)) != 0;
    }
#endif
    
    return features;
}

int main() {
    CPUFeatures features = detect_cpu_features();
    
    std::vector<std::string> detected_features;
    
    if (features.sse) detected_features.push_back("SSE");
    if (features.sse2) detected_features.push_back("SSE2");
    if (features.sse3) detected_features.push_back("SSE3");
    if (features.ssse3) detected_features.push_back("SSSE3");
    if (features.sse4_1) detected_features.push_back("SSE4_1");
    if (features.sse4_2) detected_features.push_back("SSE4_2");
    if (features.avx) detected_features.push_back("AVX");
    if (features.avx2) detected_features.push_back("AVX2");
    if (features.avx512f && features.avx512vl && features.avx512dq) {
        detected_features.push_back("AVX512");
    }
    if (features.fma) detected_features.push_back("FMA");
    if (features.aes) detected_features.push_back("AES");
    if (features.popcnt) detected_features.push_back("POPCNT");
    
    // Output detected features (CMake will capture this)
    for (size_t i = 0; i < detected_features.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << detected_features[i];
    }
    std::cout << std::endl;
    
    return 0;
}