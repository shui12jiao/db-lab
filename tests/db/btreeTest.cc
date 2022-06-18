#include "../catch.hpp"
#include <db/btree.h>
#include <db/datatype.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace db;

TEST_CASE("db/btree.h")
{
    SECTION("compare")
    {
        DataType *type = findDataType("BIGINT");
        unsigned int len = 8;
        long long i1 = 2021;
        type->htobe(&i1);
        long long i2 = 2022;
        type->htobe(&i2);

        Key k1(0, &i1, len);
        Key k2(0, &i2, len);
        REQUIRE(k1.less(k2, type) == true);
        REQUIRE(k2.more(k1, type) == true);
        REQUIRE(k1.equal(k1, type) == true);
        REQUIRE(k1.equal(k2, type) == false);
    }

    SECTION("insert")
    {
        srand((unsigned) time(NULL));
        DataType *type = findDataType("BIGINT");
        unsigned int len = 8;
        const int num = 1000;
        BTree tree(type);
        long long keys[num];

        for (int i = 0; i < num; i++) {
            keys[i] = rand() % 50000;
            type->htobe(keys + i);
            tree.insert(1, keys + i, len);
        }
    }

    SECTION("search1")
    {
        DataType *type = findDataType("BIGINT");
        unsigned int len = 8;
        BTree tree(type);
        long long i1 = 13124122;
        type->htobe(&i1);
        long long i2 = 784273;
        type->htobe(&i2);
        long long i3 = 14781473;
        type->htobe(&i3);
        long long i4 = 13740318;
        type->htobe(&i4);
        long long i6 = 37483801;
        type->htobe(&i6);
        tree.insert(11, &i1, len);
        tree.insert(22, &i2, len);
        tree.insert(33, &i3, len);
        tree.insert(44, &i4, len);
        tree.insert(66, &i6, len);
        REQUIRE(tree.search(&i1, len) == 11);
        REQUIRE(tree.search(&i2, len) == 22);
        REQUIRE(tree.search(&i3, len) == 33);
        REQUIRE(tree.search(&i4, len) == 44);
        REQUIRE(tree.search(&i6, len) == 66);
        long long i5 = 2025;
        type->htobe(&i5);
        REQUIRE(tree.search(&i5, len) == 0);
    }

    SECTION("search2")
    {
        srand(2);
        DataType *type = findDataType("BIGINT");
        unsigned int len = 8;
        BTree tree(type);
        const int num = 2000;
        long long keys[num];

        for (int i = 0; i < num; i++) {
            keys[i] = 2 * i + 1;
            type->htobe(keys + i);
        }
        std::random_shuffle(keys, keys + num);
        for (int i = 0; i < num; i++) {
            tree.insert(i + 1, keys + i, len);
        }

        long long i1 = keys[1];
        long long i2 = keys[2];
        long long i3 = keys[3];

        REQUIRE(tree.search(&i1, len) == 2);
        REQUIRE(tree.search(&i2, len) == 3);
        REQUIRE(tree.search(&i3, len) == 4);

        type->htobe(&i3);
        long long i4 = i3 + 1; //比i3大，树中不存在
        type->htobe(&i4);
        REQUIRE(tree.search(&i4, len) == 4);
    }
}