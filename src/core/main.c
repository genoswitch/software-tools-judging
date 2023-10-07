// SPDX-License-Identifier: MIT

// FreeRTOS
#include "FreeRTOS.h"
// FreeRTOS version macro (tskKERNEL_VERSION_NUMBER)
#include "task.h"

#include "semaphores.h"

#include "determine_device_type.h"

// Standard libraries
#include "pico/stdio.h"
#include "pico/stdlib.h"

// Accompanying header file
#include "main.h"

// USB FreeRTOS tasks
#include "usb/tasks.h"

#include "tasks/bulk.h"
#include "tasks/mcu_temperature.h"

// Source control information embedded at build-time
#include "git.h"

#include "mcp3008/mcp3008.h"
#include "mcp3008/pio/mcp3008_pio.h"
#include "mcp3008/hardware/mcp3008_hardware.h"

extern void *__BUILD_INCLUDES_FLASHLOADER;

#ifdef INCLUDES_FLASHLOADER
#include "../flashloader/main.h"
// lib/pico-flashloader (FLASH_APP_UPDATED)
#include "flashloader.h"

// RP2040 Watchdog (flashloader will set a scratch register if it
// has updated something)
#include "hardware/watchdog.h"
#endif

int main(void)
{
    // Setup hardware (init stdio, etc.)
    prvSetupHardware();

    printf("-----\nGenoswitch Measurement Platform\n-----\n");

    // Determine and display which device is being used.
    printf("Detected Device Type: %i\n", determineDevice());

#ifdef INCLUDES_FLASHLOADER
    // printf("INCLUDES: Flashloader support!\n");

    // Check if we have just upgraded.
    if (watchdog_hw->scratch[0] == FLASH_APP_UPDATED)
    {
        printf("Application has just updated!\n");
        // Reset the scratch register.
        watchdog_hw->scratch[0] = 0;
    }
#endif

    const int has_flashloader = (int)&__BUILD_INCLUDES_FLASHLOADER;
    printf("Flashloader supported?: %i\n", has_flashloader);

    prvSerialDisplaySystemInfo();

    pvCreateBulkQueue();

    // Init some tasks!

    pvCreateUsbTasks();
    pvRegisterMcuTempTask();

    // Create semaphores (mutex locks)
    pvCreateSemaphores();

#ifdef INCLUDES_FLASHLOADER
    pvRegisterFlashloaderTask();
#endif

    printf("HELLO, world!");

    spi_inst_t *spi = spi0;
    spi_pinout_t pinout = {
        .sck = 2,
        .csn = 5,
        .miso = 4,
        .mosi = 3,
    };
    spi_dual_inst inst = mcp3008_init_hardware(spi, 3600000, &pinout);
    int16_t res = mcp3008_read_hardware(&inst, 1, false);
    printf("\nDATA HW_0: %x", res);

    spi_inst_t *spi2 = spi1;
    spi_pinout_t pinout2 = {
        .sck = 10,
        .csn = 9,
        .miso = 8,
        .mosi = 11,
    };
    spi_dual_inst inst2 = mcp3008_init_hardware(spi2, 3600000, &pinout2);
    int16_t res2 = mcp3008_read_hardware(&inst2, 0, false);
    printf("\nDATA HW_1: %x", res2);

    spi_pinout_t pinout3 = {
        .sck = 14,
        .csn = 13,
        .miso = 12,
        .mosi = 15,
    };
    spi_dual_inst inst3 = mcp3008_init_pio(pio0, 3600000, &pinout3);
    uint16_t res3 = mcp3008_read_pio(&inst3, 1, false);
    printf("\nDATA PIO_0: %x", res3);

    spi_pinout_t pinout4 = {
        .sck = 18,
        .csn = 17,
        .miso = 16,
        .mosi = 19,
    };
    spi_dual_inst inst4 = mcp3008_init_pio(pio1, 3600000, &pinout4);
    uint16_t res4 = mcp3008_read_pio(&inst4, 1, false);
    printf("\nDATA PIO_1: %x", res4);

    printf("\n");

    // TinyUSB demos call this after creating tasks.
    vTaskStartScheduler();

    return 0;
}

static void prvSetupHardware(void)
{
    stdio_init_all(); // init the things uwu
}

// Display hardware and firmware information over standard output.
void prvSerialDisplaySystemInfo(void)
{
    // Determine which RTOS type we are running
    const char *rtos_name;
#if (portSUPPORT_SMP == 1)
    rtos_name = "FreeRTOS SMP";
#else
    rtos_name = "FreeRTOS";
#endif
    printf("%s, %s\n", rtos_name, tskKERNEL_VERSION_NUMBER);

#if (portSUPPORT_SMP == 1) && (configNUM_CORES == 2)
    printf("Running on both cores\n");
#endif

#if (mainRUN_ON_CORE == 1)
    printf("Kernel running on core 1\n");
#else
    printf("Kernel running on core 0\n");
#endif

    if (git_IsPopulated())
    {
        printf("Commit: %s\n", git_CommitSHA1());
        printf("Commit date: %s\n", git_CommitDate());
    }
    else
    {
        printf("This image was built without source control.\n");
    }
}