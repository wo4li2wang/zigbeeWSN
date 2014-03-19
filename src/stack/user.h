/*

*2007/09/26

*/
#define ID_STH        0x01
#define ID_FROG       0x02
#define ID_WENBIAN    0x04
#define ID_FAMEN      0x04
#define ID_DIANDONG   0x03

#define Wireless_device ID_STH                   //LINK DEVICE ,define in user.h

//服务号定义
#define NPMA_TAG                   (0x20)
#define ENPMA_TAG                  (0x21)

#define	EMS_FINDTAGQUERY           (1)		//设备查询请求服务
#define	EMS_FINDTAGREPLY           (2)		//设备查询请求应答服务

#define	EMS_GETATTRIB              (3)		//设备信息读取服务
#define	EMS_GETATTRIB_POS          (3 + 64)
#define	EMS_GETATTRIB_NEG          (3 + 128)

#define	EMS_ANNUNCIATION           (196)   //设备声明服务
#define EMS_WIRELESSANNU           (18)		//无线
#define	EMS_SETATTRIB              (5)		//设备信息设置服务
#define	EMS_SETATTRIB_POS          (5 + 64)
#define	EMS_SETATTRIB_NEG          (5 + 128)

#define	EMS_CLEARATTRIB            (6)
#define	EMS_CLEARATTRIB_POS        (6 + 64)
#define	EMS_CLEARATTRIB_NEG        (6 + 128)

#define	EDS_DOMAINDOWNLOAD         (10)
#define	EDS_DOMAINDOWNLOAD_POS     (10 + 64)
#define	EDS_DOMAINDOWNLOAD_NEG     (10 + 128)

#define	EDS_DOMAINUPLOAD           (11)		//域下载服务
#define	EDS_DOMAINUPLOAD_POS       (11 + 64)
#define	EDS_DOMAINUPLOAD_NEG       (11 + 128)

#define	EVS_READ                   (22)		//变量读服务
#define	EVS_READ_POS               (22 + 64)
#define	EVS_READ_NEG               (22 + 128)

#define	EVS_WRITE                  (24)		//变量写服务
#define	EVS_WRITE_POS              (24 + 64)
#define	EVS_WRITE_NEG              (24 + 128)

#define	EVS_DISTRIBUTE             (206)   //信息发布服务
#define EVS_WIRELESSDB				(20)	//无线信息分发
#define	EES_EVENT_NOTI             (15)		//事件通知服务

#define	EES_ACK_NOTI               (16)		//事件通知确认服务
#define	EES_ACK_NOTI_POS           (16 + 64)
#define	EES_ACK_NOTI_NEG           (16 + 128)

#define	EES_ALTER_MONITOR          (17)		//改变事件条件监视服务
#define	EES_ALTER_MONITOR_POS      (17 + 64)
#define	EES_ALTER_MONITOR_NEG      (17 + 128)
#define NSID_AUTHEN           (41)
#define NSID_AUTHEN_POS       (41+64)
#define NSID_AUTHEN_NEG       (41+128)
#define NSID_ACSCTRL          (43)
#define NSID_ACSCTRL_POS      (107)
#define NSID_ACSCTRL_NEG      (171)




// 各种服务报文长度定义，不含头部

#define LEN_AUTHEN            (35)
#define LEN_AUTHEN_REPLY      (32)
#define LEN_ACCTRL            (12)
#define LEN_ACCTRL_POS        (4)
#define LEN_ACCTRL_NEG        (8 + n)

#define	LEN_EPAHEADER              (8)
#define LEN_ERR_DES                (36)

#define	LEN_FINDTAGQUERY           (70)

#define	LEN_FINDTAGREPLY           (72)

#define	LEN_GETATTRIB              (4)
#define	LEN_GETATTRIB_POS          (82)
#define	LEN_GETATTRIB_NEG          (4 + LEN_ERR_DES)

#define	LEN_ANNUNCIATION           (81) //80+1-24//57

#define	LEN_SETATTRIB              (80)
#define	LEN_SETATTRIB_POS          (5)
#define	LEN_SETATTRIB_NEG          (4 + LEN_ERR_DES)

#define	LEN_CLEARATTRIB            (68)
#define	LEN_CLEARATTRIB_POS        (4)
#define	LEN_CLEARATTRIB_NEG        (4 + LEN_ERR_DES)

#define	LEN_DOMAINDOWNLOAD(N)      (12 + N)
#define	LEN_DOMAINDOWNLOAD_POS     (2)
#define	LEN_DOMAINDOWNLOAD_NEG     (4 + LEN_ERR_DES)

#define	LEN_DOMAINUPLOAD           (8)
#define	LEN_DOMAINUPLOAD_POS(N)    (8 + N)
#define	LEN_DOMAINUPLOAD_NEG       (4 + LEN_ERR_DES)

#define	LEN_READ                   (8)
#define	LEN_READ_POS(N)            (4 + N)
#define	LEN_READ_NEG               (4 + LEN_ERR_DES)

#define	LEN_WRITE(N)               (8 + N)
#define	LEN_WRITE_POS              (2)
#define	LEN_WRITE_NEG              (4 + LEN_ERR_DES)

#define	LEN_DISTRIBUTE(N)          (4 + N)

#define	LEN_EVENT_NOTI(N)          (8 + N)

#define	LEN_ACK_NOTI               (6)
#define	LEN_ACK_NOTI_POS           (2)
#define	LEN_ACK_NOTI_NEG           (4 + LEN_ERR_DES)

#define	LEN_ALTER_MONITOR          (8)
#define	LEN_ALTER_MONITOR_POS      (2)
#define	LEN_ALTER_MONITOR_NEG      (4 + LEN_ERR_DES)


#define ZigBee_MIB_APP_ID             (01)

extern BYTE usrRxData[85];
void Read_rsp();
void Read_cmf();
void Write_rsp();
void Read_POS(BYTE dstMode,LADDR_UNION *dstADDR,BYTE dstEP,BYTE cluster,BYTE srcEP,BYTE* pload,BYTE plen,BYTE tsn,BYTE reqack);
void Read_NES(BYTE dstMode,LADDR_UNION *dstADDR,BYTE dstEP,BYTE cluster,BYTE srcEP,BYTE* pload, BYTE plen,BYTE tsn,BYTE reqack);
void Read_POS_rsp();
void Read_NES_rsp();
void servicefunction(void);
void EVS_DISTRIBUTE_rsp();
//void EVS_WIRELESSDB(BYTE kkkk[],BYTE llll);
void  WIRELESSDB_Device(BYTE *devicedata,BYTE length);
void LED_FLASH(void);
#ifdef LRWPAN_COORDINATOR
void usrRecedataFromGateway(void);
void usrSendDataToGateway(void);

#endif

