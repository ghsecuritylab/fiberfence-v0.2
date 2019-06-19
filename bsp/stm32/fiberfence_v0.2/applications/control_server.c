#include <rtthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dfs_posix.h>
#include "displayInfo.h"

#define DRV_DEBUG
#define LOG_TAG             "UDP_SERVER"
#include <drv_log.h>

//��ѯ����
#define CMD_EXIT        0x100f   
#define CMD_CPU_USAGE   0x1001   //��ѯcpuʹ����
#define CMD_OPTIC_POWER 0x1002   //��ѯ�⹦��
#define CMD_ALARM_COUNT 0x1003   //��ѯ��������
#define CMD_ALL         0x1000   //��ѯ���в���

//��������
#define CMD_DAC_GAIN_A        0x2001  //����dac�������a
#define CMD_DAC_GAIN_B        0x2002  //����dac�������b
#define CMD_ADC_SAMPLE_FRQ    0x2003  //����adc����Ƶ��
#define CMD_ALARM_THRESHOLD_A 0x2004  //����a����������ֵ
#define CMD_ALARM_THRESHOLD_B 0x2005  //����b����������ֵ
#define CMD_POWER_THRESHOLD   0x2006  //���ù⹦����ֵ
#define CMD_ALARM_INTERVAL    0x2007  //���ñ���ʱ����
#define CMD_ALARM_SENSITIVE   0x2008  //���ñ���������
#define CMD_ENABLE_SENDDATA   0x2009  //�����Ƿ���ԭʼ����
#define CMD_ENABLE_DEFENCE    0x200a  //���ò�������

//��Ӧ����
#define CMD_RESPONSE_OK        0x0000    //������Ӧ�ɹ�����ѯ���߿��Ƴɹ���
#define CMD_RESPONSE_UNKNOW    0x1111    //���յ�δ֪����
#define CMD_RESPONSE_MAGIC_ERR 0x2222    //֡У�����

#define DISABLE 0;
#define ENABLE  1;

extern void cpu_usage_get(rt_uint8_t *major, rt_uint8_t *minor);
extern int set_dac(rt_uint16_t value, rt_uint16_t chip_id);

struct Cmd_Data{
    uint16_t magic;
    uint16_t cmd;
    uint16_t p1;
    uint16_t p2;
};

static void response(int sock, int cmd, struct sockaddr *client_addr, int addr_len)
{
	struct Cmd_Data r;
	r.magic = 0x1234;
	r.cmd = cmd;
	r.p1 = 0xffff;
	r.p2 = 0xffff;
	sendto(sock, (char *)&r, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
}

/*********************************************************
* ��������cmd_process
* 
* ��  �ܣ�������յ�������
*********************************************************/
static void cmd_process(int sock, struct Cmd_Data *cd, struct sockaddr *client_addr, int addr_len)
{
	switch(cd->cmd)
	{
		//��ѯCPUʹ����
		case CMD_CPU_USAGE:
		{
			uint8_t major, minor;
			cpu_usage_get(&major, &minor);
			cd->p1 = major;
			cd->p2 = minor;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Query cpu usage: %d.%d%%", major, minor);
			break;
		}
		//��ѯ�⹦��
		case CMD_OPTIC_POWER:
		{
			cd->p1 = info.item5.param1;
			cd->p2 = info.item6.param1;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Query optic power: %d, %d", info.item5.param1, info.item6.param1);
			break;
		}
		//��ѯ������Ŀ
		case CMD_ALARM_COUNT:
		{
			cd->p1 = info.item7.param1;
			cd->p2 = info.item8.param1;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Query alarm count: %d, %d", info.item7.param1,info.item8.param1);
			break;
		}
		//���÷���A�ź�����
		case CMD_DAC_GAIN_A:
		{
			set_dac(cd->p1, 2);
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Set dac gain(A): %d", cd->p1);
			break;
		}
		//���÷���B�ź�����
		case CMD_DAC_GAIN_B:
		{
			set_dac(cd->p1, 3);
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Set dac gain(B): %d", cd->p1);
			break;
		}
		//����AD������
		case CMD_ADC_SAMPLE_FRQ:
			response(sock, CMD_RESPONSE_OK, client_addr, addr_len);
			LOG_I("Set adc sample frequency: %d", cd->p1);
			break;
		//���÷���A������ֵ
		case CMD_ALARM_THRESHOLD_A:
		{
			info.item1.param1=cd->p1;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Set alarm threshold(A): %d", cd->p1);
			break;
		}
		//���÷���B������ֵ
		case CMD_ALARM_THRESHOLD_B:
		{
			info.item2.param1=cd->p1;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Set alarm threshold(B): %d", cd->p1);
			break;
		}
		//���ù⹦�ʱ�����ֵ
		case CMD_POWER_THRESHOLD:
		{
			info.item3.param1=cd->p1;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Set optic power threshold: %d", cd->p1);
			break;
		}
		//���ñ���ʱ����
		case CMD_ALARM_INTERVAL:
		{
			info.item4.param1=cd->p1;
			sendto(sock, (char *)cd, sizeof(struct Cmd_Data), 0, client_addr, addr_len);
			LOG_I("Set alarm interval: %d", cd->p1);
			break;
		}
			
		//δ֪����
		default:
			response(sock, CMD_RESPONSE_UNKNOW, client_addr, addr_len);
			LOG_I("unknow CMD");
			break;
	}
}

static void udp_server_entry(void* paramemter)
{
	int sock;
    int bytes_read;
    char *recv_data;
		struct Cmd_Data *cmd_data;
    rt_uint32_t addr_len;
    struct sockaddr_in server_addr, client_addr;

    /* ��������õ����ݻ��� */
    recv_data = rt_malloc(1024);
    if (recv_data == RT_NULL)
    {
        /* �����ڴ�ʧ�ܣ����� */
        rt_kprintf("No memory\n");
        return;
    }

    /* ����һ��socket��������SOCK_DGRAM��UDP���� */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        rt_kprintf("Socket error\n");

        /* �ͷŽ����õ����ݻ��� */
        rt_free(recv_data);
        return;
    }

    /* ��ʼ������˵�ַ */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* ��socket������˵�ַ */
    if (bind(sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr))
            == -1)
    {
        /* �󶨵�ַʧ�� */
        rt_kprintf("Bind error\n");

        /* �ͷŽ����õ����ݻ��� */
        rt_free(recv_data);
        return;
    }

    addr_len = sizeof(struct sockaddr);
    rt_kprintf("UDPServer Waiting for client on port 5000...\n");
	
	while (1)
    {
        /* ��sock����ȡ���1024�ֽ����� */
        bytes_read = recvfrom(sock, recv_data, 1024, 0,
                              (struct sockaddr *) &client_addr, &addr_len);			
			
				if(bytes_read <=0 )
				{
						rt_kprintf("recive error\n");
						continue;
				}

				cmd_data=(struct Cmd_Data *)recv_data;
				
				/* У��֡ͷ */
				if(cmd_data->magic!=0x1234)
				{
						rt_kprintf("magic error\n");
						continue;
				}
				/* ������յ����� */
				cmd_process(sock, cmd_data, (struct sockaddr *) &client_addr, addr_len);
    }
		
		close(sock);
		rt_free(recv_data);
		
		return;
}

static int start_udp_server()
{
	rt_thread_t uid;
	uid = rt_thread_create("udp_control_server", udp_server_entry, NULL, 1024, 12, 20);
	if(uid!=NULL){
		rt_thread_startup(uid);
	}
	//rt_kprintf("Udp control Server Waiting for control client on port 5000...\n");
	//rt_kprintf("Udp controld\n");
	//rt_kprintf("Udp controld\n");
	return 0;
}
INIT_APP_EXPORT(start_udp_server);
//#ifdef RT_USING_FINSH
//#include <finsh.h>
//MSH_CMD_EXPORT_ALIAS(start_udp_server, start_udp_server, start_udp_server);
//#endif



