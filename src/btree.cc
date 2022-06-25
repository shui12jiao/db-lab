#include <db/btree.h>

namespace db {

// key
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

// BNode
int BNode::insert(Key key, DataType *type, BNode *who)
{
    int idx =
        this->binarySearch(key, type) + 1; // i表示key应当插入在keys中的位置
    if (idx != 0 && this->keys[idx - 1].equal(key, type)) {
        return -1; //重复值则返回-1
    }

    if (this->isFull()) { // BNode已满，发生分裂
        //产生newNode节点，暂时在this插入新值(来自该函数)
        BNode *newNode = this->split(idx);
        this->keys[idx] = key;
        this->sons[idx + 1] = who;

        int i = (M + 1) / 2; //右侧数量>=左侧
        int j = 0;

        // this节点右部移动至newNode，并向上插入新值(来自分裂)
        if (this->isLeaf) { //叶子节点分裂
            // this右部分key移动至newNode
            for (; i < M + 1; i++, j++) {
                newNode->keys[j] = this->keys[i];
                this->keys[i].blkid = 0;
            }

            //向上插入新节点
            if (newNode->parent == nullptr) { //该叶子节点是根节点
                BNode *newRoot = new BNode(false, nullptr); //构建新根节点
                newRoot->keys[0] = newNode->keys[0];
                newRoot->sons[0] = this;
                newRoot->sons[1] = newNode;
                this->parent = newRoot;
                newNode->parent = newRoot;
            } else { //非根节点分裂
                //向父节点插入newNode的最小值，父节点可能发生分裂，newNode->parent在split相关函数中可能更改(挂载到父节点的分裂节点上)
                newNode->parent->insert(
                    newNode->keys[0], type, newNode); //向上层非根节点插入
            }
        } else { //非叶子节点分裂，newNode需要抛去最小值
            // 本来newNode第一个key向上插入，注意此时尚未移动this中keys，sons
            if (newNode->parent == nullptr) { //该内部节点为根节点
                BNode *newRoot = new BNode(false, nullptr); //构建新根节点
                newRoot->keys[0] = this->keys[i];
                newRoot->sons[0] = this;
                newRoot->sons[1] = newNode;
                this->parent = newRoot;
                newNode->parent = newRoot;
            } else { //非根节点分裂
                //向父节点插入newNode的最小值，父节点可能发生分裂，newNode->parent在split相关函数中可能更改(挂载到父节点的分裂节点上)
                newNode->parent->insert(
                    this->keys[i], type, newNode); //向上层非根节点插入
            }
            this->keys[i++].blkid = 0;        //第一个值在this中去掉
            newNode->sons[0] = this->sons[i]; //第一个的指针作为sons[0]
            newNode->sons[0]->parent = newNode; //更改该子节点的parent
            this->sons[i] = nullptr;

            for (; i < M + 1; i++, j++) {
                newNode->keys[j] = this->keys[i];
                this->keys[i].blkid = 0;

                newNode->sons[j + 1] = this->sons[i + 1];
                newNode->sons[j + 1]->parent = newNode;
                this->sons[i + 1] = nullptr;
            }
        }

        if (idx >= (M + 1) / 2) { // key插入在newNode，index发生变化
            idx = newNode->binarySearch(key, type);
        }

    } else {             //未发生分裂，直接插入
        this->move(idx); // j=M-1
        this->keys[idx] = key;
        this->sons[idx + 1] = who;
    }
    return idx;
}

BNode *BNode::split(int index)
{ //搬运右部分到新的右侧节点
    // this节点暂时插入新节点
    this->move(index, M); // j=M+1

    //分裂出的新节点
    return new BNode(this->isLeaf, this->parent);
}

//int BNode::remove(const Key key, DataType *type)
//{
//    int i = this->binarySearch(key, type);
//    if (i == -1) { //小于最小值
//        if (this->isLeaf) {
//            return false; //无，返回false
//        } else {
//            return this->sons[0]->remove(key, type);
//        }
//    }
//
//    Key *searchKey = &this->keys[i];
//    if (searchKey->blkid != 0 && searchKey->equal(key, type)) { //直接找到
//        if (this->isLeaf) { 
//            this->move(i, M - 1, false);
//            return true;
//        } else {
//            if (this->sons[i + 1]->keys[1].blkid == 0) {//
//
//            }
//        }
//
//
//
//        if (searchKey->blkid == key.blkid) {                    //不用修改
//            return true;
//        }
//        searchKey->blkid = key.blkid;
//        if (!this->isLeaf) { //继续向下修改
//            this->sons[i + 1]->update(key, type);
//        }
//        
//
//
//
//    } else if (this->isLeaf) {
//        return true; //不存在，返回true
//    } else {
//        return this->sons[i + 1]->remove(key, type);
//    }
//
//}

bool BNode::update(const Key key, DataType *type)
{
    int i = this->binarySearch(key, type);
    if (i == -1) { //小于最小值
        if (this->isLeaf) {
            return false; //无，返回false
        } else {
            return this->sons[0]->update(key, type); //最左侧子节点进行查找
        }
    }

    Key *searchKey = &this->keys[i];
    if (searchKey->blkid != 0 && searchKey->equal(key, type)) { //直接找到
        if (searchKey->blkid == key.blkid) {//不用修改
            return true;
        }
        searchKey->blkid = key.blkid;
        if (!this->isLeaf) { //继续向下修改
            this->sons[i + 1]->update(key, type);
        }
        return true;
    } else if (this->isLeaf) {
        return false; //没有找到
    } else {
        return this->sons[i + 1]->update(key, type);
    }
}

unsigned int BNode::search(const Key key, DataType *type) const
{
    int i = this->binarySearch(key, type);
    if (i == -1) { //小于最小值
        if (this->isLeaf) {
            return 0; //无，返回blkid=0
        } else {
            return this->sons[0]->search(key, type); //查找最左侧子节点
        }
    }

    const Key searchKey = this->keys[i];
    if (searchKey.blkid != 0 && searchKey.equal(key, type)) { //直接找到
        return searchKey.blkid;
    } else if (this->isLeaf) {
        return searchKey.blkid; //间接寻找(稀疏索引，只插入block的第一条记录)
    } else {
        return this->sons[i + 1]->search(key, type);
    }
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
        } else if (key.more(this->keys[mid], type)) {
            lo = mid + 1;
        } else {
            return mid;
        }
    }
    return hi;
}

// BTree
int BTree::insert(unsigned int blkid, void *keybuf, unsigned int len)
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

bool BTree::update(unsigned int blkid, void *keybuf, unsigned int len)
{
    if (len != this->root->keys[0].len) { return false; }
    Key key = Key(blkid, keybuf, len);
    return root->update(key, this->type);
}

unsigned int BTree::search(void *keybuf, unsigned int len) const
{
    Key key = Key(0, keybuf, len);
    return root->search(key, this->type);
}

} // namespace db
