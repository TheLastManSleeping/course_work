//
// Created by Arter on 26.11.2020.
//

#include "gtest/gtest.h"
#include "../src/Memory.h"

TEST(tests, NotLdOrStDataReq){
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);

    Word ip1 = 576;
    cache.Request(ip1, IType::Unsupported);

    ASSERT_EQ(true, cache.getSkip());
}

TEST(tests, NotInLastUsedDataReq) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    bool test1;
    Word ip1 = 512;
    Word ip2 = 576;
    cache.setCacheDataLastUsed(ip1);
    cache.setCacheDataLastUsed(ip2);

    cache.Request(ip2, IType::Ld);

    std::list<Word> test_line = cache.getCodeList();

    if (std::find(test_line.begin(), test_line.end(), ip2) == test_line.begin()) {
        test1 = true;
    }

    ASSERT_EQ(test1, true);
    ASSERT_EQ(cache.getWaitCycles(), 3);
}

TEST(tests, InLastUsedDataReq){
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word ip2 = 1984;
    std::map<Word, Word> test_line;
    std::map<Word, Word> test_line2;

    mem.Write(ip1, 2);
    mem.Write(ip1 + 4, 1);
    mem.Write(ip2, 2);
    mem.Write(ip2 + 4, 1);

    int i = 0;
    while (i < 16) {
        test_line[i] = 0;
        i++;
    }

    test_line[0] = 2;
    test_line[1] = 1;
    i = 0;

    while (i < 63) {
        cache.Request(ip2 + i * 100, IType::Ld);
        i++;
    }

    cache.Request(ip1, IType::Ld);

    Word tag = ToLineAddr(ip2);
    i = 0;

    while (i < 16) {
        Word word = mem.Read(tag + i * 4);
        test_line2[ToLineOffset(tag + i * 4)] = word;
        i++;
    }

    ASSERT_EQ(test_line, cache.getLine());
    ASSERT_EQ(ToLineAddr(ip2), cache.getEraseTag());
    ASSERT_EQ(test_line, test_line2);
    ASSERT_EQ(136, cache.getWaitCycles());
}
