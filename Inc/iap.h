#ifndef __IAP_H__
#define __IAP_H__

#include "sys.h"


void iap_write_appbin(u32 appxaddr,u8 *appbuf, u32 appsize); 
typedef  void (*iapfun)(void);				//����һ���������͵Ĳ���.    
void iap_load_app(u32 appxaddr);			//��ת��APP����ִ��


#endif






































