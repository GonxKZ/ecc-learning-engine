#include "src/core/types.hpp"
#include "src/core/log.hpp"
#include "src/core/time.hpp"
#include "src/core/id.hpp"
#include <iostream>

using namespace ecscope::core;

int main() {
    std::cout << "=== ECScope Core Functionality Test ===" << std::endl;
    
    // Test basic types
    std::cout << "Testing basic types..." << std::endl;
    u32 test_u32 = 42;
    f64 test_f64 = 3.14159;
    std::cout << "u32: " << test_u32 << ", f64: " << test_f64 << std::endl;
    
    // Test EntityID
    std::cout << "Testing EntityID..." << std::endl;
    EntityID entity1(1, 1);
    EntityID entity2(2, 1);
    std::cout << "Entity1 valid: " << (entity1.is_valid() ? "true" : "false") << std::endl;
    std::cout << "Entity1 == Entity2: " << (entity1 == entity2 ? "true" : "false") << std::endl;
    std::cout << "Entity1 as u64: " << entity1.as_u64() << std::endl;
    
    // Test EntityIDGenerator
    std::cout << "Testing EntityIDGenerator..." << std::endl;
    auto& generator = entity_id_generator();
    EntityID new_entity = generator.create();
    std::cout << "New entity: " << new_entity.index << ":" << new_entity.generation << std::endl;
    
    // Test Timer
    std::cout << "Testing Timer..." << std::endl;
    Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    f64 elapsed = timer.elapsed_seconds();
    std::cout << "Timer elapsed: " << elapsed << " seconds" << std::endl;
    
    // Test logging (basic)
    std::cout << "Testing logging..." << std::endl;
    LOG_INFO("This is a test log message");
    LOG_INFO("Test message with number: {}", 42);
    
    std::cout << "=== Core functionality test completed ===" << std::endl;
    return 0;
}