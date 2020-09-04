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

//定义超声波模块引脚
sbit Trig = P1^0;	//发送端
sbit Echo = P1^1;	//接收端

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
/********************延时函数********************/
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
/********************初始化函数******************/
void PCA_init(){
	P_SW1 &= 0xcf;	//(P1.2/ECI, P1.1/CCP0, P1.0/CCP1, P3.7/CCP2)
	CCON = 0;       //初始化PCA控制寄存器
                  //PCA定时器停止
                  //清除溢出中断标志
                  //清除捕获中断标志
  CL = 0;         //清零阵列寄存器
  CH = 0;
  CMOD = 0x01;    //设置PCA时钟源为SYSclk/12,允许溢出中断
  CCAPM0 = 0x10;  //PCA模块0为下降沿触发,关闭中断。
  EA = 1;
}

void Timer0Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;		//定时器时钟12T模式
	TMOD &= 0xF0;		//设置定时器模式
	TL0 = 0x18;			//设置定时初值
	TH0 = 0xFC;			//设置定时初值
	TF0 = 0;				//清除中断标志
	TR0 = 1;				//定时器0开始计时
	
	ET0 = 1;				//使能定时器0中断
	EA = 1;
}
/****************中断处理函数********************/
void PCA_isr() interrupt 7		//PCA中断处理函数
{	
	//捕获成功
	if (CCF0){
		len_t = (CCAP0H<<8)|CCAP0L;  //保存本次的捕获值
		echo_sign = 1;
		CR = 0;												//PCA定时器停止工作
		CCAPM0 &= 0xfe;								//关闭中断
	}
	//超时
	else if (CF){
		time_out_sign = 1;
		CR = 0;												//PCA定时器停止工作
		CCAPM0 &= 0xfe;								//关闭中断
	}
	CCF0 = 0;												//清理中断标志
	CF = 0;
}

void T0_isr() interrupt 1		//T0中断处理函数，每1000ms发射一次超声波
{
	if(--trig_cnt == 0) {
		trig_cnt = 1000;
		trig_sign = 1;
	}
}
/*********************主函数*********************/
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
*函数：trig_len()
*功能：发射超声波，开启PCA计时及中断
*************************************************/
void trig_len() {
	u8 i=8;
	
	//产生八个周期7kHz方波信号
	while(i--){
		Trig = 1;
		delay12us();
		Trig = 0;
		delay12us();
	}
	CL = 0;									//计时器清零
	CH = 0;
	CCF0 = 0;								//清标志(开启前必须清标志)
	CF = 0;
	CCAPM0 |= 0x01;					//开启中断
	CR = 1;                 //PCA定时器开始工作
	
	trig_sign = 0;
}

/*************************************************
*函数：echo_len()
*功能：计算距离，产生数码管字模
*************************************************/
void echo_len() {
	u8 i;
	
	if(echo_sign){
		//计算距离
		len=len_t *0.017 *10;	//保留1位小数
		//生成字模
		dis[3]=font[len/10000];
		dis[4]=font[len/1000%10];
		dis[5]=font[len/100%10];
		dis[6]=font[len/10%10]&0x7f;
		dis[7]=font[len%10];
		//消零
		for(i=3;dis[i]==font[0];i++) dis[i]=0xff;
	}else if(time_out_sign){
		//超时设置
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
*函数：dis_smg()
*功能：数码管显示函数
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