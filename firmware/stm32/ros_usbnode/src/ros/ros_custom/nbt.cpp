/*
 * non_blocking_timer.c
 *
 *  Created on: Jun 10, 2018
 *      Author: Itamar Eliakim
 */

#include "stm32f_board_hal.h"
#include "nbt.h"
#include "hal/hal_time.h"

//NBT - Non Blocking Timer
void NBT_init(nbt_t * nbt, uint32_t interval)
{
	nbt->timeout = interval;
	nbt->previousMillis = hal_millis();
}

bool NBT_handler(nbt_t * nbt)
{
	if(hal_millis()-nbt->previousMillis>nbt->timeout){
		nbt->previousMillis = hal_millis();
		return true;
	}

	return false;
}


