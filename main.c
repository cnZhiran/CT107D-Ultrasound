#include <STC15F2K60S2.H>
#include <intrins.h>

#ifndef u8
#define u8 unsigned char
#endif

#ifndef u16
#define u16 unsigned int
#endif

#ifndef u32
#define u32 unsigned long
#endif

//���峬����ģ������
sbit Trig = P1^0;	//���Ͷ�
sbit Echo = P1^1;	//���ն�

u8 code font[10]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
u8 code y4=0x80,y5=0xa0,y6=0xc0,y7=0xe0;
u8 dis[8]={0xc7,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

bit trig_sign=0,echo_sign=1,time_out_sign=0;
u16 trig_cnt=1000;
u16 len,len_t;

void PCA_init();
void trig_len();
void echo_len();

void dis_smg();
/********************��ʱ����********************/
void delay100us()		//@12.000MHz
{
	unsigned char i, j;

	i = 2;
	j = 39;
	do
	{
		while (--j);
	} while (--i);
}
void delay12us()		//@12.000MHz
{
	unsigned char i;

	_nop_();
	_nop_();
	i = 33;
	while (--i);
}
/********************��ʼ������******************/
void PCA_init(){
	P_SW1 &= 0xcf;	//(P1.2/ECI, P1.1/CCP0, P1.0/CCP1, P3.7/CCP2)
	CCON = 0;       //��ʼ��PCA���ƼĴ���
                  //PCA��ʱ��ֹͣ
                  //�������жϱ�־
                  //��������жϱ�־
  CL = 0;         //�������мĴ���
  CH = 0;
  CMOD = 0x01;    //����PCAʱ��ԴΪSYSclk/12,��������ж�
  CCAPM0 = 0x10;  //PCAģ��0Ϊ�½��ش���,�ر��жϡ�
  EA = 1;
}

void Timer0Init(void)		//1����@12.000MHz
{
	AUXR &= 0x7F;		//��ʱ��ʱ��12Tģʽ
	TMOD &= 0xF0;		//���ö�ʱ��ģʽ
	TL0 = 0x18;			//���ö�ʱ��ֵ
	TH0 = 0xFC;			//���ö�ʱ��ֵ
	TF0 = 0;				//����жϱ�־
	TR0 = 1;				//��ʱ��0��ʼ��ʱ
	
	ET0 = 1;				//ʹ�ܶ�ʱ��0�ж�
	EA = 1;
}
/****************�жϴ�����********************/
void PCA_isr() interrupt 7		//PCA�жϴ�����
{	
	//����ɹ�
	if (CCF0){
		len_t = (CCAP0H<<8)|CCAP0L;  //���汾�εĲ���ֵ
		echo_sign = 1;
		CR = 0;												//PCA��ʱ��ֹͣ����
		CCAPM0 &= 0xfe;								//�ر��ж�
	}
	//��ʱ
	else if (CF){
		time_out_sign = 1;
		CR = 0;												//PCA��ʱ��ֹͣ����
		CCAPM0 &= 0xfe;								//�ر��ж�
	}
	CCF0 = 0;												//�����жϱ�־
	CF = 0;
}

void T0_isr() interrupt 1		//T0�жϴ�������ÿ1000ms����һ�γ�����
{
	if(--trig_cnt == 0) {
		trig_cnt = 1000;
		trig_sign = 1;
	}
}
/*********************������*********************/
void main() {
	Timer0Init();
	PCA_init();
	while(1) {
		dis_smg();
		if(trig_sign) trig_len();
		if(echo_sign | time_out_sign) echo_len();
	}
}

/*************************************************
*������trig_len()
*���ܣ����䳬����������PCA��ʱ���ж�
*************************************************/
void trig_len() {
	u8 i=8;
	
	//�����˸�����7kHz�����ź�
	while(i--){
		Trig = 1;
		delay12us();
		Trig = 0;
		delay12us();
	}
	CL = 0;									//��ʱ������
	CH = 0;
	CCF0 = 0;								//���־(����ǰ�������־)
	CF = 0;
	CCAPM0 |= 0x01;					//�����ж�
	CR = 1;                 //PCA��ʱ����ʼ����
	
	trig_sign = 0;
}

/*************************************************
*������echo_len()
*���ܣ�������룬�����������ģ
*************************************************/
void echo_len() {
	u8 i;
	
	if(echo_sign){
		//�������
		len=len_t *0.017 *10;	//����1λС��
		//������ģ
		dis[3]=font[len/10000];
		dis[4]=font[len/1000%10];
		dis[5]=font[len/100%10];
		dis[6]=font[len/10%10]&0x7f;
		dis[7]=font[len%10];
		//����
		for(i=3;dis[i]==font[0];i++) dis[i]=0xff;
	}else if(time_out_sign){
		//��ʱ����
		len=9999;
		dis[3]=font[9];
		dis[4]=font[9];
		dis[5]=font[9];
		dis[6]=font[9]&0x7f;
		dis[7]=font[9];
	}
	time_out_sign = 0;
	echo_sign = 0;
}

/*************************************************
*������dis_smg()
*���ܣ��������ʾ����
*************************************************/
void dis_smg() {
	u8 i;

	for(i=0;i<8;i++){
		P2&=0x1f;
		P0=1<<i;
		P2|=y6;
		P2&=0x1f;
		P0=dis[i];
		P2|=y7;
		delay100us();
		P0=0xff;
	}
}