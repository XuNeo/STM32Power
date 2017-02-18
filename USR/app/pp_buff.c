/*
*	ping pong buffer
*	By xxL
*	2015.7.24
*/

#include "pp_buff.h"
#include "string.h"

void pp_buff_init(pp_buff_def*pp_buff)
{
	memset(pp_buff,0,sizeof(pp_buff_def));
	pp_buff->read_buff_id = 1;
}

unsigned char pp_buff_write(pp_buff_def*pp_buff,unsigned char c)
{
	if(pp_buff->data_count[pp_buff->write_buff_id]< PP_BUFF_SIZE)
	{
		pp_buff->buff[pp_buff->write_buff_id][pp_buff->data_count[pp_buff->write_buff_id]++] = c;
		
		return 1;
	}
	else
		return 0;
}

unsigned char pp_buff_nwrite(pp_buff_def*pp_buff,unsigned char *pc,unsigned char num)
{
	unsigned char data_count_old = pp_buff->data_count[pp_buff->write_buff_id];
	while((pp_buff->data_count[pp_buff->write_buff_id]<PP_BUFF_SIZE) && num)
	{
		pp_buff->buff[pp_buff->write_buff_id][pp_buff->data_count[pp_buff->write_buff_id]++] = *pc++;
		num --;
	}
	return pp_buff->data_count[pp_buff->write_buff_id] - data_count_old;
}

unsigned char pp_buff_read(pp_buff_def*pp_buff,unsigned char *c)
{
	if(pp_buff->data_count[pp_buff->read_buff_id])
	{
		pp_buff->data_count[pp_buff->read_buff_id]--;
		*c = pp_buff->buff[pp_buff->read_buff_id][pp_buff->read_index++];
		return 1;
	}
	else
		return 0;
}

unsigned char pp_buff_nread(pp_buff_def*pp_buff,unsigned char *pc,unsigned char num)
{
	unsigned char data_count_old = pp_buff->data_count[pp_buff->read_buff_id];
	
	while(pp_buff->data_count[pp_buff->read_buff_id]&&num)
	{
		pp_buff->data_count[pp_buff->read_buff_id]--;
		*pc++ = pp_buff->buff[pp_buff->read_buff_id][pp_buff->read_index++];
		num--;
	}
	
	return data_count_old-pp_buff->data_count[pp_buff->read_buff_id];
}

pp_buff_error_def pp_buff_change(pp_buff_def*pp_buff)
{
	if(pp_buff->data_count[pp_buff->write_buff_id])
	{
		//there is some data in write buff	
		pp_buff->data_count[pp_buff->read_buff_id] = 0;
		//switch
		pp_buff->read_buff_id = (pp_buff->read_buff_id+1)&0x01;
		pp_buff->write_buff_id = (pp_buff->write_buff_id+1)&0x01;
		pp_buff->read_index = 0;
		
		return PP_BUFF_EOK;
	}
	else
	{
		//no data, no need to change write buffer
		return PP_BUFF_EEMPTY;
	}
	
}
