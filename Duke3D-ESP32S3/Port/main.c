/***************************************************************************//**
 * @file main.c
 * @brief main function for Quake  on EFR32xg24 MCUs
 *
 *  Copyright (C) 2022-2024 by Nicola Wrachien
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *******************************************************************************
 ******************************************************************************/
// BIG NOTE!!! This project is configured for a 20 dBm part, to support
// the Sparkfun Thing Plus Matter board.
// Unfortunately, this means BLE won't work on 10 dBm parts.
// If you have a 10 dBm part, then you'll have to configure the to a 10 dBm part
// (e.g. MGM240PB22VNA).
// Note: conversely, starting from GSDK 4.3 if you configure for 10 dBm,
// it won't work on a 20 dBm part.
// To configure, double click on DoomMG24BLE.slcp, select Oveview on top of
// the window, then on target and tools settings press "Change Target/SDK/Generators".
// Finally select the correct part number and click save.
// -----------------------------------------------------------------------------
//                                   Includes
// -----------------------------------------------------------------------------
#include "main.h"
#if MG24
#include "sl_component_catalog.h"

#include "sl_system_init.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
  #include "sl_power_manager.h"
#endif
#if defined(SL_CATALOG_KERNEL_PRESENT)
  #include "sl_system_kernel.h"
  #include "app_task_init.h"
#else // SL_CATALOG_KERNEL_PRESENT
  #include "sl_system_process_action.h"
#endif // SL_CATALOG_KERNEL_PRESENT
#include "sl_event_handler.h"
#include "keyboard.h"
#include "delay.h"
#include "sharedUsart.h"

#include "audio.h"
#include "extMemory.h"
#include "extMemory.h"
#include "em_cmu.h"
#include "boards.h"
#include "graphics.h"
#include "diskio.h"
#include "i_memory.h"
unsigned int I_GetTimeMicrosecs(void);
#define KEY_COMBINATION_FOR_GRP_UPLOAD        (KEY_ALT | KEY_FIRE | KEY_UP | KEY_DOWN)
#if BOARD == BOARD_THEGAMEPAD_ARDUINONANOMATTER
#define KEY_COMBINATION_FOR_ERASE_OPTIONS     (KEY_DOWN | KEY_R_DOWN | KEY_LT_DOWN | KEY_LT_UP | KEY_RT_DOWN | KEY_RT_UP)
#else
#define KEY_COMBINATION_FOR_ERASE_OPTIONS     (KEY_DOWN | KEY_USE | KEY_LEFT | KEY_RIGHT | KEY_ALT)
#endif
#include "sl_device_init_dpll_config.h"
#if MG24
#include "printf.h"
#endif
#include "sl_device_init_dcdc.h"
#include "sl_device_init_hfxo.h"
#include "sl_device_init_dpll.h"
#include "sl_device_init_clocks.h"
#include "em_chip.h"
#include "radio.h"
#include "sl_memory_manager_region_config.h"
#include "sl_common.h"
#include "game.h"
#include "duke3d.h"
#define LED_R_PIN       1
#define LED_G_PIN       2
#define LED_B_PIN       3

// -----------------------------------------------------------------------------
//                              Macros and Typedefs
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                          Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                                Static Variables
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//                          Public Function Definitions
// -----------------------------------------------------------------------------
/******************************************************************************
 * Main function
 *****************************************************************************/
void dumpToDisplay(int start)
{

  extMemSetCurrentAddress(start);
  for (int i = 0; i < 40; i++)
  {
    char linebuffer[9+8] = {0};
    char clinebuffer[8];
    char printbuffer[43];
    extMemGetDataFromCurrentAddress(linebuffer, 8);
    linebuffer[8] = 0;
    memcpy(clinebuffer, linebuffer, 8);
    for (int x = 0; x < 8; x++)
    {
      if (linebuffer[x] < 32 || linebuffer[x] > 127)
        linebuffer[x] = '.';
      snprintf(printbuffer, sizeof(printbuffer), "0x%08x: %02x%02x%02x%02x%02x%02x%02x%02x   %s\r\n",
               start + i*8,
               clinebuffer[0],
               clinebuffer[1],
               clinebuffer[2],
               clinebuffer[3],
               clinebuffer[4],
               clinebuffer[5],
               clinebuffer[6],
               clinebuffer[7],
               linebuffer);
    }
    printf(printbuffer);
    displayPrintln(1, "%s", printbuffer );
  }
}
void overclock(void)
{

  CMU_DPLLInit_TypeDef dpll_init = {
    .frequency = 80000000,    // this is just to shut up the asserts.
    .n = OC_PLLN,
    .m = OC_PLLD,
    .refClk = SL_DEVICE_INIT_DPLL_REFCLK,
    .edgeSel = SL_DEVICE_INIT_DPLL_EDGE,
    .lockMode = SL_DEVICE_INIT_DPLL_LOCKMODE,
    .autoRecover = SL_DEVICE_INIT_DPLL_AUTORECOVER,
    .ditherEn = SL_DEVICE_INIT_DPLL_DITHER
  };

  CMU_Select_TypeDef selected_sysclk = CMU_ClockSelectGet(cmuClock_SYSCLK);

  if (selected_sysclk == cmuSelect_HFRCODPLL) {
    // From Reference Manual:
    // The CMU should not be running from the HFRCO. If necessary, the CMU
    // should switch to the FSRCO until after the DPLL has locked to avoid
    // over-clocking due to overshoot.
    CMU_CLOCK_SELECT_SET(SYSCLK, FSRCO);
  }

  CMU_ClockEnable(cmuClock_DPLL0, true);
#if 0
  CMU_DPLLLock(&dpll_init);
#else
  bool hclkDivIncreased = false;
  uint32_t hfrcoCalVal, lockStatus = 0, hclkDiv = 0;
  uint32_t hfrcoFreqRangeExpected;
  uint32_t hfrcoFreqRangeActual;
  uint32_t hfrcoCalCurrent;
  bool hfrcoClamped = false;
  bool restoreDpll;

  CMU->CLKEN0_SET = CMU_CLKEN0_DPLL0 | CMU_CLKEN0_HFRCO0;

  restoreDpll = DPLL0->EN & _DPLL_EN_EN_MASK;

  // Make sure DPLL is disabled before configuring
  DPLL0->EN_CLR = DPLL_EN_EN;
#if defined(DPLL_EN_DISABLING)
  while (DPLL0->EN & DPLL_EN_DISABLING) {
  }
#else
  while ((DPLL0->STATUS & (DPLL_STATUS_ENS | DPLL_STATUS_RDY)) != 0UL) {
  }
#endif


  hfrcoCalVal = HFRCOCAL_VALUE;

  // updates to the CAL register are deferred if FREQBSY is high, so wait
  // until HFRCO is not busy to keep going
  while (HFRCO0->STATUS & (HFRCO_STATUS_SYNCBUSY | HFRCO_STATUS_FREQBSY)) {
  }

  /*
   * Some devices have clamped frequency ranges, so instead of the usual [0:16]
   * interval, the upper limit is 12. Hardware takes care of clamping the value,
   * but a situation might occur where tuning and frequency range are not
   * in sync. So try to detect if the value has been clamped, and if it happened
   * revert back to the previous value.
   */
  hfrcoCalCurrent = HFRCO0->CAL;
  HFRCO0->CAL = hfrcoCalVal;

  // values are not shifted, not necessary for comparison
  hfrcoFreqRangeExpected = (hfrcoCalVal & _HFRCO_CAL_FREQRANGE_MASK);
  hfrcoFreqRangeActual   = (HFRCO0->CAL & _HFRCO_CAL_FREQRANGE_MASK);

  if (hfrcoFreqRangeExpected == hfrcoFreqRangeActual) {
    DPLL0->CFG1   = ((uint32_t)dpll_init.n   << _DPLL_CFG1_N_SHIFT)
                    | ((uint32_t)dpll_init.m << _DPLL_CFG1_M_SHIFT);

    /* For series 2 silicon, macro expansion is used to select clock
     * sources since it results in less code size when compared to the legacy
     * CMU_ClockSelectSet function.
     */
      CMU_CLOCK_SELECT_SET(DPLLREFCLK, HFXO);

    DPLL0->CFG = ((dpll_init.autoRecover ? 1UL : 0UL) << _DPLL_CFG_AUTORECOVER_SHIFT)
                 | ((dpll_init.ditherEn ? 1UL : 0UL)  << _DPLL_CFG_DITHEN_SHIFT)
                 | ((uint32_t)dpll_init.edgeSel  << _DPLL_CFG_EDGESEL_SHIFT)
                 | ((uint32_t)dpll_init.lockMode << _DPLL_CFG_MODE_SHIFT);

  } else {
    hfrcoClamped = true;
    HFRCO0->CAL = hfrcoCalCurrent;
  }

  /*
   * if HFRCO frequency range has been clamped, re-enable DPLL only if it was
   * previously enabled
   */
  if (!hfrcoClamped || restoreDpll) {
    DPLL0->IF_CLR = DPLL_IF_LOCK | DPLL_IF_LOCKFAILLOW | DPLL_IF_LOCKFAILHIGH;

    // Lock DPLL
    DPLL0->EN_SET = DPLL_EN_EN;
    while ((lockStatus = (DPLL0->IF & (DPLL_IF_LOCK
                                       | DPLL_IF_LOCKFAILLOW
                                       | DPLL_IF_LOCKFAILHIGH))) == 0UL) {
    }
  }

  if (CMU_ClockSelectGet(cmuClock_SYSCLK) == cmuSelect_HFRCODPLL) {
    if (hclkDivIncreased) {
      // Restore original HCLK divider
      CMU_ClockDivSet(cmuClock_HCLK, hclkDiv);
    }

    // Call @ref SystemCoreClockGet() to update CMSIS core clock variable.
    SystemCoreClockGet();
  }
#endif
  CMU_CLOCK_SELECT_SET(SYSCLK, HFRCODPLL);
  //
  // mandatory for RAIL operation
  SystemHFRCODPLLClockSet(CPU_FREQUENCY);
}
#if PROFILE_STACK
extern uint32_t __StackLimit[];                // not really an array!
#endif
void checkForEraseOptions(int c)
{
    if ((c & KEY_COMBINATION_FOR_ERASE_OPTIONS) == (KEY_COMBINATION_FOR_ERASE_OPTIONS))
    {
        nvmData_t *nvm = (nvmData_t*) ((byte*) extMemGetSize() - sizeof(nvmData_t));
        extMemErase((uint32_t)&nvm->settings, sizeof(nvm->settings));
        displayPrintln(1, "Erased options.");
        extMemErase((uint32_t)&nvm->savegames, sizeof(nvm->savegames));
        displayPrintln(1, "Erased savegames.");
    }
}
void checkForDataFileUpdate(int c)
{
  if ((c & KEY_COMBINATION_FOR_GRP_UPLOAD) == (KEY_COMBINATION_FOR_GRP_UPLOAD))
  {
    // let's first try to mount SD
#if HAS_SD
      // let's first check if there is a SD card on the disk
      displayPrintln(1, "Trying to init SD CARD.");
      FATFS fs;
      uint32_t stat =  f_mount(&fs, "", 1);
      printf("f_mount() %d\r\n", stat);
      if (stat == 0)
      {
        displayPrintln(1, "SD Card init successful!");
        displayPrintln(1, "Opening " DATA_FILE_NAME "...");

        FIL fil;
        stat = f_open(&fil, "0:" DATA_FILE_NAME, FA_READ);
        int32_t fsize = fil.obj.objsize;
        printf("f_open() %d, size %d\r\n", stat, fsize);
        if (stat == 0)
        {

          displayPrintln(1, "Success! size: %d bytes", fsize);
          if (fsize > 0)
          {
            uint32_t address = (uint32_t) GRP_ADDRESS;
            displayPrintln(1, "Erasing flash, please wait");
            displayPrintln(1, "(this might take up to 100-200 seconds)");
            SET_FLASH_MODE();
            extMemEraseAll();
            displayPrintln(1, "Programming, please wait");
            int i = 0;
            while (fsize > 0)
            {
              UINT br = 0;
              uint32_t  btr = fsize;
#define MAX_STACK_BUFFER 16384
              uint32_t maxbtr =  MAX_STACK_BUFFER & ~4095;
              if (btr > maxbtr)
              {
                btr = maxbtr;
              }
              uint8_t buffer[MAX_STACK_BUFFER];  // we have plenty of stack here.
              stat = f_read(&fil, buffer , btr, &br);
              if (!br || stat)
              {
                displayPrintln(1, "Error, read 0 bytes, stat %d", stat);
                break;
              }
              extMemProgram(address, buffer, br);
              fsize -= br;
              address += br;
              if (!(i % 8)) // displayPrintln takes time. Let's print only 1 over 8
              {
                  displayPrintln(1, "%d bytes remaining...", fsize);
              }
              i++;
            }
            displayPrintln(1, "Game data copy success!");
          }
          else
          {
            displayPrintln(1, "Error, non-positve size!", fsize);
          }
        }
        else
        {
          displayPrintln(1, "Can't open file " DATA_FILE_NAME "!");
        }
      }
      else
      {
        displayPrintln(1, "Can't open SD card.");
      }

#endif
      displayPrintln(1, "Reset in 2 seconds!");
      delay(2000);
      NVIC_SystemReset();
  }
}


int main(void)
{

#if PROFILE_STACK
  for (int i = 0; i < SL_STACK_SIZE / 4; i++)
  {
    __StackLimit[i] = 0xEFBEADDE;  // stack canary.
  }
#endif
  // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
  // instead of calling sl_platform_init() we are explicitly calling the functions.
  // this is because overclock must be called just before the rail init.
  // Also, we don't want any bloatware to be present such as sl_gpio.c
  //sl_platform_init();
  CHIP_Init();
  // WE ARE NOT USING INTERRUPT MANAGER
  //  sl_interrupt_manager_init();
  sl_device_init_dcdc();
  sl_device_init_hfxo();
  sl_device_init_dpll();
  sl_device_init_clocks();
  // enabling GPIO clock just because we might need some LEDs for debug...
  // As Arduino Nano Matter Board does not support JLINK.
  CMU_ClockEnable(cmuClock_GPIO, true);
  // WE ARE NOT USING EMU
  //sl_device_init_emu();
  // DRIVERS? NO THANK YOU
  //  sl_driver_init();

    // SERVICES? Not used.
  //sl_service_init();

#if OVERCLOCK
  overclock();
#endif


  sl_stack_init();

  // NOT USED
  //sl_internal_app_init();
  //
  // Enable clocks and overclock PCLK so it is the same as CPU clk
  CMU_ClockDivSet(cmuClock_PCLK, 1);
  const uint32_t clocks[] =
  {
      cmuClock_EUSART0, cmuClock_EUSART1, cmuClock_PRS, cmuClock_LDMA,
      cmuClock_LDMAXBAR, cmuClock_SYSCFG, cmuClock_USART0, CAT(cmuClock_TIMER, TICK_TIMER_NUMBER),
      cmuClock_GPIO, cmuClock_ICACHE, cmuClock_DMEM, cmuClock_MSC
  };
  for (unsigned int i = 0; i < sizeof (clocks) / sizeof(clocks[0]); i++)
  {
    CMU_ClockEnable(clocks[i], true);
  }
  //
  // Enable TICK_TIMER for generic delay
  TICK_TIMER->CFG = TIMER_CFG_PRESC_DIV8;
  TICK_TIMER->EN = TIMER_EN_EN;
  TICK_TIMER->TOP = 0xFFFFFFFF;
  TICK_TIMER->CMD = TIMER_CMD_START;
  // Enable TIME TIMER for ms delay
  //TIME_TIMER->CFG = TIMER_CFG_PRESC_DIV1024;
  //TIME_TIMER->EN = TIMER_EN_EN;
  //TIME_TIMER->TOP = 0xFFFFFFFF;
  //TIME_TIMER->CMD = TIMER_CMD_START;
  //
  FLASH_NCS_HIGH();
  //
#ifdef VCOM_ENABLE_PORT
  GPIO_PinModeSet(VCOM_ENABLE_PORT, VCOM_ENABLE_PIN, gpioModePushPull, 1);
#endif
  GPIO_PinModeSet(VCOM_TX_PORT, VCOM_TX_PIN, gpioModePushPull, 1);
  GPIO_PinModeSet(VCOM_RX_PORT, VCOM_RX_PIN, gpioModePushPull, 1);
  //
  displayInit();
  initGraphics();
  displayPrintln(0, "Duke Nukem 3D on EFR32MG24");
  displayPrintln(0, "by Nicola Wrachien");
  displayPrintln(1, "Build date %s", __DATE__);
  displayPrintln(1, "Build time %s", __TIME__);
  //
  // measure refresh time!
  //
  while(displayData.dmaBusy)
  {
      // wait
  }
  // get time
  uint32_t oldTime = I_GetTimeMicrosecs();
  startDisplayRefresh(0, 240);
  while(displayData.dmaBusy)
  {
      // wait till the end of display refresh.
  }
  oldTime = I_GetTimeMicrosecs() - oldTime;

  displayPrintln(1, "Frame refresh time %d us!", oldTime);
  displayPrintln(1, "Code Size: %d bytes", FLASH_CODE_SIZE);
  displayPrintln(1, "Trying to read external flash...");
  delay(500);
  extMemInit();
  displayPrintln(1, "SPI Flash Size: %d MB.", extMemGetSize() / 1048576);
  displayPrintln(1, "HFXO: %d Hz", SystemHFXOClockGet());
  displayPrintln(1, "HCLK: %d Hz", SystemHCLKGet());
  FLASH_NCS_HIGH();
  displayPrintln(1, "");
#if BOARD == BOARD_THEGAMEPAD_ARDUINONANOMATTER
    displayPrintln(1, "Press UP & DOWN & 4 & 3");

    #else
    displayPrintln(1, "Press ALT & FIRE & UP & DOWN");
#endif
  displayPrintln(1, "to start GRP installation.");
#if BOARD == BOARD_THEGAMEPAD_ARDUINONANOMATTER
    displayPrintln(1, "Press all the trigger buttons and");
    displayPrintln(1, "down and 3 to erase options & savegames.");
#endif
  delay(200);
  FLASH_NCS_LOW();
  SET_FLASH_MODE();
#if 0 // test speed
  int t = TICK_TIMER->CNT;
#define TESTSIZE 256
  char cc[TESTSIZE]; int n[TESTSIZE];
  for (int i = 0; i < sizeof(cc); i++)
  {
    interleavedSpiFlashAsynchReadByteDMA(i + 16);
    cc[i] = interleavedSpiFlashGetAsynchReadByteDMA();

    //cc[i] = interleavedSpiFlashReadByte(i+16);
    n[i] = TICK_TIMER->CNT;
  }
  for (int i = 0; i < sizeof(cc); i++)
  {
    printf("Char %d: 0x%02x %c time: %d\r\n", i + 16, cc[i], cc[i] >= 32? cc[i] : '.', n[i] - t);
  }
  while(1);
#endif
#if 0   // for testing
  #define BSIZE 1024
  for (int i = 0; i < 1; i++)
  {
    memset(interleavedSpiData.rxBuffer, 0xaa, sizeof(interleavedSpiData.rxBuffer));

    interleavedSpiFlashStartRead(4 + i * BSIZE, &interleavedSpiData.rxBuffer[0], BSIZE);
    interleavedSpiWaitDma();
    for (int y = 0; y < BSIZE/16; y++)
    {
      printf("\r\n%08x:  ", i * BSIZE  + y * 16);
      for (int x = 0; x < 16; x++)
      {
        uint8_t c = interleavedSpiData.rxBuffer[8 + y * 16 + x];
        printf("%02x", c);
      }
      printf(" : ");
      for (int x = 0; x < 16; x++)
      {
        char c = interleavedSpiData.rxBuffer[8 + y * 16 + x];
        if ( c >= 32  && c <= 127)
        {
          printf("%c", c);
        }
        else
        {
          printf(".");
        }
      }
    }
  }
#endif
  //
  // enable radio
  radioInit();
  // Let's check if we shall go in GRP upload mode
  initKeyboard();
  uint16_t c;
  getKeys(&c);

  displayPrintln(1, "Key Pressed: 0x%04x", c);
  //
  checkForEraseOptions(c);
  //
  checkForDataFileUpdate(c);
  GPIO_PinModeSet(gpioPortC, LED_R_PIN, gpioModePushPull, 0);

// dumpToDisplay(0x1282C373 + 921);
// dumpToDisplay(0x1282C373 + 0x300);

 //while(1);
  //
#if BOARD == BOARD_THEGAMEPAD_ARDUINONANOMATTER
#define TEST_JOYSTICK 0
#if TEST_JOYSTICK
  testJoystick();
#endif
#endif
  //
  SET_FLASH_MODE();
  //
  //    turn off SD
  GPIO_PinModeSet(SD_NCS_PORT, SD_NCS_PIN, gpioModePushPull, 1);
  //
  dn3dmain(0, NULL);
}
#if DEBUG_HARDFAULT
void HardFault_Handler(void)
{

    __asm volatile (
        "TST LR, #4 \n"
        "ITE EQ \n"
        "MRSEQ R0, MSP \n"
        "MRSNE R0, PSP \n"
        "B hard_fault_handler_c \n"
    );
}

__attribute__((used)) void hard_fault_handler_c(uint32_t *fault_stack_address)
{
    do
    {
        uint32_t r0  = fault_stack_address[0];
        uint32_t r1  = fault_stack_address[1];
        uint32_t r2  = fault_stack_address[2];
        uint32_t r3  = fault_stack_address[3];
        uint32_t r12 = fault_stack_address[4];
        uint32_t lr  = fault_stack_address[5];
        uint32_t pc  = fault_stack_address[6];
        uint32_t psr = fault_stack_address[7];

        uint32_t cfsr  = *((volatile uint32_t *)0xE000ED28);
        uint32_t hfsr  = *((volatile uint32_t *)0xE000ED2C);
        uint32_t dfsr  = *((volatile uint32_t *)0xE000ED30);
        uint32_t afsr  = *((volatile uint32_t *)0xE000ED3C);
        uint32_t mmfar = *((volatile uint32_t *)0xE000ED34);
        uint32_t bfar  = *((volatile uint32_t *)0xE000ED38);

        printf("HardFault detected!\r\n");
        printf("R0  = 0x%08lX\r\n", r0);
        printf("R1  = 0x%08lX\r\n", r1);
        printf("R2  = 0x%08lX\r\n", r2);
        printf("R3  = 0x%08lX\r\n", r3);
        printf("R12 = 0x%08lX\r\n", r12);
        printf("LR  = 0x%08lX\r\n", lr);
        printf("PC  = 0x%08lX\r\n", pc);
        printf("PSR = 0x%08lX\r\n", psr);

        printf("CFSR = 0x%08lX\r\n", cfsr);
        printf("HFSR = 0x%08lX\r\n", hfsr);
        printf("DFSR = 0x%08lX\r\n", dfsr);
        printf("AFSR = 0x%08lX\r\n", afsr);
        printf("MMFAR = 0x%08lX\r\n", mmfar);
        printf("BFAR  = 0x%08lX\r\n", bfar);

        // Decode CFSR
        uint8_t mmfsr = (uint8_t)(cfsr & 0xFF);
        uint8_t bfsr  = (uint8_t)((cfsr >> 8) & 0xFF);
        uint16_t ufsr = (uint16_t)((cfsr >> 16) & 0xFFFF);

        printf("\r\nMemory Management Faults:\r\n");
        if (mmfsr & (1 << 0)) printf("- Instruction access violation\r\n");
        if (mmfsr & (1 << 1)) printf("- Data access violation\r\n");
        if (mmfsr & (1 << 3)) printf("- Unstacking error during exception return\r\n");
        if (mmfsr & (1 << 4)) printf("- Stacking error during exception entry\r\n");
        if (mmfsr & (1 << 5)) printf("- MMFAR holds valid fault address: 0x%08lX\r\n", mmfar);

        printf("\r\nBus Faults:\r\n");
        if (bfsr & (1 << 0)) printf("- Instruction bus error\r\n");
        if (bfsr & (1 << 1)) printf("- Precise data bus error\r\n");
        if (bfsr & (1 << 2)) printf("- Imprecise data bus error\r\n");
        if (bfsr & (1 << 3)) printf("- Unstacking error during exception return\r\n");
        if (bfsr & (1 << 4)) printf("- Stacking error during exception entry\r\n");
        if (bfsr & (1 << 7)) printf("- BFAR holds valid fault address: 0x%08lX\r\n", bfar);

        printf("\r\nUsage Faults:\r\n");
        if (ufsr & (1 << 0)) printf("- Undefined instruction\r\n");
        if (ufsr & (1 << 1)) printf("- Invalid state\r\n");
        if (ufsr & (1 << 2)) printf("- Invalid PC load by EXC_RETURN\r\n");
        if (ufsr & (1 << 3)) printf("- No coprocessor\r\n");
        if (ufsr & (1 << 8)) printf("- Unaligned access\r\n");
        if (ufsr & (1 << 9)) printf("- Divide by zero\r\n");

        printf("\r\nHardFault Status:\r\n");
        if (hfsr & (1 << 1)) printf("- Vector table read fault\r\n");
        if (hfsr & (1 << 30)) printf("- Forced HardFault (escalated from other fault)\r\n");

        printf("\r\nDebug Faults:\r\n");
        if (dfsr & (1 << 0)) printf("- Halt requested by debugger\r\n");
        if (dfsr & (1 << 1)) printf("- Breakpoint triggered\r\n");
        if (dfsr & (1 << 2)) printf("- Data watchpoint triggered\r\n");
        if (dfsr & (1 << 3)) printf("- Debug event occurred\r\n");
        if (dfsr & (1 << 4)) printf("- Vector catch triggered\r\n");

        printf("End of fault list\r\n");
        delay(5000);
    } while (1); // Halt system
}
#else
void HardFault_Handler(void)
{
    while(1)
    {
        printf("Hardfault\r\n");
        delay(5000);
    }
}
#endif

unsigned int I_GetTimeMicrosecs(void)
{
    return TICK_TIMER->CNT * (1000000.0f / TICK_TIMER_FREQUENCY_HZ);
}
// Override this crap function.
void __assert_func( const char *filename, int line, const char *assert_func, const char *expr )
{
    UNUSED(filename);
    UNUSED(line);
    UNUSED(assert_func);
    UNUSED(expr);
    while(1);
}
// Override also sl_gpio, taking 360 bytes for nothing.
sl_status_t __wrap_sl_gpio_toggle_pin()
{
    return 0;
}
sl_status_t __wrap_sl_gpio_clear_pin()
{
    return 0;

}
sl_status_t __wrap_sl_gpio_set_pin()
{
    return 0;

}
sl_status_t __wrap_sl_gpio_set_pin_mode()
{
    return 0;

}
// -----------------------------------------------------------------------------
//                          Static Function Definitions
// -----------------------------------------------------------------------------
#endif
