// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once

extern char* MULTICAM_DISCOVER_MSG;
extern char* MULTICAM_ATTACH_MSG;
extern char* MULTICAM_ADVANCE_MSG;
extern char* MULTICAM_KICK_MSG;
extern char* MULTICAM_PING_MSG;
extern char* MULTICAM_PONG_MSG;
extern char* MULTICAM_RESET_MSG;

enum Multicam_keystroke_API {
	MULTICAM_ATTACH_MSG_SUCCESS=0,
	MULTICAM_ATTACH_MSG_NOT_ATTEMPTED=1
};
