#include "../catch.hpp"
#include <db/btree.h>
#include <db/datatype.h>

using namespace db;

TEST_CASE("db/btree.h")
{
    SECTION("insert")
    {
        DataType *type = findDataType("BIGINT");
        long long i1 = 11;
        long long i2 = 22;
        long long i3 = 33;
        long long i4 = 44;
        unsigned int size = 8;
        BTree tree(type);
        tree.Insert(1, &i1, size);
        tree.Insert(1, &i2, size);
        tree.Insert(1, &i3, size);
        tree.Insert(1, &i4, size);
    }
}