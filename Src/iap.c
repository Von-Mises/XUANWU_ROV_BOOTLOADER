#include "iap.h" 
#include "stdio.h"
#include "string.h"  
#include "stmflash.h"
#include "sys.h"


iapfun jump2app; 
//��ת��Ӧ�ó����
//appxaddr:�û�������ʼ��ַ.

void iap_load_app(u32 appxaddr)
{ 
//	printf("%X\r\n",*(vu32*)(appxaddr));
//	printf("%X\r\n",*(vu32*)appxaddr);
	if(((*(vu32*)(appxaddr))&0x2FF00000)==0x24000000)	//���ջ����ַ�Ƿ�Ϸ�
	{ 
    printf("Jump to APP\r\n");
		printf("Stack top addr:0x%X\r\n",*(vu32*)(appxaddr));
		printf("Jump addr:0x%X\r\n",*(vu32*)(appxaddr+4));
		jump2app=(iapfun)*(vu32*)(appxaddr+4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
		MSR_MSP(*(vu32*)appxaddr);					//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		jump2app();									//��ת��APP.
	}
	else
		printf("��ת��ַ���ϸ�\r\n");
}	


















