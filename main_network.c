/*
 * CFile1.c
 *
 * Created: 2024/06/24 12:36:02
 *  Author: HARUK
 */ 

/**
 * \file
 *
 * \brief CDC Application Main functions
 *
 * Copyright (c) 2011-2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <stdio.h>
#include <string.h>
#include <asf.h>
#include "conf_usb.h"
#include "ui.h"
#include "uart.h"

static volatile bool main_b_cdc_enable = false;

void Bin2Hex( char *Src_ptr, short SrcVal_Size,  char *dst_buf);
signed short RE_CNT();
void LED_STATE(char);

/*! \brief Main function. Execution starts here.
 */
int main(void)
{
	unsigned char cnt = 0;
	
	unsigned short rotary_cnt;
	unsigned char SW1, SW2;
	static char tmp_buf[8]; //やり取りする8byteのデータ
	
	PORTC.DIR =0xff; //LED PORT　全消灯
	PORTA.DIR =0x00; //タクトSW PORTA6bit, PORTA7bit目

	irq_initialize_vectors();
	cpu_irq_enable();

	// Initialize the sleep manager
	sleepmgr_init();

#if !SAMD21 && !SAMR21
	sysclk_init();
	board_init();
#else
	system_init();
#endif
	ui_init();
	ui_powerdown();

	// Start USB stack to authorize VBus monitoring
	udc_start();

	
	// The main loop manages only the power mode
	// because the USB management is done by interrupt
	while (true) 
	{
		int data;
		//udi_cdc_putc( ++cnt )
		
		rotary_cnt = RE_CNT();					// ロータリエンコーダのカウント値取得
		Bin2Hex( &rotary_cnt, sizeof(rotary_cnt), tmp_buf);		// バイナリ値(2進数形式の数値)をHEX ASCII文字列に変換
		SW1 = (PORTA.IN&0x40)>>6;						// SW１はPORTAの6bit目　6bit目だけ抽出できるように他をマスク　>>6 6bitシフト
		SW2 = (PORTA.IN&0x80)>>7;						// SW2はPORTAの7bit目　 7bit目だけ抽出できるように他をマスク　>>7 7bitシフト
		// SW1・SW2の状態を書き込み　　　ゲームのプログラムではSWの初期状態は"aa"としてある
		if (SW1==0x01) tmp_buf[4] = 'a';                //SW1が押された場合、はtmp_buf[4]がn ゲームでは"na"がSW1が押された状態
		else tmp_buf[4] = 'n';
		if (SW2==0x01) tmp_buf[5] = 'a';                //SW2が押された場合、tmp_buf[5]がn　ゲームでは"an"がSW2が押された状態
		else tmp_buf[5] = 'n';
		tmp_buf[6] = 0x0d;			// Set CR code 表記\r ASCIIコード0x0d キャリッジを行の先頭に戻す
		tmp_buf[7] = 0x0a;			// Set LF code 表記\n ASCIIコード0x0a キャリッジを次の行に進める #ゲームでは6bit,7bitは使用しない
		
		while( udi_cdc_get_nb_received_data() )		// USB CDCでの受信バッファ内のデータ数を返す
		{
			data = udi_cdc_getc();					// USB CDCでの受信バッファから1byte取得
			if (data == 'NULL')	break; //dataがない場合break 送受信ループから抜ける
			LED_STATE(data);
					
			if (!udi_cdc_is_tx_ready()) //USB CDCで送信可能かチェック
			{
				udi_cdc_signal_overrun(); //FIfo full ->送信できないのでOverrunとして処理
			}
			else
			{
				udi_cdc_write_buf(tmp_buf, 8);	// 上記の8byte分書き込み
			}
		}
	}
	
}
//変更しない部分
void main_suspend_action(void)
{
	ui_powerdown();
}

void main_resume_action(void)
{
	ui_wakeup();
}

void main_sof_action(void)
{
	if (!main_b_cdc_enable)
		return;
	ui_process(udd_get_frame_number());
}

#ifdef USB_DEVICE_LPM_SUPPORT
void main_suspend_lpm_action(void)
{
	ui_powerdown();
}

void main_remotewakeup_lpm_disable(void)
{
	ui_wakeup_disable();
}

void main_remotewakeup_lpm_enable(void)
{
	ui_wakeup_enable();
}
#endif

bool main_cdc_enable(uint8_t port)
{
	main_b_cdc_enable = true;
	// Open communication
	uart_open(port);
	return true;
}

void main_cdc_disable(uint8_t port)
{
	main_b_cdc_enable = false;
	// Close communication
	uart_close(port);
}

void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if (b_enable) {
		// Host terminal has open COM
		ui_com_open(port);
	}else{
		// Host terminal has close COM
		ui_com_close(port);
	}
}
//変更しない部分　終了

// Binary Valiable ->  HEX String
// リトルエンディアン
//1バイト単位に分解された数値データを最下位のバイト（LSB）から順番にメモリに格納していく方式

void Bin2Hex( char *SrcVal_ptr, short SrcVal_Size,  char *dst_buf) //*SrcVal_ptr ソースのバイナリデータを指すポインタ　　| SrcVal_Size 変換するバイナリデータのサイズ（バイト数）| *dst_buf: 結果のHEX ASCII文字列を格納するバッファのポインタ
{
	SrcVal_ptr += SrcVal_Size; //入力バイナリデータの最後を示すようにポインタを増やし（リトルエンディアン形式を前提としているため）、それから1バイトずつ後方に移動しながら各バイトを処理
	
	for( short i=0; i<SrcVal_Size; ++i)
	{//各バイトは2つの4ビットのハーフバイト（ニブル）に分割され、それぞれが個別の16進数の文字に変換される
		--SrcVal_ptr;										
		
		unsigned char tmp_ch = ((*SrcVal_ptr)>>4)&0x0f;		// 上位４Bitを変換 
		if( tmp_ch <= 9) *(dst_buf++)= tmp_ch +'0';
		else *(dst_buf++)= tmp_ch + 'A'-10;

		tmp_ch = (*SrcVal_ptr)&0x0f;						// 下位 4Bitを変換
		if( tmp_ch <= 9) *(dst_buf++)= tmp_ch +'0';
		else *(dst_buf++)= tmp_ch + 'A'-10;
		//上位4ビットと下位4ビットはそれぞれ独立して処理. 0-9の範囲ならそのままASCII数字に変換され、10-15の範囲ならA-Fに変換
	}
	return;
}

// RotaryEncode カウント処理 & カウント値の取得
signed short RE_CNT()
{
	unsigned char A, B; //取得する状態
	static unsigned char A_=0; //前のA相の状態
	static unsigned char B_=0; //前のB相の状態
	static signed short cnt = 0;
	unsigned char tmp_ch;
	
	tmp_ch = PORTA.IN; //
	
	A = (tmp_ch & 0x10)>>4; //A相の部分だけマスクしてbitシフト
	B = (tmp_ch & 0x20)>>5;	//B相の部分だけマスクしてbitシフト
	
	if( A_ != A)
	{
		if( A != 0 )	// A相が0ではない　つまりA相立上り
		{
			if( B == 0)	++cnt;//B相の立ち下がりとA相が0であれば、時計回りに回転していると解釈し、カウンタをインクリメント
			else				--cnt;//B相の立ち下がりとA相が1であれば、反時計回りに回転していると解釈し、カウンタをデクリメント
			
		}
		else//A相が0
		{
			if( B == 0)	--cnt;//逆回転　デクリメント
			else				++cnt;//正転　インクリメント
		}
	}
	
	if( B_ != B)
	{
		if( B != 0 )	// B相が0ではない　つまりB相立上り
		{
			if( A == 0)	--cnt;
			else				++cnt;
			
		}
		else
		{
			if( A == 0)	++cnt;
			else				--cnt;
		}
	}
	
	A_ = A; //A_(前の状態)の更新
	B_ = B; //B_(前の状態)の更新
	
	for( volatile short i=0 ;i<1000; ++i);
	
	return( cnt );
}

//LEDの状態
void LED_STATE(char i)
{
	if (i == '0')
	{
		PORTC.OUT = 0xFF; //全消灯
	}
	else if (i == '1')
	{
		PORTC.OUT = 0x7F;
	}
	else if (i == '2')
	{
		PORTC.OUT = 0x3F;
	}
	else if (i == '3')
	{
		PORTC.OUT = 0x1F;
	}
	else if (i == '4')
	{
		PORTC.OUT = 0x0F;
	}
	else if (i == '5')
	{
		PORTC.OUT = 0x07;
	}
	else if (i == '6')
	{
		PORTC.OUT = 0x03;
	}
	else if (i == '7')
	{
		PORTC.OUT = 0x01;
	}
	else if (i == '8')
	{
		PORTC.OUT = 0x00;
	}
	
}

/**
 * \mainpage ASF USB Device CDC
 *
 * \section intro Introduction
 * This example shows how to implement a USB Device CDC
 * on Atmel MCU with USB module.
 * The application note AVR4907 provides more information
 * about this implementation.
 *
 * \section desc Description of the Communication Device Class (CDC)
 * The Communication Device Class (CDC) is a general-purpose way to enable all
 * types of communications on the Universal Serial Bus (USB).
 * This class makes it possible to connect communication devices such as
 * digital telephones or analog modems, as well as networking devices
 * like ADSL or Cable modems.
 * While a CDC device enables the implementation of quite complex devices,
 * it can also be used as a very simple method for communication on the USB.
 * For example, a CDC device can appear as a virtual COM port, which greatly
 * simplifies application development on the host side.
 *
 * \section startup Startup
 * The example is a bridge between a USART from the main MCU
 * and the USB CDC interface.
 *
 * In this example, we will use a PC as a USB host:
 * it connects to the USB and to the USART board connector.
 * - Connect the USART peripheral to the USART interface of the board.
 * - Connect the application to a USB host (e.g. a PC)
 *   with a mini-B (embedded side) to A (PC host side) cable.
 * The application will behave as a virtual COM (see Windows Device Manager).
 * - Open a HyperTerminal on both COM ports (RS232 and Virtual COM)
 * - Select the same configuration for both COM ports up to 115200 baud.
 * - Type a character in one HyperTerminal and it will echo in the other.
 *
 * \note
 * On the first connection of the board on the PC,
 * the operating system will detect a new peripheral:
 * - This will open a new hardware installation window.
 * - Choose "No, not this time" to connect to Windows Update for this installation
 * - click "Next"
 * - When requested by Windows for a driver INF file, select the
 *   atmel_devices_cdc.inf file in the directory indicated in the Atmel Studio
 *   "Solution Explorer" window.
 * - click "Next"
 *
 * \copydoc UI
 *
 * \section example About example
 *
 * The example uses the following module groups:
 * - Basic modules:
 *   Startup, board, clock, interrupt, power management
 * - USB Device stack and CDC modules:
 *   <br>services/usb/
 *   <br>services/usb/udc/
 *   <br>services/usb/class/cdc/
 * - Specific implementation:
 *    - main.c,
 *      <br>initializes clock
 *      <br>initializes interrupt
 *      <br>manages UI
 *      <br>
 *    - uart_xmega.c,
 *      <br>implementation of RS232 bridge for XMEGA parts
 *    - uart_uc3.c,
 *      <br>implementation of RS232 bridge for UC3 parts
 *    - uart_sam.c,
 *      <br>implementation of RS232 bridge for SAM parts
 *    - specific implementation for each target "./examples/product_board/":
 *       - conf_foo.h   configuration of each module
 *       - ui.c        implement of user's interface (leds,buttons...)
 */
