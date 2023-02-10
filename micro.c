#include <MK64F12.h>
void systickMs(int n); // ประกาศ Fuction Delay เพื่อใช้งานใน Code

int main(void)
{
	int i;
	// Enable Clock Section
	SIM->SCGC5 |= (1 << SIM_SCGC5_PORTA_SHIFT) | (1 << SIM_SCGC5_PORTB_SHIFT) | (1 << SIM_SCGC5_PORTD_SHIFT); // Enable Clock ไปที่ port A และ B และ D

	// FTM section
	SIM->SCGC6 |= 0x01000000; // Enable Clock ไปที่ FTM0 (bit ที่ 24)
	SIM->SOPT2 |= 0x00000000; // เลือกสัญญาน Clock เป็น MCGFLLCLK (00) ที่ bit [16,17]

	FTM0->SC = 0x00; // Disable timer ก่อนในระหว่าง config

	// การ Config ค่าหน่วงเวลาของ FTM Set ได้จากการกำหนดค่าเริ้มนับที่ CNTIN และ ค่าสุดท้ายที่จะนับถึงที่ MOD และคำนวนกับการหาร Clock ที่ SC bit [0,1,2]
	// หากต้องการ ค่าหน่วงเวลาที่ 0.1 Second, ใช้ PS = 32, Clock = 20.97*10^6, ค่าเริ้มนับ = 0
	// ## ต้องหาค่า MOD ##
	//  Solution-->
	//  1/X = 0.1 ดังนั้น X = 10
	//  10 = (((20.97*10^6)/32)/MOD)
	//  MOD = 65536 - 1
	//  MOD = 0xFFFF
	// ดังนั้นหากจากเงื่อนใขของเราหากตั้งค่า MOD = 0xFFFF จะได้การหน่วงเวลาต่อรอบ = 0.1 Second

	FTM0->CNTIN = 0x00; // เลือกค่าเริ้มต้น
	FTM0->MOD = 0xFFFF; // Set ว่าจะให้นับถึงค่าใหน
	FTM0->SC = 0x05;	// หารสัญญาน Clock ด้วย 32 (101) ที่ bit [0,1,2]
	FTM0->SC = 0x08;	// Enable Free running mode ที่ bit [3]

	// Buzzer Section
	PORTA->PCR[1] = 0x100; // Set Port A[1] ให้เป็น GPIO

	// switch Section
	PORTB->PCR[3] = 0x100;								   // Set Port  B[3] ให้เป็น GPIO
	PORTB->PCR[10] = 0x100;								   // Set Port B[10] ให้เป็น GPIO
	PORTB->PCR[11] = 0x100;								   // Set Port B[11] ให้เป็น GPIO
	PTB->PDDR = (~(1 << 3)) & (~(1 << 10)) & (~(1 << 11)); // Set Data Direction ให้ PTB[3,10,11] เป็น Input (Set เป็น 0 คือ Input)

	// LED Section
	PORTD->PCR[0] = 0x100; // Set Port D[0] ให้เป็น GPIO
	PORTD->PCR[1] = 0x100; // Set Port D[1] ให้เป็น GPIO
	PORTD->PCR[2] = 0x100; // Set Port D[2] ให้เป็น GPIO
	PORTD->PCR[3] = 0x100; // Set Port D[3] ให้เป็น GPIO
	PTD->PDDR = 0x0F;	   // Set Data Direction ให้ PTD[0,1,2,3] เป็น Output (Set เป็น 1 คือ Output) F = 1111
	PTD->PSOR = 0x0F;	   // Set bit PTD[0,1,2,3] ให้เป็น 1 (ปิด LED ตัวที่ 0,1,2,3) F = 1111

	// Interrupt Setup part Section
	__disable_irq();			 // ก่อน Config ค่า Parameter ต้องปิดการใช้งาน Interrupt ก่อน
	PORTB->PCR[3] &= ~0xF0000;	 // Clear interrupt selection ที่ bit [16,17,18,19]
	PORTB->PCR[10] &= ~0xF0000;	 // Clear interrupt selection ที่ bit [16,17,18,19]
	PORTB->PCR[11] &= ~0xF0000;	 // Clear interrupt selection ที่ bit [16,17,18,19]
	PORTB->PCR[3] |= 0xA0000;	 // เปิดใช้งาน Interrupt Port B[3] ให้เป็น Interrupt Falling Edge (ทำงานเมื่อเป็น 0) ที่ PCR[3] Bit ที่ [16,17,18,19],[0,1,0,1]
	PORTB->PCR[10] |= 0xA0000;	 // เปิดใช้งาน Interrupt Port B[10] ให้เป็น Interrupt Falling Edge (ทำงานเมื่อเป็น 0) ที่ PCR[10] Bit ที่ [16,17,18,19],[0,1,0,1]
	PORTB->PCR[11] |= 0xA0000;	 // เปิดใช้งาน Interrupt Port B[11] ให้เป็น Interrupt Falling Edge (ทำงานเมื่อเป็น 0) ที่ PCR[11] Bit ที่[16,17,18,19],[0,1,0,1]
	NVIC->ISER[1] |= 0x10000000; // เปิดใช้งาน Interrupt ที่โมดูล ISER[1]-> (ตำแหน่งที่ต้องการดูในตาราง) ใน bit ที่ 60 % 32 = 28 (60 คือหมายเลข IRQ ของ Port B)
	__enable_irq();				 // เปิดการใช้งาน Interrupt

	// Main Loop (Contain SysTick)
	while (1)
	{
		for (i = 0; i < 5; i++) // loop สำหรับเช็ค TOF 5 รอบ
		{
			while ((FTM0->SC & 0x80) == 0) // เชค TOF Flag ว่านับเสร็จรึยัง
			{
			}
			FTM0->SC &= ~(0x80); // Clear TOF
		}
		PTD->PTOR = 0x01; // สลับ Bit PTD[0] ไปมาระหว่าง 0 กับ 1 (เปิด , ปิด)
		systickMs(500);	  // Fuction delay ส่งค่า n = 500
	}
}
// Interrupt Working part Section
void PORTB_IRQHandler(void) // Fuction สำหรับ Interrupt Port B
{
	if ((PTB->PDIR & 0x08) == 0) // เช็คหาก Switch A1 ถูกกด (จะเป็น 0 เมื่อถูกกด)
	{
		PTD->PCOR = 0x0E;	// ไห้ LED ดวงที่ 1,2,3 ติด (bit [1,2,3])
		PORTB->ISFR = 0x08; // Clear bit ISF ที่ Port B bit[3] เพื่อให้ไม่เรียกใช้งานต่อเมื่อทำงานเสร็จแล้ว
	}
	if ((PTB->PDIR & 0x400) == 0) // เช็คหาก Switch A2 ถูกกด (จะเป็น 0 เมื่อถูกกด)
	{
		PTA->PDDR |= 0x02;	 // ให้ Buzzer ติด โดยการกำหนดทิศทางไห้เป็น Output (Port A bit [1])
		PORTB->ISFR = 0x400; // Clear bit ISF ที่ Port B bit[8] เพื่อให้ไม่เรียกใช้งานต่อเมื่อทำงานเสร็จแล้ว
	}
	if ((PTB->PDIR & 0x800) == 0) // เช็คหาก Switch A3 ถูกกด (จะเป็น 0 เมื่อถูกกด)
	{
		PTD->PSOR = 0x0E;	 // ไห้ LED ดวงที่ 1,2,3 ดับ (Pot D bit [1,2,3])
		PTA->PDDR &= ~0x02;	 // ให้ Buzzer ดับ โดยการกำหนดทิศทางไห้เป็น Input (Port A bit [1])
		PORTB->ISFR = 0x800; // Clear bit ISF ที่ Port B bit[11] เพื่อให้ไม่เรียกใช้งานต่อเมื่อทำงานเสร็จแล้ว
	}
}
// Delay (SysTick) Section ---> 1 Ms
void systickMs(int n)
{
	int i;
	// การ Set ค่าหน่วงเวลาที่ต้องการ Set ได้จากการกำหนดค่าเริ้มนับที่ Reg RVR โดยคำนวนได้จากสูตรข้างล่าง
	// หากต้องการ การหน่วงเวลารอบละ 1 Ms หรือ 1*10^-3
	// System Clock  / RVR == 1 / เวลาที่ต้องการ
	// 20.48*10^6 / RVR == 1 / 1*10^-3
	// RVR == 20480
	// หลังจากนั้นต้องนำ RVR - 1 เพราะ เรื้มนับจาก 0
	SysTick->LOAD = 20480 - 1;				   // Set ค่า  RVR(ค่าเริ้มนับ)
	SysTick->CTRL = 5;						   // Control Setting 5 ใน Hex คือ 101 (1-ใช้ System Clock 0-No Interrupt 1-Enable Counter) Set ที่ Rex CTRL bit ที่ 0,1,2
	for (i = 0; i < n; i++)					   // ทำการ เช็ค Count Flag  จำนวน n ครั้ง
		while ((SysTick->CTRL & 0x10000) == 0) // เช็ค Count Flag ที่ Reg CTRL ตัวที่ 16 ว่าหากเป็น 1 แล้วให้ออกจาก  Loop
		{
		}
	SysTick->CTRL = 0; // หยุด Timer โดยการ Disable (Enable = 1)
}
