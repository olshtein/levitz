#include "embsys_sms_protocol.h"
#include "tebahpla.h"
#include "string.h"
#define SMS_DELIVER_OCTET (0x04)
#define SMS_PROBE_OCTET (0x02)
#define SMS_SUBMIT_OCTET (0x11)
#define SMS_PROBE_ACK_OCTET (0x12)
#define SMS_SUBMIT_ACK_OCTET (0x07)
#define ADDRESS_LENGTH_OCTET (0X09)
#define TYPE_OF_ADDRESS_OCTET (0Xc9)
//#define Address-Length
#define NULL_ (0)
// return fail if couldn't fill  buf with a decimal semi-octets id
#define RETURN_FAIL_IF_fillDecimalSemiOctetsWithTrailing_FAILED if(buf==NULL_) return FAIL;
// the # of chars used for display the n size id
#define DecimalSemiOctetsWithTrailing_DIGITS_NUM(n) (n%2+((unsigned)n/2))

/*
 * return the length of the id
 */
unsigned int getLen(char * id){
	unsigned int i;
	for( i=1;i<=ID_MAX_LENGTH &&((*id)!=NULL_);i++){
		id++;
	}
	return i-1;
}
/*
 * fill the buf with a decimal semi-octets, with a trailing F for padding of the id number.
 * return buf (after the number was adedd to it.
 */
char * fillDecimalSemiOctetsWithTrailing(char *buf,unsigned int idLen,char * id){
	unsigned int i;
	for(i=0;i<idLen;i+=2) {
		if(id[i]<'0'||id[i]>'9') return NULL_;//chk legal input
		*buf=(char)((id[i]-'0')<<4);
		if(i+1<idLen){
			if(id[i+1]<'0'||id[i+1]>'9') return NULL_;//chk legal input
			*buf++|=(id[i+1]-'0');
		}
		else *buf++|=0xFF;
	}
	return buf;
}
/*
  Description:
    Fill the buffer with an SMS_PROBE / SMS_PROBE_ACK message fields
  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
    is_ack - a value other then NULL_ indicates this is an SMS_PROBE_ACK
             and the appropriate fields in the struct are applicable
    len - the actual used size of the supllied buffer
  Return value:
 */
EMBSYS_STATUS embsys_fill_probe1(char *buf, SMS_PROBE *msg_fields, char is_ack, unsigned *len){
	if(is_ack==NULL_) *buf++=SMS_PROBE_OCTET;
	else  *buf++=SMS_PROBE_ACK_OCTET;

	//fill device address len
	unsigned int idLen=getLen(msg_fields->device_id);
	*buf++=(char)idLen;

	// fill device num
	*buf++=TYPE_OF_ADDRESS_OCTET;
	buf=fillDecimalSemiOctetsWithTrailing(buf,idLen,msg_fields->device_id);
	RETURN_FAIL_IF_fillDecimalSemiOctetsWithTrailing_FAILED;

	(*len)=3+DecimalSemiOctetsWithTrailing_DIGITS_NUM(idLen);

	if(is_ack==NULL_)return SUCCESS;

	//fill probe ack:

	// fill time stamp
	buf=fillDecimalSemiOctetsWithTrailing(buf,TIMESTAMP_MAX_LENGTH,msg_fields->timestamp);
	RETURN_FAIL_IF_fillDecimalSemiOctetsWithTrailing_FAILED;

	//fill sender address len
	unsigned int senderIdLen=getLen(msg_fields->sender_id);
	*buf++=(char)senderIdLen;

	// fill sender num
	*buf++=TYPE_OF_ADDRESS_OCTET;
	buf=fillDecimalSemiOctetsWithTrailing(buf,senderIdLen,msg_fields->sender_id);
	RETURN_FAIL_IF_fillDecimalSemiOctetsWithTrailing_FAILED;

	*len+=DecimalSemiOctetsWithTrailing_DIGITS_NUM(TIMESTAMP_MAX_LENGTH)+2+ DecimalSemiOctetsWithTrailing_DIGITS_NUM(senderIdLen);
	return SUCCESS;
}
#define SMSC_DEFAULT_VALIDITY_PERIOD (0x3b)
/*
 * fill buf with data content
 * change 7-bit data (septets) into octets
 * return the size of the data (after changed into octets)
 *
 */
unsigned fillBuffwithData(char* buf,unsigned data_length,char* data){
	char current;
	char next;
	unsigned brrow=1; // num of bits the current char take from the next
	unsigned size_after_convert=0;
	for(unsigned i=0;i<data_length;i++){
		if(brrow>7){
			brrow=1;
		}
		else {
			current=get7bits(*data++);
			if(i<data_length-1)next=get7bits(*data);
			else next=NULL_;
			*buf=(char)((current>>(brrow-1)) | (next<<(8-brrow)));
			size_after_convert++;
			brrow++;
		}
	}

	return size_after_convert;
}
/*
  Description:
    Fill the buffer with an SMS_SUBMIT message fields
  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
    len - the actual used size of the supllied buffer
  Return value:
 */
EMBSYS_STATUS embsys_fill_submit1(char *buf, SMS_SUBMIT *msg_fields, unsigned *len){
	*buf++=SMS_SUBMIT_OCTET;

	//fill device address len
	unsigned int idLen=getLen(msg_fields->device_id);
	*buf++=(char)idLen;

	// fill device num
	*buf++=TYPE_OF_ADDRESS_OCTET;
	buf=fillDecimalSemiOctetsWithTrailing(buf,idLen,msg_fields->device_id);
	RETURN_FAIL_IF_fillDecimalSemiOctetsWithTrailing_FAILED;

	// fill TP-Message-Reference.
	*buf++=msg_fields->msg_reference;

	//fill recipient address len
	unsigned int recipientLen=getLen(msg_fields->recipient_id);
	*buf++=(char)recipientLen;

	// fill recipient  num
	*buf++=TYPE_OF_ADDRESS_OCTET;
	buf=fillDecimalSemiOctetsWithTrailing(buf,recipientLen,msg_fields->recipient_id);
	RETURN_FAIL_IF_fillDecimalSemiOctetsWithTrailing_FAILED;

	// fill TP-PID. Protocol identifier
	*buf++=0x0;
	//fill TP-DCS. Data coding scheme.
	*buf++=0x0;
	//fill TP-Validity-Period.
	*buf++=SMSC_DEFAULT_VALIDITY_PERIOD;
	//fill    TP-User-Data-Length.
	*buf++=(char)msg_fields->data_length;
	// fill    TP-User-Data
	*len=fillBuffwithData(buf,msg_fields->data_length,msg_fields->data)+9
			+DecimalSemiOctetsWithTrailing_DIGITS_NUM(idLen)+
			DecimalSemiOctetsWithTrailing_DIGITS_NUM(recipientLen);
	return SUCCESS;
}
/*
 * probe the buf (a decimal semi-octets, with a trailing F for padding of the id number).
 * return  return buf pointing to after the id number
 */
char* parseDecimalSemiOctetsWithTrailing(char *buf,unsigned int idLen,char *id){
	unsigned int i;
	for(i=0;i<idLen;i+=2) {
		*id++=(char)('0'+((*buf)>>4));
		if(i<idLen-1) *id++=(char)('0'+(*buf++ & 0x0f));
		else *id++=NULL_;
	}
	return buf;
}

/*
  Description:
    Parse the buffer as an SMS_SUBMIT_ACK message
  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
  Return value:
 */
EMBSYS_STATUS embsys_parse_submit_ack1(char *buf, SMS_SUBMIT_ACK *msg_fields){
	if (*buf++!= SMS_SUBMIT_ACK_OCTET) return FAIL;
	// parse msg_reference
	msg_fields->msg_reference=*buf++;
	unsigned recipientLen=(unsigned)*buf++;
	if(recipientLen==0 ||recipientLen>ID_MAX_LENGTH) return FAIL; // non messages or sender len illegal

	if(*buf++!=TYPE_OF_ADDRESS_OCTET)return FAIL;
	parseDecimalSemiOctetsWithTrailing(buf,recipientLen,msg_fields->recipient_id);
	return SUCCESS;
}


/*
 * parse buf (octets) to data (7-bit data- (septets))
 *
 *
 */
void parseBuffwithData(char* buf,unsigned data_length,char* data){
	char prev=NULL_;
	unsigned brrow=0; // num of bits the current char take from the prev
	for(unsigned i=0;i<data_length;i++){

		*data=(char)((*buf&(0xff>>(brrow+1)))<<brrow);
		*data++|=prev>>(8-brrow);
		if(brrow<7){
		brrow++;
		prev=*buf++;
		}
		else {
			brrow=1;
			prev=*buf;
		}
	}

}

/*
  Description:
    Parse the buffer as an SMS_DELIVER message
  Arguments:
    buf - a pointer to a buffer to fill / parse
    msg_fields - a pointer to a struct to get / put the fields of the message
  Return value:
 */
EMBSYS_STATUS embsys_parse_deliver1(char *buf, SMS_DELIVER *msg_fields){
	if (*buf++!= SMS_DELIVER_OCTET) return FAIL;
	unsigned senderLen=(unsigned)*buf++;
	if(senderLen==0 ||senderLen>ID_MAX_LENGTH) return FAIL; // non messages or sender len illegal
	//parse sender num
	if(*buf++!=TYPE_OF_ADDRESS_OCTET)return FAIL;
	parseDecimalSemiOctetsWithTrailing(buf,senderLen,msg_fields->sender_id);
	//parse TP-PID. Protocol identifier.
	if(*buf++!=0x0)return FAIL;
	//parse TP-DCS. Data coding scheme.
	if(*buf++!=0x0)return FAIL;
	// parse timestamp
	buf=parseDecimalSemiOctetsWithTrailing(buf,TIMESTAMP_MAX_LENGTH,msg_fields->timestamp);
	// parse data len
	msg_fields->data_length=(unsigned)*buf++;
	// parse data
	parseBuffwithData(buf,msg_fields->data_length,msg_fields->data);

	return SUCCESS;

}
