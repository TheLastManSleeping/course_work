//
// Created by Arter on 26.11.2020.
//

#include "gtest/gtest.h"
#include "../src/Memory.h"

TEST(tests, NotZeroWaitCyclesCodeResp) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);

    cache.setWaitCycles(1);

    ASSERT_EQ(std::nullopt, cache.Response());
}

TEST(tests, ZeroWaitCyclesCodeResp) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word ip2 = 576;
    std::map<Word, Word> test_line;
    test_line[0] = 15;

    cache.setCacheCodeTableLines(ip2, test_line);
    cache.setCacheCodeLastUsed(ip1);
    cache.setCacheCodeLastUsed(ip2);
    cache.Request(ip2);

    ASSERT_EQ(15 , cache.Response());
}