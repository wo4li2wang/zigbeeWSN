#ifndef IEEE_LRWPAN_DEFS_H
#define IEEE_LRWPAN_DEFS_H

//IEEE 802.15.4 Frame definitions here

#define LRWPAN_BCAST_SADDR       0xFFFF //短的广播地址，表示对所有当前侦听该通信信道的设备的均有效
#define LRWPAN_BCAST_PANID       0xFFFF //广播传输方式，表示对所有当前侦听该通信信道的设备的均有效
#define LRWPAN_SADDR_USE_LADDR   0xFFFE //没有分配短地址，只能使用扩展地址

#define LRWPAN_ACKFRAME_LENGTH 5  //确认帧总长度,see Page 110

#define LRWPAN_MAX_MACHDR_LENGTH 23 //MAC层帧头最大长度，see Page 102
#define LRWPAN_MAX_NETHDR_LENGTH 8  //网络层帧头最大长度，see Page 195
#define LRWPAN_MAX_APSHDR_LENGTH 5  //应用子层帧头最大长度

//数据包所有帧头长度，包括应用子层，网络层和MAC层(不包括物理层)，为23+8+5=36
#define LRWPAN_MAXHDR_LENGTH (LRWPAN_MAX_MACHDR_LENGTH+LRWPAN_MAX_NETHDR_LENGTH+LRWPAN_MAX_APSHDR_LENGTH)



#define LRWPAN_MAX_FRAME_SIZE 127

#define LRWPAN_FRAME_TYPE_BEACON 0  //帧类型为信标帧
#define LRWPAN_FRAME_TYPE_DATA 1    //帧类型为数据帧
#define LRWPAN_FRAME_TYPE_ACK 2     //帧类型为确认帧
#define LRWPAN_FRAME_TYPE_MAC 3     //帧类型为MAC命令帧

//BYTE masks
#define LRWPAN_FCF_SECURITY_MASK 0x8    //安全允许位MASK
#define LRWPAN_FCF_FRAMEPEND_MASK 0x10  //帧未处理标记位MASK
#define LRWPAN_FCF_ACKREQ_MASK 0x20     //请求确认标记位MASK
#define LRWPAN_FCF_INTRAPAN_MASK 0x40   //内部PAN标记位MASK



#define LRWPAN_SET_FRAME_TYPE(x,f)     (x=x|f)    //设置帧类型
#define LRWPAN_GET_FRAME_TYPE(x)     (x&0x03)     //获得帧类型

//分别将安全允许位，帧未处理标志位，请求确认标志位，内部PAN标志位置 '1'
#define LRWPAN_SET_SECURITY_ENABLED(x) BITSET(x,3)
#define LRWPAN_SET_FRAME_PENDING(x)    BITSET(x,4)
#define LRWPAN_SET_ACK_REQUEST(x)      BITSET(x,5)
#define LRWPAN_SET_INTRAPAN(x)         BITSET(x,6)

//分别将安全允许位，帧未处理标志位，请求确认标志位，内部PAN标志位置 '0'
#define LRWPAN_CLR_SECURITY_ENABLED(x) BITCLR(x,3)
#define LRWPAN_CLR_FRAME_PENDING(x)    BITCLR(x,4)
#define LRWPAN_CLR_ACK_REQUEST(x)      BITCLR(x,5)
#define LRWPAN_CLR_INTRAPAN(x)         BITCLR(x,6)

//分别获取安全允许位，帧未处理标志位，请求确认标志位，内部PAN标志位
#define LRWPAN_GET_SECURITY_ENABLED(x) BITTST(x,3)
#define LRWPAN_GET_FRAME_PENDING(x)    BITTST(x,4)
#define LRWPAN_GET_ACK_REQUEST(x)      BITTST(x,5)
#define LRWPAN_GET_INTRAPAN(x)         BITTST(x,6)

//地址模式
#define LRWPAN_ADDRMODE_NOADDR 0  //PAN标志符和地址子域不存在
#define LRWPAN_ADDRMODE_SADDR  2  //包含16位短地址子域
#define LRWPAN_ADDRMODE_LADDR  3  //包含64位扩展地址子域

//获取，设置目的地址和源地址模式，x为帧控制域高字节
#define LRWPAN_GET_DST_ADDR(x) ((x>>2)&0x3)
#define LRWPAN_GET_SRC_ADDR(x) ((x>>6)&0x3)
#define LRWPAN_SET_DST_ADDR(x,f) (x=x|(f<<2))
#define LRWPAN_SET_SRC_ADDR(x,f) (x=x|(f<<6))

#define LRWPAN_FCF_DSTMODE_MASK   (0x03<<2)
#define LRWPAN_FCF_DSTMODE_NOADDR (LRWPAN_ADDRMODE_NOADDR<<2)
#define LRWPAN_FCF_DSTMODE_SADDR (LRWPAN_ADDRMODE_SADDR<<2)
#define LRWPAN_FCF_DSTMODE_LADDR (LRWPAN_ADDRMODE_LADDR<<2)

#define LRWPAN_FCF_SRCMODE_MASK   (0x03<<6)
#define LRWPAN_FCF_SRCMODE_NOADDR (LRWPAN_ADDRMODE_NOADDR<<6)
#define LRWPAN_FCF_SRCMODE_SADDR (LRWPAN_ADDRMODE_SADDR<<6)
#define LRWPAN_FCF_SRCMODE_LADDR (LRWPAN_ADDRMODE_LADDR<<6)

#define LRWPAN_IS_ACK(x) (LRWPAN_GET_FRAME_TYPE(x) == LRWPAN_FRAME_TYPE_ACK)	//判断帧类型，是否为确认帧
#define LRWPAN_IS_BCN(x) (LRWPAN_GET_FRAME_TYPE(x) == LRWPAN_FRAME_TYPE_BEACON)	//判断帧类型，是否为信标帧
#define LRWPAN_IS_MAC(x) (LRWPAN_GET_FRAME_TYPE(x) == LRWPAN_FRAME_TYPE_MAC)	//判断帧类型，是否为MAC命令
#define LRWPAN_IS_DATA(x) (LRWPAN_GET_FRAME_TYPE(x) == LRWPAN_FRAME_TYPE_DATA)	//判断帧类型，是否为数据帧

//The ASSOC Req and Rsp are not 802 compatible as more information is
//added to these packets than is in the spec.

#ifdef IEEE_802_COMPLY
#define LRWPAN_MACCMD_ASSOC_REQ       0x01			//连接请求命令
#define LRWPAN_MACCMD_ASSOC_RSP       0x02			//连接响应命令
#else
#define LRWPAN_MACCMD_ASSOC_REQ       0x81			//连接请求命令
#define LRWPAN_MACCMD_ASSOC_RSP       0x82			//连接响应命令
#endif

#define LRWPAN_MACCMD_DISASSOC        0x03			//断开连接通告命令
#define LRWPAN_MACCMD_DATA_REQ        0x04			//数据请求命令
#define LRWPAN_MACCMD_PAN_CONFLICT    0x05			//PAN ID冲突通告命令
#define LRWPAN_MACCMD_ORPHAN          0x06			//孤立通告命令
#define LRWPAN_MACCMD_BCN_REQ         0x07			//信标请求命令
#define LRWPAN_MACCMD_COORD_REALIGN   0x08			//协调器重新同步命令
#define LRWPAN_MACCMD_GTS_REQ         0x09			//GTS分配和解除命令
#define LRWPAN_MACCMD_SYNC_REQ 0X0A
#define LRWPAN_MACCMD_SYNC_RSP 0X0B

#define LRWPAN_MACCMD_ASSOC_REQ_PAYLOAD_LEN 2   //连接请求命令有效负载的长度
#define LRWPAN_MACCMD_ASSOC_RSP_PAYLOAD_LEN 4   //连接响应命令有效负载的长度

/*
//说明了一些命令有效长度，信标帧(网络层)长度
#ifdef IEEE_802_COMPLY
#define LRWPAN_MACCMD_ASSOC_REQ_PAYLOAD_LEN 2   //连接请求命令有效负载的长度
#define LRWPAN_MACCMD_ASSOC_RSP_PAYLOAD_LEN 4   //连接响应命令有效负载的长度
#else
#define LRWPAN_MACCMD_ASSOC_REQ_PAYLOAD_LEN 6  //has four extra bytes in it, 'magic number'
#define LRWPAN_MACCMD_ASSOC_RSP_PAYLOAD_LEN 7  //has three extra bytes, shortAddr & depth of parent
#endif
*/

#define LRWPAN_MACCMD_DISASSOC_PAYLOAD_LEN 2    //断开连接通告命令有效负载的长度

#define LRWPAN_MACCMD_BCN_REQ_PAYLOAD_LEN  1

#define DEVICE_DISASSOC_WITH_NETWORK	2//设备希望与PAN断开
#define FORCE_DEVICE_OUTOF_NETWORK	1//协调器希望设备与PAN断开

#define LRWPAN_MACCMD_COORD_REALIGN_PAYLOAD_LEN 8   //协调器重新同步命令有效负载的长度


//this is only for our beacons
#ifdef LRWPAN_ZIGBEE_BEACON_COMPLY
#define LRWPAN_NWK_BEACON_SIZE (9+4)    //9 byte payload, 4 byte header
#else
#define LRWPAN_NWK_BEACON_SIZE (9+4+4) //add in an extra four-byte magic number
#endif


//连接请求命令中的性能信息子域格式，see Page113
#define LRWPAN_ASSOC_CAPINFO_ALTPAN       0x01
#define LRWPAN_ASSOC_CAPINFO_DEVTYPE      0x02
#define LRWPAN_ASSOC_CAPINFO_PWRSRC       0x04
#define LRWPAN_ASSOC_CAPINFO_RONIDLE      0x08
#define LRWPAN_ASSOC_CAPINFO_SECURITY     0x40
#define LRWPAN_ASSOC_CAPINFO_ALLOCADDR    0x80

#define LRWPAN_GET_CAPINFO_ALTPAN(x)       BITTST(x,0)
#define LRWPAN_GET_CAPINFO_DEVTYPE(x)      BITTST(x,1)
#define LRWPAN_GET_CAPINFO_PWRSRC(x)       BITTST(x,2)
#define LRWPAN_GET_CAPINFO_RONIDLE(x)      BITTST(x,3)
#define LRWPAN_GET_CAPINFO_SECURITY(x)     BITTST(x,6)
#define LRWPAN_GET_CAPINFO_ALLOCADDR(x)    BITTST(x,7)

#define LRWPAN_SET_CAPINFO_ALTPAN(x)       BITSET(x,0)
#define LRWPAN_SET_CAPINFO_DEVTYPE(x)      BITSET(x,1)
#define LRWPAN_SET_CAPINFO_PWRSRC(x)       BITSET(x,2)
#define LRWPAN_SET_CAPINFO_RONIDLE(x)      BITSET(x,3)
#define LRWPAN_SET_CAPINFO_SECURITY(x)     BITSET(x,6)
#define LRWPAN_SET_CAPINFO_ALLOCADDR(x)    BITSET(x,7)

#define LRWPAN_CLR_CAPINFO_ALTPAN(x)       BITCLR(x,0)
#define LRWPAN_CLR_CAPINFO_DEVTYPE(x)      BITCLR(x,1)
#define LRWPAN_CLR_CAPINFO_PWRSRC(x)       BITCLR(x,2)
#define LRWPAN_CLR_CAPINFO_RONIDLE(x)      BITCLR(x,3)
#define LRWPAN_CLR_CAPINFO_SECURITY(x)     BITCLR(x,6)
#define LRWPAN_CLR_CAPINFO_ALLOCADDR(x)    BITCLR(x,7)

//BEACON defs
#define LRWPAN_BEACON_SF_ASSOC_PERMIT_MASK (1<<7)
#define LRWPAN_BEACON_SF_PAN_COORD_MASK    (1<<6)
#define LRWPAN_BEACON_SF_BATTLIFE_EXTEN_PERMIT_MASK   (1<<4)

#define LRWPAN_GET_BEACON_SF_ASSOC_PERMIT(x) ( (x) & (LRWPAN_BEACON_SF_ASSOC_PERMIT_MASK))

#define LRWPAN_BEACON_GF_GTS_PERMIT_MASK (0x80)
#define LRWPAN_BEACON_GF_GTS_NUMBER_MASK (0x07)


//Association status，连接状态
#define LRWPAN_ASSOC_STATUS_SUCCESS 0
#define LRWPAN_ASSOC_STATUS_NOROOM  1
#define LRWPAN_ASSOC_STATUS_DENIED  2
#define LRWPAN_ASSOC_STATUS_MASK    3

#define LRWPAN_GET_ASSOC_STATUS(x) ((x)&LRWPAN_ASSOC_STATUS_MASK)//LRWPAN_ASSOC_STATUS_MASK=0x03





#endif
