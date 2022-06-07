#include <db/btree.h>
#include <stack>

namespace db {

using std::stack;

stack<BNode *> newNodes;

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

int BNode::insert(const Key key, DataType *type)
{
    int i = this->binarySearch(key, type) + 1; // i表示key应当插入在keys中的位置
    if (this->isFull()) {
        BNode *newNode = this->split(); // this分裂，右侧节点为newNode
        bool left =
            (i < (M - (M / 2))); //分裂后，key应当插入左侧this还是右侧newNode
        if (left) {
            i = this->insert(key, type);
        } else {
            i = -newNode->insert(key, type); // i为负数表示插入了右侧节点
        }
        if (this->parent == nullptr) { // this是根节点，根节点分裂
            BNode *newRoot = new BNode(false, nullptr); //构建新根节点
            newRoot->keys[0] = newNode->keys[0];
            newRoot->sons[0] = this;
            newRoot->sons[1] = newNode;
            this->parent = newRoot;
            newNode->parent = newRoot;
        } else { //非根节点分裂
            int idx = this->parent->insert(newNode->keys[0], type);
            if (idx < 0) { // parent节点又发生了分裂
                // newNode需要插入到父节点（原父节点分裂后右侧newNode）sons中
                //可能多个分裂连续发生，需要使用stack
                BNode *tmp = newNodes.top();
                newNodes.pop();
                tmp->sons[-idx + 1] = newNode;
            } else {
                this->parent->sons[idx + 1] = newNode;
            }
        }
    } else {
        int j = i;
        while (j + 1 < M) {
            this->keys[j + 1] = this->keys[j];
            if (!this->isLeaf) { this->sons[j + 2] = this->sons[j + 1]; }
            j++;
        }
        this->keys[i] = key;
    }
    return i;
}

BNode *BNode::split()
{ //搬运右部分到新的右侧节点
    BNode *newNode = new BNode(this->isLeaf, this->parent);
    int i = M - (M / 2);
    int j = 0;
    newNode->sons[j] = this->sons[i];
    while (i < M) {
        newNode->keys[j] = this->keys[i];
        if (!this->isLeaf) {
            newNode->sons[j + 1] = this->sons[i + 1];
            newNode->sons[j + 1]->parent = newNode;
        }
        this->keys[i].blkid = 0;
        // this->sons[i + 1] = nullptr;
        i++;
        j++;
    }
    newNodes.push(newNode);
    return newNode;
}

// BNode *BNode::splitAndInsert(const Key key, int index) {
//     BNode *newNode = new BNode(this->isLeaf, this->parent);
//     int right = M / 2;
//     int left = M - right;
//     if (index < left) {  //新值插入在左侧
//         left += 1;
//         //右侧值转移到新节点
//         int i = left - 1;
//         int j = 0;
//         while (i < M) {
//             newNode->keys[j] = this->keys[i];
//             if (!this->isLeaf) {
//                 newNode->sons[j] = this->sons[i];
//                 newNode->sons[j]->parent = newNode;
//             }
//             this->keys[i].blkid = 0;
//             this->sons[i] = nullptr;
//             i++;
//             j++;
//         }
//         //左侧插入新值
//         i = index;
//         while (i + 1 < left) {
//             this->keys[i + 1] = this->keys[i];
//             if (!this->isLeaf) {
//                 this->sons[i + 1] = this->sons[i];
//             }
//             i++;
//         }
//         this->keys[index] = key;  //插入新值
//     } else {                      //新值插入在右侧
//         right += 1;
//         //右侧值转移到新节点
//         int i = left;
//         int j = 0;
//         while (i < index) {
//             newNode->keys[j] = this->keys[i];
//             this->keys[i].blkid = 0;  // key被搬走，blkid=0表示为空
//             this->sons[i] = nullptr;
//             if (!this->isLeaf) {
//                 newNode->sons[j] = this->sons[i];
//                 newNode->sons[j]->parent = newNode;
//             }
//             i++;
//             j++;
//         }
//         newNode->keys[j++] = key;  //
//         j=index-left，即右侧应该插入位置，插入后j右移一位 while (i + 1 < M) {
//             newNode->keys[j] = this->keys[i];
//             this->keys[i].blkid = 0;  // key被搬走，blkid=0表示为空
//             this->sons[i] = nullptr;
//             if (!this->isLeaf) {
//                 newNode->sons[j] = this->sons[i];
//                 newNode->sons[j]->parent = newNode;
//             }
//             i++;
//             j++;
//         }
//     }
//     return newNode;
// }

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
    if (root->isLeaf) { //一层
        ret = root->insert(key, this->type);
    } else {
        BNode *node = root;
        while (!node->isLeaf) {
            int i = node->binarySearch(key, type) + 1;
            node = node->sons[i];
        }
        ret = node->insert(key, type);
    }
    if (root->parent != nullptr) { root = root->parent; }
    return ret;
}

} // namespace db