/*
 * Id.h
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */

#ifndef INC_ID_H_
#define INC_ID_H_

typedef struct {
	uint8_t Valid;
}id_subtopics;

void* mqtt_id_get_subtopic(const char *subtopic);
void mqtt_id_handler(const char * data, u16_t len , void* subtopics_void);


#endif /* INC_ID_H_ */
