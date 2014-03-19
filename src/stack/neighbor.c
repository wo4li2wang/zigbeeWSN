#include "compiler.h"
#include "lrwpan_config.h"         //user configurations
#include "lrwpan_common_types.h"   //types common acrosss most files
#include "ieee_lrwpan_defs.h"
#include "memalloc.h"
#include "hal.h"
#include "halStack.h"
#include "phy.h"
#include "mac.h"
#include "neighbor.h"

NAYBORENTRY mac_nbr_tbl[NTENTRIES];//17

/*函数功能:通过扩展地址在邻居表中寻找对应的索引值，如存在返回TRUE(索引值为*index),
	   否则返回FALSE*/
BOOL ntFindAddressByLADDR(LADDR *ptr, BYTE *index)
{
  BYTE j,i;
  BYTE *src,*dst;
  for (j=0;j<NTENTRIES;j++) {
    if (!mac_nbr_tbl[j].options.bits.used) //表示未使用，则继续下个j++循环
      continue;
    src = &ptr->bytes[0];
    dst = &mac_nbr_tbl[j].laddr[0];
    for (i=0;i<8;i++, src++, dst++) {
      if (*src != *dst)
        break;
    }
    if (i== 8) {//找到一致的地址，位置信息赋给INDEX
      *index = j;
      break;
    }
  }
  if (j != NTENTRIES)
    return(TRUE);//返回TURE则表示地址列表里有一致的地址
  else
    return(FALSE);
}

/*函数功能:通过短地址在邻居表中寻找对应的索引值，如存在返回TRUE(索引值为*index),否则返回FALSE*/
BOOL ntFindAddressBySADDR(SADDR saddr, BYTE *index)
{
  BYTE j;
  for (j=0;j<NTENTRIES;j++) {
    if (mac_nbr_tbl[j].options.bits.used) {
      if (mac_nbr_tbl[j].saddr == saddr) {
         *index = j;
         break;
      }
    }
  }
  if (j != NTENTRIES)
    return(TRUE);
  else
    return(FALSE);
}



/*函数功能:通过短地址，寻找长地址*/
void ntSaddrToLaddr(BYTE *laddr, SADDR saddr)
{
  BYTE i,j;
  for (i=0;i<NTENTRIES;i++) {
    if (mac_nbr_tbl[i].options.bits.used) {
      if (mac_nbr_tbl[i].saddr == saddr) {
        for (j=0;j<8;j++) {
          *laddr = mac_nbr_tbl[i].laddr[j];
          laddr++;
        }
      }
    }
  }
}

/*函数功能:检查在邻居表中是否存在匹配的短地址和扩展地址，
           如存在返回TRUE(索引值为*index),否则返回FALSE*/
BOOL ntCheckAddressMapEntry(BYTE *laddr, SADDR saddr, BYTE *index)
{
  BYTE j,i;
  BYTE *src,*dst;
  for (j=0;j<NTENTRIES;j++) {
    if (!mac_nbr_tbl[j].options.bits.used)
      continue;
    if (mac_nbr_tbl[j].saddr != saddr)
      continue;
    src = laddr;
    dst = &mac_nbr_tbl[j].laddr[0];
    for (i=0;i<8;i++) {
      if (*src != *dst)
        break;
      src++;
      dst++;
    }
     if (i == 8) {// we have a match
       *index = j;
       return(TRUE);
    }
  }
  return(FALSE);
}


///////由于邻居表和地址表有变动，所以以后更改
/*函数功能:向邻居表里添入新的长地址和短地址,首先该对地址是否已经存在，
	   若在返回mac_addr_tbl[j].saddr;再看是否有空间，若无返回LRWPAN_BCAST_SADDR，
	   存在空间，将扩展地址和短地址存入mac_addr_tbl[j]，返回 0   */
SADDR ntNewAddressMapEntry(BYTE *laddr, SADDR saddr)
{
  BYTE j;
  if (ntCheckAddressMapEntry(laddr, saddr, &j)) {//entry is already in the table.
    return(mac_nbr_tbl[j].saddr);
  }
  //now find free entry in address map table
  for (j=0;j<NTENTRIES;j++) {
    if ((!mac_nbr_tbl[j].options.bits.used)||(mac_nbr_tbl[j].saddr == LRWPAN_BCAST_SADDR))
      break;
  }
  if(j==NTENTRIES)
    return(LRWPAN_BCAST_SADDR);//error, no room
  halUtilMemCopy(&mac_nbr_tbl[j].laddr[0], laddr, 8);
  mac_nbr_tbl[j].saddr = saddr;
  return(0);
}

/*
LRWPAN_MAX_DEPTH				=	5
LRWPAN_MAX_ROUTERS_PER_PARENT			=	4
LRWPAN_MAX_CHILDREN_PER_PARENT			=	17
LRWPAN_MAX_NON_ROUTER_CHILDREN			=	13
*/
/*函数功能:根据上面参数，由depth计算得Cskip(d), See Page 216 公式*/
UINT16 ntGetCskip(BYTE depth) {//根据depth获取LRWPAN_CSKIP_X ,用于计算短地址
  switch(depth){
  case 1: return(LRWPAN_CSKIP_1);//1446
  case 2: return(LRWPAN_CSKIP_2);//358
  case 3: return(LRWPAN_CSKIP_3);//86
  case 4: return(LRWPAN_CSKIP_4);//18
  case 5: return(LRWPAN_CSKIP_5);//0
  case 6: return(LRWPAN_CSKIP_6);//0
  case 7: return(LRWPAN_CSKIP_7);//0
  case 8: return(LRWPAN_CSKIP_8);//0
  case 9: return(LRWPAN_CSKIP_9);//0
  case 10: return(LRWPAN_CSKIP_10);//0
  }
  return(0);
}

/*函数功能:获取该路由器从设备最大短地址，应用于树型网络路由算法*/
SADDR ntGetMaxSADDR(SADDR router_saddr,BYTE depth)//compute the maximum SADDR given the router_saddr and depth
{
  return(router_saddr + (ntGetCskip(depth)*(LRWPAN_MAX_ROUTERS_PER_PARENT)) + LRWPAN_MAX_NON_ROUTER_CHILDREN);
}

/*函数功能:在mac_nbr_tbl中，查找与短地址相匹配的邻居入口信息*/
NAYBORENTRY *ntFindBySADDR (UINT16 saddr)
{
  NAYBORENTRY *nt_ptr;
  BYTE j;
  nt_ptr = &mac_nbr_tbl[0];
  for (j=0;j<NTENTRIES;j++,nt_ptr++) {
    if(nt_ptr->options.bits.used&&nt_ptr->saddr==saddr)
      return(nt_ptr);
  }
  return(NULL);
}

/*函数功能:在mac_nbr_tbl中，查找与扩展地址相匹配的邻居入口信息*/
NAYBORENTRY *ntFindByLADDR (LADDR *ptr){
  NAYBORENTRY *nt_ptr;
  BYTE j,i;
  nt_ptr = &mac_nbr_tbl[0];
  for (j=0;j<NTENTRIES;j++,nt_ptr++) {
    if (!nt_ptr->options.bits.used) continue;
    for (i=0;i<8;i++) {
      if (nt_ptr->laddr[i] != ptr->bytes[i])
        break;
    }
    if (i == 8)
      return(nt_ptr);
  }
  return(NULL);
}




/*函数功能:通过短地址，删除该设备的邻居表信息和地址信息*/
BOOL ntDelNbrBySADDR (UINT16 deviceaddr)
{
  NAYBORENTRY *nt_ptr;
  BYTE i;
  nt_ptr = &mac_nbr_tbl[0];
  for (i=0;i<NTENTRIES;i++,nt_ptr++){
    if ( nt_ptr->options.bits.used&&nt_ptr->saddr==deviceaddr){
      nt_ptr->options.bits.used = 0;
      nt_ptr->saddr = LRWPAN_BCAST_SADDR;
      break;
    }
  }
  if (i==NTENTRIES)
    return (FALSE);
  else
    return (TRUE);
}

/*函数功能:通过长地址，删除该设备的邻居表信息和地址信息，add now*/
BOOL ntDelNbrByLADDR(LADDR * ptr)
{
  NAYBORENTRY *nt_ptr;
  BYTE i,j;
  nt_ptr = &mac_nbr_tbl[0];
  for (i=0;i<NTENTRIES;i++,nt_ptr++) {
    if (!nt_ptr->options.bits.used) continue;
    for (j=0;j<8;j++) {
      if (nt_ptr->laddr[j] != ptr->bytes[j])
        break;
    }
    if (j==8) {
      nt_ptr->saddr = LRWPAN_BCAST_SADDR;
      nt_ptr->options.bits.used = 0;
      break;
    }
  }	
  if (i==NTENTRIES)
    return (FALSE);
  else
    return (TRUE);
}

/*函数功能:初始化邻居表，包括初始化地址表*/
void ntInitTable(void)
{
  NAYBORENTRY *nt_ptr;
  BYTE j;
  nt_ptr = &mac_nbr_tbl[0];
  for (j=0;j<NTENTRIES;j++,nt_ptr++) {
    nt_ptr->options.val = 0;
    nt_ptr->saddr = LRWPAN_BCAST_SADDR;
  }
}
#ifdef LRWPAN_FFD
/*函数功能:地址分配初始化mac_pib.ChildRFDs，mac_pib.ChildRouters，mac_pib.nextChildRFD，mac_pib.nextChildRouter
            一般是在加入网络以后，知道自己的网络深度，进行调用*/
void ntInitAddressAssignment(void)
{
  mac_pib.ChildRFDs = 0;
  mac_pib.ChildRouters = 0;
  mac_pib.nextChildRFD=macGetShortAddr()+1+ntGetCskip(mac_pib.macDepth+1)*(LRWPAN_MAX_ROUTERS_PER_PARENT);
  mac_pib.nextChildRouter = macGetShortAddr() + 1;
}

/*函数功能:在邻居表中找一个空闲单元，若无，返回广播地址(LRWPAN_BCAST_SADDR)，
          否则添加邻居，根据参数进行初始化，分配一个新的短地址*/
SADDR ntAddNeighbor(BYTE *ptr, BYTE capinfo)
{
  NAYBORENTRY *nt_ptr;
  BYTE j;
  BYTE *tmpptr;

  //First, find free entry in neighbor table
  nt_ptr = &mac_nbr_tbl[0];
  for (j=0;j<NTENTRIES;j++,nt_ptr++) {
    if (!nt_ptr->options.bits.used) {
      nt_ptr->options.bits.used = 1;
      nt_ptr->capinfo = capinfo;
      //nt_ptr->PANID = a_mac_rx_data.SrcPANID;
      nt_ptr->PANID = mac_pib.macPANID;
      nt_ptr->Relationship = LRWPAN_DEVRELATION_CHILD;
      nt_ptr->Depth = mac_pib.macDepth+1;
      nt_ptr->BeaconOrder = 15;
      nt_ptr->LQI = a_mac_rx_data.LQI;
      nt_ptr->LogicalChannel = phy_pib.phyCurrentChannel;
      if (LRWPAN_GET_CAPINFO_DEVTYPE(capinfo)) {//As a FFD
        nt_ptr->DeviceType = LRWPAN_DEVTYPE_ROUTER;
        if (LRWPAN_GET_CAPINFO_ALLOCADDR(capinfo)) {//分配短地址
          nt_ptr->saddr = mac_pib.nextChildRouter;
          mac_pib.nextChildRouter += ntGetCskip(mac_pib.macDepth+1);
        } else
          nt_ptr->saddr = LRWPAN_SADDR_USE_LADDR;
        mac_pib.ChildRouters++;
      } else {//As a RFD
        nt_ptr->DeviceType = LRWPAN_DEVTYPE_ENDDEVICE;
        if (LRWPAN_GET_CAPINFO_ALLOCADDR(capinfo)) {//分配短地址
     nt_ptr->saddr = mac_pib.nextChildRFD+*ptr-1;//mini
          //mini//nt_ptr->saddr = mac_pib.nextChildRFD;
          //minimac_pib.nextChildRFD++;
        } else
          nt_ptr->saddr = LRWPAN_SADDR_USE_LADDR;
        mac_pib.ChildRFDs++;
      }
      break;
    }
  }
  if (j== NTENTRIES)
    return(LRWPAN_BCAST_SADDR);//error, no room

  //now copy long addr
  tmpptr = &nt_ptr->laddr[0];
  for(j=0;j<8;j++) {
    *tmpptr = *ptr;
    tmpptr++;
    ptr++;
  }
  return(nt_ptr->saddr);
}
#endif

/*函数功能:根据参数dstSADDR，来确定下一跳目的地址，完成路由功能，仅应用于树型网络。
	   若dstSADDR是本设备的短地址，出错，返回广播地址(LRWPAN_BCAST_SADDR)
	   若dstSADDR是0，表示是协调器短地址，若dstSADDR不在该子网内，返回协调器短地址
	   若dstSADDR在，则返回下一跳目的地址，若无法路由则返回广播地址(LRWPAN_BCAST_SADDR)*/
SADDR ntFindNewDst(SADDR dstSADDR)
{
  SADDR tmpSADDR;
  NAYBORENTRY *nt_ptr;
  BYTE j;
  if (dstSADDR == macGetShortAddr()) {//trying to send to myself, this is an error
    return(0xFFFF);
  }
  //if destination is coordinator, has to go to our parent
  if (dstSADDR == 0)
    return(mac_pib.macCoordShortAddress);
  // See if this destination is within my routing range
  // if not, then have to send it to my parent
#ifndef LRWPAN_COORDINATOR
  //do not check this for coordinator, as all nodes in range of coordinator.
  tmpSADDR = ntGetMaxSADDR(macGetShortAddr(),mac_pib.macDepth+1);
  if (!((dstSADDR > macGetShortAddr()) && (dstSADDR <= tmpSADDR))) { //not in my range, must go to parent.
    return(mac_pib.macCoordShortAddress);
  }
#endif
  //goes to one of my children, check out each one.	
  nt_ptr = &mac_nbr_tbl[0];
  for (j=0;j<NTENTRIES;j++,nt_ptr++) {
    if (!nt_ptr->options.bits.used)
      continue;
    if (LRWPAN_GET_CAPINFO_DEVTYPE(nt_ptr->capinfo)) {
      //router. check its range, the range is mac_pib.depth+2 because we need
      //the depth of the my child's child (grandchild).
      tmpSADDR = ntGetMaxSADDR(nt_ptr->saddr,mac_pib.macDepth+2);
      if ((dstSADDR >= nt_ptr->saddr) && (dstSADDR <= tmpSADDR)) {
        //either for my child router or one of its children.
        return(nt_ptr->saddr);
      }
    }else {
      //if for a non-router child, return
      if (dstSADDR == nt_ptr->saddr) return(nt_ptr->saddr);
    }
  }
  return(0xFFFF);//if get here, then packet is undeliverable
}


