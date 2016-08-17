//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_REMOTEINFO_H
#define RELIABLEFASTUDP_REMOTEINFO_H


class RemoteInfo {
public:
    long remote_id;
    short remote_port;
    int rtt;
    int remote_download_speed;
    int remote_upload_speed;
    RemoteInfo();
    ~RemoteInfo();
};


#endif //RELIABLEFASTUDP_REMOTEINFO_H
