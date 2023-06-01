#include "tcpserver.h"
#include "lwip/netif.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "netif/etharp.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include <stdio.h>	
#include <string.h>


static err_t tcpecho_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{                                   //��Ӧ�����������ӵĿ��ƿ�   ���յ�������   
  if (p != NULL) 
  {        
	//int a = 666;
	/* ���´���*/
	tcp_recved(tpcb, p->tot_len);     //��ȡ���ݵĿ��ƿ�   �õ��������ݵĳ���   
			
    /* ���ؽ��յ�������*/  
  //tcp_write(tpcb, p->payload, p->tot_len, 1);

	uint8_t send_buf1[]= "���յ��������Ϣ����";
	uint8_t send_buf2[]= "��\n";	
	tcp_write(tpcb, send_buf1, sizeof(send_buf1), 1);
	tcp_write(tpcb, p->payload, p->tot_len, 1);	
	tcp_write(tpcb, send_buf2, sizeof(send_buf2), 1);	
    
  memset(p->payload, 0 , p->tot_len);
  pbuf_free(p);
    
  } 
  else if (err == ERR_OK)    //��⵽�Է������ر�����ʱ��Ҳ�����recv��������ʱpΪ��
  {
    return tcp_close(tpcb);
  }
  return ERR_OK;
}

static err_t tcpecho_accept(void *arg, struct tcp_pcb *newpcb, err_t err) //�������������*tcp_accept_fn���͵�
																																					//�βε����������ͱ���һ��
{     

  tcp_recv(newpcb, tcpecho_recv);    //���յ�����ʱ���ص��û��Լ�д��tcpecho_recv
  return ERR_OK;
}

void TCP_Echo_Init(void)
{
  struct tcp_pcb *server_pcb = NULL;	            		
  
  /* ����һ��TCP���ƿ�  */
  server_pcb = tcp_new();	
	printf("������һ�����ƿ�\r\n");
  
  /* ��TCP���ƿ� */
  tcp_bind(server_pcb, IP_ADDR_ANY, TCP_ECHO_PORT);       
	printf("�Ѿ���һ�����ƿ�\r\n");

  /* �������״̬ */
  server_pcb = tcp_listen(server_pcb);
	printf("�������״̬\r\n");	

  /* �������� ע�ắ��������������ʱ��ע��ĺ������ص� */	
  tcp_accept(server_pcb, tcpecho_accept);  //���������Ӻ󣬻ص��û���д��tcpecho_accept 
																	  //���������*tcp_accept_fn���͵�
}
