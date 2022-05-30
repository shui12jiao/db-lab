#ifndef __DB_BTREE_H__
#define __DB_BTREE_H__

#include "./record.h"
#include "./datatype.h"

namespace db {

#define M (4)
#define CEIL_M_2 (M % 2 ? (M + 1) / 2 : M / 2)
static const unsigned int INV = 0;

class BTree
{
  public:
    BNode *root;
    DataType *type;

  public:
};

class Key
{
  public:
    iovec record;
    unsigned int blkid;

  public:
    Key(unsigned int blkid = 0, void *keybuf = nullptr, unsigned int len = 0)
        : record(iovec{keybuf, len})
        , blkid(blkid)
    {}

    bool less(const Key &key, DataType *type) const;
    bool more(const Key &key, DataType *type) const;
    bool equal(const Key &key, DataType *type) const;
    bool isEmpty() const { return this->blkid == 0; }
};

class BNode
{
  public:
    bool isLeaf;
    Key keys[M];
    BNode *sons[M + 1];
    BNode *parent;

  public:
    BNode()
        : isLeaf(true)
        , keys()
        , sons()
        , parent()
    {}

    // block插入B+树中
    int insert(unsigned int blkid, void *keybuf, unsigned int len);
    // B+树中移除block
    int remove(void *keybuf, unsigned int len);
    //定位key在哪个block
    unsigned int search(void *keybuf, unsigned int len);

    int binarySearch(const Key &key, DataType *type) const;
};

} // namespace db

#endif // __DB_BTREE_H__
