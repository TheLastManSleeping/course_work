//
// Created by Arter on 19.11.2020.
//
#include "gtest/gtest.h"
#include "../src/Memory.h"


TEST(tests, NotInLastUsedCodeReq) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    bool test1;
    Word ip2 = 576;

    cache.Request(ip2);
    std::list<Word> test_list = cache.getCodeList();

     if (std::find(test_list.begin(), test_list.end(), ip2) == test_list.begin()){
         test1 = true;
     }

    ASSERT_EQ(test1, true);
}

TEST(tests, InLastUsedCodeReq) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word ip2 = 516;
    Word ip3 = 2000;
    mem.Write(ip1, 2);
    mem.Write(ip2, 1);
    std::map<Word, Word> test_line;
    int i = 0;

    while (i < 16) {
        test_line[i] = 0;
        i++;
    }

    test_line[0] = 2;
    test_line[1] = 1;
    i = 0;

    while (i < 7) {
        cache.Request(ip3 + i * 100);
        i++;
    }

    cache.Request(ip1);

    ASSERT_EQ(test_line, cache.getLine());
    ASSERT_EQ(ToLineAddr(ip3), cache.getEraseTag());
    ASSERT_EQ(136, cache.getWaitCycles());
}
