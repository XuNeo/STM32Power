#ifndef _PP_BUFF_H_
#define _PP_BUFF_H_

#define PP_BUFF_SIZE 16
#define PP_BUFF_EN_DEBUG

typedef struct
{
	unsigned char read_buff_id;
	unsigned char write_buff_id;
	unsigned char read_index;
	unsigned char data_count[2];	
	unsigned char buff[2][PP_BUFF_SIZE];
}pp_buff_def;

typedef enum
{
	PP_BUFF_EOK = 0,
	PP_BUFF_EFULL = 1,
	PP_BUFF_EEMPTY = 2,
	PP_BUFF_EUNKOWN = 3
}pp_buff_error_def;

void pp_buff_init(pp_buff_def*pp_buff);
unsigned char pp_buff_write(pp_buff_def*pp_buff,unsigned char c);
unsigned char pp_buff_nwrite(pp_buff_def*pp_buff,unsigned char *pc,unsigned char num);
unsigned char pp_buff_read(pp_buff_def*pp_buff,unsigned char *c);
unsigned char pp_buff_nread(pp_buff_def*pp_buff,unsigned char *pc,unsigned char num);
pp_buff_error_def pp_buff_change(pp_buff_def*pp_buff);

#endif
