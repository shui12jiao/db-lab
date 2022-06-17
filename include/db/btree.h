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
    Key keys[M + 1];
    BNode *sons[M + 2];
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
    BNode *split(int index);
    //二分搜索至最后一个小于等于插入值的位置
    int binarySearch(const Key &key, DataType *type) const;
    //检测是否keys已满
    bool isFull() const { return this->keys[M - 1].blkid != 0; }
    //插入点i后至j移动
    void move(int i, int j = M - 1)
    {
        while (this->keys[j].blkid == 0) {
            j--;
        }
        for (; j >= i; j--) {
            this->keys[j + 1] = this->keys[j];
            if (!this->isLeaf) { this->sons[j + 2] = this->sons[j + 1]; }
        }
    }
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

    unsigned int Search(void *keybuf, unsigned int len, DataType *type)
    {
        if (this->type != type) { return 0;}
        Key key = Key(0, keybuf, len);
        return root->search(key, type);
    }

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
