/* tftpsercer.c */

#include "tftpserver.h"
#include "tftputils.h"
#include "stmflash.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"

//#define MFS_MODE_READ 0
//#define MFS_MODE_WRITE 1

u32_t FLASH_APP1_ADDR=0x08020000;  	//第一个应用程序起始地址(存放在FLASH)
static u32_t	total_count=0;

u8_t upload_flag=0;

typedef struct
{
  int op;    /* RRQ/WRQ */

  /* last block read */
  char data[TFTP_DATA_PKT_LEN_MAX];
  int  data_len;
  
	ip_addr_t to_ip;
  u16_t remote_port;

  /* next block number */
  u16_t block;

  /* total number of bytes transferred */
  u32_t tot_bytes;
}tftp_connection_args;

/* server pcb to bind to port 69  */
struct udp_pcb *tftp_server_pcb = NULL;
/* tftp_errorcode error strings */
char *tftp_errorcode_string[] = {
                                  "not defined",
                                  "file not found",
                                  "access violation",
                                  "disk full",
                                  "illegal operation",
                                  "unknown transfer id",
                                  "file already exists",
                                  "no such user",
                                };

void server_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *pkt_buf, const ip_addr_t *addr, u16_t port);
//err_t tftp_send_message(struct udp_pcb *upcb, const ip_addr_t *to_ip, unsigned short to_port, char *buf, unsigned short buflen);
//int tftp_construct_error_message(char *buf, tftp_errorcode err);
//int tftp_send_error_message(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, tftp_errorcode err);
//int tftp_send_data_packet(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, unsigned short block, char *buf, int buflen);
//int tftp_send_ack_packet(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, unsigned short block);
//void tftp_cleanup_rd(struct udp_pcb *upcb, tftp_connection_args *args);
//void tftp_cleanup_wr(struct udp_pcb *upcb, tftp_connection_args *args);
//void tftp_send_next_block(struct udp_pcb *upcb, tftp_connection_args *args, const ip_addr_t *to_ip, u16_t to_port);
//void rrq_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
//void wrq_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *pkt_buf, const ip_addr_t *addr, u16_t port);
//void process_tftp_request(struct pbuf *pkt_buf, const ip_addr_t *addr, u16_t port);

/**
  * @brief  sends a TFTP message
  * @param  upcb: pointer on a udp pcb
  * @param  to_ip: pointer on remote IP address
  * @param  to_port: pointer on remote port  
  * @param  buf: pointer on buffer where to create the message  
  * @param  err: error code of type tftp_errorcode
  * @retval error code
  */
err_t tftp_send_message(struct udp_pcb *upcb, const ip_addr_t *to_ip, int to_port, char *buf, int buflen)
{

  err_t err;
  struct pbuf *pkt_buf = NULL; /* Chain of pbuf's to be sent */

  /* PBUF_TRANSPORT - specifies the transport layer */
  pkt_buf = pbuf_alloc(PBUF_TRANSPORT, buflen, PBUF_RAM);

  if (!pkt_buf)      /*if the packet pbuf == NULL exit and EndTransfertransmission */
    return ERR_MEM;

  /* Copy the original data buffer over to the packet buffer's payload */
  memcpy(pkt_buf->payload, buf, buflen);

  /* Sending packet by UDP protocol */
  err = udp_sendto(upcb, pkt_buf, to_ip, to_port);

  /* free the buffer pbuf */
  pbuf_free(pkt_buf);
  //printf("\n\tftp_send_message OK....,buflen = %d",buflen);
  return err;
}

/**
  * @brief construct an error message into buf
  * @param buf: pointer on buffer where to create the message  
  * @param err: error code of type tftp_errorcode
  * @retval 
  */
/* construct an error message into buf using err as the error code */
int tftp_construct_error_message(char *buf, tftp_errorcode err)
{

  int errorlen;
  /* Set the opcode in the 2 first bytes */
  tftp_set_opcode(buf, TFTP_ERROR);
  /* Set the errorcode in the 2 second bytes  */
  tftp_set_errorcode(buf, err);
  /* Set the error message in the last bytes */
  tftp_set_errormsg(buf, tftp_errorcode_string[err]);
  /* Set the length of the error message  */
  errorlen = strlen(tftp_errorcode_string[err]);

  /* return message size */
  return 4 + errorlen + 1;
}

/**
  * @brief Sends a TFTP error message
  * @param  upcb: pointer on a udp pcb
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote port  
  * @param  err: tftp error code
  * @retval error value
  */
/* construct and send an error message back to client */
int tftp_send_error_message(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, tftp_errorcode err)
{
  char buf[512];
  int error_len;

  /* construct error */
  error_len = tftp_construct_error_message(buf, err);
  /* sEndTransfererror  */
  return tftp_send_message(upcb, to, to_port, buf, error_len);
}

/**
  * @brief  Sends TFTP data packet
  * @param  upcb: pointer on a udp pcb
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  block: block number
  * @param  buf: pointer on data buffer
  * @param  buflen: buffer length
  * @retval error value
  */
/* construct and send a data packet */
int tftp_send_data_packet(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, int block,
                          char *buf, int buflen)
{
  //char packet[TFTP_DATA_PKT_LEN_MAX]; /* (512+4) bytes */
  //memset(packet, 'a', TFTP_DATA_PKT_LEN_MAX);
  /* Set the opcode 3 in the 2 first bytes */
  tftp_set_opcode(buf, TFTP_DATA);
  /* Set the block numero in the 2 second bytes */
  tftp_set_block(buf, block);
  /* Set the data message in the n last bytes */
  //tftp_set_data_message(packet, packet, buflen);
  /* Send DATA packet */
  return tftp_send_message(upcb, to, to_port, buf, buflen + 4);
}

/**
  * @brief  Sends TFTP ACK packet
  * @param  upcb: pointer on a udp pcb
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  block: block number
  * @retval error value
  */
int tftp_send_ack_packet(struct udp_pcb *upcb, const ip_addr_t *to, int to_port, int block)
{

  /* create the maximum possible size packet that a TFTP ACK packet can be */
  char packet[TFTP_ACK_PKT_LEN];

  /* define the first two bytes of the packet */
  tftp_set_opcode(packet, TFTP_ACK);

  /* Specify the block number being ACK'd.
   * If we are ACK'ing a DATA pkt then the block number echoes that of the DATA pkt being ACK'd (duh)
   * If we are ACK'ing a WRQ pkt then the block number is always 0
   * RRQ packets are never sent ACK pkts by the server, instead the server sends DATA pkts to the
   * host which are, obviously, used as the "acknowledgement".  This saves from having to sEndTransferboth
   * an ACK packet and a DATA packet for RRQs - see RFC1350 for more info.  */
  tftp_set_block(packet, block);

  return tftp_send_message(upcb, to, to_port, packet, TFTP_ACK_PKT_LEN);
}

/**
  * @brief Cleanup after end of TFTP read operation
  * @param upcb: pointer on udp pcb
  * @param  args: pointer on a structure of type tftp_connection_args
  * @retval None
  */
/* close the file sent, disconnect and close the connection */
void tftp_cleanup_rd(struct udp_pcb *upcb, tftp_connection_args *args)
{
  /* Free the tftp_connection_args structure reserverd for */
  mem_free(args);

  /* Disconnect the udp_pcb*/
  udp_disconnect(upcb);

  /* close the connection */
  udp_remove(upcb);

  udp_recv(tftp_server_pcb, server_recv_callback, NULL);
}

/**
  * @brief Cleanup after end of TFTP write operation
  * @param upcb: pointer on udp pcb
  * @param  args: pointer on a structure of type tftp_connection_args
  * @retval None
  */
/* close the file writen, disconnect and close the connection */
void tftp_cleanup_wr(struct udp_pcb *upcb, tftp_connection_args *args)
{
  /* Free the tftp_connection_args structure reserverd for */
  mem_free(args);

  /* Disconnect the udp_pcb*/
  udp_disconnect(upcb);

  /* close the connection */
  udp_remove(upcb);
	
	udp_recv(tftp_server_pcb, server_recv_callback, NULL);
}

/**
  * @brief  sends next data block during TFTP READ operation
  * @param  upcb: pointer on a udp pcb
  * @param  args: pointer on structure of type tftp_connection_args
  * @param  to_ip: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @retval None
  */
void tftp_send_next_block(struct udp_pcb *upcb, tftp_connection_args *args,
                          const ip_addr_t *to_ip, u16_t to_port)
{
  /* Function to read 512 bytes from the file to sEndTransfer(file_SD), put them
   * in "args->data" and return the number of bytes read */
  int total_block = args->tot_bytes/TFTP_DATA_LEN_MAX;
  total_block +=1;

  if(total_block < 1 || args->block > total_block )
  {
       return;
  }

  args->data_len = TFTP_DATA_LEN_MAX;
  if(total_block == args->block)
  {
	   if(args->tot_bytes%TFTP_DATA_LEN_MAX == 0)
	   {
	       args->data_len = 0;
	   }else
	   {
	       args->data_len = args->tot_bytes - (total_block - 1)*TFTP_DATA_LEN_MAX;
	   }
  }
  
  memset(args->data + TFTP_DATA_PKT_HDR_LEN, ('a'-1) + args->block%26 , args->data_len);
  /*   NOTE: We need to send another data packet even if args->data_len = 0
     The reason for this is as follows:
     1) This function is only ever called if the previous packet payload was
        512 bytes.
     2) If args->data_len = 0 then that means the file being sent is an exact
         multiple of 512 bytes.
     3) RFC1350 specifically states that only a payload of <= 511 can EndTransfera
        transfer.
     4) Therefore, we must sEndTransfer another data message of length 0 to complete
        the transfer.                */

  /* Send the data */
  tftp_send_data_packet(upcb, to_ip, to_port, args->block, args->data, args->data_len);

}

/**
  * @brief  receive callback during tftp read operation (not on standard port 69)
  * @param  arg: pointer on argument passed to callback
  * @param  udp_pcb: pointer on the udp pcb
  * @param  pkt_buf: pointer on the received pbuf
  * @param  addr: pointer on remote IP address
  * @param  port: pointer on remote udp port
  * @retval None
  */
void rrq_recv_callback(void *_args, struct udp_pcb *upcb, struct pbuf *p,
                       const ip_addr_t *addr, u16_t port)
{
  /* Get our connection state  */
  tftp_connection_args *args = (tftp_connection_args *)_args;
  if(port != args->remote_port)
  {
    /* Clean the connection*/
    tftp_cleanup_rd(upcb, args);

    pbuf_free(p);
	  return;
  }
  
  if (tftp_is_correct_ack(p->payload, args->block))
  {
    /* increment block # */
    args->block++;
  }
  else
  {
    /* we did not receive the expected ACK, so do not update block #. 
		This causes the current block to be resent. */
  }

  /* if the last read returned less than the requested number of bytes
   * (i.e. TFTP_DATA_LEN_MAX), then we've sent the whole file and we can quit
   */
  if (args->data_len < TFTP_DATA_LEN_MAX)
  {
    /* Clean the connection*/
    tftp_cleanup_rd(upcb, args);

    pbuf_free(p);
	  return;
  }

  /* if the whole file has not yet been sent then continue  */
  tftp_send_next_block(upcb, args, addr, port);

  pbuf_free(p);

}

/**
  * @brief  processes tftp read operation
  * @param  upcb: pointer on udp pcb 
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  FileName: pointer on filename to be read
  * @retval error code
  */
int tftp_process_read(struct udp_pcb *upcb, const ip_addr_t *to, unsigned short to_port, char* FileName)
{
  tftp_connection_args *args = NULL;

  /* This function is called from a callback,
   * therefore, interrupts are disabled,
   * therefore, we can use regular malloc. */

  args = mem_malloc(sizeof(tftp_connection_args));
  /* If we aren't able to allocate memory for a "tftp_connection_args" */
  if (!args)
  {
    /* unable to allocate memory for tftp args  */
    tftp_send_error_message(upcb, to, to_port, TFTP_ERR_NOTDEFINED);

    /* no need to use tftp_cleanup_rd because no "tftp_connection_args" struct has been malloc'd   */
    tftp_cleanup_rd(upcb, args);

    return 0;
  }

  /* initialize connection structure  */
  args->op = TFTP_RRQ;
  args->to_ip.addr = to->addr;
  args->remote_port = to_port;
  args->block = 1; /* block number starts at 1 (not 0) according to RFC1350  */
  args->tot_bytes = 10*1024*1024;


  /* set callback for receives on this UDP PCB (Protocol Control Block) */
  udp_recv(upcb, rrq_recv_callback, args);

  /* initiate the transaction by sending the first block of data
   * further blocks will be sent when ACKs are received
   *   - the receive callbacks need to get the proper state    */

  tftp_send_next_block(upcb, args, to, to_port);

  return 1;
}

/**
  * @brief  receive callback during tftp write operation (not on standard port 69)
  * @param  arg: pointer on argument passed to callback
  * @param  udp_pcb: pointer on the udp pcb
  * @param  pkt_buf: pointer on the received pbuf
  * @param  addr: pointer on remote IP address
  * @param  port: pointer on remote port
  * @retval None
  */
void wrq_recv_callback(void *_args, struct udp_pcb *upcb, struct pbuf *pkt_buf, const ip_addr_t *addr, u16_t port)
{
  tftp_connection_args *args = (tftp_connection_args *)_args;
  //u16_t next_block = 0;
  uint32_t data_buffer[512] = {0xFFFFFFFF};
  uint16_t count=0;
  
	/* we expect to receive only one pbuf (pbuf size should be 
  configured > max TFTP frame size */
  if (port != args->remote_port || pkt_buf->len != pkt_buf->tot_len)
  {
    tftp_cleanup_wr(upcb, args);
    pbuf_free(pkt_buf);
    return;
  }
	
  //next_block = args->block + 1;
	//printf("%d\r\n", next_block);
	
  /* Does this packet have any valid data to write? */
  if ((pkt_buf->len > TFTP_DATA_PKT_HDR_LEN) && (tftp_extract_block(pkt_buf->payload) == (args->block + 1)))
  {
		
	  /* copy packet payload to data_buffer */
    int len = pbuf_copy_partial(pkt_buf, data_buffer, pkt_buf->len - TFTP_DATA_PKT_HDR_LEN, TFTP_DATA_PKT_HDR_LEN);
		
		//printf("Copy Packet size %d Bytes\r\n", len);
		
	  total_count += pkt_buf->len - TFTP_DATA_PKT_HDR_LEN; 
    
    count = (pkt_buf->len - TFTP_DATA_PKT_HDR_LEN)/4;
	
    if (((pkt_buf->len - TFTP_DATA_PKT_HDR_LEN)%4)!=0) 
    count++;
		
	  /* Write received data in Flash */
    STMFLASH_Write(&FLASH_APP1_ADDR, data_buffer ,count);

    /* update our block number to match the block number just received */
    args->block++;
		
    /* update total bytes  */
    (args->tot_bytes) += (pkt_buf->len - TFTP_DATA_PKT_HDR_LEN);

    /* This is a valid pkt but it has no data.  This would occur if the file being
       written is an exact multiple of 512 bytes.  In this case, the args->block
       value must still be updated, but we can skip everything else.    */
  }
  else if (tftp_extract_block(pkt_buf->payload) == (args->block + 1))
  {
    /* update our block number to match the block number just received  */
    args->block++;
  }

  /* SEndTransferthe appropriate ACK pkt (the block number sent in the ACK pkt echoes
   * the block number of the DATA pkt we just received - see RFC1350)
   * NOTE!: If the DATA pkt we received did not have the appropriate block
   * number, then the args->block (our block number) is never updated and
   * we simply sEndTransfera "duplicate ACK" which has the same block number as the
   * last ACK pkt we sent.  This lets the host know that we are still waiting
   * on block number args->block+1. 
	*/
  tftp_send_ack_packet(upcb, addr, port, args->block);

  /* If the last write returned less than the maximum TFTP data pkt length,
   * then we've received the whole file and so we can quit (this is how TFTP
   * signals the EndTransferof a transfer!)
   */
  if (pkt_buf->len < TFTP_DATA_PKT_LEN_MAX)
  {
	  upload_flag=2;
    tftp_cleanup_wr(upcb, args);
    pbuf_free(pkt_buf);
	  printf("%d bytes\r\n",total_count);
	  printf("%#X\r\n",FLASH_APP1_ADDR);
		printf("Upload Finish\r\n");
  }
  else
  {
    pbuf_free(pkt_buf);
    return;
  }

}

/**
  * @brief  processes tftp write operation
  * @param  upcb: pointer on upd pcb 
  * @param  to: pointer on remote IP address
  * @param  to_port: pointer on remote udp port
  * @param  FileName: pointer on filename to be written 
  * @retval error code
  */
int tftp_process_write(struct udp_pcb *upcb, const ip_addr_t *to, unsigned short to_port, char* FileName)
{
  tftp_connection_args *args = NULL;

  /* This function is called from a callback,
   * therefore interrupts are disabled,
   * therefore we can use regular malloc   */
  args = mem_malloc(sizeof(tftp_connection_args));
  if (!args)
  {
    tftp_send_error_message(upcb, to, to_port, TFTP_ERR_NOTDEFINED);

    tftp_cleanup_wr(upcb, args);

    return 0;
  }

  args->op = TFTP_WRQ;
  args->to_ip.addr = to->addr;
  args->remote_port = to_port;
  /* the block # used as a positive response to a WRQ is _always_ 0!!! (see RFC1350)  */
  args->block = 0;
  args->tot_bytes = 0;

  /* set callback for receives on this UDP PCB (Protocol Control Block) */
  udp_recv(upcb, wrq_recv_callback, args);
	
	total_count =0;
  /* initiate the write transaction by sending the first ack */
  tftp_send_ack_packet(upcb, to, to_port, args->block);
	//printf("block number:%d\r\n", args->block);
	
  return 0;
}

/**
  * @brief  processes the tftp request on port 69
  * @param  pkt_buf: pointer on received pbuf
  * @param  ip_addr: pointer on source IP address
  * @param  port: pointer on source udp port
  * @retval None
  */
/* for each new request (data in p->payload) from addr:port,
 * create a new port to serve the response, and start the response
 * process
 */
void process_tftp_request(struct pbuf *pkt_buf, const ip_addr_t *addr, u16_t port)
{
  tftp_opcode op = tftp_decode_op(pkt_buf->payload);
  char FileName[50] = {0};
  struct udp_pcb *upcb = NULL;
  err_t err;
  
  /* create new UDP PCB structure */
  upcb = udp_new();
  if (!upcb)
  {     
		/* Error creating PCB. Out of Memory  */
    printf("alloc upcb error\n");
    return;
  }
  
  /* connect to remote ip:port, and a random local port will be set too */
  /* NOTE:  This is how TFTP works.  There is a UDP PCB for the standard port
   * 69 which all transactions begin communication on, however, _all_ subsequent
   * transactions for a given "stream" occur on another port!  */
  err = udp_connect(upcb, addr, port);
  if (err != ERR_OK)
  {    
		/* Unable to connect to port   */
    printf("connect upcb error\n");
    return;
  }
  
	/* Read the name of the file asked by the client to be received and writen in the Flash*/
  tftp_extract_filename(FileName, pkt_buf->payload);

  switch (op)
  {

    case TFTP_RRQ:    /* TFTP RRQ (read request)  */
      /* Start the TFTP read mode*/
      printf("\n\rTFTP client start to read file..[%s]..", FileName);
      tftp_process_read(upcb, addr, port, FileName);
      break;

    case TFTP_WRQ:    /* TFTP WRQ (write request) */
	    printf("\n\rTFTP client start to write file..[%s]..\r\n", FileName);
	    upload_flag=1;
      /* Start the TFTP write mode*/
      tftp_process_write(upcb, addr, port, FileName);
			printf("%s\r\n", FileName);
      break;

    default:          /* TFTP unknown request op */
      /* send generic access violation message */
      tftp_send_error_message(upcb, addr, port, TFTP_ERR_ACCESS_VIOLATION);
      /* no need to use tftp_cleanup_wr because no "tftp_connection_args" struct has been malloc'd   */
      udp_remove(upcb);

      break;
  }
}

/**
  * @brief  tftp receive callback on port 69
  * @param  arg: pointer on argument passed to callback (not used here)
  * @param  upcb: pointer on udp pcb
  * @param  pkt_buf: pointer on received pbuf
  * @param  ip_addr: pointer on source IP address
  * @param  port: pointer on source udp port
  * @retval None
  */
/*  the recv_callback function is called when there is a packet received
 *  on the main tftp server port (69)
 */
void server_recv_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                        const ip_addr_t *addr, u16_t port)
{
  /* process new connection request */
  process_tftp_request(p, addr, port);
	//printf("get udp\r\n");
	
	/* free pbuf */
  pbuf_free(p);
}

/**
  * @brief  Initializes the udp pcb for TFTP 
  * @param  None
  * @retval None
  */
#define TFTP_PORT 69 //TFTP协议使用UDP 69号端口
void tftp_server_init(void)
{
  err_t err;

  /* create a new UDP PCB structure */
  tftp_server_pcb = udp_new();
  if (NULL == tftp_server_pcb)
  {  /* Error creating PCB. Out of Memory  */
    return;
  }

  /* Bind this PCB to port 69  */
  err = udp_bind(tftp_server_pcb, IP_ADDR_ANY, TFTP_PORT);
  if (err != ERR_OK)
  {    /* Unable to bind to port  */
    return;
  }

  /* TFTP server start  */
  udp_recv(tftp_server_pcb, server_recv_callback, NULL);
}
