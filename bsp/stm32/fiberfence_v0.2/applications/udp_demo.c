#include "udp_demo.h"

#include "lwip/api.h"
#include "lwip/lwip_sys.h"
//#include "lwip/ip4.h"
#include "lwip/ip_addr.h"
#include "string.h"

#define DRV_DEBUG
#define LOG_TAG             "NET"
#include <drv_log.h>

#include <fal.h>
 
//TCP�ͻ�������
#define UDP_PRIO		6
//�����ջ��С
#define UDP_STK_SIZE	300


typedef struct{
    uint16_t magic;
    uint16_t pkgSize;
    uint32_t pkgId;
		uint32_t fileSize;
} Header_t;

//extern struct rt_mailbox mb;
//extern rt_sem_t sem;
//extern volatile uint32_t CT_flag;
static int update_file_cur_size = 0;
int flash_download(uint8_t *buf, int len, int cur_size);
int flash_erase(int size);
void data_process(uint8_t *buf, int len);

uint8_t udp_demo_recvbuf[2048];	//UDP�������ݻ�����
//UDP������������
const uint8_t *udp_demo_sendbuf="Apollo STM32F4/F7 NETCONN UDP demo send data\r\n";
uint8_t udp_flag = 0x80;							//UDP���ݷ��ͱ�־λ
uint32_t data_len = 0;
struct pbuf *q;

//udp������
void udp_demo_test(void *parameter)
{
	LOG_I("udp_test thread is created.");
	err_t err;
	struct netconn *udpconn;
	struct netbuf  *recvbuf;
//	struct netbuf  *sentbuf;
	ip_addr_t destipaddr, localipaddr;
	
	udpconn = netconn_new(NETCONN_UDP);  //����һ��UDP����
	udpconn->recv_timeout = 10; 
	
	if(udpconn != NULL)  //����UDP���ӳɹ�
	{
		IP4_ADDR(&localipaddr, 192, 168, 1, 250);
		err = netconn_bind(udpconn,&localipaddr,UDP_DEMO_PORT); 
		
		IP4_ADDR(&destipaddr, 192, 168, 1, 102); //����Ŀ��IP��ַ
    netconn_connect(udpconn,&destipaddr,8088); 	//���ӵ�Զ������
		if(err == ERR_OK)//�����
		{
			while(1)
			{
//				if((udp_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA) //������Ҫ����
//				{
//					//rt_kprintf("udp_send_data\n");
//					
//					sentbuf = netbuf_new();
//					if(sentbuf==NULL)
//						rt_kprintf("netbuf_new failed\n");
//					netbuf_alloc(sentbuf,strlen((char *)udp_demo_sendbuf));
//					memcpy(sentbuf->p->payload,(void*)udp_demo_sendbuf,strlen((char*)udp_demo_sendbuf));
//					
//					err = netconn_send(udpconn,sentbuf);  	//��netbuf�е����ݷ��ͳ�ȥ
//					if(err != ERR_OK)
//					{
//						//rt_kprintf("����ʧ��:%d\r\n", err);
//					}
//					//udp_flag &= ~LWIP_SEND_DATA;	//������ݷ��ͱ�־
//					netbuf_delete(sentbuf);      	//ɾ��buf
//				}
//				rt_thread_delay(1);  //��ʱ5ms
				
				netconn_recv(udpconn,&recvbuf); //��������
				if(recvbuf != NULL)          //���յ�����
				{ 
					//rt_kprintf("recive ok\n");
					//OS_ENTER_CRITICAL(); //���ж�
					memset(udp_demo_recvbuf,0,UDP_DEMO_RX_BUFSIZE);  //���ݽ��ջ���������
					for(q=recvbuf->p;q!=NULL;q=q->next)  //����������pbuf����
					{
						//�ж�Ҫ������UDP_DEMO_RX_BUFSIZE�е������Ƿ����UDP_DEMO_RX_BUFSIZE��ʣ��ռ䣬�������
						//�Ļ���ֻ����UDP_DEMO_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
						if(q->len > (UDP_DEMO_RX_BUFSIZE-data_len)) memcpy(udp_demo_recvbuf+data_len,q->payload,(UDP_DEMO_RX_BUFSIZE-data_len));//��������
						else memcpy(udp_demo_recvbuf+data_len,q->payload,q->len);
						data_len += q->len;  	
						if(data_len > UDP_DEMO_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
					}
					//OS_EXIT_CRITICAL();  //���ж�
					
					//rt_kprintf("%s\r\n",udp_demo_recvbuf);  //��ӡ���յ�������
					data_process(udp_demo_recvbuf, data_len);
					data_len=0;  //������ɺ�data_lenҪ���㡣
					netbuf_delete(recvbuf);      //ɾ��buf
					//udp_flag |= 0x80;
				}else rt_thread_delay(5);  //��ʱ5ms
			}
			
			//netbuf_delete(sentbuf);      	//ɾ��buf
			
		}else rt_kprintf("UDP��ʧ��\r\n");
	}else rt_kprintf("UDP���Ӵ���ʧ��\r\n");
}

int flash_download(uint8_t *buf, int len, int cur_size)
{
		char *recv_partition = "download";
		const struct fal_partition * dl_part = RT_NULL;
		
		if ((dl_part = fal_partition_find(recv_partition)) == RT_NULL)
		{
				LOG_E("Firmware download failed! Partition (%s) find error!", recv_partition);
				return -1;
		}
		

		if (fal_partition_write(dl_part, cur_size, (uint8_t *)buf, len) < 0)
		{
				LOG_E("Firmware download failed! Partition (%s) write data error!", dl_part->name);
				return -1;
		}
				
		return 0;
}

int flash_erase(int size)
{
		char *recv_partition = "download";
		const struct fal_partition * dl_part = RT_NULL;
	
		if ((dl_part = fal_partition_find(recv_partition)) == RT_NULL)
		{
				LOG_E("Firmware download failed! Partition (%s) find error!", recv_partition);
				return -1;
		}
		
		if (fal_partition_erase(dl_part, 0, size) < 0)
		{
				LOG_E("Firmware download failed! Partition (%s) erase error!", dl_part->name);
				return -1;
		}
		return 0;
}

void data_process(uint8_t *buf, int len)
{
		int ret=0;
		Header_t *header;
		header = (Header_t *)buf;
		if(header->magic == 0x1234){
			if(header->pkgId == 0){
				rt_kprintf("\nStart to erase download partition!\n");
				ret=flash_erase(header->fileSize);
				if(ret==0){
					rt_kprintf("Erase download partition succesfull!\n");
					rt_kprintf("Start to update firmware!\n\n");
				}
				else
					rt_kprintf("Erase download partition failed!\n\n");
				
				return;
			}
			
			flash_download((uint8_t *)(buf+12), header->pkgSize, update_file_cur_size);
			update_file_cur_size += header->pkgSize;
			rt_kprintf("update_file_cur_size:%d, total filezie:%d\n", update_file_cur_size, header->fileSize);
			if(update_file_cur_size == header->fileSize){
				rt_kprintf("update succesfull, update file size: %d\n", update_file_cur_size);
				update_file_cur_size=0;
			}
		}
		else{
			rt_kprintf("%s\r\n",buf);  //��ӡ���յ�������
		}
		
		
}

void cmd_flash_erase(int argc, char **argv)
{
	flash_erase(262144);
}

static rt_thread_t udpid;
	
void start_update()
{
	udpid = rt_thread_create("udp_test", udp_demo_test, RT_NULL,
													2048, 15, 20);
	if (udpid != RT_NULL)
			rt_thread_startup(udpid);
}

void stop_update()
{
	rt_thread_delete(udpid);
}


#ifdef RT_USING_FINSH
#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(cmd_flash_erase, flash_erase, erase download partition of flash);
MSH_CMD_EXPORT_ALIAS(start_update, start_update,);
MSH_CMD_EXPORT_ALIAS(stop_update, stop_update,);
#endif
