#include <db/btree.h>

namespace db {

bool Key::less(const Key &key, DataType *type) const
{
    return type->less(
        (unsigned char *) this->record.iov_base,
        this->record.iov_len,
        (unsigned char *) key.record.iov_base,
        key.record.iov_len);
}

bool Key::more(const Key &key, DataType *type) const
{
    return type->less(
        (unsigned char *) key.record.iov_base,
        key.record.iov_len,
        (unsigned char *) this->record.iov_base,
        this->record.iov_len);
}

bool Key::equal(const Key &key, DataType *type) const
{
    return !type->less(
               (unsigned char *) key.record.iov_base,
               key.record.iov_len,
               (unsigned char *) this->record.iov_base,
               this->record.iov_len) &&
           !type->less(
               (unsigned char *) this->record.iov_base,
               this->record.iov_len,
               (unsigned char *) key.record.iov_base,
               key.record.iov_len);
}

int BNode::insert(unsigned int blkid, void *keybuf, unsigned int len)
{
    Key key = Key(blkid, keybuf, len);
    0;
    0;
}

int BNode::remove(void *keybuf, unsigned int len)
{
    0;
    0;
}

unsigned int BNode::search(void *keybuf, unsigned int len)
{
    0;
    0;
}

//返回arr中大于key的第一个值的位置
int BNode::binarySearch(const Key &key, DataType *type) const
{
    int lo = 0, hi = M;
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

} // namespace db