/*
 * common_component.h
 *
 *  Created on: 11.05.2017
 *      Author: michaelboeckling
 */

#ifndef _INCLUDE_COMMON_COMPONENT_H_
#define _INCLUDE_COMMON_COMPONENT_H_

typedef enum {
    CS_UNINITIALIZED,
    CS_INITIALIZED,
    CS_RUNNING,
    CS_STOPPED,
    CS_PAUSED
} component_status_t;

#endif /* _INCLUDE_COMMON_COMPONENT_H_ */
