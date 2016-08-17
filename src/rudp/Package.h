//
// Created by jyjia on 2016/8/7.
//

#ifndef SHARPVPN_PACKAGE_H
#define SHARPVPN_PACKAGE_H

#include <cstdint>
#include "TimeMarco.h"

#ifdef TARGET_WIN32
typedef DWORD time_struct_ms;
#else
typedef struct timeval time_struct_ms;
#endif

typedef enum PackageType {
    ConnectionCreate,
    ConnectionCreateR,
    ConnectionClose,
    ConnectionCloseR,
    Ping,
    Data,
    Ack
} PackageType;

namespace netstruct {
    class AckQueue {
    private:
        uint64_t *arr;
        long len = 0, start = 0, end = 0;
        long capacity;
        bool mem;
    public:
        AckQueue(long __capacity) {
            capacity = __capacity;
            arr = new uint64_t[capacity];
            mem = true;
        }
        AckQueue(const AckQueue& queue) {
            arr = queue.arr;
            len = queue.len;
            start = queue.start;
            end = queue.end;
            capacity = queue.capacity;
            mem = false;
        }
        ~AckQueue() {
            if (mem)
                delete[] arr;
        }
        void push(uint64_t id) {
            arr[end] = id;
            end = (end + 1) % capacity;
            len++;
        }
        uint64_t pop() {
            if (len <= 0)
                return 0;
            uint64_t return_val = arr[start];
            start = (start + 1) % capacity;
            len--;
            return return_val;
        }
        void rebase() {
            len = 0;
            start = end;
        }
        void de_init() {
            delete[] arr;
        }
        long size() { return len; }
    };
}

typedef struct PackageBaseFormat {
    PackageType type;
} PackageBaseFormat;

typedef struct PackageAckFormat {
    uint64_t una;
	uint64_t send_from;
    uint64_t ack_count;
} PackageAckFormat;

typedef struct PackageDataFormat {
    uint64_t package_id;
    /* The biggest id of package received. */
    uint64_t una;
} PackageDataFormat;

typedef struct PackagePingFormat {
    /* to indicate if this ping should be sent back
     * if false, this message should be sent back*/
    bool ping_back = false;
} PackagePingFormat;

typedef struct PackageCreateFormat {
    int download_speed;
} PackageCreateFormat;

typedef struct PackageCreateRFormat {
    int port;
    int download_speed;
} PackageCreateRFormat;

typedef struct PackageCloseFormat {

} PackageCloseFormat;

typedef struct PackageCloseRFormat {

} PackageCloseRFormat;

class PackageData {
private:
    //from buf
public:
    unsigned char *bytes;
    int len;
    int data_len;
    PackageBaseFormat *base;
    PackageDataFormat *data;
	int send_count = 0;
    time_struct_ms send_time;
    PackageData(unsigned char *__bytes, int __len, bool from_data) {
        bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackageDataFormat) + __len];
        base = (PackageBaseFormat*)bytes;
        data = (PackageDataFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = Data;

        data_len = __len;
		memcpy(bytes + sizeof(PackageBaseFormat) + sizeof(PackageDataFormat), __bytes, __len);
        len = __len + sizeof(PackageBaseFormat) + sizeof(PackageDataFormat);
    }

    PackageData(int __len) {
        bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackageDataFormat) + __len];
        base = (PackageBaseFormat*)bytes;
        data = (PackageDataFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = Data;

        data_len = __len;
        len = __len + sizeof(PackageBaseFormat) + sizeof(PackageDataFormat);
    }

    //from buf
    PackageData(unsigned char *__bytes, int __total_len) {
		bytes = new unsigned char[__total_len];
		memcpy(bytes, __bytes, __total_len);
        base = (PackageBaseFormat*)bytes;
        data = (PackageDataFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = Data;
        data_len = __total_len - sizeof(PackageBaseFormat) - sizeof(PackageDataFormat);
        len = __total_len;
    }

    ~PackageData(){
        delete[] bytes;
    }

    unsigned char *get_data() {
        return bytes + sizeof(PackageBaseFormat)
               + sizeof(PackageDataFormat);
    }

    int length() { return len; }

};

class PackageAck {
public:
    unsigned char *bytes;
    int len;
    PackageBaseFormat *base;
    PackageAckFormat *ack;
    uint64_t *ack_list;
    bool init = false;

    //from buf
    PackageAck(unsigned char *__bytes, int __total_len) {
        bytes = __bytes;
        base = (PackageBaseFormat*)bytes;
        ack = (PackageAckFormat*)(bytes + sizeof(PackageBaseFormat));
//        base->type = Ack;
        len = __total_len;
        init = false;
    }

	PackageAck(uint64_t max_ack) {
		bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackageAckFormat) + sizeof(uint64_t) * max_ack];
		base = (PackageBaseFormat*)bytes;
		ack = (PackageAckFormat*)(bytes + sizeof(PackageBaseFormat));
		ack_list = (uint64_t*)(bytes + sizeof(PackageBaseFormat) + sizeof(PackageAckFormat));
        init = true;
	}

    void load(uint64_t max_ack, uint64_t una, netstruct::AckQueue& acklist) {
        ack->una = una;
        base->type = Ack;
        int i;
        uint64_t ack_id;
        for (i = 0; i < max_ack && acklist.size() > 0; ) {
            ack_id = acklist.pop();
            if (ack_id > una) {
                ack_list[i] = ack_id;
                i++;
            }
        }
        ack->ack_count = i;
        len = sizeof(PackageBaseFormat) + sizeof(PackageAckFormat) + sizeof(uint64_t) * ack->ack_count;
    }

	void load(unsigned char *__bytes, int __len) {
        if (init)
            delete[] bytes;
        init = false;
		bytes = __bytes;
		len = __len;
		base = (PackageBaseFormat*)bytes;
		ack = (PackageAckFormat*)(bytes + sizeof(PackageBaseFormat));
		ack_list = (uint64_t*)(bytes + sizeof(PackageBaseFormat) + sizeof(PackageAckFormat));
	}

    int length() { return len; }

    ~PackageAck() {
        if(init)
            delete[] bytes;
    }

};

class PackagePing {
public:
    unsigned char *bytes;
    int len;
    PackageBaseFormat *base;
    PackagePingFormat *ping;
    bool init = false;
    PackagePing() {
        bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackagePingFormat)];
        base = (PackageBaseFormat*)(bytes);
        ping = (PackagePingFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = Ping;
        ping->ping_back = false;
        len = sizeof(PackageBaseFormat) + sizeof(PackagePingFormat);
        init = true;
    }
    void load(unsigned char *__bytes, int __len) {
        if (init)
            delete[] bytes;
        init = false;
        bytes = __bytes;
        len = __len;
        base = (PackageBaseFormat*)bytes;
        ping = (PackagePingFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = Ping;
    }
	int length() { return len; }
    ~PackagePing() {
        if(init)
            delete[] bytes;
    }
};

class PackageConnectionCreate {
public:
    unsigned char *bytes;
    int len;
    PackageBaseFormat *base;
	PackageCreateFormat *create;
    PackageConnectionCreate(int download_speed) {
        bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackageCreateFormat)];
        len = sizeof(PackageBaseFormat) + sizeof(PackageCreateFormat);
        base = (PackageBaseFormat*)bytes;
        create = (PackageCreateFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = ConnectionCreate;
        create->download_speed = download_speed;
    }

    //!!do not use
    void load(unsigned char *__bytes, int __len) {
        bytes = __bytes;
        len = __len;
        base = (PackageBaseFormat*)bytes;
        create = (PackageCreateFormat*)(bytes + sizeof(PackageCreateFormat));
    }

    int length() { return len; }

    ~PackageConnectionCreate() {
        delete[] bytes;
    }
};

class PackageConnectionCreateR {
public:
    unsigned char *bytes;
    int len;
    PackageBaseFormat *base;
    PackageCreateRFormat *create_r;
    PackageConnectionCreateR(int download_speed, int port) {
        bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackageCreateRFormat)];
        len = sizeof(PackageBaseFormat) + sizeof(PackageCreateRFormat);
        base = (PackageBaseFormat*)bytes;
        create_r = (PackageCreateRFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = ConnectionCreateR;
        create_r->download_speed = download_speed;
        create_r->port = port;
    }

    //!!do not use
    void load(unsigned char *__bytes, int __len) {
        bytes = __bytes;
        len = __len;
        base = (PackageBaseFormat*)bytes;
        create_r = (PackageCreateRFormat*)(bytes + sizeof(PackageCreateFormat));
    }

    int length() { return len; }

    ~PackageConnectionCreateR() {
        delete[] bytes;
    }
};

class PackageConnectionClose {
public:
    unsigned char *bytes;
    int len;
    PackageBaseFormat *base;
    PackageCloseFormat *close;
    PackageConnectionClose() {
        bytes = new unsigned char[sizeof(PackageBaseFormat) + sizeof(PackageCloseFormat)];
        len = sizeof(PackageBaseFormat) + sizeof(PackageCloseFormat);
        base = (PackageBaseFormat*)bytes;
        close = (PackageCloseFormat*)(bytes + sizeof(PackageBaseFormat));
        base->type = ConnectionClose;
    }

	int length() { return len; }

    ~PackageConnectionClose() {
        delete[] bytes;
    }
};

static PackageType parse_package_type(unsigned char *bytes) {
    PackageBaseFormat *base = (PackageBaseFormat*)bytes;
    return base->type;
}

#endif //SHARPVPN_PACKAGE_H
