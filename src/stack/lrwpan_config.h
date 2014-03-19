//用户配置文件
#ifndef LRWPAN_CONFIG_H
#define LRWPAN_CONFIG_H

#define LRWPAN_VERSION  "0.2.2"

//存储区动态分配的函数大小
#ifndef LRWPAN_HEAPSIZE
#define LRWPAN_HEAPSIZE  1024
#endif

//add by weimin for 安全性
#define SECURITY_GLOBALS


//现在仅支持2.4GHz
#define LRWPAN_DEFAULT_FREQUENCY PHY_FREQ_2405M
#define LRWPAN_SYMBOLS_PER_SECOND   62500     //一个16微秒的符号,(1000000)/16=62500


#define LRWPAN_DEFAULT_START_CHANNEL  20   //2.4GHz.(2450MHz)有效信道是11到26

//0表示信道不可用,0xFFFFFFFF允许所有的16个信道
//2.4GHz 信道在11-26位上
#define LRWPAN_DEFAULT_CHANNEL_MASK 0xFFFFFFFF

//这个网络的PANID
#define LRWPAN_USE_STATIC_PANID      //如果这个被定义,默认的PANID总会被使用
#define LRWPAN_DEFAULT_PANID 0x1347
#define LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION 3
//#define LRWPAN_DEFAULT_CHANNEL_SCAN_DURATION 7

//MAC层接收(RX)包最大数量的缓存
#define LRWPAN_MAX_MAC_RX_PKTS   4

//等待被转发到其他节点的网络层的数据包的最大数量
//只为FFDs定义的
#define LRWPAN_MAX_NWK_RX_PKTS   4


//最大数量的待解决的间接数据包
//协调器使用
#define LRWPAN_MAX_INDIRECT_RX_PKTS 2


/*
如果LRWPAN_ENABLE_SLOW_TIMER已经定义,那么HAL层也
将通过使用SLOWTICKS_PER_SECOND的值配置一个周期中断定时器
,hal层将会在每个中断发生时调用usr的函数usrSlowTimerInt().

如果slow timer生效,那么EVB开关将会以这个比率进行采样

看halStack.c 那些定时器资源会用到这个
它与定时器资源使用macTimer是不一样的

你若是不想使用它,就让它禁用

*/



//如果你希望ASSOC_RESPONSE, ASSOC_REQUEST与802.15.4兼容不要注释掉这一句
#define IEEE_802_COMPLY



//如果没有设备用这句
//其它设置地址的方法
//你若是重新定义了一位,其它位也要重新定义
#ifndef aExtendedAddress_B7
#define aExtendedAddress_B7 0x00
#define aExtendedAddress_B6 0x00
#define aExtendedAddress_B5 0x00
#define aExtendedAddress_B4 0x00
#define aExtendedAddress_B3 0x00
#define aExtendedAddress_B2 0x00
#define aExtendedAddress_B1 0x00
#define aExtendedAddress_B0 0x00
#endif

//uncomment this if you want to force association to a particular target
//#ifdef LRWPAN_FORCE_ASSOCIATION_TARGET
//set the following to the long address of the parent to associate with
//if using forced association.
//if you use forced association, then you must NOT define IEEE_802_COMPLY
//as forced association depends upon our special associate request/response
#define parent_addr_B0 0x00
#define parent_addr_B1 0x00
#define parent_addr_B2 0x00
#define parent_addr_B3 0x00
#define parent_addr_B4 0x00
#define parent_addr_B5 0x00
#define parent_addr_B6 0x00
#define parent_addr_B7 0x00



//MAC Capability Info

//if either router or coordinator, then one of these must be defined
//#define LRWPAN_COORDINATOR
//#define LRWPAN_ROUTER




#if (defined (LRWPAN_COORDINATOR) || defined (LRWPAN_ROUTER) )
#define LRWPAN_FFD
#define LRWPAN_ROUTING_CAPABLE
#endif
#if !defined (LRWPAN_FFD)
#define LRWPAN_RFD
#endif

//define this if ACMAIN POWERED
#define LRWPAN_ACMAIN_POWERED
//define this if Receiver on when idle
#define LRWPAN_RCVR_ON_WHEN_IDLE
//define this if capable of RX/TX secure frames
//#define LRWPAN_SECURITY_CAPABLE



//comment this if you want the phy to call the EVBPOLL function
//do this if you want to poll EVB inputs during the stack idle
//time
//#define LRWPAN_PHY_CALL_EVBPOLL

#define LRWPAN_ZIGBEE_PROTOCOL_ID   0
#define LRWPAN_ZIGBEE_PROTOCOL_VER  0
#define LRWPAN_STACK_PROFILE  0         //indicates this is a closed network.
#define LRWPAN_APP_PROFILE    0xFFFF    //filter data packets by this profile number
#define LRWPAN_APP_CLUSTER    0x2A    //default cluster, random value for debugging

//define this if you want the beacon payload to comply with the Zigbee standard
#define LRWPAN_ZIGBEE_BEACON_COMPLY

//Network parameters



//this is a magic number exchanged with nodes wishing to join our
//network. If they do not match this number, then they are rejected.
//Sent in beacon payload
#define LRWPAN_NWK_MAGICNUM_B0 0x0AA
#define LRWPAN_NWK_MAGICNUM_B1 0x055
#define LRWPAN_NWK_MAGICNUM_B2 0x0C3
#define LRWPAN_NWK_MAGICNUM_B3 0x03C



/*
These numbers determine affect the size of the neighbor
table, and the maximum number of nodes in the network,
and how short addresses are assigned to nodes.

*/
#define LRWPAN_MAX_DEPTH                   5
#define LRWPAN_MAX_ROUTERS_PER_PARENT      4
//these are total children, includes routers
#define LRWPAN_MAX_CHILDREN_PER_PARENT    17
#define LRWPAN_MAX_NON_ROUTER_CHILDREN    (LRWPAN_MAX_CHILDREN_PER_PARENT-LRWPAN_MAX_ROUTERS_PER_PARENT)//17－4



//if using Indirect addressing, then this number determines the
//maximum size of the address table map used by the coordinator
//that matches long addresses with short addresses.
//You should set this value to the maximum number of RFDs that
//use indirect addressing. The value below is just chosen for testing.
//Its minimum value must be the maximum number of neighbors (RFDs+Routers+1), as this
//is also used in the neighbor table construction.
#ifdef LRWPAN_COORDINATOR
#define LRWPAN_MAX_ADDRESS_MAP_ENTRIES   (LRWPAN_MAX_CHILDREN_PER_PARENT*2)//17*2=34
#endif


#ifndef LRWPAN_MAX_ADDRESS_MAP_ENTRIES
//this is the minimum value for this, minimum value used by routers
#ifdef LRWPAN_ROUTER
#define LRWPAN_MAX_ADDRESS_MAP_ENTRIES (LRWPAN_MAX_CHILDREN_PER_PARENT+1)//17+1=18
#endif
#ifdef LRWPAN_RFD
#define LRWPAN_MAX_ADDRESS_MAP_ENTRIES 1
#endif
#endif

#ifdef LRWPAN_FFD
#if (LRWPAN_MAX_ADDRESS_MAP_ENTRIES < (LRWPAN_MAX_CHILDREN_PER_PARENT+1))
#error "In lrwpan_config.h, LRWPAN_MAX_ADDRESS_MAP_ENTRIES too small!"
#endif
#endif



//these precalculated based upon MAX_DEPTH, MAX_ROUTERS, MAX_CHILDREN
//Coord at depth 0, only endpoints are at depth MAX_DEPTH
//LRWPAN_CSKIP_(MAX_DEPTH) must be a value of 0.
//this hardcoding supports a max depth of 10, should be PLENTY
//Use the spreadsheet supplied with the distribution to calculate these
#define LRWPAN_CSKIP_1     1446
#define LRWPAN_CSKIP_2      358
#define LRWPAN_CSKIP_3       86
#define LRWPAN_CSKIP_4       18
#define LRWPAN_CSKIP_5       0
#define LRWPAN_CSKIP_6       0
#define LRWPAN_CSKIP_7       0
#define LRWPAN_CSKIP_8       0
#define LRWPAN_CSKIP_9       0
#define LRWPAN_CSKIP_10      0


#define LRWPAN_NWK_MAX_RADIUS  LRWPAN_MAX_DEPTH*2//5*2=10

//Binding
//if the following is defined, then the EVB binding functions use
//the binding resolution functions defined in stack/staticbind.c
//#define LRWPAN_USE_DEMO_STATIC_BIND

//Define this if you want to use the binding functions
//in pcbind.c/h that store the binding table on a PC client
//using the bindingdemo application
//#define LRWPAN_USE_PC_BIND
//PC_BIND_CACHE_SIZE only needed if USE_PC_BIND is defined
//number of bindings cached by the PC bind code
#define LRWPAN_PC_BIND_CACHE_SIZE  4

//these are defaults, can be changed by user
#define LRWPAN_APS_ACK_WAIT_DURATION 200   //in milliseconds, for depth=1
#define LRWPAN_NWK_JOIN_WAIT_DURATION 200  //in milliseconds



#define LRWPAN_APS_MAX_FRAME_RETRIES 3  //for acknowledge frames
#define LRWPAN_MAC_MAX_FRAME_RETRIES 3  //for MAC ack requests

//maximum number of endpoints, controls size of endpoint data structure
//in APS.h
#define LRWPAN_MAX_ENDPOINTS    6

//data for node descriptor, not currently used
#define LRWPAN_MAX_USER_PAYLOAD   93      //currently 93 bytes
#define LRWPAN_MANUFACTURER_CODE  0x0000  //assigned by Zigbee Alliance



//unsupported at this time
//#define LRWPAN_ALT_COORDINATOR
//#define LRWPAN_SECURITY_ENABLED

//HAL Stuff

#define LRWPAN_ENABLE_SLOW_TIMER

#define SLOWTICKS_PER_SECOND 10
#define LRWPAN_DEFAULT_BAUDRATE 57600
//#define LRWPAN_DEFAULT_BAUDRATE 9600   //9600
#define LRWPAN_ASYNC_RX_BUFSIZE   32

#define LRWPAN_ASYNC_INTIO

#if (defined(LRWPAN_USE_PC_BIND)&&defined(LRWPAN_COORDINATOR)&&!defined(LRWPAN_ASYNC_INTIO))
//ASYNC RX interrupt IO *must* be used with coordinator if using PC Binding application
//so that serial input from the PC client is not missed.
#define LRWPAN_ASYNC_INTIO

#endif


#endif
