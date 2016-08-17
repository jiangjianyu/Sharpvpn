//
// Created by maxxie on 16-8-10.
//

#include "NetStruct.h"

    using namespace netstruct;

    IdSet::IdSet(long arr_size) {
        arr = new uint64_t[arr_size];
        size = arr_size;
    }

    void IdSet::insert(uint64_t a) {
        arr[a % size] = a;
    }

    uint64_t IdSet::find(uint64_t index) {
        if (arr[index % size] != index) {
            return 0;
        }
        return arr[index % size];
    }

    IdSet::~IdSet() {
        delete[] arr;
    }



    PackageMap::PackageMap(int arr_size) {
        data_pointer_list = new PackageData*[arr_size];
        arr_id = new uint64_t[arr_size];
        for (int i = 0; i < arr_size; i++) {
            data_pointer_list[i] = nullptr;
            arr_id[i] = 0;
        }
        capacity = arr_size;
    }
    PackageData* PackageMap::find(uint64_t index) {
        if (arr_id[index % capacity] == index) {
            return data_pointer_list[index % capacity];
        }
        return nullptr;
    }

    void PackageMap::set(uint64_t index, PackageData *data) {
		if (arr_id[index % capacity] != 0) {
			delete data_pointer_list[index % capacity];
		}
        arr_id[index % capacity] = index;
        data_pointer_list[index % capacity] = data;
    }

    void PackageMap::erase(uint64_t index) {
        if (arr_id[index % capacity] == index) {
            arr_id[index % capacity] = 0;
			delete data_pointer_list[index % capacity];
        }
    }

    void PackageMap::push(PackageData *data, pthread::Mutex *mutex) {
        uint64_t index = data->data->package_id % capacity;
        while (len >= capacity || arr_id[index] != 0) {
            mutex->unlock();
            msleep(2);
            mutex->lock();
        }
        if (data_pointer_list[tail] != nullptr) {
            delete data_pointer_list[tail];
        }
        data_pointer_list[tail] = data;
        arr_id[tail] = data->data->package_id;
        tail = (tail + 1) % capacity;
        len++;
    }

    PackageData* PackageMap::pop() {
        if (len <= 0)
            return nullptr;
        PackageData *return_data = data_pointer_list[head];
        head = (head + 1) % capacity;
        len--;
        return return_data;
    }

    long PackageMap::size() { return len; }

    void PackageMap::de_init() {
        for (int i = 0; i < capacity; i++) {
            if (data_pointer_list[i]) {
                delete data_pointer_list[i];
            }
        }
        delete[] arr_id;
    }


    PackageQueueMap::PackageQueueMap(long __capacity) {
        capacity = __capacity;
        data_list = new PackageData*[capacity];
        arr = new uint64_t[capacity];
        queue = new uint64_t[capacity * 2];
        resend_queue = new uint64_t[capacity];
        ref = new int[capacity];
        memset(ref, 0, sizeof(int) * capacity);
        for (long i = 0; i < capacity; i++) {
            arr[i] = 0;
            queue[i] = 0;
            queue[i + capacity] = 0;
            resend_queue[i] = 0;
            data_list[i] = nullptr;
        }
    }


    //delete old one?
    void PackageQueueMap::push(PackageData *data, uint64_t id) {
        queue[end] = id;
        end = (end + 1) % (2 * capacity);
        if (arr[id % capacity] != 0) {
            delete data_list[id % capacity];
        }
        data_list[id % capacity] = data;
        arr[id % capacity] = id;
        ref[id % capacity] = 1;
        len++;
    }

    void PackageQueueMap::erase(uint64_t id) {
        if (arr[id % capacity] != id) {
            return;
        }
        ref[id % capacity]--;
        if (ref[id % capacity] > 0) {
            return;
        }
        delete data_list[id % capacity];
        arr[id % capacity] = 0;
    }

    //for resend
    void PackageQueueMap::push(uint64_t id) {
        resend_queue[resend_end] = id;
        resend_end = (resend_end + 1) % (capacity);
        ref[id % capacity]++;
        resend_len++;
    }

    PackageData* PackageQueueMap::pop() {
        if (len <= 0 && resend_len <= 0) {
            return nullptr;
        }
        uint64_t return_id;
        if (resend_len > 0) {
            return_id = resend_queue[resend_start];
            resend_start = (resend_start + 1) % capacity;
            resend_len--;
        } else {
            return_id = queue[start];
            start = (start + 1) % (2 * capacity);
            len--;
        }
        if (return_id != arr[return_id % capacity]) {
            return nullptr;
        }
        return data_list[return_id % capacity];
    }
    long PackageQueueMap::size() { return resend_len + len; }
    PackageQueueMap::~PackageQueueMap() {
        for (int i = 0; i < capacity; i++) {
            if (arr[i] != 0) {
                delete data_list[i];
            }
        }
        delete[] arr;
        delete[] queue;
        delete[] data_list;
        delete[] ref;
    }



    AckWaitList::AckWaitList(long __capacity) {
        capacity = __capacity;
        queue = new PackageData*[capacity];
        count_list = new int[capacity];
        memset(count_list, 0, sizeof(int) * capacity);
        memset(queue, 0, sizeof(PackageData*) * capacity);
    }
    void AckWaitList::push(PackageData *__data) {
        queue[end] = __data;
        end = (end + 1) % capacity;
        count_list[__data->data->package_id % capacity]++;
        len++;
    }
    PackageData* AckWaitList::pop() {
        if (len <= 0)
            return nullptr;
        PackageData *return_data = queue[start];
        while (count_list[return_data->data->package_id % capacity] > 1) {
            count_list[return_data->data->package_id % capacity]--;
            start = (start + 1) % capacity;
            len--;
            return_data = queue[start];
        }
        start = (start + 1) % capacity;
        len--;
        count_list[return_data->data->package_id % capacity] = 0;
        return return_data;
    }
    long AckWaitList::size() {return len;}
    AckWaitList::~AckWaitList() {
        delete[] count_list;
        delete[] queue;
    }


    PackageSegment::PackageSegment(unsigned char *__buf, int __len) {
        buf = new unsigned char[__len];
        memcpy(buf, __buf, __len);
        len = __len;
    }
    PackageSegment::~PackageSegment() {
        delete[] buf;
    }


    PackageSegmentList::PackageSegmentList(long __capacity) {
        capacity = __capacity;
        segment_list = new PackageSegment*[capacity];
    }
    PackageSegmentList::PackageSegmentList(const PackageSegmentList& queue) {
        segment_list = queue.segment_list;
        len = queue.len;
        start = queue.start;
        end = queue.end;
        capacity = queue.capacity;
    }
    void PackageSegmentList::push(PackageSegment *segment) {
        segment_list[end] = segment;
        end = (end + 1) % capacity;
        len++;
    }
    PackageSegment* PackageSegmentList::pop() {
        if (len <= 0)
            return nullptr;
        PackageSegment *segment = segment_list[start];
        start = (start + 1) % capacity;
        len--;
        return segment;
    }
    PackageSegment* PackageSegmentList::front() {
        if (len <= 0)
            return nullptr;
        return segment_list[start];
    }
    void PackageSegmentList::rebase() {
        len = 0;
        start = end;
    }
    void PackageSegmentList::de_init() {
        delete[] segment_list;
    }
    long PackageSegmentList::size() { return len; }


    SegmentParser::SegmentParser() {
        buf = new unsigned char[1600];
    }
    void SegmentParser::load(unsigned char *__buf, int __len) {
//        memcpy(buf, __buf, __len);
        unsigned long de_r = crypto->decrypt(buf, __buf, __len);
        if (de_r == 0)
            return;
        len = de_r;
        start = 0;
    }
    int SegmentParser::next_segment(unsigned char *__buf) {
        if (len == 0) {
            return 0;
        }
        int seg_len = *((uint16_t*)(buf + start));
        if (seg_len == 0) {
            return 0;
        }
        memcpy(__buf, buf+ start + 2, seg_len);
        start += (seg_len + 2);
        return seg_len;
    }
    SegmentParser::~SegmentParser() {
        delete[] buf;
    }

