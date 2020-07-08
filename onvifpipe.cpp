#include "onvifpipe.h"

/************************************************************************
**函数：open_rtsp
**功能：从RTSP获取音视频流数据
**参数：
        [in]  rtsp - RTSP地址
**返回：无
************************************************************************/
void ONVIFPipe::OpenRtsp(const std::string& rtsp)
{
    auto pThread = std::make_unique<decodethread>();
    pThread->setRtsp(rtsp);
    pThread->SetSize(1280,720);
    pThread->run();
}



/************************************************************************
**函数：ONVIF_GetStreamUri
**功能：获取设备码流地址(RTSP)
**参数：
        [in]  MediaXAddr    - 媒体服务地址
        [in]  ProfileToken  - the media profile token
        [out] uri           - 返回的地址
        [in]  sizeuri       - 地址缓存大小
**返回：
        0表明成功，非0表明失败
**备注：
************************************************************************/
int ONVIFPipe::ONVIFGetStreamUri(const char *MediaXAddr, char *ProfileToken, char *uri, unsigned int sizeuri,const char *username, const char *password)
{
    int result = 0;
    struct soap *soap = NULL;
    struct tt__StreamSetup              ttStreamSetup;
    struct tt__Transport                ttTransport;
    struct _trt__GetStreamUri           req;
    struct _trt__GetStreamUriResponse   rep;

    SOAP_ASSERT(NULL != MediaXAddr);
    SOAP_ASSERT(NULL != uri);
    memset(uri, 0x00, sizeuri);
    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(SOAP_SOCK_TIMEOUT)));

    memset(&req, 0x00, sizeof(req));
    memset(&rep, 0x00, sizeof(rep));
    memset(&ttStreamSetup, 0x00, sizeof(ttStreamSetup));
    memset(&ttTransport, 0x00, sizeof(ttTransport));
    ttStreamSetup.Stream                = tt__StreamType__RTP_Unicast;
    ttStreamSetup.Transport             = &ttTransport;
    ttStreamSetup.Transport->Protocol   = tt__TransportProtocol__RTSP;
    ttStreamSetup.Transport->Tunnel     = NULL;
    req.StreamSetup                     = &ttStreamSetup;
    req.ProfileToken                    = ProfileToken;

    ONVIF_SetAuthInfo(soap, username, password);
    result = soap_call___trt__GetStreamUri(soap, MediaXAddr, NULL, &req, &rep);
    SOAP_CHECK_ERROR(result, soap, "GetServices");

    dump_trt__GetStreamUriResponse(&rep);

    result = -1;
    if (NULL != rep.MediaUri) {
        if (NULL != rep.MediaUri->Uri) {
            if (sizeuri > strlen(rep.MediaUri->Uri)) {
                strcpy(uri, rep.MediaUri->Uri);
                result = 0;
            } else {
                SOAP_DBGERR("Not enough cache!\n");
            }
        }
    }

EXIT:

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    return result;
}

void ONVIFPipe::Authorize(const char *DeviceXAddr,std::string username, std::string password)
{
    int stmno = 0;                                                              // 码流序号，0为主码流，1为辅码流
    int profile_cnt = 0;                                                        // 设备配置文件个数
    struct tagProfile *profiles = NULL;                                         // 设备配置文件列表
    struct tagCapabilities capa;                                                // 设备能力信息

    char uri[ONVIF_ADDRESS_SIZE] = {0};                                         // 不带认证信息的URI地址
    char uri_auth[ONVIF_ADDRESS_SIZE + 50] = {0};                               // 带有认证信息的URI地址

    int retcode = ONVIF_GetCapabilities(DeviceXAddr, &capa, username.c_str(),password.c_str());// 获取设备能力信息（获取媒体服务地址）
    if (retcode == 0) std::cout << "ONVIF_GetCapabilities success" << std::endl;
    else std::cout << "ONVIF_GetCapabilities failed" << std::endl;
    std::cout << "after ONVIF_GetCapabilities, addr: " << DeviceXAddr << " user:" << username_ << " paswd:" << paswd_ << std::endl;

    // 获取媒体配置信息（主/辅码流配置信息）
    profile_cnt = ONVIF_GetProfiles(capa.MediaXAddr, &profiles, username.c_str(), password.c_str());

    std::cout << "profilecount: " << profile_cnt << std::endl;

    if (profile_cnt > stmno) {
        ONVIFGetStreamUri(capa.MediaXAddr, profiles[stmno].token, uri, sizeof(uri),
                           username.c_str(), password.c_str()); // 获取RTSP地址

        make_uri_withauth(uri, (char*)username.c_str(), (char*)password.c_str(), 
                          uri_auth, sizeof(uri_auth)); // 生成带认证信息的URI（有的IPC要求认证）
       std::cout<<uri_auth<<std::endl;
       OpenRtsp(uri_auth);                                                    // 读取主码流的音视频数据
    }
}

void ONVIFPipe::ONVIFDetectDevice(int timeout)
{
    int i;
    int result = 0;
    unsigned int count = 0;                                                     // 搜索到的设备个数
    struct soap *soap = NULL;                                                   // soap环境变量
    struct wsdd__ProbeType      req;                                            // 用于发送Probe消息
    struct __wsdd__ProbeMatches rep;                                            // 用于接收Probe应答
    struct wsdd__ProbeMatchType *probeMatch;

    SOAP_ASSERT(NULL != (soap = ONVIF_soap_new(timeout)));

    ONVIF_init_header(soap);                                                    // 设置消息头描述
    ONVIF_init_ProbeType(soap, &req);                                           // 设置寻找的设备的范围和类型
    result = soap_send___wsdd__Probe(soap, SOAP_MCAST_ADDR, NULL, &req);        // 向组播地址广播Probe消息
    while (SOAP_OK == result)                                                   // 开始循环接收设备发送过来的消息
    {
        memset(&rep, 0x00, sizeof(rep));
        result = soap_recv___wsdd__ProbeMatches(soap, &rep);
        if (SOAP_OK == result) {
            if (soap->error) {
                soap_perror(soap, "ProbeMatches");
            } else {                                                            // 成功接收到设备的应答消息
                dump__ProbeMatches_ip(&rep);

                if (NULL != rep.wsdd__ProbeMatches) {
                    count += rep.wsdd__ProbeMatches->__sizeProbeMatch;
                    for(i = 0; i < rep.wsdd__ProbeMatches->__sizeProbeMatch; i++) {
                        probeMatch = rep.wsdd__ProbeMatches->ProbeMatch + i;
                    //    cb_discovery(probeMatch->XAddrs);                             // 使用设备服务地址执行函数回调
                    //    authorize(probeMatch->XAddrs, username_.c_str(), paswd_.c_str());
                        printf("detect camera: %s\n", probeMatch->wsa__EndpointReference.Address);
                        printf("detect camera: %s\n", probeMatch->XAddrs);
                    //    cameras_.emplace_back(probeMatch->XAddrs);
                        cameras_[probeMatch->wsa__EndpointReference.Address] = probeMatch->XAddrs;

                    }
                }
            }
        } else if (soap->error) {
            break;
        }
    }

    SOAP_DBGLOG("\ndetect end! It has detected %d devices!\n", count);

    if (NULL != soap) {
        ONVIF_soap_delete(soap);
    }

    auto ite = cameras_.find(mac_);
    if (ite != cameras_.end()) {
        Authorize(ite->second.c_str(), username_.c_str(), paswd_.c_str());
    } else {
        std::cout << "can not find camera mac addr: " << mac_ << std::endl;
    }
}
