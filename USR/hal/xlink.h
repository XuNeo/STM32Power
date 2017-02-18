#ifndef _XLINK_H_
#define _XLINK_H_
#include <stdbool.h>
//#define XLINK_MASTER

#define XLINK_CMD_SYS 0x80
typedef enum
{
	xlink_cmd_user = 0,
	xlink_cmd_nop=XLINK_CMD_SYS|1,
	xlink_cmd_conf_rf = XLINK_CMD_SYS|2,
	xlink_cmd_test = XLINK_CMD_SYS|3
}xlink_cmd_def;

typedef enum
{
	xlink_state_init=0,
	xlink_state_pairing,
	xlink_state_configing,
	xlink_state_testing,
	xlink_state_normal
}xlink_state_def;

typedef struct
{
	xlink_cmd_def cmd;
	void (*cmd_func)(bool tx,unsigned char *data,unsigned char num);
}xlink_cmd_list_def;

void xlink_init(void(*preceived_func)(xlink_cmd_def cmd,unsigned char*,unsigned char),
								bool(*prequest_func)(xlink_cmd_def*user_cmd,unsigned char*,unsigned char*));
void xlink_routin(void);
void xlink_timer_isr(void);
bool xlink_is_busy(void);
bool xlink_write_data(unsigned char*pdata,unsigned char data_num);
xlink_state_def xlink_get_status(void);

#endif
