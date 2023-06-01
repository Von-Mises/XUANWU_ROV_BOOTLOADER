#include "iap.h" 
#include "stdio.h"
#include "string.h"  
#include "stmflash.h"
#include "sys.h"


iapfun jump2app; 
//跳转到应用程序段
//appxaddr:用户代码起始地址.

void iap_load_app(u32 appxaddr)
{ 
//	printf("%X\r\n",*(vu32*)(appxaddr));
//	printf("%X\r\n",*(vu32*)appxaddr);
	if(((*(vu32*)(appxaddr))&0x2FF00000)==0x24000000)	//检查栈顶地址是否合法
	{ 
    printf("Jump to APP\r\n");
		printf("Stack top addr:0x%X\r\n",*(vu32*)(appxaddr));
		printf("Jump addr:0x%X\r\n",*(vu32*)(appxaddr+4));
		jump2app=(iapfun)*(vu32*)(appxaddr+4);		//用户代码区第二个字为程序开始地址(复位地址)		
		MSR_MSP(*(vu32*)appxaddr);					//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		jump2app();									//跳转到APP.
	}
	else
		printf("跳转地址不合格\r\n");
}	


















