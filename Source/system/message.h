#pragma once

#include "system_consts.h"
#include <string>
#include <cstdint>
#include "SDL_events.h"
#include "windows.h"

// Message struct in the global namespace. It contains an EventStructType so
// existing code that uses "msg.type" continues to work. Additional fields
// are provided for input events (joystick/keyboard/mouse) so message payload
// can be transported via the EventManager.
struct Message
{
	EventStructType struct_type = EventStructType::EventStructType_Olympe;
	EventType msg_type = EventType::Olympe_EventType_Any; // optional message identifier (e.g. OlympeMessage)
	MSG *msg = nullptr; // optional Win32 MSG structure
	SDL_Event* sdlEvent = nullptr; // optional SDL_Event structure

    uint64_t targetUid = 0; // target object UID for operations (create/destroy/add property)

    // Generic integer / float payload fields. For input events these are used as:
    //  - deviceId : joystick instance id, keyboard id, mouse id
    //  - controlId: button index or axis index or scancode
    //  - state    : button pressed (1) / released (0) or other integer state
    //  - value    : axis value normalized to [-1,1] or primary float payload
    //  - value2   : secondary float payload (e.g. mouse Y coordinate)
    int deviceId = -1;
    int controlId = -1;
    int state = 0;
    float param1 = 0.0f;
    float param2 = 0.0f;

	static Message Create(
		EventStructType _st_ev_t,
		EventType _ev_t,
		int _d_id,
		int _c_id, 
		uint64_t _t_uid = 0
	)
	{
		Message msg;
		msg.struct_type = _st_ev_t;
		msg.msg_type = _ev_t;
		msg.deviceId = _d_id;
		msg.controlId = _c_id;
		msg.targetUid = _t_uid;
		return msg;
	}
};
