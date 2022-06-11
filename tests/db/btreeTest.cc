#include "../catch.hpp"
#include <db/btree.h>
#include <db/datatype.h>
#include <cstdlib>
#include <ctime>

using namespace db;

TEST_CASE("db/btree.h")
{
    SECTION("insert")
    {
        srand((unsigned) time(NULL));
        DataType *type = findDataType("BIGINT");
        unsigned int len = 8;
        const int num = 10000;
        BTree tree(type);
        long long keys[num];

        for (int i = 0; i < num; i++) {
            keys[i] = rand() % 50000;
            type->htobe(keys + i);
            tree.Insert(1, keys + i, len);
        }

        // long long i1 = 11;
        // long long i2 = 22;
        // long long i3 = 33;
        // long long i4 = 44;
        // unsigned int size = 8;
        // DataType *type = findDataType("BIGINT");
        // BTree tree(type);
        // tree.Insert(1, &i1, size);
        // tree.Insert(1, &i2, size);
        // tree.Insert(1, &i3, size);
        // tree.Insert(1, &i4, size);
    }
}