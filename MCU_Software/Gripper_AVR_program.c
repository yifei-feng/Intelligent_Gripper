//----------��Ӳ������˵������------------------------------------------------------------
//�����������19200����ԴDC12V��ID��0��1
//��Դ�ߺͿ����ߵĵ�ƽ��������
//��Դ2����RS485����
//UART0����MCU��PC��ͨ�ţ�������19200
//UART1����MCU�Ͷ����ͨ�ţ�������19200

//----------��ͷ�ļ�����------------------------------------------------------------
#include <iom128v.h>
#include <macros.h>
#include <string.h>

//----------���궨�塿��------------------------------------------------------------
#define  uchar unsigned char
#define  uint  unsigned int

#define  mclk   8000000 //ʱ��Ƶ��8.0MHz

//----------����������------------------------------------------------------------
const uchar Ratio1=1;
const uchar Ratio2=2;
const uchar Force=3;


const uchar no0release[]={0xff,0xff,0x00,0x05,0x03,0x20,0xff,0x07,0xd1};//0�Ŷ���ɿ���˳ʱ�룬����100%��
const uchar no0stop[]={0xff,0xff,0x00,0x05,0x03,0x20,0x00,0x00,0xd7}; //0�Ŷ��ֹͣ����ʱ�룬����0%��
const uchar no1release[]={0xff,0xff,0x01,0x05,0x03,0x20,0xff,0x07,0xd0};//1�Ŷ���ɿ���˳ʱ�룬����100%��
const uchar no1stop[]={0xff,0xff,0x01,0x05,0x03,0x20,0x00,0x00,0xd6}; //1�Ŷ��ֹͣ����ʱ�룬����0%��

//----------��ȫ�ֱ������塿��------------------------------------------------------------

uchar gripper_mood=0;//�г���ģʽ��0-δָ����1-һ�㹤��ģʽ��2-��������ģʽ

uchar cage0_state,cage1_state;//�����г���������λ���

/*���±������ڽ�����λ��ָ�������*/
uchar uart0_rdata_byte;//UART0ÿ�ν��յ��ĵ��ֽ���Ϣ
uchar uart0_r_instr_chk=0;//uart0���յ����ַ�����ͷ��x�ַ��ĸ���
uchar uart0_instr[5];//�洢PC����MCU��ָ�������xx����������λָ����룬���һλΪNULL
uchar uart0_instr_flag=0;//UART0�жϽ��յ���ͷ�ͳ��ȷ���Ҫ���instrʱ��Ϊ1

//�г������ⲿ��ײ�������жϣ���������
uchar ext_collision_alert_allow_int0=0;//�ϲ�
uchar ext_collision_alert_allow_int1=0;//�²�
uchar ext_collision_alert_allow_int4=0;//ָ��

uchar force_high8;//Ӧ��Ƭ����ֵ��8λaa��long�� 00 aa bb cc
uchar force_judge;//�������EEPROM�е�Ӧ��Ƭ����Чֵ�߰�λ�������жϼн����Ƿ񵽴�Ҫ��
unsigned long force_ulong;//��ǰӦ��Ƭ�������ֵ��ռ��4 byte��ʵ����Чֵ3 byte��


//--------------��������ʱ������--------------------------------------------------------------------

//��ʱ����������ΪҪ��ʱ�ĺ�����
void delay(uint ms)
{
    uint i,j;
	for(i=0;i<ms;i++)
	{
	 for(j=0;j<1141;j++);
    }
}


//----------��UART0����غ�������---------------------------------------------------------------

/*UART0�Ĵ��ڳ�ʼ������*/
void uart0_init(uint baud)
{
   UCSR0B=0x00; 
   UCSR0A=0x00; 		   //���ƼĴ�������
   UCSR0C=(0<<UPM00)|(3<<UCSZ00); //ѡ��UCSRC���첽ģʽ����ֹУ�飬1λֹͣλ��8λ����λ                       
   
   baud=mclk/16/baud-1;    //���������Ϊ65K
   UBRR0L=baud; 					     	  
   UBRR0H=baud>>8; 		   //���ò�����

   UCSR0B|=(1<<TXEN0);   //UART0����ʹ��
   SREG=BIT(7);	           //ȫ���жϿ���
   DDRE|=BIT(1);	           //����TXΪ���������Ҫ�����ƺ�����MEGA1280��˵û��
}

/*UART0�Ĵ��ڷ��ͺ�����ÿ�η���һ���ֽڣ�Byte��*/
void uart0_sendB(uchar data)
{
   while(!(UCSR0A&(BIT(UDRE0))));//�ж�׼��������
   UDR0=data;
   while(!(UCSR0A&(BIT(TXC0))));//�ж���ɷ��ͷ�
   UCSR0A|=BIT(TXC0);//TXC0��־λ�ֶ����㣬ͨ����TXC0��1ʵ��
}

#pragma interrupt_handler uart0_rx:19

/*UART0�Ĵ��ڽ��պ�����ÿ�ν���һ���ֽڣ�Byte��*/
void uart0_rx(void)
{
 	uchar uart0_r_byte;//UART0ÿ���жϽ��յ����ַ���1byte��
	UCSR0B&=~BIT(RXCIE0);//�ر�RXCIE1������λ���ֲ���
	uart0_r_byte=UDR0;
	if(uart0_instr_make(uart0_r_byte)==0)//ͨ�������ַ�������������Ҫ���instr
	    {UCSR0B|=BIT(RXCIE0);}//ʹ��RXCIE1������λ���ֲ���
}

//UART0��ָ��ʶ�������ӽ��յ����ַ�����ȡ����xx��ͷ��ָ���ַ���
uchar uart0_instr_make(uchar r_byte)
{
    uchar instr_num;//instr�����е��ַ���
	uchar fun_ret;//�洢����������ֵ 
    switch(uart0_r_instr_chk)//��������x�ĸ������в���
	{
	    case 0:
			 {
			 if(r_byte=='x')
			     {				 
				 uart0_r_instr_chk=1;
				 }
			 fun_ret=0;
			 break;
			 }
		case 1:
			 {
			 if(r_byte=='x')
			     {uart0_r_instr_chk=2;}
			 else
			 	 {uart0_r_instr_chk=0;}
			 fun_ret=0;
			 break;
			 }
		case 2:
			 {
			 instr_num=strlen(uart0_instr);
			 if(instr_num==3)
			 {
			     uart0_instr[instr_num]=r_byte;
				 uart0_instr_flag=1;//instr�Ѿ����㿪ͷxx�ͳ���Ҫ��flag��1�����������
			     fun_ret=1;				 
			 }
			 else
			 {
			     uart0_instr[instr_num]=r_byte;
				 fun_ret=0;
			 }
			 break;
			 }
		default:break;
	}
	return fun_ret;
}

/*UART0�ַ������ͺ���*/
void uart0_send_string(uchar *str_send)//�βΣ��������ַ���
{
 	 uchar str_send_num=strlen(str_send);//�������ַ����������ַ�����
	 	   								//����str_send���һλֵΪNULL
	 uchar i=0;
	 while(i<str_send_num)
	 {
	   uart0_sendB(*(str_send+i));
	   i+=1;
	 }
}

void uart0_send_string_with_num(uchar *str_send,uchar char_num)//�βΣ��������ַ������ַ����ַ���
{
	 uchar i=0;
	 while(i<char_num)
	 {
	   uart0_sendB(*(str_send+i));
	   i+=1;
	 }
}

//------------��UART1����غ�������-------------------------------------------------------------

/*UART1�Ĵ��ڳ�ʼ������*/
void uart1_init(uint baud)
{
    UCSR1B=0x00; 
    UCSR1A=0x00; 		   //���ƼĴ�������
    UCSR1C=(0<<UPM10)|(3<<UCSZ10); //ѡ��UCSRC���첽ģʽ����ֹУ�飬1λֹͣλ��8λ����λ                       
   
    baud=mclk/16/baud-1;    //���������Ϊ65K
    UBRR1L=baud; 					     	  
    UBRR1H=baud>>8; 		   //���ò�����
   
    UCSR1B|=(1<<TXEN1)|(1<<RXEN1)|(1<<RXCIE1);   //���ա�����ʹ�ܣ������ж�ʹ��
    SREG=BIT(7);	           //ȫ���жϿ���
    DDRD|=BIT(3);	           //����TXΪ���������Ҫ�����ƺ�����MEGA1280��˵û��
   
   	//RS485оƬ����Ϊ���ͣ�DE=PD5=1
	//ע�⣡��оƬΪ��˫��ͨ�ţ�����ͬʱ�պͷ�����������ʱӦע����һ��
    DDRD|=BIT(5);
    PORTD|=BIT(5);

	DDRD|=BIT(4);
    PORTD|=BIT(4);
}


/*UART1�Ĵ��ڷ��ͺ�����ÿ�η���һ���ֽڣ�Byte��*/
void uart1_sendB(uchar data)
{
   while(!(UCSR1A&(BIT(UDRE1))));//�ж�׼��������
   UDR1=data;
   while(!(UCSR1A&(BIT(TXC1))));//�ж���ɷ��ͷ�
   UCSR1A|=BIT(TXC1);//TXC1��־λ�ֶ����㣬ͨ����TXC1��1ʵ��
}

/*UART1�ַ������ͺ���*/
void uart1_send_string(uchar *str_send,uchar str_num)//�βΣ��������ַ���
{
	 uchar i=0;
	 while(i<str_num)
	 {
	   uart1_sendB(*(str_send+i));
	   i+=1;
	   //delay(10);
	 }
}

//#pragma interrupt_handler uart1_rx:31

/*
void uart1_rx(void)
{	
    UCSR1B&=~BIT(RXCIE1);
	//rdata=UDR1;
	//flag=1;
	UCSR1B|=BIT(RXCIE1);
}
*/


//------------------���ַ���������������-------------------------------------------------------

//����Ԫ�ؿ�������
void array_copy(uchar *array1,uchar start_index,uchar *array2,uchar copy_num)
//��array1���Ե�start_indexλ���copy_num��Ԫ�ؿ�����array2�ĵ�0��copy_num-1λ
//array1��Ԫ����Ŀ��ӦС��start_index+copy_num+1����array2��Ԫ����Ŀ��ӦС��copy_num��
{
    uchar i;
	for(i=0;i<copy_num;i++)
	{
	    array2[i]=array1[start_index+i];
	}
}

//�ַ�������ַ����ȽϺ�����������0�����ʾ��ȣ����򲻵�
//*str0��*str1����ʹ����(����array_eg[])��Ҳ�������ַ�������(����"abcd")
int array_cmp(char * str0, char * str1)
{
    int i;
    for(i=0;str0[i]!=0 && str1[i]!=0 && str0[i]==str1[i];i++);
    return str0[i]-str1[i];
}

uchar* Type(uchar* Instruction)
{
    static uchar type_name[3];//static�ؼ��ʺ���Ҫ�������ӳ��������ɺ�����������ʧ
	type_name[0]=Instruction[0];
	type_name[1]=Instruction[1];
	type_name[2]=0;
	return type_name;
}

//----------------��Ӧ��Ƭ��غ�������--------------------------------

//Ӧ��Ƭ��ȡ����
void force_data_init(void)
{
	/*A0-DT ADDO����Ƭ����DT��ȡ����;A1-SCK ADSK����Ƭ������ߵ͵�ƽ��SCK*/
	//PA0���óɸ���̬����
	DDRA&=(~BIT(0));//DDRA0=0
	PORTA&=(~BIT(0));//PORTA0=0

	//PA1���ó����
	DDRA|=BIT(1);//DDRA1=1
}

//unsigned long������ת���ַ��������ڽ�Ӧ��Ƭ�ɼ��ص������ϴ�
uchar* ulong_to_uchar_array(unsigned long data_num)
{
 	//long�����ڴ��еĴ洢 0x12345678 ���͵�ַ78+56+34+12�ߵ�ַ
	uchar* pNum;
	uchar force_data[5];
	pNum=(uchar *)&data_num;
	force_data[3]=*pNum;
	force_data[2]=*(++pNum);
	force_data[1]=*(++pNum);
	force_data[0]=*(++pNum);
	force_data[4]=0;
	force_high8=0x7f-force_data[1];//��Чֵ�߰�λ����ȫ�ֱ�����
	//ע�����Ӧ��Ƭ�������force_high8��ֵ�����Ƿ���0x7f��ȥ��Чֵ�߰�λ
	
	return force_data;
}

//Ӧ��ɼ�ģ�����ݶ�ȡ���򣬲�������ʾ����д
unsigned long ReadCount(void)
{
    unsigned long Count;
    unsigned char i;
	uchar* ptr_count;
	PORTA&=(~BIT(1));//ADSK=PORTA1=0
    Count=0;
    while(PINA&BIT(0));//��ȡPINA0=ADDO
    for(i=0;i<24;i++)
    {
        PORTA|=BIT(1);//ADSK=PORTA1=1
        Count=Count<<1;
        PORTA&=(~BIT(1));//ADSK=PORTA1=0
        if(PINA&BIT(0)) Count++;
    }
    PORTA|=BIT(1);//ADSK=PORTA1=1
    Count=Count^0x800000;
    PORTA&=(~BIT(1));//ADSK=PORTA1=0
    return(Count);
}

//����Ӧ��Ƭ������Чֵ�ĸ߰�λ
void command_data_save_force_high8(force_save)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0030;//д��ַ
	EEDR=force_save;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ1�ƶ���һ�׶�
void command_data_read_force_high8(uchar* PARA)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0030;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA=EEDR;//����EEDR�е�����

	SREG |= BIT(3);//��ȫ���ж�
}

//��ʱ��������ʼ������
void timer1_init(void)
{
    TCCR1B=0X04;//256�ڲ���Ƶ
	TCNT1=0xC2F6;//��ʱ����500ms�����㷽���������ĵ�
	TIFR=0x04;//��ʱ������1�����־λ�������1������ϵ�Ĭ��Ϊ0
}

//�����жϺ���
#pragma interrupt_handler timer1_interrupt_handler:15

//��ʱ������1�жϴ�������
void timer1_interrupt_handler(void)
{
    uchar msg_force_array[]="zz21w";
	force_ulong=ReadCount();//��ȡ���ݷ���ȫ�ֱ���
	ulong_to_uchar_array(force_ulong);//��������ת��
	msg_force_array[4]=force_high8;
	uart0_send_string(msg_force_array);//����λ�����ͼн���ʵʱ��ֵ������Чֵ�߰�λ��
	TCNT1=0xC2F6;//��Ҫ�����趨����500ms
}

//----------------�����������غ�������---------------------------------------------

//ָ��У�������ɺ�������ʽ�ɶ��ʹ��˵����ָ��
uchar ratio_command_check(uchar ID,uchar PARA2,uchar PARA3)
{
    uchar check_sum;
	check_sum=0x05+0x03+0x20+ID+PARA2+PARA3;
	return ~check_sum;
}

//�������ָ�����ݴ洢���������ENDLESS TURNģʽ��PARA2��PARA3��ŵ�EEPROM�У���ָ�ƶ�0��һ�׶�
void command_data_save_finger_0_ratio_1(uchar PARA2,uchar PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0000;//д��ַ
	EEDR=PARA2;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0001;//д��ַ
	EEDR=PARA3;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݴ洢���������ENDLESS TURNģʽ��PARA2��PARA3��ŵ�EEPROM�У���ָ0�ƶ��ڶ��׶�
void command_data_save_finger_0_ratio_2(uchar PARA2,uchar PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0002;//д��ַ
	EEDR=PARA2;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0003;//д��ַ
	EEDR=PARA3;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݴ洢���������ENDLESS TURNģʽ��PARA2��PARA3��ŵ�EEPROM�У���ָ0�ƶ��ɿ��׶�
void command_data_save_finger_0_ratio_3(uchar PARA2,uchar PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0004;//д��ַ
	EEDR=PARA2;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0005;//д��ַ
	EEDR=PARA3;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ0�ƶ���һ�׶�
void command_data_read_finger_0_ratio_1(uchar* PARA2,uchar* PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0000;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA2=EEDR;//����EEDR�е�����

	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0001;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA3=EEDR;//����EEDR�е�����
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ0�ƶ��ڶ��׶�
void command_data_read_finger_0_ratio_2(uchar* PARA2,uchar* PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0002;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA2=EEDR;//����EEDR�е�����

	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0003;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA3=EEDR;//����EEDR�е�����
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ0�ƶ��ƶ��׶�
void command_data_read_finger_0_ratio_3(uchar* PARA2,uchar* PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0004;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA2=EEDR;//����EEDR�е�����

	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0005;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA3=EEDR;//����EEDR�е�����
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݴ洢���������ENDLESS TURNģʽ��PARA2��PARA3��ŵ�EEPROM�У���ָ1�ƶ���һ�׶�
void command_data_save_finger_1_ratio_1(uchar PARA2,uchar PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0010;//д��ַ
	EEDR=PARA2;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0011;//д��ַ
	EEDR=PARA3;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݴ洢���������ENDLESS TURNģʽ��PARA2��PARA3��ŵ�EEPROM�У���ָ1�ƶ��ڶ��׶�
void command_data_save_finger_1_ratio_2(uchar PARA2,uchar PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0012;//д��ַ
	EEDR=PARA2;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0013;//д��ַ
	EEDR=PARA3;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݴ洢���������ENDLESS TURNģʽ��PARA2��PARA3��ŵ�EEPROM�У���ָ1�ƶ��ɿ��׶�
void command_data_save_finger_1_ratio_3(uchar PARA2,uchar PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0014;//д��ַ
	EEDR=PARA2;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	while(EECR & BIT(EEWE));//�ж�EEWE�Ƿ�Ϊ0
	EEAR=0x0015;//д��ַ
	EEDR=PARA3;//д����
	EECR|=BIT(EEMWE);//EEMWE��1
	EECR&=(~BIT(EEWE));//EEWE��0
	EECR|=BIT(EEWE);//EEWE��1
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ1�ƶ���һ�׶�
void command_data_read_finger_1_ratio_1(uchar* PARA2,uchar* PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0010;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA2=EEDR;//����EEDR�е�����

	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0011;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA3=EEDR;//����EEDR�е�����
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ1�ƶ��ڶ��׶�
void command_data_read_finger_1_ratio_2(uchar* PARA2,uchar* PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0012;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA2=EEDR;//����EEDR�е�����

	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0013;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA3=EEDR;//����EEDR�е�����
	
	SREG |= BIT(3);//��ȫ���ж�
}

//�������ָ�����ݶ�ȡ��������EEPROM�ж�ȡ���ƶ�������PARA2��PARA3����ָ1�ƶ��ƶ��׶�
void command_data_read_finger_1_ratio_3(uchar* PARA2,uchar* PARA3)
{
    SREG &=(~BIT(3));//�ر�ȫ���ж�
	
	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0014;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA2=EEDR;//����EEDR�е�����

	while(EECR & BIT(EEWE));//�ȴ�ǰ��ġ�д���������
	EEAR=0x0015;//д��ַ
	EECR|=BIT(EERE);//������λ��1
	*PARA3=EEDR;//����EEDR�е�����
	
	SREG |= BIT(3);//��ȫ���ж�
}


//----------------���ⲿ�ж������������ⲿ�жϴ�����������--------------------------------

void ext_interrupt_init(void)
{
    //PD0=INT0=�г����Ϸ���������������������
    DDRD&=(~BIT(0));//��˼��DDRD0=0������λ���䡣��ע�ⲻ�ɰ�ע�͵ķ�ʽд��
    PORTD|=BIT(0);//��˼��PORTD0=1������λ���䡣��ע�ⲻ�ɰ�ע�͵ķ�ʽд��
	
    //PD1=INT1=�г����·���������������������
    DDRD&=(~BIT(1));
    PORTD|=BIT(1);

	//PE4=INT4=�г���ǰ����������������������
    DDRE&=(~BIT(4));
    PORTE|=BIT(4);
	
	//INT0��INT1���ⲿ�жϼĴ�������
    EICRA|=BIT(ISC01)|BIT(ISC11);//�жϴ�����ʽ���½��ش���
    EIMSK|=BIT(0)|BIT(1);//�ж�ʹ��	

    //INT4���ⲿ�жϼĴ�������
    EICRB|=BIT(ISC41);//�жϴ�����ʽ���½��ش���
    EIMSK|=BIT(4);//�ж�ʹ��
}

//�г����Ϸ�������INT0���ж��������� 
#pragma interrupt_handler interrupt_0_handler:2

//�г����Ϸ�������INT0���жϴ�������
void interrupt_0_handler(void)
{
	if(ext_collision_alert_allow_int0)
	{
	    delay(50);
		uart0_send_string("zz41");
        ext_collision_alert_allow_int0=0;//ȫ�ֱ���
	}
}

//�г����·�������INT1���ж��������� 
#pragma interrupt_handler interrupt_1_handler:3

//�г����·�������INT1���жϴ�������
void interrupt_1_handler(void)
{
	if(ext_collision_alert_allow_int1)
	{
	    delay(50);
		uart0_send_string("zz42");
        ext_collision_alert_allow_int1=0;//ȫ�ֱ���
	}
}

//�г���ָ�ⴥ����INT4���ж��������� 
#pragma interrupt_handler interrupt_4_handler:6

//�г���ָ�ⴥ����INT4���жϴ�������
void interrupt_4_handler(void)
{
	if(ext_collision_alert_allow_int4)
	{
	    delay(50);
		uart0_send_string("zz43");
        ext_collision_alert_allow_int4=0;//ȫ�ֱ���
	}
}



//---------------------------������������-----------------------------------------------------------
void main(void)
{

 	//.....................[�������ڱ�������]...............................

 	uchar i;//���ָ��洢����ʱ����ѭ���ļ�������
	
	//���������ƶ�����ʱ���õı���������2������
	uchar motor_command[9]={0xff,0xff,0x00,0x05,0x03,0x20,0x00,0x00,0x00};//
	uchar CHECK;//���ָ�����һλУ����
	
	//�ϵ���ָ��λ�׶��õ��ı���������2������
	//��֤���ֹͣ����ֻ����һ��
	uchar cage_0=1;
	uchar cage_1=1;
	
	//��������ģʽ��ģʽһ����ָ�ɿ��׶��õ��ı���������2������
	//��֤ÿ�μн�����ʱ���ֹͣ����ֻ����һ��	
	uchar approach_0;
	uchar approach_1;
	
	//�н��ڶ��׶ε�whileѭ������ָʾ
	uchar hold_stage_2_continue=1;
	
	//��ѭ���в�ѯ������λ�Ϳռ����õ��ı���������7������
	//while��ѭ����������λ��ʼ����˲������ٴη��Ͷ��ֹͣ������ǵȴ���λ����
	uchar stop_allow_cage_0=0;
	uchar stop_allow_cage_1=0;
	//��������ָ��˲��ƶ���������ָ���м��ƶ��������ĸ���������Ϊ���ǵ�������λ���ռ�
	uchar release_allow_motor_0=0;
	uchar release_allow_motor_1=0;
	uchar hold_allow_motor_0=1;
	uchar hold_allow_motor_1=1;
	//�����ϵ縴λ��Ϳ�ʼ����Ƿ�ռ�
	uchar stop_allow_empty=1;
	
	//����ģʽ��ָ�ƶ��ٶȣ���ʼֵΪͨ�������趨ǰ��Ĭ��ֵ
	uchar PARA2=0x10,PARA3=0x01;//��֤���٣�������ͨ���������������ֵ��
	
	//һ��ģʽ��ָ�ƶ��ٶȣ����ʴ�EEPROM�л�ȡ��ֵ
	uchar com_finger0_ratio_1_PARA2,com_finger0_ratio_1_PARA3;//��ָ0��Ratio 1
	uchar com_finger0_ratio_2_PARA2,com_finger0_ratio_2_PARA3;//��ָ0��Ratio 2
	uchar com_finger0_ratio_3_PARA2,com_finger0_ratio_3_PARA3;//��ָ0��Ratio 3
	uchar com_finger1_ratio_1_PARA2,com_finger1_ratio_1_PARA3;//��ָ1��Ratio 1
	uchar com_finger1_ratio_2_PARA2,com_finger1_ratio_2_PARA3;//��ָ1��Ratio 2
	uchar com_finger1_ratio_3_PARA2,com_finger1_ratio_3_PARA3;//��ָ1��Ratio 3
	
	uchar msg_eeprom_array[17];//����λ������EEPROM����ֵ
	uchar msg_interrupt_array[7]={'z','z','4','4','0','0','0'};//����λ�����ؼг������ⲿ��ײ�������жϣ�����������ֵ
	
	
	//.......................[��ʼ������].........................
	
    uart0_init(19200);//����0������λ��ͨ�ţ���ʼ���������ʾ�Ϊ19200
    uart1_init(19200);//����1������ͨ�ţ���ʼ���������ʾ�Ϊ19200
	timer1_init();//��ʱ������1��ʼ��
	force_data_init();//Ӧ��Ƭ��ȡ��ʼ��
	
	
	//��λ������
	
	//PE2=�ӽ�����0������̬����
	DDRE&=(~BIT(2));//DDRE2=0
	PORTE&=(~BIT(2));//PORTE2=0
	
	//PE3=�ӽ�����1������̬����
	DDRE&=(~BIT(3));//DDRE2=0
	PORTE&=(~BIT(3));//PORTE2=0
	
    //PE5=INT5=��λ0����������������
    DDRE&=(~BIT(5));//��˼��DDRE5=0������λ���䡣��ע�ⲻ�ɰ�ע�͵ķ�ʽд��
    PORTE|=BIT(5);//��˼��PORTE5=1������λ���䡣��ע�ⲻ�ɰ�ע�͵ķ�ʽд��
	
    //PE6=INT6=��λ1����������������
    DDRE&=(~BIT(6));
    PORTE|=BIT(6);

	//PE7=�ռУ���������������
    DDRE&=(~BIT(7));
    PORTE|=BIT(7);
	
	
	//................[���ܣ��ϵ����ָ��λ]....................................

    SREG |= 0X80;//��ȫ���ж�
    
    //��ر�����ʼ��
    cage0_state=0;
    cage1_state=0;

    //������ֹͣת��
    uart1_send_string((uchar*)no0stop,9);
	delay(50);
	uart1_send_string((uchar*)no1stop,9);
    delay(50);
	
    //ʹ��צ�ɿ�
    uart1_send_string((uchar*)no0release,9);
	delay(50);
    uart1_send_string((uchar*)no1release,9);

    //�ȴ�������λ����	
	while(cage_0|cage_1)
	{
	    if(cage_0)
		{
    	    if(!(PINE & BIT(5)))//PE5=0����
	    	{
		        delay(50);
				if(!(PINE & BIT(5)))
				{
			        uart1_send_string((uchar*)no0stop,9);
					//uart0_send_string("zz30");
					cage_0=0;
			    }
			
		    }
		}
		
	    if(cage_1)
		{
    	    if(!(PINE & BIT(6)))//PE6=0����
	    	{
		        delay(50);
				if(!(PINE & BIT(6)))
				{
			        uart1_send_string((uchar*)no1stop,9);
					//uart0_send_string("zz31");
					cage_1=0;
			    }
			
		    }
		}
	}
	
    uart0_send_string("zz00");//����λ������׼������
	
	UCSR0B|=(1<<RXEN0)|(1<<RXCIE0);   //UART0����ʹ�ܣ������ж�ʹ��
	
	
//........................[while(1)��ѭ��]............................................
	
    while(1)
	{
	 	 if(uart0_instr_flag==1)
		 {
	         switch(gripper_mood)
		     {
	             case 0:
			     {
			         if(array_cmp(uart0_instr,"0100")==0)
				     {
				         //uart0_send_string(" mood 0: enter 1-regular working mood! ");
					 	 gripper_mood=1;
						 ext_interrupt_init();//�ⲿ�жϣ��г������ⲿ������ײ����ʼ��
						 //��ʼ�������������һ��INT0��INT1�����Ա�������������Ҫ����0����1
						 
						 //�г������ⲿ��ײ�������жϣ���������
						 ext_collision_alert_allow_int0=1;//�ϲ�
						 ext_collision_alert_allow_int1=1;//�²�
						 ext_collision_alert_allow_int4=1;//ָ��

						 
						 //��ȡEEPROM�д洢��RATIOֵ
    					 command_data_read_finger_0_ratio_1(&com_finger0_ratio_1_PARA2,&com_finger0_ratio_1_PARA3);
						 command_data_read_finger_0_ratio_2(&com_finger0_ratio_2_PARA2,&com_finger0_ratio_2_PARA3);
						 command_data_read_finger_0_ratio_3(&com_finger0_ratio_3_PARA2,&com_finger0_ratio_3_PARA3);
						 command_data_read_finger_1_ratio_1(&com_finger1_ratio_1_PARA2,&com_finger1_ratio_1_PARA3);
						 command_data_read_finger_1_ratio_2(&com_finger1_ratio_2_PARA2,&com_finger1_ratio_2_PARA3);
						 command_data_read_finger_1_ratio_3(&com_finger1_ratio_3_PARA2,&com_finger1_ratio_3_PARA3);
	
						 //��ȡEEPROM�д洢�ļн�����ֵ��Чֵ�߰�λ
						 command_data_read_force_high8(&force_judge);
						 
	
				     }
				 
				 	 if(array_cmp(uart0_instr,"0200")==0)
				 	 {
				         //uart0_send_string(" mood 0: enter 2-configuration mood! ");
					 	 gripper_mood=2;
						 ext_interrupt_init();//�ⲿ�жϣ��г������ⲿ������ײ����ʼ��
						 //��ʼ�������������һ��INT0��INT1�����Ա�������������Ҫ����0����1
						 
						 //�г������ⲿ��ײ�������жϣ���������
						 ext_collision_alert_allow_int0=1;//�ϲ�
						 ext_collision_alert_allow_int1=1;//�²�
						 ext_collision_alert_allow_int4=1;//ָ��
				 	 }
				 
				 	 break;
			     }
			 
		         case 1:
			     {
			     	 if(array_cmp(uart0_instr,"1000")==0)//����ģʽ����ȡEEPROM�д洢��RATIO������ֵ
				 	 {
						 //����ratio����ֵ������ֵ����Ϣ�����ֵ
						 
						 //��ȡEEPROM�д洢��RATIOֵ
    					 command_data_read_finger_0_ratio_1(&com_finger0_ratio_1_PARA2,&com_finger0_ratio_1_PARA3);
						 command_data_read_finger_0_ratio_2(&com_finger0_ratio_2_PARA2,&com_finger0_ratio_2_PARA3);
						 command_data_read_finger_0_ratio_3(&com_finger0_ratio_3_PARA2,&com_finger0_ratio_3_PARA3);
						 command_data_read_finger_1_ratio_1(&com_finger1_ratio_1_PARA2,&com_finger1_ratio_1_PARA3);
						 command_data_read_finger_1_ratio_2(&com_finger1_ratio_2_PARA2,&com_finger1_ratio_2_PARA3);
						 command_data_read_finger_1_ratio_3(&com_finger1_ratio_3_PARA2,&com_finger1_ratio_3_PARA3);
	
						 msg_eeprom_array[0]='z';
						 msg_eeprom_array[1]='z';
						 msg_eeprom_array[2]='3';
						 msg_eeprom_array[3]='3';
						 msg_eeprom_array[4]=com_finger0_ratio_1_PARA2;
						 msg_eeprom_array[5]=com_finger0_ratio_1_PARA3;
						 msg_eeprom_array[6]=com_finger0_ratio_2_PARA2;
						 msg_eeprom_array[7]=com_finger0_ratio_2_PARA3;
						 msg_eeprom_array[8]=com_finger0_ratio_3_PARA2;
						 msg_eeprom_array[9]=com_finger0_ratio_3_PARA3;
						 msg_eeprom_array[10]=com_finger1_ratio_1_PARA2;
						 msg_eeprom_array[11]=com_finger1_ratio_1_PARA3;
						 msg_eeprom_array[12]=com_finger1_ratio_2_PARA2;
						 msg_eeprom_array[13]=com_finger1_ratio_2_PARA3;
						 msg_eeprom_array[14]=com_finger1_ratio_3_PARA2;
						 msg_eeprom_array[15]=com_finger1_ratio_3_PARA3;
	
						 //��ȡEEPROM�д洢�ļн�����ֵ��Чֵ�߰�λ
						 command_data_read_force_high8(&force_judge);
						 msg_eeprom_array[16]=force_judge;
						 
						 delay(50);
						 uart0_send_string_with_num(msg_eeprom_array,17);//�ϴ�EEPROM�д洢����ֵ
	
					 }
	
					 
					 if(array_cmp(uart0_instr,"1100")==0)//�ɿ�
				 	 {
				         if(release_allow_motor_0)
						 {
						     //������ָ��
							 motor_command[2]=0x00;//ID=0
						 	 motor_command[6]=com_finger0_ratio_3_PARA2;
						 	 motor_command[7]=com_finger0_ratio_3_PARA3+0x04;
						 	 CHECK=ratio_command_check(0,com_finger0_ratio_3_PARA2,com_finger0_ratio_3_PARA3+0x04);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);//�·�ָ��
						 }
						 
						 if(release_allow_motor_1)
						 {
						     //������ָ��
							 motor_command[2]=0x01;//ID=1
						 	 motor_command[6]=com_finger1_ratio_3_PARA2;
						 	 motor_command[7]=com_finger1_ratio_3_PARA3+0x04;
						 	 CHECK=ratio_command_check(1,com_finger1_ratio_3_PARA2,com_finger1_ratio_3_PARA3+0x04);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);//�·�ָ��
						 }
				 	 }
				 
				 	 if(array_cmp(uart0_instr,"1200")==0)//�н�
				 	 {
				         if(hold_allow_motor_0 & hold_allow_motor_1)
						 {
						     //��һ�׶�
							 //������ָ��
							 motor_command[2]=0x00;//ID=0
						 	 motor_command[6]=com_finger0_ratio_1_PARA2;
						 	 motor_command[7]=com_finger0_ratio_1_PARA3;
						 	 CHECK=ratio_command_check(0,com_finger0_ratio_1_PARA2,com_finger0_ratio_1_PARA3);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);//�·�ָ��
							 
							 //������ָ��
							 motor_command[2]=0x01;//ID=1
						 	 motor_command[6]=com_finger1_ratio_1_PARA2;
						 	 motor_command[7]=com_finger1_ratio_1_PARA3;
						 	 CHECK=ratio_command_check(1,com_finger1_ratio_1_PARA2,com_finger1_ratio_1_PARA3);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);//�·�ָ��
	
							 approach_0=1;
							 approach_1=1;
							 
							 //�Ƚӽ����ش�����ͬʱ��ѯ����Ƿ�ռ�
							 while((approach_0|approach_1)&&(PINE & BIT(7)))
							 {
	   						     if(approach_0)
								 {
								     if(!(PINE & BIT(2)))//PE2=0����
									 {
		        					     delay(50);
										 if(!(PINE & BIT(2)))
										 {
			        					     uart1_send_string((uchar*)no0stop,9);
											 //uart0_send_string(" interrupt 5 ");
											 approach_0=0;
			    						 }
			
		    						 }
							 	 }
		
	    					 	 if(approach_1)
								 {
    	    					     if(!(PINE & BIT(3)))//PE3=0����
	    						 	 {
		       					          delay(50);
								  	  	  if(!(PINE & BIT(3)))
								  	  	  {
			        			  	          uart1_send_string((uchar*)no1stop,9);
								  		  	  //uart0_send_string(" interrupt 6 ");
								  		  	  approach_1=0;
			    				          }
		    					     }
								 }
							 }
							 
							 if(PINE & BIT(7))//���ռ�����еڶ��׶�
							 {
							    //�ڶ��׶�
							 	//������ָ��
							 	motor_command[2]=0x00;//ID=0
						 	 	motor_command[6]=com_finger0_ratio_2_PARA2;
						 	 	motor_command[7]=com_finger0_ratio_2_PARA3;
						 	 	CHECK=ratio_command_check(0,com_finger0_ratio_2_PARA2,com_finger0_ratio_2_PARA3);
						 	 	motor_command[8]=CHECK;
						 	 	delay(50);
						 	 	uart1_send_string(motor_command,9);//�·�ָ��
							 
							 	//������ָ��
								 motor_command[2]=0x01;//ID=1
						 	 	 motor_command[6]=com_finger1_ratio_2_PARA2;
						 		 motor_command[7]=com_finger1_ratio_2_PARA3;
						 	 	 CHECK=ratio_command_check(1,com_finger1_ratio_2_PARA2,com_finger1_ratio_2_PARA3);
						 		 motor_command[8]=CHECK;
						 	 	 delay(50);
						 	 	 uart1_send_string(motor_command,9);//�·�ָ��
							 
							 	 //�ȴ��н����ﵽ��ֵ�������ж��Ƿ�ռ�
							 	 force_high8=0;
								 hold_stage_2_continue=1;
							 	 while(hold_stage_2_continue && (PINE & BIT(7)))
							 	 {
								     force_ulong=ReadCount();
								 	 ulong_to_uchar_array(force_ulong);//Ŀ���ǻ�ȡforce_high8
									 if(force_high8>=force_judge)
									 {
									     hold_stage_2_continue=0;//�´�ѭ������
										 
										 //ֹͣ�������
							 	 		 delay(50);
							 	 		 uart1_send_string((uchar*)no0stop,9);
							 	 		 delay(50);
							 	 		 uart1_send_string((uchar*)no1stop,9);
							 
							 	 		 hold_allow_motor_0=0;//��ֹ0����ָ��˲�����
							 	 		 hold_allow_motor_1=0;//��ֹ1����ָ��˲�����
							 
							 	 		 //������λ���Ѿ��н�
							 	 		 delay(50);
							 	 		 uart0_send_string("zz10");
										 delay(50);
							 	 		 uart0_send_string("zz10");
									 }
							 	 }
							 
							 	 
							 }
							 
						 }
				 	 }
					 
					 if(array_cmp(Type(uart0_instr),"13")==0)//�趨��ָ0��Ratio 1
				 	 {
					     com_finger0_ratio_1_PARA2=uart0_instr[2];
						 com_finger0_ratio_1_PARA3=uart0_instr[3];
						 command_data_save_finger_0_ratio_1(com_finger0_ratio_1_PARA2,com_finger0_ratio_1_PARA3);
					 }
					 
					 if(array_cmp(Type(uart0_instr),"14")==0)//�趨��ָ0��Ratio 2
				 	 {
					     com_finger0_ratio_2_PARA2=uart0_instr[2];
						 com_finger0_ratio_2_PARA3=uart0_instr[3];
						 command_data_save_finger_0_ratio_2(com_finger0_ratio_2_PARA2,com_finger0_ratio_2_PARA3);
					 }
					 
					 if(array_cmp(Type(uart0_instr),"15")==0)//�趨��ָ0��Ratio 3
				 	 {
					     com_finger0_ratio_3_PARA2=uart0_instr[2];
						 com_finger0_ratio_3_PARA3=uart0_instr[3];
						 command_data_save_finger_0_ratio_3(com_finger0_ratio_3_PARA2,com_finger0_ratio_3_PARA3);
					 }
					 
					 if(array_cmp(Type(uart0_instr),"16")==0)//�趨��ָ1��Ratio 1
				 	 {
					     com_finger1_ratio_1_PARA2=uart0_instr[2];
						 com_finger1_ratio_1_PARA3=uart0_instr[3];
						 command_data_save_finger_1_ratio_1(com_finger1_ratio_1_PARA2,com_finger1_ratio_1_PARA3);
					 }
					 
					 if(array_cmp(Type(uart0_instr),"17")==0)//�趨��ָ1��Ratio 2
				 	 {
					     com_finger1_ratio_2_PARA2=uart0_instr[2];
						 com_finger1_ratio_2_PARA3=uart0_instr[3];
						 command_data_save_finger_1_ratio_2(com_finger1_ratio_2_PARA2,com_finger1_ratio_2_PARA3);
					 }
					 
					 if(array_cmp(Type(uart0_instr),"18")==0)//�趨��ָ1��Ratio 3
				 	 {
					     com_finger1_ratio_3_PARA2=uart0_instr[2];
						 com_finger1_ratio_3_PARA3=uart0_instr[3];
						 command_data_save_finger_1_ratio_3(com_finger1_ratio_3_PARA2,com_finger1_ratio_3_PARA3);
					 }
					 
					 if(array_cmp(Type(uart0_instr),"19")==0)//�趨force_judge
				 	 {
					     force_judge=uart0_instr[2];
						 command_data_save_force_high8(force_judge);
					 }
				 
				 	 break;
			     }
			 
		         case 2:
			     {
					 if(array_cmp(uart0_instr,"2100")==0)//����ģʽ����ָ0ֹͣ
				 	 {
					     TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 uart1_send_string((uchar*)no0stop,9);
				 	 }

					 if(array_cmp(uart0_instr,"2101")==0)//����ģʽ����ָ0�ɿ������ƶ�
				 	 {
					     if(release_allow_motor_0)
						 {
						     TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 	 motor_command[2]=0x00;//ID=0
						 	 motor_command[6]=PARA2;
						 	 motor_command[7]=PARA3+0x04;//˳ʱ�룬���Բ����ڴ˸���PARA3��ֵ��
						 	 CHECK=ratio_command_check(0,PARA2,PARA3+0x04);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);
						 	 //delay(50);
						 	 //uart0_send_string_with_num(motor_command,9);
						 	 TIMSK|=BIT(2);//�򿪶�ʱ����1�жϣ����Ϸ��ؼг���ֵ
						 }
				 	 }
					 
					 if(array_cmp(uart0_instr,"2102")==0)//����ģʽ����ָ0�н������ƶ�
				 	 {
					     if(hold_allow_motor_0)
						 {
						     TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 	 motor_command[2]=0x00;//ID=0
						 	 motor_command[6]=PARA2;
						 	 motor_command[7]=PARA3;
						 	 CHECK=ratio_command_check(0,PARA2,PARA3);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);
						 	 //delay(50);
						 	 //uart0_send_string_with_num(motor_command,9);
						 	 TIMSK|=BIT(2);//�򿪶�ʱ����1�жϣ����Ϸ��ؼг���ֵ
						 }
				 	 }

					 if(array_cmp(uart0_instr,"2110")==0)//����ģʽ����ָ1ֹͣ
				 	 {
					     TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 uart1_send_string((uchar*)no1stop,9);
				 	 }

					 if(array_cmp(uart0_instr,"2111")==0)//����ģʽ����ָ1�ɿ������ƶ�
				 	 {
					     if(release_allow_motor_1)
						 {
						     TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 	 motor_command[2]=0x01;//ID=1
						 	 motor_command[6]=PARA2;
						 	 motor_command[7]=PARA3+0x04;//˳ʱ�룬���Բ����ڴ˸���PARA3��ֵ��
						 	 CHECK=ratio_command_check(1,PARA2,PARA3+0x04);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);
						 	 //delay(50);
						 	 //uart0_send_string_with_num(motor_command,9);
						 	 TIMSK|=BIT(2);//�򿪶�ʱ����1�жϣ����Ϸ��ؼг���ֵ
						 }
				 	 }
					 
					 if(array_cmp(uart0_instr,"2112")==0)//����ģʽ����ָ1�н������ƶ�
				 	 {
					     if(hold_allow_motor_1)
						 {
						     TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 	 motor_command[2]=0x01;//ID=1
						 	 motor_command[6]=PARA2;
						 	 motor_command[7]=PARA3;
						 	 CHECK=ratio_command_check(1,PARA2,PARA3);
						 	 motor_command[8]=CHECK;
						 	 delay(50);
						 	 uart1_send_string(motor_command,9);
						 	 //delay(50);
						 	 //uart0_send_string_with_num(motor_command,9);
						 	 TIMSK|=BIT(2);//�򿪶�ʱ����1�жϣ����Ϸ��ؼг���ֵ
						 }
				 	 }

					 if(array_cmp(Type(uart0_instr),"22")==0)
				 	 {
						 TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
						 delay(50);
						 uart1_send_string((uchar*)no0stop,9);
						 delay(50);
						 uart1_send_string((uchar*)no1stop,9);
						 PARA2=uart0_instr[2];
						 PARA3=uart0_instr[3];
						 //delay(50);
						 //uart0_send_string("ratio changed");
				 	 }
					 
					 if(array_cmp(uart0_instr,"2301")==0)//����ģʽ��������ָ0�ٶ�1(��ָ��ͬ)�н���һ�׶�
				 	 {
					     command_data_save_finger_0_ratio_1(PARA2,PARA3);
						 //delay(50);
						 //uart0_send_string(" Finger-0 Ratio-1 Set Successfully! ");
					 }
					 
					 if(array_cmp(uart0_instr,"2302")==0)//����ģʽ��������ָ0�ٶ�2(��ָ��ͬ)�н��ڶ��׶�
				 	 {
					     command_data_save_finger_0_ratio_2(PARA2,PARA3);
						 //delay(50);
						 //uart0_send_string(" Finger-0 Ratio-2 Set Successfully! ");
					 }
					 
					 if(array_cmp(uart0_instr,"2303")==0)//����ģʽ��������ָ0�ٶ�3(��ָ��ͬ)�ɿ��׶�
				 	 {
					     command_data_save_finger_0_ratio_3(PARA2,PARA3);
						 //delay(50);
						 //uart0_send_string(" Finger-0 Ratio-3 Set Successfully! ");
					 }
					 
					 if(array_cmp(uart0_instr,"2311")==0)//����ģʽ��������ָ1�ٶ�1(��ָ��ͬ)�н���һ�׶�
				 	 {
					     command_data_save_finger_1_ratio_1(PARA2,PARA3);
						 //delay(50);
						 //uart0_send_string(" Finger-1 Ratio-1 Set Successfully! ");
					 }
					 
					 if(array_cmp(uart0_instr,"2312")==0)//����ģʽ��������ָ1�ٶ�2(��ָ��ͬ)�н��ڶ��׶�
				 	 {
					     command_data_save_finger_1_ratio_2(PARA2,PARA3);
						 //delay(50);
						 //uart0_send_string(" Finger-1 Ratio-2 Set Successfully! ");
					 }
					 
					 if(array_cmp(uart0_instr,"2313")==0)//����ģʽ��������ָ1�ٶ�3(��ָ��ͬ)�ɿ��׶�
				 	 {
					     command_data_save_finger_1_ratio_3(PARA2,PARA3);
						 //delay(50);
						 //uart0_send_string(" Finger-1 Ratio-3 Set Successfully! ");
					 }
					 
					 if(array_cmp(Type(uart0_instr),"24")==0)//����ģʽ�����üн�����ֵ
				 	 {
					     force_judge=uart0_instr[2];
						 command_data_save_force_high8(force_judge);
						 //delay(50);
						 //uart0_send_string(" Force Set Successfully! ");
					 }
					 
					 
					 if(array_cmp(uart0_instr,"2500")==0)//����ģʽ����ȡEEPROM�д洢��RATIO������ֵ
				 	 {
						 //����ratio����ֵ������ֵ����Ϣ�����ֵ
						 
						 //��ȡEEPROM�д洢��RATIOֵ
    					 command_data_read_finger_0_ratio_1(&com_finger0_ratio_1_PARA2,&com_finger0_ratio_1_PARA3);
						 command_data_read_finger_0_ratio_2(&com_finger0_ratio_2_PARA2,&com_finger0_ratio_2_PARA3);
						 command_data_read_finger_0_ratio_3(&com_finger0_ratio_3_PARA2,&com_finger0_ratio_3_PARA3);
						 command_data_read_finger_1_ratio_1(&com_finger1_ratio_1_PARA2,&com_finger1_ratio_1_PARA3);
						 command_data_read_finger_1_ratio_2(&com_finger1_ratio_2_PARA2,&com_finger1_ratio_2_PARA3);
						 command_data_read_finger_1_ratio_3(&com_finger1_ratio_3_PARA2,&com_finger1_ratio_3_PARA3);
	
						 msg_eeprom_array[0]='z';
						 msg_eeprom_array[1]='z';
						 msg_eeprom_array[2]='3';
						 msg_eeprom_array[3]='3';
						 msg_eeprom_array[4]=com_finger0_ratio_1_PARA2;
						 msg_eeprom_array[5]=com_finger0_ratio_1_PARA3;
						 msg_eeprom_array[6]=com_finger0_ratio_2_PARA2;
						 msg_eeprom_array[7]=com_finger0_ratio_2_PARA3;
						 msg_eeprom_array[8]=com_finger0_ratio_3_PARA2;
						 msg_eeprom_array[9]=com_finger0_ratio_3_PARA3;
						 msg_eeprom_array[10]=com_finger1_ratio_1_PARA2;
						 msg_eeprom_array[11]=com_finger1_ratio_1_PARA3;
						 msg_eeprom_array[12]=com_finger1_ratio_2_PARA2;
						 msg_eeprom_array[13]=com_finger1_ratio_2_PARA3;
						 msg_eeprom_array[14]=com_finger1_ratio_3_PARA2;
						 msg_eeprom_array[15]=com_finger1_ratio_3_PARA3;
	
						 //��ȡEEPROM�д洢�ļн�����ֵ��Чֵ�߰�λ
						 command_data_read_force_high8(&force_judge);
						 msg_eeprom_array[16]=force_judge;
						 
						 delay(50);
						 uart0_send_string_with_num(msg_eeprom_array,17);//�ϴ�EEPROM�д洢����ֵ
	
					 }
					 
				     break;
			     }
			 
		         default:break;
			 }
			 
			 if(array_cmp(uart0_instr,"3100")==0)//�ָ��������� ext interrupt 0 
			 {
			     ext_collision_alert_allow_int0=1;//�ϲ�
			 }
			 
			 if(array_cmp(uart0_instr,"3200")==0)//�ָ��������� ext interrupt 1 
			 {
			     ext_collision_alert_allow_int1=1;//�²�
			 }
			 
			 if(array_cmp(uart0_instr,"3300")==0)//�ָ��������� ext interrupt 4 
			 {
			     ext_collision_alert_allow_int4=1;//ָ��
			 }
			 
			 if(array_cmp(uart0_instr,"3400")==0)//��ȡ��������������״̬
			 {
			     msg_interrupt_array[4]=ext_collision_alert_allow_int0;
				 msg_interrupt_array[5]=ext_collision_alert_allow_int1;
				 msg_interrupt_array[6]=ext_collision_alert_allow_int4;
				 uart0_send_string_with_num(msg_interrupt_array,7);
			 }
			 
			 /*ĩβӦ�������ִ�к�Ļ�ԭ����A-D*/
			 uart0_instr_flag=0; //A.������ձ�־λ��0
			 uart0_r_instr_chk=0;//B.������ַ���������0
			 for(i=0;i<12;i++)//C.�������
			 {
		         uart0_instr[i]=0;
			 }
			 UCSR0B|=BIT(RXCIE0);//D.�ָ�UART0�Ľ����ж�			
	     }
		 

		/*
		    ��ƫ��ײ�������ı�̡�
			��1��һ����ָ��ײ����λ���ز����͵�ƽ�������Ƕ��������ȶ��ĵ͵�ƽ��
			ҲҪ��ֹ��ָ��������ײλ���ƶ�����ʱ����Ҫ�ӳٷ����Ĵ�����
			��2��ֻ�е���ָ������ȫ���뿪����ײ�ص㣬��λ����IO��Ϊ�ȶ��ĸߵ�ƽ��
			��������ָ�ٴ�����ײ�ķ����ƶ���
		*/
		
		if(!(PINE & BIT(5)))//�����ָ0�Ƿ�λ
		{
			if(stop_allow_cage_0)
			{
			    release_allow_motor_0=0;//��ֹ1����ָ��˲�����
				hold_allow_motor_0=1;//����1����ָ���м俿��
				TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
				delay(50);
				uart1_send_string((uchar*)no0stop,9);
				delay(50);
				uart0_send_string("zz30");
				delay(50);
				uart0_send_string("zz30");
				stop_allow_cage_0=0;
			}
		}
		else
		{
		    if(!stop_allow_cage_0)
			{
			    delay(500);
			    if(PINE & BIT(5))
				{
				    release_allow_motor_0=1;//����1����ָ��˲�����
					stop_allow_cage_0=1;
				}
			}
		}
		
		if(!(PINE & BIT(6)))//�����ָ1�Ƿ�λ
		{
			if(stop_allow_cage_1)
			{
			    release_allow_motor_1=0;//��ֹ1����ָ��˲�����
				hold_allow_motor_1=1;//����1����ָ���м俿��
				TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
				delay(50);
				uart1_send_string((uchar*)no1stop,9);
				delay(50);
				uart0_send_string("zz31");
				delay(50);
				uart0_send_string("zz31");
				stop_allow_cage_1=0;
			}
		}
		else
		{
			if(!stop_allow_cage_1)
			{
			    delay(500);
			    if(PINE & BIT(6))
				{
				    release_allow_motor_1=1;//����1����ָ��˲�����
					stop_allow_cage_1=1;
				}
			}
		}

		if(!(PINE & BIT(7)))//����Ƿ�ռ�
		{
			if(stop_allow_empty)
			{
			    hold_allow_motor_0=0;//��ֹ0����ָ��˲�����
				hold_allow_motor_1=0;//��ֹ1����ָ��˲�����
				TIMSK&=(~BIT(2));//���ζ�ʱ����1�жϣ�ֹͣ���Ϸ��ؼг���ֵ
				delay(50);
				uart1_send_string((uchar*)no1stop,9);
				delay(50);
				uart1_send_string((uchar*)no0stop,9);
				delay(50);
				uart0_send_string("zz32");
				delay(50);
				uart0_send_string("zz32");
				stop_allow_empty=0;
			}
		}
		else
		{
			if(!stop_allow_empty)
			{
			    delay(500);
			    if(PINE & BIT(7))
				{
				    hold_allow_motor_0=1;//����0����ָ���м俿��
					hold_allow_motor_1=1;//����1����ָ���м俿��
					stop_allow_empty=1;
				}
			}
		}
		
	}
}

/*
��������˵��

��ѭ����λ���ʱ��Ϊ�˱�������Ҫ�ļ�⣬����ʹ����һЩ�����������ƣ���Щ����
�Ǵӡ���ʱ���ܡ��������Ƕ����õģ������Щ������ֵ��������ȫ������ָ�˶���
״̬����������ȱ���ǲ��ɱ��������������Ҽг���ִ�ж��������磺���г�����ָ��
�м䷽���ƶ�ʱ�����н�����ʱ���������˹��ⰴ��������λ���أ���г�������Ϊ��
ָ�Ѿ��ƶ����˲����������������ǲ����ܵģ���Ϊ�������û�����˲������ƶ�����
��Ҫ��ֹ���˹������У����齫��Щ���ڶԼ�������Ƶı�����Ϊ��ӳ��ָ����״̬
�ı�����������ָ����״̬�������Ƿ���ĳЩ��λ��

���⣬��ʹ���ж�����Ϊʼ��û�н����δ��������⣬��ʹ��ʹ������������ǰ���¡�

��������ģʽ�º�ʱ����ֹͣ��ȡ�н���ֵҲ����bug������ָ�����У���ʱֹͣ����һ
��������һ�������˶�ʱ����ȡ�Ѿ�ֹͣ�ˡ������bug��ʵ�����ò�������ֵʱӦ�ò�
����֣���Ϊ������������ֹһ����ָ���˶�����������һ����ָ��λ������������ֵ��

*/