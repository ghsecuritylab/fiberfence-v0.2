#include "drv_key.h"
#include "delay.h"
/*********************************************************************************	 
*	   
* �����������룬ʵ�ְ�����ɨ��
*				  
**********************************************************************************/

//����������
//���ذ���ֵ
//mode:0,��֧��������;1,֧��������;
//0��û���κΰ�������
//ע��˺�������Ӧ���ȼ�,KEY0>KEY1>KEY2>....
rt_uint8_t KEY_Scan(rt_uint8_t mode)
{
    static u8 key_up=1;     //�����ɿ���־
    if(mode==1)key_up=1;    //֧������
    if(key_up&&(KEY0==0||KEY1==0||KEY2==0||KEY3==0||KEY4==0||KEY5==0))
    {
        //delay_ms(10);
				rt_thread_delay(100);
        key_up=0;
        if(KEY0==0)       return KEY0_PRES;
        else if(KEY1==0)  return KEY1_PRES;
        else if(KEY2==0)  return KEY2_PRES;
				else if(KEY3==0)  return KEY3_PRES;
				else if(KEY4==0)  return KEY4_PRES;
				else if(KEY5==0)  return KEY5_PRES;          
    }else if(KEY0==1&&KEY1==1&&KEY2==1&&KEY3==1&&KEY4==1&&KEY5==1)key_up=1;
    return 0;   //�ް�������
}
