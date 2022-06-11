#ifndef __DB_BTREE_H__
#define __DB_BTREE_H__

#include "./record.h"
#include "./datatype.h"

namespace db {

#define M (5)

class Key
{
  public:
    void *keybuf;
    unsigned int len;
    unsigned int blkid;

  public:
    Key(unsigned int blkid = 0, void *keybuf = nullptr, unsigned int len = 0)
        : keybuf(keybuf)
        , len(len)
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
    BNode(bool leaf = true, BNode *parent = nullptr)
        : isLeaf(leaf)
        , parent(parent)
        , sons()
    {
        for (int i = 0; i < M; i++) {
            keys[i] = Key();
        }
    }

    // block插入B+树中，仅插入key，对于内部节点，在函数外处理sons的插入
    int insert(const Key key, DataType *type, BNode *who = nullptr);
    // B+树中移除block
    int remove(const Key key, DataType *type);
    //定位key在哪个block
    unsigned int search(const Key key, DataType *type) const;
    //分裂节点，返回分裂出的新节点指针
    BNode *split();
    //二分搜索至最后一个小于等于插入值的位置
    int binarySearch(const Key &key, DataType *type) const;
    //检测是否keys已满
    bool isFull() const { return this->keys[M - 1].blkid != 0; }
};

class BTree
{
  public:
    BNode *root;
    DataType *type;

  public:
    BTree(DataType *type)
        : type(type)
    {
        root = new BNode();
    }
    ~BTree() { release(root); }

    int Insert(unsigned int blkid, void *keybuf, unsigned int len);

    int Remove(unsigned int blkid, void *keybuf, unsigned int len);

    unsigned int Search(void *keybuf, unsigned int len, DataType *type);

  private:
    void release(BNode *node)
    {
        for (int i = 0; i <= M; i++) {
            if (node->sons[i] == nullptr) { break; }
            release(node->sons[i]);
        }
        delete (node);
        node = nullptr;
    }
};

} // namespace db

#endif // __DB_BTREE_H__
