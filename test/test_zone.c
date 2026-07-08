#include <radikant-messagepack-c.h>
#include <radikant-probe-c.h>
#include <string.h>

bool test_zone_init_destroy(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 1024);
    mp_zone_destroy(&zone);
    return test_end(test);
}

bool test_zone_alloc(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 1024);
    
    void *ptr = mp_zone_alloc(&zone, 256);
    if (!ptr) append_error(test, "Failed to allocate memory", 0);
    
    memset(ptr, 0xAA, 256);
    
    mp_zone_destroy(&zone);
    return test_end(test);
}

bool test_zone_chaining(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 128);
    
    for (int i = 0; i < 10; i++) {
        void *ptr = mp_zone_alloc(&zone, 64);
        if (!ptr) {
            append_error(test, "Failed to allocate memory on iteration", i);
            break;
        }
        memset(ptr, i, 64);
    }
    
    mp_zone_destroy(&zone);
    return test_end(test);
}

bool test_zone_large_alloc(test_result_t *test) {
    mp_zone_t zone;
    mp_zone_init(&zone, 1024);
    
    void *ptr = mp_zone_alloc(&zone, 4096);
    if (!ptr) append_error(test, "Failed to allocate large memory", 0);
    
    memset(ptr, 0xFF, 4096);
    
    mp_zone_destroy(&zone);
    return test_end(test);
}

test_suite_t suite_zone = {.name = "MessagePack Zone Suite", .standard = "MsgPack-Zone"};

int main(void) {
    r_set_global_verbosity(R_VERBOSE);
    enable_memleak_detection(&suite_zone);
    
    add_test(&suite_zone, test_zone_init_destroy, "Zone: Init and Destroy", "Zone-Spec");
    add_test(&suite_zone, test_zone_alloc, "Zone: Basic Allocation", "Zone-Spec");
    add_test(&suite_zone, test_zone_chaining, "Zone: Block Chaining", "Zone-Spec");
    add_test(&suite_zone, test_zone_large_alloc, "Zone: Large Allocation", "Zone-Spec");
    
    register_suite(&suite_zone);
    bool success = run_all_suites();
    return success ? 0 : 1;
}
