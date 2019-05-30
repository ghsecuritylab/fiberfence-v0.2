/*
 * �����嵥��UDP����ˣ����պʹ�����λ�����͵�����
 */
#include <rtthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dfs_posix.h>
#include "displayInfo.h"

#define MAX_DATA_LEN 512
#define BASE_PACK_LEN 10

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

extern float get_cpu(void);
extern int set_dac(rt_uint16_t value, rt_uint16_t chip_id);

typedef struct {
	uint16_t magic[4];                      //У����
	uint16_t device_id;                     //�豸ID
	uint16_t cmd;                           //�������ͣ��ο��궨��
	uint16_t cpu_usage_major;               //cpuʹ������������
	uint16_t cpu_usage_minor;               //cpuʹ����С������
	uint16_t alarm_count_a;                 //����a��������
	uint16_t alarm_count_b;                 //����b��������
	uint16_t optic_power_a;                 //����a�⹦��
	uint16_t optic_power_b;                 //����b�⹦��
	uint16_t dac_gain_a;                    //dac�������a
	uint16_t dac_gain_b;                    //dac�������b
	uint16_t adc_sample_frq;                //adc����Ƶ��
	uint16_t alarm_threshold_a;             //a����������ֵ
	uint16_t alarm_threshold_b;             //b����������ֵ
	uint16_t optic_power_threshold;         //�⹦����ֵ
	uint16_t alarm_time_interval;           //����ʱ����
	uint16_t alarm_sensitive;               //����������
	uint16_t enable_send_data;              //���ݷ���
	uint16_t enable_defence;                //��������
} Cmd_Data_t;

struct Cmd_Data{
    uint16_t magic;
    uint16_t cmd;
    float    r1;
    uint16_t r2;
    uint16_t len;
    char data[MAX_DATA_LEN];
};

void response(int sock, int cmd, struct sockaddr *client_addr, int addr_len)
{
	struct Cmd_Data r;
	r.magic = 0x1234;
	r.cmd = cmd;
	r.r1 = 0.25;
	r.r2 = 1024;
	sendto(sock, (char *)&r, BASE_PACK_LEN+2, 0, client_addr, addr_len);
}

/*********************************************************
* ��������cmd_process
* 
* ��  �ܣ�������յ�������
*********************************************************/
void cmd_process(int sock, struct Cmd_Data *cd, struct sockaddr *client_addr, int addr_len)
{
	char str[100];
	sprintf(str, "param1:%d  param2:%f\n", cd->r2, cd->r1);
	rt_kprintf("%s", str);
	switch(cd->cmd)
	{
		case CMD_EXIT:
			rt_kprintf("CMD:CMD_EXIT\n");
			response(sock, CMD_RESPONSE_OK, client_addr, addr_len);
			break;
		//��ѯCPUʹ����
		case CMD_CPU_USAGE:
		{
			rt_kprintf("CMD:CMD_CPU_USAGE\n");
			cd->r1 = get_cpu();
			cd->r2 = 1024;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//��ѯ�⹦��
		case CMD_OPTIC_POWER:
		{
			rt_kprintf("CMD:CMD_OPTIC_POWER\n");
			cd->r1 = info.item5.param1;
			cd->r2 = info.item6.param1;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//��ѯ������Ŀ
		case CMD_ALARM_COUNT:
		{
			rt_kprintf("CMD:CMD_ALARM_COUNT\n");
			cd->r1 = info.item7.param1;
			cd->r2 = info.item8.param1;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//���÷���A�ź�����
		case CMD_DAC_GAIN_A:
		{
			rt_kprintf("CMD:CMD_DAC_GAIN_A\n");
			set_dac(cd->r2, 2);
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//���÷���B�ź�����
		case CMD_DAC_GAIN_B:
		{
			rt_kprintf("CMD:CMD_DAC_GAIN_B\n");
			set_dac(cd->r2, 3);
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//����AD������
		case CMD_ADC_SAMPLE_FRQ:
			rt_kprintf("CMD:CMD_ADC_SAMPLE_FRQ\n");
			response(sock, CMD_RESPONSE_OK, client_addr, addr_len);
			break;
		//���÷���A������ֵ
		case CMD_ALARM_THRESHOLD_A:
		{
			rt_kprintf("CMD:CMD_ALARM_THRESHOLD_A\n");
			info.item1.param1=cd->r2;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//���÷���B������ֵ
		case CMD_ALARM_THRESHOLD_B:
		{
			rt_kprintf("CMD:CMD_ALARM_THRESHOLD_B\n");
			info.item2.param1=cd->r2;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//���ù⹦�ʱ�����ֵ
		case CMD_POWER_THRESHOLD:
		{
			rt_kprintf("CMD:CMD_POWER_THRESHOLD\n");
			info.item3.param1=cd->r2;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
		//���ñ���ʱ����
		case CMD_ALARM_INTERVAL:
		{
			rt_kprintf("CMD:CMD_POWER_THRESHOLD\n");
			info.item4.param1=cd->r2;
			sendto(sock, (char *)cd, BASE_PACK_LEN+2, 0, client_addr, addr_len);
			break;
		}
			
		//����������Ӧ
		default:
			rt_kprintf("CMD:CMD_RESPONSE_UNKNOW\n");
			response(sock, CMD_RESPONSE_UNKNOW, client_addr, addr_len);
			break;
	}
}

/*********************************************************
* ��������udpserv
* 
* ��  �ܣ�udp���񣬽�����λ�����͵��������Ӧ��λ��
*********************************************************/
void udpserv(void* paramemter)
{
    int sock;
    int bytes_read;
    char *recv_data;
		struct Cmd_Data *cmd_data_t;
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

				cmd_data_t=(struct Cmd_Data *)recv_data;
				
				/* У��֡ͷ */
				if(cmd_data_t->magic!=0x1234)
				{
						rt_kprintf("magic error\n");
						continue;
				}
				/* ������յ����� */
				cmd_process(sock, cmd_data_t, (struct sockaddr *) &client_addr, addr_len);
    }
		
		close(sock);
		rt_free(recv_data);

    return;
}
