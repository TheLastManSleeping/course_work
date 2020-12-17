//
// Created by Arter on 26.11.2020.
//

#include "gtest/gtest.h"
#include "../src/Memory.h"

TEST(tests, NotLdOrStDataResp) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word data1 = 5;

    ASSERT_EQ(true, cache.Response(ip1, IType::Unsupported, data1));

}

TEST(tests, NotZeroWaitCycles) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word data1 = 5;
    cache.setWaitCycles(1);

    ASSERT_EQ(false, cache.Response(ip1, IType::St, data1));
}

TEST(tests, LdDataResp) {
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word ip2 = 576;
    Word data1 = 5;
    std::map<Word, Word> test_line;
    test_line[0] = 15;
    cache.setCacheDataTableLines(ip2, test_line);
    cache.setCacheDataLastUsed(ip1);
    cache.setCacheDataLastUsed(ip2);
    cache.Request(ip2, IType::Ld);
    cache.setWaitCycles(0);

    ASSERT_EQ(true , cache.Response(ip2, IType::Ld, data1));
    ASSERT_EQ(15, cache.getData());
}

TEST(tests, StDataReq){
    MemoryStorage mem;
    CachedMem cache = CachedMem (mem);
    Word ip1 = 512;
    Word ip2 = 576;
    Word data1 = 5;
    std::map<Word, Word> test_line;
    test_line[0] = 1;
    cache.setCacheDataTableLines(ip2, test_line);
    cache.setCacheDataLastUsed(ip1);
    cache.setCacheDataLastUsed(ip2);

    cache.Request(ip2, IType::St);
    cache.setWaitCycles(0);

    ASSERT_EQ(true , cache.Response(ip2, IType::St, data1));
    ASSERT_EQ(mem.Read(ip2), cache.getDataTables()[ToLineAddr(ip2)][ToLineOffset(ip2)]);
}