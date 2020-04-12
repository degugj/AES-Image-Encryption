#include <stdio.h>
#include <stdlib.h>

#include "aes.c"


#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000
#define SWITCH_BASE    		  0xFF200040
#define CHAR_BASE    		  0x09000000

/* This program demonstrates the use of the D5M camera with the DE1-SoC Board
 * It performs the following: 
 * 	1. Capture one frame of video when any key is pressed.
 * 	2. Display the captured frame when any key is pressed.		  
*/
/* Note: Set the switches SW1 and SW2 to high and rest of the switches to low for correct exposure timing while compiling and the loading the program in the Altera Monitor program.
*/


int main(void) {
		
		volatile int * KEY_ptr				= (int *) KEY_BASE;
		volatile int * Video_In_DMA_ptr	= (int *) VIDEO_IN_BASE;
		volatile short * Video_Mem_ptr	= (short *) FPGA_ONCHIP_BASE;
		volatile int * Switch_ptr = (int *) SWITCH_BASE;
		volatile uint8_t * Char_ptr = (uint8_t *) 0xC9000000;
		
		
		// using AES encryption and decryption
		
		// 128 bit key
		uint8_t key[16] =        { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16,
								   (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6, 
								   (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88, 
								   (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };
								 
		// placeholder for plaintext
		uint8_t plaintext[16] =  { (uint8_t) 0x6b, (uint8_t) 0xc1, (uint8_t) 0xbe, (uint8_t) 0xe2, 
								   (uint8_t) 0x2e, (uint8_t) 0x40, (uint8_t) 0x9f, (uint8_t) 0x96,
								   (uint8_t) 0xe9, (uint8_t) 0x3d, (uint8_t) 0x7e, (uint8_t) 0x11, 
								   (uint8_t) 0x73, (uint8_t) 0x93, (uint8_t) 0x17, (uint8_t) 0x2a };
								   
		// init AES with key
		struct AES_ctx ctx;
		AES_init_ctx(&ctx, key);
		
		// ecb
		//AES_ECB_encrypt(&ctx, plaintext);
		//AES_ECB_decrypt(&ctx, plaintext);
		
		// cbc , note that 16 is the blockchain length, do not change
		AES_CBC_encrypt_buffer(&ctx, plaintext,16);
		AES_CBC_decrypt_buffer(&ctx, plaintext,16);
		
		
		int offset;
		int x;
		int y;
		int isEncrypted = 0;
		
		char *text_ptr = "Unencrypted Image       ";
		
		while (1) {
			
			if (*(Switch_ptr) & 0b00000001) {	
				text_ptr = "Unencrypted Video      ";			
				*(Video_In_DMA_ptr + 3)	= 0x4;						// Enable the video
			}else{
				if(isEncrypted == 0){
					text_ptr = "Unencrypted Image";
				}
				*(Video_In_DMA_ptr + 3)	= 0x0; // freezes
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short temp2 = *(Video_Mem_ptr + (y << 9 ) + x);
						*(Video_Mem_ptr + (y << 9) + x) = temp2;
					}
				}
			}		
			
			
			if (*(KEY_ptr) == 0b1000) {
				// Encrypt ECB
				text_ptr = "ECB Encrypted        ";
				int i = 0;
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short currPixel = *(Video_Mem_ptr + (y << 9 ) + x);
						plaintext[i] = (currPixel & 0xff00) >> 8;
						plaintext[i+1] = currPixel & 0x00ff;
						i=i+2;
						if(i>15){
							i=0;
							AES_ECB_encrypt(&ctx, plaintext);
							int j = 0;
							int k;
							for (k=(x-7); k<=x; k++) {
								
								short returnPixel= (plaintext[j]<<8) | plaintext[j+1];
								*(Video_Mem_ptr + (y << 9) + k) = returnPixel;
								j=j+2;						
							}
						}
					}
				}
			isEncrypted = 1;
				
			}
			
			if (*(KEY_ptr) == 0b0100) {
				// Decrypt ECB
				int i = 0;
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short currPixel = *(Video_Mem_ptr + (y << 9 ) + x);
						plaintext[i] = (currPixel & 0xff00) >> 8;
						plaintext[i+1] = currPixel & 0x00ff;
						i=i+2;
						if(i>15){
							i=0;
							AES_ECB_decrypt(&ctx, plaintext);
							int j = 0;
							int k;
							for (k=(x-7); k<=x; k++) {
								
								short returnPixel= (plaintext[j]<<8) | plaintext[j+1];
								*(Video_Mem_ptr + (y << 9) + k) = returnPixel;
								j=j+2;						
							}
						}
					}
				}
				isEncrypted = 0;
			}
				
			if (*(KEY_ptr) == 0b0010) {
				// Encrypt CBC
				text_ptr = "CBC Encrypted       ";
				int i = 0;
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short currPixel = *(Video_Mem_ptr + (y << 9 ) + x);
						plaintext[i] = (currPixel & 0xff00) >> 8;
						plaintext[i+1] = currPixel & 0x00ff;
						i=i+2;
						if(i>15){
							i=0;
							AES_CBC_encrypt_buffer(&ctx, plaintext,16);
							int j = 0;
							int k;
							for (k=(x-7); k<=x; k++) {
								
								short returnPixel= (plaintext[j]<<8) | plaintext[j+1];
								*(Video_Mem_ptr + (y << 9) + k) = returnPixel;
								j=j+2;						
							}
						}
					}
				}
				isEncrypted = 1;
			}
			
			if (*(KEY_ptr) == 0b0001) {
				// Decrypt CBC
				int i = 0;
				for (y = 0; y < 240; y++) {
					for (x = 0; x < 320; x++) {
						short currPixel = *(Video_Mem_ptr + (y << 9 ) + x);
						plaintext[i] = (currPixel & 0xff00) >> 8;
						plaintext[i+1] = currPixel & 0x00ff;
						i=i+2;
						if(i>15){
							i=0;
							AES_CBC_decrypt_buffer(&ctx, plaintext,16);
							int j = 0;
							int k;
							for (k=(x-7); k<=x; k++) {
								
								short returnPixel= (plaintext[j]<<8) | plaintext[j+1];
								*(Video_Mem_ptr + (y << 9) + k) = returnPixel;
								j=j+2;						
							}
						}
					}
				}
				isEncrypted = 0;
			}
			
			
			int cordx = 10;
			int cordy = 10;
			offset = (cordy<<7)+cordx;
			if (*(Switch_ptr) & 0b00001000) {
				while (*(text_ptr)) {
					*(Char_ptr + offset) = *(text_ptr); // write to the character buffer
					++text_ptr;
					++offset;
				}	
			}else{
				text_ptr = "                           ";
				while (*(text_ptr)) {
					*(Char_ptr + offset) = *(text_ptr); // write to the character buffer
					++text_ptr;
					++offset;
				}
			}
			
			
		}	

}


