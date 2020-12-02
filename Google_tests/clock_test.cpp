//
// Created by Arter on 26.11.2020.
//

#include "gtest/gtest.h"
#include "../src/Memory.h"


TEST(tests, ClockTest){
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);

    cache.setWaitCycles(3);
    cache.Clock();

    ASSERT_EQ(0 , cache.getWaitCycles());
}
