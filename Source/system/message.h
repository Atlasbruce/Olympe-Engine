#pragma once

#include "system_consts.h"
#include <string>
#include <cstdint>

// Message struct in the global namespace. ECS-first event architecture with domain routing.
// Generic payload fields support input events and other event types without platform dependencies.
struct Message
{
	EventType msg_type = EventType::Olympe_EventType_Any; // message identifier (transition: kept for compatibility)
	EventDomain domain = EventDomain::Gameplay; // domain for routing to appropriate systems

    uint64_t targetUid = 0; // target object UID for operations (create/destroy/add property)

    // Generic integer / float payload fields. For input events these are used as:
    //  - deviceId : joystick instance id, keyboard id (-1), mouse id
    //  - controlId: button index or axis index or scancode
    //  - state    : button pressed (1) / released (0) or other integer state
    //  - param1   : axis value normalized to [-1,1] or primary float payload
    //  - param2   : secondary float payload (e.g. mouse Y coordinate)
    int deviceId = -1;
    int controlId = -1;
    int state = 0;
    float param1 = 0.0f;
    float param2 = 0.0f;

	static Message Create(
		EventType _ev_t,
		EventDomain _domain,
		int _d_id = -1,
		int _c_id = -1, 
		uint64_t _t_uid = 0
	)
	{
		Message msg;
		msg.msg_type = _ev_t;
		msg.domain = _domain;
		msg.deviceId = _d_id;
		msg.controlId = _c_id;
		msg.targetUid = _t_uid;
		return msg;
	}
};
