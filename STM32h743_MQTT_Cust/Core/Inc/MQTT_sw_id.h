/*
 * Id.h
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */

#ifndef INC_SW_ID_H_
#define INC_SW_ID_H_
#include "main.h"


void* Id_Subtopics_Handler(const char *subtopic);
void  Id_Data_Handler(const char * data, uint16_t len , void* subtopics_void);


#endif /* INC_SW_ID_H_ */
