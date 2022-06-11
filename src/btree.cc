#include <db/btree.h>

namespace db {

bool Key::less(const Key &key, DataType *type) const
{
    return type->less(
        (unsigned char *) this->keybuf,
        this->len,
        (unsigned char *) key.keybuf,
        key.len);
}

bool Key::more(const Key &key, DataType *type) const
{
    return type->less(
        (unsigned char *) key.keybuf,
        key.len,
        (unsigned char *) this->keybuf,
        this->len);
}

bool Key::equal(const Key &key, DataType *type) const
{
    return !type->less(
               (unsigned char *) key.keybuf,
               key.len,
               (unsigned char *) this->keybuf,
               this->len) &&
           !type->less(
               (unsigned char *) this->keybuf,
               this->len,
               (unsigned char *) key.keybuf,
               key.len);
}

int BNode::insert(const Key key, DataType *type, BNode *who)
{
    int i = this->binarySearch(key, type) + 1; // i表示key应当插入在keys中的位置
    if (i != 0 && this->keys[i - 1].equal(key, type)) { return -1; }
    if (this->isFull()) {
        BNode *newNode = this->split(); // this分裂，右侧节点为newNode
        if (i < (M / 2)) { //分裂后，key应当插入左侧this还是右侧newNode
            i = this->insert(key, type);
        } else {
            i = newNode->insert(key, type); // i插入了右侧节点
            if (who != nullptr) { who->parent = newNode; }
        }
        if (this->parent == nullptr) { // this是根节点，根节点分裂
            BNode *newRoot = new BNode(false, nullptr); //构建新根节点
            newRoot->keys[0] = newNode->keys[0];
            newRoot->sons[0] = this;
            newRoot->sons[1] = newNode;
            this->parent = newRoot;
            newNode->parent = newRoot;
        } else { //非根节点分裂
            //向父节点插入newNode的最小值，父节点可能发生分裂，子节点parent在split中更改
            int idx = this->parent->insert(newNode->keys[0], type, newNode);
            newNode->parent->sons[idx + 1] = newNode;
        }
    } else {
        int j = M - 1;
        while (this->keys[j].blkid == 0) {
            j--;
        }
        while (j >= i) {
            this->keys[j + 1] = this->keys[j];
            if (!this->isLeaf) { this->sons[j + 2] = this->sons[j + 1]; }
            j--;
        }
        this->keys[i] = key;
    }
    return i;
}

BNode *BNode::split()
{ //搬运右部分到新的右侧节点
    BNode *newNode = new BNode(this->isLeaf, this->parent);
    int i = M / 2;
    int j = 0;

    this->keys[i++].blkid = 0; //第一个值去掉，指针作为sons[0]
    if (!this->isLeaf) {
        newNode->sons[0] = this->sons[i];
        newNode->sons[0]->parent = newNode;
        this->sons[i] = nullptr;
    }
    while (i < M) {
        newNode->keys[j] = this->keys[i];
        if (!this->isLeaf) {
            newNode->sons[j + 1] = this->sons[i + 1];
            newNode->sons[j + 1]->parent = newNode;
            this->sons[i + 1] = nullptr;
        }
        this->keys[i].blkid = 0;
        i++;
        j++;
    }
    return newNode;
}

int BNode::remove(const Key key, DataType *type)
{
    0;
    0;
    return 0;
}

unsigned int BNode::search(const Key key, DataType *type) const
{
    0;
    0;
    return 0;
}

//返回arr中小于key的第一个值的位置
int BNode::binarySearch(const Key &key, DataType *type) const
{
    int lo = 0, hi = M - 1;
    while (hi >= 0 && this->keys[hi].blkid == 0) {
        --hi;
    }
    while (lo <= hi) {
        int mid = (hi + lo) / 2;
        if (key.less(this->keys[mid], type)) {
            hi = mid - 1;
        } else if (this->keys[mid].less(key, type)) {
            lo = mid + 1;
        } else {
            return mid;
        }
    }
    return hi;
}

int BTree::Insert(unsigned int blkid, void *keybuf, unsigned int len)
{
    Key key = Key(blkid, keybuf, len);

    int ret = -1;

    BNode *node = root;
    while (!node->isLeaf) {
        int i = node->binarySearch(key, type);
        node = node->sons[i + 1];
    }
    ret = node->insert(key, type);

    if (root->parent != nullptr) { root = root->parent; }
    return ret;
}

} // namespace db
