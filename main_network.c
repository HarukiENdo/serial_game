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
	static char tmp_buf[8]; //����肷��8byte�̃f�[�^
	
	PORTC.DIR =0xff; //LED PORT�@�S����
	PORTA.DIR =0x00; //�^�N�gSW PORTA6bit, PORTA7bit��

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
		
		rotary_cnt = RE_CNT();					// ���[�^���G���R�[�_�̃J�E���g�l�擾
		Bin2Hex( &rotary_cnt, sizeof(rotary_cnt), tmp_buf);		// �o�C�i���l(2�i���`���̐��l)��HEX ASCII������ɕϊ�
		SW1 = (PORTA.IN&0x40)>>6;						// SW�P��PORTA��6bit�ځ@6bit�ڂ������o�ł���悤�ɑ����}�X�N�@>>6 6bit�V�t�g
		SW2 = (PORTA.IN&0x80)>>7;						// SW2��PORTA��7bit�ځ@ 7bit�ڂ������o�ł���悤�ɑ����}�X�N�@>>7 7bit�V�t�g
		// SW1�ESW2�̏�Ԃ��������݁@�@�@�Q�[���̃v���O�����ł�SW�̏�����Ԃ�"aa"�Ƃ��Ă���
		if (SW1==0x01) tmp_buf[4] = 'a';                //SW1�������ꂽ�ꍇ�A��tmp_buf[4]��n �Q�[���ł�"na"��SW1�������ꂽ���
		else tmp_buf[4] = 'n';
		if (SW2==0x01) tmp_buf[5] = 'a';                //SW2�������ꂽ�ꍇ�Atmp_buf[5]��n�@�Q�[���ł�"an"��SW2�������ꂽ���
		else tmp_buf[5] = 'n';
		tmp_buf[6] = 0x0d;			// Set CR code �\�L\r ASCII�R�[�h0x0d �L�����b�W���s�̐擪�ɖ߂�
		tmp_buf[7] = 0x0a;			// Set LF code �\�L\n ASCII�R�[�h0x0a �L�����b�W�����̍s�ɐi�߂� #�Q�[���ł�6bit,7bit�͎g�p���Ȃ�
		
		while( udi_cdc_get_nb_received_data() )		// USB CDC�ł̎�M�o�b�t�@���̃f�[�^����Ԃ�
		{
			data = udi_cdc_getc();					// USB CDC�ł̎�M�o�b�t�@����1byte�擾
			if (data == 'NULL')	break; //data���Ȃ��ꍇbreak ����M���[�v���甲����
			LED_STATE(data);
					
			if (!udi_cdc_is_tx_ready()) //USB CDC�ő��M�\���`�F�b�N
			{
				udi_cdc_signal_overrun(); //FIfo full ->���M�ł��Ȃ��̂�Overrun�Ƃ��ď���
			}
			else
			{
				udi_cdc_write_buf(tmp_buf, 8);	// ��L��8byte����������
			}
		}
	}
	
}
//�ύX���Ȃ�����
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
//�ύX���Ȃ������@�I��

// Binary Valiable ->  HEX String
// ���g���G���f�B�A��
//1�o�C�g�P�ʂɕ������ꂽ���l�f�[�^���ŉ��ʂ̃o�C�g�iLSB�j���珇�ԂɃ������Ɋi�[���Ă�������

void Bin2Hex( char *SrcVal_ptr, short SrcVal_Size,  char *dst_buf) //*SrcVal_ptr �\�[�X�̃o�C�i���f�[�^���w���|�C���^�@�@| SrcVal_Size �ϊ�����o�C�i���f�[�^�̃T�C�Y�i�o�C�g���j| *dst_buf: ���ʂ�HEX ASCII��������i�[����o�b�t�@�̃|�C���^
{
	SrcVal_ptr += SrcVal_Size; //���̓o�C�i���f�[�^�̍Ō�������悤�Ƀ|�C���^�𑝂₵�i���g���G���f�B�A���`����O��Ƃ��Ă��邽�߁j�A���ꂩ��1�o�C�g������Ɉړ����Ȃ���e�o�C�g������
	
	for( short i=0; i<SrcVal_Size; ++i)
	{//�e�o�C�g��2��4�r�b�g�̃n�[�t�o�C�g�i�j�u���j�ɕ�������A���ꂼ�ꂪ�ʂ�16�i���̕����ɕϊ������
		--SrcVal_ptr;										
		
		unsigned char tmp_ch = ((*SrcVal_ptr)>>4)&0x0f;		// ��ʂSBit��ϊ� 
		if( tmp_ch <= 9) *(dst_buf++)= tmp_ch +'0';
		else *(dst_buf++)= tmp_ch + 'A'-10;

		tmp_ch = (*SrcVal_ptr)&0x0f;						// ���� 4Bit��ϊ�
		if( tmp_ch <= 9) *(dst_buf++)= tmp_ch +'0';
		else *(dst_buf++)= tmp_ch + 'A'-10;
		//���4�r�b�g�Ɖ���4�r�b�g�͂��ꂼ��Ɨ����ď���. 0-9�͈̔͂Ȃ炻�̂܂�ASCII�����ɕϊ�����A10-15�͈̔͂Ȃ�A-F�ɕϊ�
	}
	return;
}

// RotaryEncode �J�E���g���� & �J�E���g�l�̎擾
signed short RE_CNT()
{
	unsigned char A, B; //�擾������
	static unsigned char A_=0; //�O��A���̏��
	static unsigned char B_=0; //�O��B���̏��
	static signed short cnt = 0;
	unsigned char tmp_ch;
	
	tmp_ch = PORTA.IN; //
	
	A = (tmp_ch & 0x10)>>4; //A���̕��������}�X�N����bit�V�t�g
	B = (tmp_ch & 0x20)>>5;	//B���̕��������}�X�N����bit�V�t�g
	
	if( A_ != A)
	{
		if( A != 0 )	// A����0�ł͂Ȃ��@�܂�A�������
		{
			if( B == 0)	++cnt;//B���̗����������A����0�ł���΁A���v���ɉ�]���Ă���Ɖ��߂��A�J�E���^���C���N�������g
			else				--cnt;//B���̗����������A����1�ł���΁A�����v���ɉ�]���Ă���Ɖ��߂��A�J�E���^���f�N�������g
			
		}
		else//A����0
		{
			if( B == 0)	--cnt;//�t��]�@�f�N�������g
			else				++cnt;//���]�@�C���N�������g
		}
	}
	
	if( B_ != B)
	{
		if( B != 0 )	// B����0�ł͂Ȃ��@�܂�B�������
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
	
	A_ = A; //A_(�O�̏��)�̍X�V
	B_ = B; //B_(�O�̏��)�̍X�V
	
	for( volatile short i=0 ;i<1000; ++i);
	
	return( cnt );
}

//LED�̏��
void LED_STATE(char i)
{
	if (i == '0')
	{
		PORTC.OUT = 0xFF; //�S����
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
