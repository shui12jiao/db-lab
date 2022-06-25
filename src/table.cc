////
// @file table.cc
// @brief
// 实现存储管理
//
// @author niexw
// @email niexiaowen@uestc.edu.cn
//
#include <db/table.h>
#include <db/btree.h>

namespace db {

Table::BlockIterator::BlockIterator()
    : bufdesp(nullptr)
{}
Table::BlockIterator::~BlockIterator()
{
    if (bufdesp) kBuffer.releaseBuf(bufdesp);
}
Table::BlockIterator::BlockIterator(const BlockIterator &other)
    : block(other.block)
    , bufdesp(other.bufdesp)
{
    if (bufdesp) bufdesp->addref();
}

// 前置操作
Table::BlockIterator &Table::BlockIterator::operator++()
{
    if (block.buffer_ == nullptr) return *this;
    unsigned int blockid = block.getNext();
    kBuffer.releaseBuf(bufdesp);
    if (blockid) {
        bufdesp = kBuffer.borrow(block.table_->name_.c_str(), blockid);
        block.attach(bufdesp->buffer);
    } else
        block.buffer_ = nullptr;
    return *this;
}
// 后置操作
Table::BlockIterator Table::BlockIterator::operator++(int)
{
    BlockIterator tmp(*this);
    if (block.buffer_ == nullptr) return *this;
    unsigned int blockid = block.getNext();
    kBuffer.releaseBuf(bufdesp);
    if (blockid) {
        bufdesp = kBuffer.borrow(block.table_->name_.c_str(), blockid);
        block.attach(bufdesp->buffer);
    } else
        block.buffer_ = nullptr;
    return tmp;
}
// 数据块指针
DataBlock *Table::BlockIterator::operator->() { return &block; }
void Table::BlockIterator::release()
{
    bufdesp->relref();
    block.detach();
}

int Table::open(const char *name)
{
    // 查找table
    std::pair<Schema::TableSpace::iterator, bool> bret = kSchema.lookup(name);
    if (!bret.second) return EEXIST; // 表不存在

    // 填充结构
    name_ = name;
    info_ = &bret.first->second;

    // 加载超块
    SuperBlock super;
    BufDesp *desp = kBuffer.borrow(name, 0);
    super.attach(desp->buffer);

    //设置DataType
    BTree *tree = super.getTree();
    tree->type = info_->fields[info_->key].type;

    // 获取元数据
    maxid_ = super.getMaxid();
    idle_ = super.getIdle();
    first_ = super.getFirst();

    // 释放超块
    super.detach();
    desp->relref();
    return S_OK;
}

unsigned int Table::allocate()
{
    // 空闲链上有block
    DataBlock data;
    SuperBlock super;
    BufDesp *desp;

    if (idle_) {
        // 读idle块，获得下一个空闲块
        desp = kBuffer.borrow(name_.c_str(), idle_);
        data.attach(desp->buffer);
        unsigned int next = data.getNext();
        data.detach();
        desp->relref();

        // 读超块，设定空闲块
        desp = kBuffer.borrow(name_.c_str(), 0);
        super.attach(desp->buffer);
        super.setIdle(next);
        super.setIdleCounts(super.getIdleCounts() - 1);
        super.setDataCounts(super.getDataCounts() + 1);
        super.setChecksum();
        super.detach();
        kBuffer.writeBuf(desp);
        desp->relref();

        unsigned int current = idle_;
        idle_ = next;

        desp = kBuffer.borrow(name_.c_str(), current);
        data.attach(desp->buffer);
        data.clear(1, current, BLOCK_TYPE_DATA);
        desp->relref();

        return current;
    }

    // 没有空闲块
    ++maxid_;
    // 读超块，设定空闲块
    desp = kBuffer.borrow(name_.c_str(), 0);
    super.attach(desp->buffer);
    super.setMaxid(maxid_);
    super.setDataCounts(super.getDataCounts() + 1);
    super.setChecksum();
    super.detach();
    kBuffer.writeBuf(desp);
    desp->relref();
    // 初始化数据块
    desp = kBuffer.borrow(name_.c_str(), maxid_);
    data.attach(desp->buffer);
    data.clear(1, maxid_, BLOCK_TYPE_DATA);
    desp->relref();

    return maxid_;
}

void Table::deallocate(unsigned int blockid)
{
    // 读idle块，获得下一个空闲块
    DataBlock data;
    BufDesp *desp = kBuffer.borrow(name_.c_str(), blockid);
    data.attach(desp->buffer);
    data.setNext(idle_);
    data.setChecksum();
    data.detach();
    kBuffer.writeBuf(desp);
    desp->relref();

    // 读超块，设定空闲块
    SuperBlock super;
    desp = kBuffer.borrow(name_.c_str(), 0);
    super.attach(desp->buffer);
    super.setIdle(blockid);
    super.setIdleCounts(super.getIdleCounts() + 1);
    super.setDataCounts(super.getDataCounts() - 1);
    super.setChecksum();
    super.detach();
    kBuffer.writeBuf(desp);
    desp->relref();

    // 设定自己
    idle_ = blockid;
}

Table::BlockIterator Table::beginblock()
{
    // 通过超块找到第1个数据块的id
    BlockIterator bi;
    bi.block.table_ = this;

    // 获取第1个blockid
    BufDesp *bd = kBuffer.borrow(name_.c_str(), 0);
    SuperBlock super;
    super.attach(bd->buffer);
    unsigned int blockid = super.getFirst();
    kBuffer.releaseBuf(bd);

    bi.bufdesp = kBuffer.borrow(name_.c_str(), blockid);
    bi.block.attach(bi.bufdesp->buffer);
    return bi;
}

Table::BlockIterator Table::endblock()
{
    BlockIterator bi;
    bi.block.table_ = this;
    return bi;
}

unsigned int Table::locate(void *keybuf, unsigned int len)
{
    unsigned int key = info_->key;
    DataType *type = info_->fields[key].type;

    BlockIterator prev = beginblock();
    for (BlockIterator bi = beginblock(); bi != endblock(); ++bi) {
        // 获取第1个记录
        Record record;
        bi->refslots(0, record);

        // 与参数比较
        unsigned char *pkey;
        unsigned int klen;
        record.refByIndex(&pkey, &klen, key);
        bool bret = type->less(pkey, klen, (unsigned char *) keybuf, len);
        if (bret) {
            prev = bi;
            continue;
        }
        // 要排除相等的情况
        bret = type->less((unsigned char *) keybuf, len, pkey, klen);
        if (bret)
            return prev->getSelf(); //
        else
            return bi->getSelf(); // 相等
    }
    return prev->getSelf();
}

int Table::insert(unsigned int blkid, std::vector<struct iovec> &iov)
{
    DataBlock data;
    SuperBlock super;
    data.setTable(this);

    Record record;
    unsigned char *pkey;
    unsigned int klen;

    // 从buffer中借用
    BufDesp *bd = kBuffer.borrow(name_.c_str(), blkid);
    data.attach(bd->buffer);

    // 尝试插入
    std::pair<bool, unsigned short> ret = data.insertRecord(iov);
    if (ret.first) {
        kBuffer.releaseBuf(bd); // 释放buffer
        // 修改表头统计
        bd = kBuffer.borrow(name_.c_str(), 0);
        super.attach(bd->buffer);
        super.setRecords(super.getRecords() + 1);

        //插入索引
        data.refslots(0, record);
        record.refByIndex(&pkey, &klen, 0);
        BTree *tree = super.getTree();
        if (tree->insert(blkid, pkey, klen) == -1) { //重复，则update
            tree->update(blkid, pkey, klen);
        }

        bd->relref();
        return S_OK; // 插入成功
    } else if (ret.second == (unsigned short) -1) {
        kBuffer.releaseBuf(bd); // 释放buffer
        return EEXIST;          // key已经存在
    }

    // 分裂block
    unsigned short insert_position = ret.second;
    std::pair<unsigned short, bool> split_position =
        data.splitPosition(Record::size(iov), insert_position);
    // 先分配一个block
    DataBlock next;
    next.setTable(this);
    blkid = allocate();
    BufDesp *bd2 = kBuffer.borrow(name_.c_str(), blkid);
    next.attach(bd2->buffer);

    //获取索引树
    BufDesp *bds = kBuffer.borrow(name_.c_str(), 0);
    super.attach(bds->buffer);
    BTree *tree = super.getTree();

    // 移动记录到新的block上
    while (data.getSlots() > split_position.first) {
        Record record;
        data.refslots(split_position.first, record);

        //修改索引
        record.refByIndex(&pkey, &klen, 0);
        tree->update(next.getSelf(), pkey, klen);

        next.copyRecord(record);
        data.deallocate(split_position.first);

        
    }
    bds->relref();

    // 插入新记录，不需要再重排顺序
    if (split_position.second)
        data.insertRecord(iov);
    else
        next.insertRecord(iov);
    // 维持数据链
    next.setNext(data.getNext());
    data.setNext(next.getSelf());

    // bd关联super
    bd = kBuffer.borrow(name_.c_str(), 0);
    super.attach(bd->buffer);

    //获取并修改新block首条记录的blkid
    next.refslots(0, record);
    record.refByIndex(&pkey, &klen, 0);
    if (tree->insert(next.getSelf(), pkey, klen) == -1) { //重复，则update
        tree->update(next.getSelf(), pkey, klen);
    }

    bd2->relref();

    super.setRecords(super.getRecords() + 1);
    bd->relref();
    return S_OK;
}

int Table::remove(unsigned int blkid, void *keybuf, unsigned int len)
{
    DataBlock data;
    SuperBlock super;
    data.setTable(this);

    Record record;
    unsigned char *pkey;
    unsigned int klen;

    // 从buffer中借用
    BufDesp *bd = kBuffer.borrow(name_.c_str(), blkid);
    data.attach(bd->buffer);

    // 先确定删除位置
    unsigned short index = data.searchRecord(keybuf, len);
    if (index >= data.getSlots()) { return ESRCH; }

    data.deallocate(index);

    DataHeader *header = reinterpret_cast<MetaHeader *>(data.buffer_);

    // next block合并至该block
    if (header->freesize > BLOCK_SIZE / 2 && header->next != 0) {
        DataBlock next;
        BufDesp *bdn = kBuffer.borrow(name_.c_str(), header->next);
        next.attach(bdn->buffer);
        next.setTable(this);
        if (next.getFreeSize() > BLOCK_SIZE / 2) {
            //获取索引树
            BufDesp *bds = kBuffer.borrow(name_.c_str(), 0);
            super.attach(bds->buffer);
            BTree *tree = super.getTree();
            

            // 移动next记录到data上
            while (next.getSlots() > 0) {
                next.refslots(0, record);

                //修改索引
                record.refByIndex(&pkey, &klen, 0);
                tree->update(blkid, pkey, klen);

                data.copyRecord(record);
                next.deallocate(0);              
            }
            data.setNext(next.getNext());

            //获取并修改新block首条记录的blkid
            data.refslots(0, record);
            record.refByIndex(&pkey, &klen, 0);
            if (tree->insert(blkid, pkey, klen) == -1) { //重复，则update
                tree->update(blkid, pkey, klen);
            }
            bds->relref();

            // data page最后重排
            RelationInfo *info = this->info_;
            unsigned int key = info->key;
            DataType *type = info->fields[key].type;
            data.reorder(type, key);

            kBuffer.releaseBuf(bd); // 释放buffer
            // 修改表头统计
            bd = kBuffer.borrow(name_.c_str(), 0);
            super.attach(bd->buffer);
            next.setNext(super.getIdle());
            super.setIdle(next.getSelf());
            super.setIdleCounts(super.getIdleCounts() - 1);
            super.setRecords(super.getRecords() - 1);

            bd->relref();
            bdn->relref();
            return S_OK;
        }
        bdn->relref();
    }

    kBuffer.releaseBuf(bd); // 释放buffer
    
    // 修改表头统计
    bd = kBuffer.borrow(name_.c_str(), 0);
    super.attach(bd->buffer);
    super.setRecords(super.getRecords() - 1);

    //获取并修改新block首条记录的blkid
    BTree *tree = super.getTree();
    data.refslots(0, record);
    record.refByIndex(&pkey, &klen, 0);
    if (tree->insert(blkid, pkey, klen) == -1) { //重复，则update
        tree->update(blkid, pkey, klen);
    }

    bd->relref();

    return S_OK; // 删除成功
}

int Table::update(unsigned int blkid, std::vector<struct iovec> &iov)
{
    DataBlock data;
    SuperBlock super;
    data.setTable(this);

    // 从buffer中借用
    BufDesp *bd = kBuffer.borrow(name_.c_str(), blkid);
    data.attach(bd->buffer);
    // 尝试更新
    bool success = data.updateRecord(iov);

    kBuffer.releaseBuf(bd);

    //bd关联super
    bd = kBuffer.borrow(name_.c_str(), 0);
    super.attach(bd->buffer);
    BTree *tree = super.getTree();

    Record record;
    unsigned char *pkey;
    unsigned int klen;

    //插入索引
    data.refslots(0, record);
    record.refByIndex(&pkey, &klen, 0);
    if (tree->insert(blkid, pkey, klen) == -1) { //重复，则update
        tree->update(blkid, pkey, klen);
    }

    bd->relref();

    if (!success) { return S_FALSE; }
    return S_OK;
}

// btree搜索
unsigned int Table::search(void *keybuf, unsigned int len)
{
    BufDesp *bd = kBuffer.borrow(name_.c_str(), 0);
    SuperBlock super;
    super.attach(bd->buffer);
    BTree *tree = super.getTree();
    return tree->search(keybuf, len);
}

size_t Table::recordCount()
{
    BufDesp *bd = kBuffer.borrow(name_.c_str(), 0);
    SuperBlock super;
    super.attach(bd->buffer);
    size_t count = super.getRecords();
    kBuffer.releaseBuf(bd);
    return count;
}

unsigned int Table::dataCount()
{
    BufDesp *bd = kBuffer.borrow(name_.c_str(), 0);
    SuperBlock super;
    super.attach(bd->buffer);
    unsigned int count = super.getDataCounts();
    kBuffer.releaseBuf(bd);
    return count;
}

unsigned int Table::idleCount()
{
    BufDesp *bd = kBuffer.borrow(name_.c_str(), 0);
    SuperBlock super;
    super.attach(bd->buffer);
    unsigned int count = super.getIdleCounts();
    kBuffer.releaseBuf(bd);
    return count;
}

} // namespace db