/*
 * Id.h
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */

#ifndef INC_SW_ID_H_
#define INC_SW_ID_H_
#include "main.h"
typedef struct {
	uint8_t Valid;
}id_subtopics;

void* mqtt_id_get_subtopic(const char *subtopic);
void mqtt_id_handler(const char * data, uint16_t len , void* subtopics_void);


#endif /* INC_SW_ID_H_ */
