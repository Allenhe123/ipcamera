#ifndef ONVIFPIPE_H
#define ONVIFPIPE_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <unordered_map>

#include "onvif_comm.h"
#include "onvif_dump.h"
#include "decodethread.h"

class ONVIFPipe
{
public:
    ONVIFPipe(const std::string& user, const std::string& paswd, const  std::string& mac): 
        username_(user), paswd_(paswd), mac_(mac) {}
    ~ONVIFPipe() = default;

    void OpenRtsp(const std::string& mrtsp);
    int  ONVIFGetStreamUri(const char *MediaXAddr, char *ProfileToken, char *uri, unsigned int sizeuri,const char *username, const char *password);
    void Authorize(const char *DeviceXAddr,std::string username,std::string password);
    void ONVIFDetectDevice(int timeout);

private:
    std::unordered_map<std::string, std::string> cameras_;
    std::string username_;
    std::string paswd_;
    std::string mac_;
};

#endif // ONVIFPIPE_H
