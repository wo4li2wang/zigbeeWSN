/*	
//
// Refer to Standard for Architecture and Digital Data Communication for
//	 ZigBee-based Industrial Control Systems
//-------------------------------------------------------------------

#ifndef SECURITY_H
#define SECURITY_H
*/
/*****************************************************
for security join to add this parameter by weimin
***************************************************/
#define NS_DEFAULT_SEC_ID     "cqupt_zigbee_1.0_sec_device_0001"
#define NSMIB_BASE_OBJID_MIBHDR         (10)
#define	NSMIB_BASE_OBJID_KEY_MAN        (8001)
#define	NSMIB_BASE_OBJID_SEC_MEASURE    (8002)
#define	NSMIB_BASE_OBJID_PTCL_MAN       (8003)
#define	NSMIB_BASE_OBJID_AUTHEN         (8004)
#define 	NSMIB_BASE_OBJID_MACPIB	 (8005)	
#define NSMIB_BASE_OBJID_ACCTRL_OBJ     (8500)


typedef enum Keytype {
	Initialkey,
	Transferkey,
	Communicat_key
} Keytype;
typedef struct {
	UINT16	obj_id;
	UINT16	revision;
	BYTE	initial_key[16];
	BYTE	transfer_key[16];
	BYTE	communicat_key[16];
	UINT16	key_off;	
	UINT16	key_life;
	UINT16	key_len;
	Keytype	current_key_type;
	UINT16	attack_time;
	UINT16	attack_type;
} KeyMan, *PKeyMan;

typedef struct {
	UINT16	obj_id;
	UINT16	revision;
	BYTE	sec_id[32];
	sTime	stamp;
	UINT8	state;
} AuthenObj, *PAuthenObj;

void GetSecID(BYTE secid[]);

/***********************************************************
add by weimin for MAC security PIB
***********************************************************/
typedef struct _ACL_Descrip_Set{
LADDR	ACLExtendedAddress;
UINT16	ACLShortAddress;
UINT16	ACLPANId;
UINT8	ACLSecurityMateriallength;
BYTE	*ACLSecurityMaterial;
UINT8	ACLSecuritySuit;
}ACL_Descrip_Set;
typedef struct _MAC_SECURITY_PIB {
ACL_Descrip_Set	macACLEntryDescriptorSet;
UINT8	macACLEntryDescriporSetSize;
BOOL 	macDefaultSecurity;
UINT8	macDefautSecurityMaterialLength;
BYTE	*macDefautSecurityMaterial;
UINT8	macDefautSecuritySuite;
UINT8	macSecurityMode;
}MAC_SECURITY_PIB;



typedef struct {
	UINT16       obj_id;
	UINT16       revision;
	UINT8        sec_mode;
	UINT8        authen_mode;
	UINT8        encypt_mode;
} SecMeasure, *PSecMeasure;

typedef struct {
	UINT16       obj_id;
	UINT16       local_app;
	UINT16       local_obj;
	UINT16       rmt_app;
	UINT16       rmt_obj;
	UINT8        srv_op;
	UINT8        srv_role;
	UINT32       rmt_ip;
	sTime     send_time_offset;
	UINT8        right;
	UINT8        group;
	UINT16       psw;
} ACCtrlObj, *PACCtrlObj;

typedef struct {
	UINT16       obj_id;
	UINT16       revision;
	UINT16       time_limit;
	UINT8        run_mode;
	UINT8        link_mode;
	UINT8        authen_mode;
	UINT8        Ptcl_type;
} SecPtclMan, *PSecPtclMan;




void  KeyMan_Init(void);
void  AuthenObj_Init(void);
void  MAC_SECURITY_PIB_Init(void);
void Authen_Reply(void);
void Authen_POS_rsp(void);
void Authen_NEG_rsp(void);

void SecMeasure_Init(void) ;
void SecPtclMan_Init(void) ;
void AcctrlObj_Init(void) ;

void AuthenObj_Read(void);
void SecMeasure_Read(void);
void KeyMan_Read(void);
void SecPtclMan_Read(void);
void MAC_SECURITY_PIB_READ(void);
void KeyMan_WRITE(BYTE *load);
void SecMeasure_WRITE(BYTE *load);
void SecPtclMan_WRITE(BYTE *load);
void AuthenObj_WRITE(BYTE *load);
void MAC_SECURITY_PIB_WRITE(BYTE *load);



