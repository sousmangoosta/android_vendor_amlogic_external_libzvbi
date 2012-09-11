#undef NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "vbi.h"
#include "exp-gfx.h"
#include "hamm.h"
#include "dvb_demux.h"
#include "sliced.h"
#include "sliced_vbi.h"
#include "am_ntsc_cc.h"
#include "vbi_dmx.h"

/********************define variable***************************/


typedef struct
{
	vbi_decoder       *dec;
	//vbi_search        *search;
	AM_NTSC_CC_Para_t      para;
	int                page_no;
	int                sub_page_no;
	vbi_bool          disp_update;
	vbi_bool          running;
	uint64_t           pts;
	pthread_mutex_t    lock;
	pthread_cond_t     cond;
	pthread_t          thread;
}AM_NTSC_CC_Parser_t;

vbi_pgno		pgno = -1;

//AM_NTSC_CC_Parser_t *parser = NULL;

static void
reset				(AM_NTSC_CC_Parser_t *parser)
{
	vbi_page page;
	vbi_bool success;
	int row;

	success = vbi_fetch_cc_page (parser->dec, &page, pgno, TRUE);
	assert (success);

	for (row = 0; row <= page.rows; ++row)
		render (parser,&page, row);

	vbi_unref_page (&page);
}

/**********************************************************/


 void vbi_cc_show(AM_NTSC_CC_Parser_t *parser)
{
	
	vbi_page page;
	vbi_bool success;
	int row;
	//user_data = user_data;

	//if (pgno != -1 && parser->page_no != pgno)
	//	return;

	/* Fetching & rendering in the handler
           is a bad idea, but this is only a test */
	AM_DEBUG("NTSC--------------------  vbi_cc_show****************\n");
	success = vbi_fetch_cc_page (parser->dec, &page, parser->page_no, TRUE);
	AM_DEBUG("NTSC--------------1212------vbi_fetch_cc_page  success****************\n");
	assert (success);
	
	 int i ,j;
	 if(parser->para.draw_begin)
		 parser->para.draw_begin(parser);
	
	vbi_draw_cc_page_region (&page, VBI_PIXFMT_RGBA32_LE, parser->para.bitmap,
			parser->para.pitch, 0, 0, page.columns, page.rows);
	
	 if(parser->para.draw_end)
		 parser->para.draw_end(parser);
	vbi_unref_page (&page);
}


static void* vbi_cc_thread(void *arg)
{
	AM_NTSC_CC_Parser_t *parser = (AM_NTSC_CC_Parser_t*)arg;

	pthread_mutex_lock(&parser->lock);
	AM_DEBUG("NTSC***************________________ vbi_cc_thread   parser->running = %d\n",parser->running);   
	while(parser->running)
	{
		AM_DEBUG("NTSC***************________________ vbi_cc_thread   disp_update = %d\n",parser->disp_update);   
		while(parser->running && !parser->disp_update){
			pthread_cond_wait(&parser->cond, &parser->lock);
		}
		AM_DEBUG("NTSC***************________________ vbi_cc_thread   disp_update = %d\n",parser->disp_update);  
		if(parser->disp_update){
			vbi_cc_show(parser);
			parser->disp_update = FALSE;
		}
	}
	pthread_mutex_unlock(&parser->lock);

	return NULL;
}

static void vbi_cc_handler(vbi_event *		ev,void *	user_data)
{
	AM_NTSC_CC_Parser_t *parser = (AM_NTSC_CC_Parser_t*)user_data;
	AM_DEBUG("NTSC--------------------  vbi_cc_handler****************");
	//if(parser->page_no != ev->ev.caption.pgno){
		parser->page_no = ev->ev.caption.pgno;
		parser->disp_update = AM_TRUE;
		pthread_cond_signal(&parser->cond);
		
		//vbi_cc_show(parser);
	//}
}

/**********************************************************/

static vbi_bool
decode_frame			(const vbi_sliced *	sliced,
				 unsigned int		n_lines,
				 const uint8_t *	raw,
				 const vbi_sampling_par *sp,
				 double			sample_time,
				 int64_t		stream_time,
				 void *	user_data)
{
	AM_DEBUG("NTSC--------------------  decode_frame\n");
	if(user_data == NULL) return FALSE;
	AM_NTSC_CC_Parser_t *parser = (AM_NTSC_CC_Parser_t*)user_data;
	raw = raw;
	sp = sp;
	stream_time = stream_time; /* unused */

	vbi_decode (parser->dec, sliced, n_lines, sample_time);
	return TRUE;
}



 vbi_bool
decode_vbi		(int dev_no, int fid, const uint8_t *data, int len, void *user_data){

	AM_DEBUG("NTSC--------------------  decode_vbi  len = %d\n",len);
	AM_NTSC_CC_Parser_t *parser = (AM_NTSC_CC_Parser_t*)user_data;
	if(user_data == NULL)	
		AM_DEBUG("NTSC--------------------  decode_vbi NOT user_data ");
	
    int length =  len;
	struct stream *st;
	if(len < 0  || data == NULL)
		goto error;
	st = read_stream_new (data,length,FILE_FORMAT_SLICED,
					0,decode_frame,parser);
	
	stream_loop (st);
	stream_delete (st);
	return AM_SUCCESS;
	
	error:
		return AM_FAILURE;
  
}



  vbi_bool AM_NTSC_Create(AM_NTSC_CC_Handle_t *handle, AM_NTSC_CC_Para_t *para)
{
	if(!para || !handle)
	{
		return AM_VBI_DMX_ERROR_BASE;
	}
	AM_NTSC_CC_Parser_t* parser = (AM_NTSC_CC_Parser_t*)malloc(sizeof(AM_NTSC_CC_Parser_t));
	if(!parser)
	{
		return AM_VBI_DMX_ERR_NOT_ALLOCATED;
	}
	memset(parser, 0, sizeof(AM_NTSC_CC_Parser_t));
	vbi_bool success;
	parser->dec = vbi_decoder_new ();
	assert (NULL != parser->dec);
	success = vbi_event_handler_add (parser->dec, VBI_EVENT_CAPTION,
					 vbi_cc_handler, parser);
	assert (success);
	pthread_mutex_init(&parser->lock, NULL);
	pthread_cond_init(&parser->cond, NULL);
	
	*handle = parser;
	parser->para    = *para;
	return AM_SUCCESS;
}


 AM_ErrorCode_t AM_NTSC_CC_Start(AM_NTSC_CC_Handle_t handle)
{
	AM_DEBUG("NTSC--------------------  ******************AM_NTSC_CC_Start \n");
	AM_NTSC_CC_Parser_t *parser = (AM_NTSC_CC_Para_t*)handle;
	vbi_bool ret = AM_SUCCESS;

	if(!parser)
	{	
		AM_DEBUG("NTSC--------------------  ******************AM_CC_ERR_INVALID_HANDLE \n");
		return AM_CC_ERR_INVALID_HANDLE;
	}

	pthread_mutex_lock(&parser->lock);
	
	if(!parser->running)
	{
		parser->running = AM_TRUE;
		if(pthread_create(&parser->thread, NULL, vbi_cc_thread, parser))
		{
			parser->running = AM_FALSE;
			ret = AM_CC_ERR_CANNOT_CREATE_THREAD;
		}
	}
	pthread_mutex_unlock(&parser->lock);

	return AM_SUCCESS;
}

void* AM_VBI_CC_GetUserData(AM_NTSC_CC_Handle_t handle)
{
	AM_NTSC_CC_Parser_t *parser = (AM_NTSC_CC_Para_t*)handle;
	if(!parser)
	{
		return NULL;
	}
	return parser->para.user_data;
}
