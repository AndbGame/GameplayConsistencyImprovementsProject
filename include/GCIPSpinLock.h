#pragma once

namespace GCIP {
    class SpinLock {
        mutable std::atomic_flag locked = ATOMIC_FLAG_INIT;

    public:
        void spinLock() {
            while (locked.test_and_set(std::memory_order_acquire)) {
#if defined(__cpp_lib_atomic_flag_test)
                while (locked.test(std::memory_order_relaxed)) {
                    ;
                }
#endif
            }
        }
        void spinUnlock() {
            locked.clear(std::memory_order_release); 
        }
    };
}