/*

*2007/09/26

*/
//************************************************************
// #ifndef MIB_H
// #define MIB_H


#define MIB_BASE_OBJID_DEVDES           (2)
#define ZjgBee_DEFAULT_ANN_INTERVAL   (5000)
#define ZigBee_RESERVED_VALUE         (0x20)
#define ZigBee_DEFAULT_DEVID1         "cqupt_zigbee_001                "
#define ZigBee_DEFAULT_DEVID2	      "cqupt_zigbee_002                "
#define ZigBee_DEFAULT_DEVID3	      "cqupt_zigbee_003                "
#define ZigBee_DEFAULT_DEVID4	      "cqupt_zigbee_004                "
#define ZigBee_DEFAULT_DEVID5	      "cqupt_zigbee_005"
#define ZigBee_DEV_VER                0x01  //标示网桥
#define ZigBee_DEFAULT_PDTAG          "AI_CONTROL                      "
#define ZigBee_DEV_TYPE               (0x01)//温湿度
//#define ZigBee_DEV_TYPE               (0x01)//烟雾
//#define ZigBee_DEV_TYPE               (0x01)//保留
//#define ZigBee_DEV_TYPE               (0x01)//阀门
//#define ZigBee_DEV_TYPE               (0x01)//保留

/*------------------------------------------------------------------------------*
 *- EPA Mib const definition
 *------------------------------------------------------------------------------*/

#define MIB_NUM_FBAPP                   (5)
#define MIB_NUM_DOMAINAPP               (5)
#define MIB_NUM_LINKOBJ                 (5)

#define MIB_BASE_OBJID_MIBHDR           (1)
#define MIB_BASE_OBJID_DEVDES           (2)
#define MIB_BASE_OBJID_CLKSYNC          (3)
#define MIB_BASE_OBJID_MAXRSPTIME       (4)
#define MIB_BASE_OBJID_COMSCHEDULE      (5)
#define MIB_BASE_OBJID_DEVAPP           (6)
#define MIB_BASE_OBJID_FBAPPHDR         (7)
#define MIB_BASE_OBJID_FBAPP            (2000)
#define MIB_BASE_OBJID_DOMAINHDR        (9)
#define MIB_BASE_OBJID_DOMAINAPP        (4000)
#define MIB_BASE_OBJID_LINKOBJHDR       (8)
#define MIB_BASE_OBJID_LINKOBJ          (5000)
#define ZigBee_PIB		(10)

#define PHY_AIB_ID (1)
#define MAC_AIB_ID (2)
#define NWK_AIB_ID (3)
#define APS_AIB_ID (4)
#define All_Attribute_ID     (0xe1)
#define NODE_DESC_CLASS 10
#define NODE_POWER_DESC_CLASS 11
#define SIMPLE_CLASS 12
#define localtypeID (0x00)
#define flags_valID (0x01)
#define APS_flagsID 0x02
#define FrequencybandID 0x03
#define MACcapabilityflagsID 0x04
#define ManufacturercodeID 0x05
#define MaximumbuffersizeID 0x06
#define MaximumtransfersizeID 0x07
#define mode_valID 0x10
#define CurrentpowermodeID 0x11
#define AvailablepowersourcesID 0x12
#define source_valID 0x13
#define CurrentpowersourceID 0x14
#define CurrentpowersourcelevelID 0x15
#define endpointID 0x20
#define ApplicationprofileidentifierID 0x21
#define ApplicationdeviceidentifierID 0x22
#define S_flags_valID 0x23
#define ApplicationdeviceversionID 0x24
#define ApplicationflagsID 0x25

typedef struct {
	UINT16	obj_id;
	UINT8	res[2];
	BYTE	dev_id[32];                          // 设备标识
	BYTE   pd_tag[32];                          // 设备位号
	UINT16	 act_ShortAddress;               //当前可操作的短地址
	LADDR     act_ExtendedAddress;           // 当前可操作IP地址
	UINT8          dev_type;                        // 设备类型
	UINT8          status;                          // 设备所处状态
	UINT16         dev_ver;                         // 设备版本号
	UINT16         ann_interval;                    // 设备声明发送的时间间隔
	UINT16         ann_ver;                         // 所发送声明的版本号
	UINT8          dev_r_state;                     // 设备冗余状态
	UINT8          dev_r_num;                       // 设备冗余号
	UINT16         lan_r_port;                      // 设备冗余消息处理端口
	UINT16         max_r_num;                       // 设备最大冗余个数
	BOOL	dup_tag_detected;                // 设备的物理位号是否与网络中其他设备重复
	UINT8          zigbeeID;						//无线设备ID
} DevDes, *PDevDes;


void  DevDes_Init(void) ;
void  MIBInit(void);
void MIBRead();
void ZigBeePIB_read(void);
void MIBWRITE();
void ZigBeePIB_WRITE();
void h2n16(UINT16 src, UINT8 dst[]) ;
void h2n32(UINT32 src, UINT8 dst[]);
void n2h32(UINT32* dst,UINT8 src[]) ;
void n2h16(UINT16* dst,UINT8 src[]) ;




