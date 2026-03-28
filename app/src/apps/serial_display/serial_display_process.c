#include "SWM341.h"
#include "synwit_ui_framework/synwit_ui.h"
#include "board.h"
#include "serial_display_process.h"
#include "CircleBuffer.h"

static CircleBuffer_t CirBuf;

#define UART_BUFF_MAX_LEN	1024			//串口数据接收处理buff

#define  CRC_NEED			1				//校验位启动宏定义，置1开启校验功能

/* 串口接收数据缓存 */
static unsigned char revBuff[UART_BUFF_MAX_LEN] = {0};
static unsigned char respBuff[UART_BUFF_MAX_LEN] = {0};

/*串口中断函数*/
void UART0_Handler(void)
{
    uint32_t chr;

    if (UART_INTStat(UART0, UART_IT_RX_THR | UART_IT_RX_TOUT))
    {
        while (UART_IsRXFIFOEmpty(UART0) == 0)
        {
            if (UART_ReadByte(UART0, &chr) == 0)
            {
				CirBuf_Write(&CirBuf, (uint8_t *)&chr, 1);
            }
        }
        if (UART_INTStat(UART0, UART_IT_RX_TOUT))
        {
            UART_INTClr(UART0, UART_IT_RX_TOUT);
        }
    }
}

static void sdisp_uart_init_once(void)
{
    static bool sdisp_uart_inited = false;
    
    if(sdisp_uart_inited == false) {
        UART_InitStructure UART_initStruct;

        PORT_Init(PORTM, PIN0, PORTM_PIN0_UART0_RX, 1); //GPIOM.0???UART0????
        PORT_Init(PORTM, PIN1, PORTM_PIN1_UART0_TX, 0); //GPIOM.1???UART0????

        UART_initStruct.Baudrate = 115200;
        UART_initStruct.DataBits = UART_DATA_8BIT;
        UART_initStruct.Parity = UART_PARITY_NONE;
        UART_initStruct.StopBits = UART_STOP_1BIT;
        UART_initStruct.RXThreshold = 7;
        UART_initStruct.RXThresholdIEn = 1;
        UART_initStruct.TXThreshold = 3;
        UART_initStruct.TXThresholdIEn = 0;
        UART_initStruct.TimeoutTime = 10;
        UART_initStruct.TimeoutIEn = 1;
        UART_Init(UART0, &UART_initStruct);
        UART_Open(UART0);
        
        sdisp_uart_inited = true;
	}
}

/**
 * @brief   硬件 CRC 初始化
 * @note    CRC 校验配置为 CRC16, 宽度:8bit    多项式:1021  初始值:0x0000  结果异或值:0x0000   输入反转:false   输出反转:false
 */
static void crc_init(void)
{
    static bool crc_inited = false;
    if(!crc_inited) {
        CRC_InitStructure CRC_initStruct;
        CRC_initStruct.Poly = CRC_POLY_11021;
        CRC_initStruct.init_crc = 0;
        CRC_initStruct.in_width = CRC_WIDTH_8;
        CRC_initStruct.in_rev = CRC_REV_NOT;
        CRC_initStruct.in_not = false;
        CRC_initStruct.out_rev = CRC_REV_NOT;
        CRC_initStruct.out_not = false;
        CRC_Init(CRC, &CRC_initStruct);
        
        crc_inited = true;
    }
}


/**
 * @brief   计算校验值
 * 
 * @param[in]   dataBuff        数据内容
 * @param[in]   dataLen         数据长度
 * @return  unsigned int        返回校验和
 */
static unsigned int Uart_CRC_CCITT(unsigned char *dataBuff, unsigned short dataLen)
{
    crc_init();
    
    CRC_SetInitVal(CRC, 0);
    for(uint32_t i = 0; i < dataLen; i++)
        CRC_Write(dataBuff[i]);
	uint16_t crc_val = CRC_Result() & 0xFFFF;
    return crc_val;
}

/**
 * @brief   msg封包发送
 * 
 * @param[in]   msg    自定义信息  
 * @param[in]	len	   信息数据长度
 */
void sdisp_notify(const uint8_t *data, uint32_t len)
{
	uint32_t frame_len = len + 2 + 2 + 2;    // 2bytes len + 2 bytes header + 2 bytes crc
	uint8_t *buf = (uint8_t *)malloc(frame_len); 
	if (buf == NULL) {
		// 处理内存分配错误
		printf("Error allocating memory for data\r\n");
		return;
	}
	
	buf[0] =  0xFB;
	buf[1] =  0x83;
	
	buf[2] = (uint8_t)(len & 0xFF);
    buf[3] = (uint8_t)(len >> 8);
	
	memcpy(&buf[4], data, len);

	//进行帧校验计算
	unsigned int crc = Uart_CRC_CCITT(buf, len + 4);
	buf[len + 4]   =  (uint8_t)(crc & 0xFF);
	buf[len + 4 + 1] =  (uint8_t)(crc >> 8);

	sdisp_write(buf, frame_len);
	free(buf);
}

/**
 * @brief   错误码反馈
 * 
 * @param[in]   err_code       错误码
 */
static void response_rx(uint8_t err_code)
{
    uint8_t tx_buf[] = {0xFB, 0xFF, 0x00, 0x00, 0x00};
    tx_buf[2] = err_code;

    uint16_t crc = Uart_CRC_CCITT(tx_buf, 3) & 0xFFFF;
    tx_buf[3] = crc & 0xFF;
    tx_buf[4] = crc >> 8;

    sdisp_write(tx_buf, sizeof(tx_buf));
}


/**
 * @brief   校验值比对
 * 
 * @param[in]   crc_value       校验计算值
 * @return  	true    校验通过	false	检验失败
 */
static bool UART_Check_CRC(unsigned int crc_value)
{
    uint8_t CRC_buff[2];
    CirBuf_Read(&CirBuf, CRC_buff, 2);
    if (crc_value == ((CRC_buff[1] << 8) + CRC_buff[0]))
    {
        //校验正确。
        //printf("crc check ok\r\n");
        return true;
    }
    else
    {
        //校验错误。
        response_rx(0);
        //printf("crc check err\r\n");
        return false;
    }
}

/**
 * @brief uart发送函数 
 * uint8_t *buf 发送数据的地址 
 * uint8_t len 发送数据的长度
 */
void sdisp_write(uint8_t *buf, int len)
{
    sdisp_uart_init_once();
    
    for (uint32_t i = 0; i < len; i++)
    {
        UART_WriteByte(UART0, buf[i]);
        while (UART_IsTXBusy(UART0))
            ;
    }
}

/* 定义包头及指令信息 */
#define HEADER0 0xFA  		// 起始码
#define HEADER_setter 0x80  // 起始码_setter
#define HEADER_getter 0x81  // 起始码_setter

/* 枚举读取数据报文的状态 */
typedef enum
{
    FSM_IDLE,
	FSM_HEADER,
	FSM_LEN,
	FSM_END
} rx_fsm_t;

/* 定义数据包接收状态的变量,并初始化为空闲状态 */
typedef struct rx
{
    rx_fsm_t fsm;
    uint32_t len;          // 指令参数长度
} rx_t;

static rx_t rx = {FSM_IDLE, 0};

/**
 * @brief uart接收函数，需要在main_tick或其他函数循环调用
 * 
 */
void sdisp_handler()
{
    uint8_t receivedbyte;
    static uint32_t timeout_start = 0;
	static int pos = 0;
    static uint32_t resp_len = 0;
	
    sdisp_uart_init_once();
    if (CirBuf_Count(&CirBuf) <= (CB_ITEM_N-64)) // 缓存中有数据
    {   
        switch (rx.fsm)
        {
			case FSM_IDLE:
				CirBuf_Read(&CirBuf, &receivedbyte, 1);
				if(HEADER0 == receivedbyte)
				{
					
					revBuff[0] = receivedbyte;
					if (CirBuf_Count(&CirBuf))
					{
						CirBuf_Read(&CirBuf, &receivedbyte, 1);
						
						if (HEADER_setter == receivedbyte || HEADER_getter == receivedbyte)
						{
                            timeout_start = swm_gettick();
							revBuff[1] = receivedbyte;
							rx.fsm = FSM_HEADER;
						}
						else
						{
							//帧头2错误处理
							//to do...
							//printf ("header1 err\r\n");
						}
					}
				}
				else
				{
					//帧头1错误处理
					//to do...
					//printf ("header1 err\r\n");
				}
			break;
			
			case FSM_HEADER:
				if (CirBuf_Count(&CirBuf) >= 2)
				{
					CirBuf_Read(&CirBuf, &revBuff[2], 2); // 指令长度
					rx.len = (revBuff[3] << 8) + (revBuff[2] << 0);	
					timeout_start = swm_gettick();
					rx.fsm = FSM_LEN;
				}
			break;
				
			case FSM_LEN:
                #if (CRC_NEED)
				if (CirBuf_Count(&CirBuf) >= rx.len+2)
				{
					CirBuf_Read(&CirBuf, &revBuff[4], rx.len); // 读取数据层内容

					uint16_t crc = Uart_CRC_CCITT(revBuff, rx.len+4) ; //进行crc校验
					if(UART_Check_CRC(crc))
					{
						//校验通过，除去校验位，数据丢入处理
						resp_len = synwit_sdcmd_run((const uint8_t*)revBuff,respBuff,sizeof(respBuff),NULL);
						uint16_t resp_crc = Uart_CRC_CCITT(respBuff, resp_len);
						respBuff[resp_len] = resp_crc & 0xFF;
						respBuff[resp_len+1] = resp_crc >> 8;
						sdisp_write(respBuff, resp_len+2);
					}
					else
					{
						//校验错误，返回错误提示
					}
                    
                    timeout_start = swm_gettick();
					rx.fsm = FSM_END;
				}
                #else
                if (CirBuf_Count(&CirBuf) >= rx.len)
				{
					CirBuf_Read(&CirBuf, &revBuff[4], rx.len); // 读取数据层内容

					resp_len = synwit_sdcmd_run((const uint8_t*)revBuff,respBuff,sizeof(respBuff),NULL);
					sdisp_write(respBuff, resp_len);

					timeout_start = swm_gettick();
					rx.fsm = FSM_END;
				}
                #endif
			break;
				
			case FSM_END:
				rx.len = 0;
				resp_len = 0;
				memset(respBuff, 0, sizeof(respBuff));
				memset(revBuff, 0, sizeof(revBuff));
				rx.fsm = FSM_IDLE;
			break;
        }
    }
	else
	{
		CirBuf_Clear(&CirBuf);
		rx.len = 0;
		resp_len = 0;		
		memset(respBuff, 0, sizeof(respBuff));
		memset(revBuff, 0, sizeof(revBuff));	
		rx.fsm = FSM_IDLE;	
	}
	
	if((rx.fsm != FSM_IDLE) &&
        (swm_gettick() - timeout_start)>=2000)
	{
		timeout_start = swm_gettick();
		rx.len = 0;
		resp_len = 0;
		memset(respBuff, 0, sizeof(respBuff));
		memset(revBuff, 0, sizeof(revBuff));	
		rx.fsm = FSM_IDLE;		
	}
}
