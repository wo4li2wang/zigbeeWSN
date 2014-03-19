#ifndef NEIGHBOR_H
#define NEIGHBOR_H

#define LRWPAN_DEVTYPE_COORDINATOR 0x00
#define LRWPAN_DEVTYPE_ROUTER 0x01
#define LRWPAN_DEVTYPE_ENDDEVICE 0x02

#define LRWPAN_DEVRELATION_PARENT 0x00
#define LRWPAN_DEVRELATION_CHILD 0x01
#define LRWPAN_DEVRELATION_SIBING 0x02

//定义邻居表结构体,see page 214,协调器，父设备，本设备的信息在mac层信息库
typedef struct _NAYBORENTRY {
  UINT16 saddr;//16位短地址	
  BYTE laddr[8];//64位长地址
  BYTE capinfo;//节点性能的信息
  UINT16 PANID;
  BYTE DeviceType;
  BYTE Relationship;
  BYTE Depth;
  BYTE BeaconOrder;
  BYTE LQI;//link quality indictor//LQI值是计算所有邻居结点链路的成本?	
  BYTE LogicalChannel;
  union {
    BYTE val;
    struct {
      unsigned used: 1;  //true if used//设置为1则表示使用该指针
      unsigned RxOnWhenIdle: 1;
      unsigned PermitJoining: 1;
      unsigned PotentialParent: 1;
    }bits;
  }options;
}NAYBORENTRY;

#define NTENTRIES (LRWPAN_MAX_CHILDREN_PER_PARENT)//17,holds the neighbor table entries.
extern NAYBORENTRY mac_nbr_tbl[NTENTRIES];//定义外部变量MAC邻居表

UINT16 ntGetCskip(BYTE depth);
SADDR ntGetMaxSADDR(SADDR router_saddr,BYTE depth);//获得最大的短地址
SADDR ntNewAddressMapEntry(BYTE *laddr, SADDR saddr);//新的地址映射入口
BOOL ntFindAddressBySADDR(SADDR saddr, BYTE *index);//通过短地址寻址,找到邻居表中的索引值
BOOL ntFindAddressByLADDR(LADDR *ptr, BYTE *index);//通过长地址寻址,找到邻居表中的索引值
void ntSaddrToLaddr(BYTE *laddr, SADDR saddr);//通过短地址，寻找长地址
NAYBORENTRY *ntFindBySADDR (UINT16 saddr);//定义短地址寻址指针
NAYBORENTRY *ntFindByLADDR (LADDR *ptr);//定义长地址寻址指针
BOOL ntDelNbrBySADDR (UINT16 saddr);//通过短地址，删除该设备的邻居表信息和地址信息
BOOL ntDelNbrByLADDR (LADDR *ptr);//通过长地址，删除该设备的邻居表信息和地址信息
void ntInitTable(void);//初始化邻居表
SADDR ntFindNewDst(SADDR dstSADDR);//寻找新的目标地址
#ifdef LRWPAN_FFD
void ntInitAddressAssignment(void);//初始化地址分配
SADDR ntAddNeighbor(BYTE *ptr, BYTE capinfo);//添加邻居
#endif

#endif

