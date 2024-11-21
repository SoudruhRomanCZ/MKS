/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#include "fsl_power.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
bool password_decrypt(char check[20]);
bool check_password(const char *input, const char *stored);
/*******************************************************************************
 * Variables
 ******************************************************************************/
uint32_t DWT1,DWT2;
bool status;
char password_stored[20] = "1234";
char input[20];
/*******************************************************************************
 * Code
 ******************************************************************************/

bool password_decrypt(char check[20]) {
    // Return true if the passwords match, false otherwise
    return strcmp(check, password_stored) == 0;
}

bool check_password(const char *input, const char *stored) {
    size_t len = strlen(stored);
    bool result = true; // Assume they match initially

    // Compare each character
    for (size_t i = 0; i < len; i++) {
        // If characters differ, set result to false
        if (input[i] != stored[i]) {
            result = false;
        }
        // Introduce a delay (no-op) to ensure constant time
        // This loop does nothing but takes time
        for (volatile int j = 0; j < 100; j++);
    }

    return result;
}

/*!
 * @brief Main function
 */
int main(void)
{
    /* Init board hardware. */
    /* set BOD VBAT level to 1.65V */
    POWER_SetBodVbatLevel(kPOWER_BodVbatLevel1650mv, kPOWER_BodHystLevel50mv, false);
    /* attach main clock divide to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
#if !defined(DONT_ENABLE_FLASH_PREFETCH)
    /* enable flash prefetch for better performance */
    SYSCON->FMCCR |= SYSCON_FMCCR_PREFEN_MASK;
#endif

    while (1){
        PRINTF("\r\nEnter password: ");
        SCANF("%s",input);
    	DWT1 = DWT -> CYCCNT;
    	// Call the password_decrypt function and store the result in status
    	status = password_decrypt(input);
    	DWT2 = DWT -> CYCCNT;

    	PRINTF("\r\ninput: %s",input);
    	if(status != 0){PRINTF("\r\ninput correct");}else{PRINTF("\r\ninput incorrect");}

    	PRINTF("\r\nCycles decrypting password (variable time): %d", DWT2-DWT1);

    	DWT1 = DWT -> CYCCNT;
    	status = check_password(input, password_stored);
    	DWT2 = DWT -> CYCCNT;
    	PRINTF("\r\nCycles checking password (constant time): %d", DWT2-DWT1);
    }
}
