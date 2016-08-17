//
// Created by maxxie on 16-8-10.
//

#ifndef SHARPVPN_NETSTRUCT_H
#define SHARPVPN_NETSTRUCT_H

#include "Package.h"
#include "pthread_wrapper/Mutex.h"
#include "pthread_wrapper/ConditionVariable.h"
#include "../SharpCrypto.h"

namespace netstruct {

    class IdSet {
    private:
        uint64_t *arr;
        long size;
    public:
        IdSet(long arr_size);
        void insert(uint64_t a);
        //		uint64_t operator[](uint64_t index) {
        //			return arr[index % size];
        //		}
        uint64_t find(uint64_t index);
        ~IdSet();
    };

    class PackageMap {
    private:
        PackageData **data_pointer_list;
        long capacity;
        uint64_t *arr_id;
        long head = 1;
        long tail = 1;
        long len = 0;
    public:
        PackageMap(int arr_size);
        PackageData* find(uint64_t index);
        PackageData*& operator[](uint64_t index) {
            return data_pointer_list[index % capacity];
        }
        void set(uint64_t index, PackageData *data);
        //does not delete content
        void erase(uint64_t index);
        void push(PackageData *data, pthread::Mutex *mutex);
        PackageData* pop();
        long size();
        void de_init();
    };

    class PackageQueueMap {
    private:
        uint64_t *queue;
        uint64_t *resend_queue;
        uint64_t *arr;
        int *ref;
        long len = 0;
        long resend_len = 0;
        long capacity;
        PackageData **data_list;
        long start = 0, end = 0;
        long resend_start = 0, resend_end = 0;
    public:
        PackageQueueMap(long __capacity);

        PackageData* operator[](uint64_t id) {
            if (arr[id % capacity] != id)
                return nullptr;
            return data_list[id % capacity];
        }

        //delete old one?
        void push(PackageData *data, uint64_t id);

        void erase(uint64_t id);

        //for resend
        void push(uint64_t id);
        PackageData* pop();
        long size();
        ~PackageQueueMap();
    };

    class AckWaitList{
    private:
        long start = 0, end = 0, len = 0;
        long capacity;
        PackageData **queue;
        int *count_list;
    public:
        AckWaitList(long __capacity);
        void push(PackageData *__data);
        PackageData* pop();
        long size();
        ~AckWaitList();
    };

    /*
    class PackageListMap {
    private:
        uint64_t *arr;
        long len = 0;
        long capacity;
        PackageData **data_list;
        long start = 0;
        long end = 0;
    public:
        PackageListMap(long __capacity) {
            capacity = __capacity;
            data_list = new PackageData*[capacity];
            arr = new uint64_t[capacity];
            for (long i = 0; i < capacity; i++) {
                arr[i] = 0;
                data_list[i] = nullptr;
            }
        }
        ~PackageListMap() {
            delete[] arr;
            delete[] data_list;
        }
        //TODO if the cache is full
        void push_back(PackageData *data, uint64_t id) {
            arr[id % capacity] = id;
            data_list[end] = data;
            end = (end + 1) % capacity;
            len++;
        }
        PackageData* pop_front() {
            PackageData* return_data;
            if (len != 0) {
                return_data = data_list[start];
                start = (start + 1) % capacity;
                len--;
                return return_data;
            }
            return nullptr;
        }
        bool find(uint64_t id) {
            if (arr[id % capacity] == id) {
                return true;
            }
            else {
                return false;
            }
        }
        long size() { return len; }
    };
*/

    class PackageSegment {
    public:
        unsigned char *buf;
        int len;
        PackageSegment(unsigned char *__buf, int __len);
        ~PackageSegment();
    };

    class PackageSegmentList {
    private:
        PackageSegment **segment_list;
        long len = 0, start = 0, end = 0;
        long capacity;
    public:
        PackageSegmentList(long __capacity);
        PackageSegmentList(const PackageSegmentList& queue);
        void push(PackageSegment *segment);
        PackageSegment* pop();
        PackageSegment* front();
        void rebase();
        void de_init();
        long size();
    };

    class SegmentParser {
    public:
        unsigned char *buf = nullptr;
        SharpCrypto *crypto;
        int len = 0;
        int start = 0;
        SegmentParser();
        void load(unsigned char *__buf, int __len);
        int next_segment(unsigned char *__buf);
        ~SegmentParser();
    };
}

#endif //SHARPVPN_NETSTRUCT_H
