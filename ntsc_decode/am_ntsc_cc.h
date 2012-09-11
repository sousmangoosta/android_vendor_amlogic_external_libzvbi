/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief Teletext模块(version 2)
 *
 * \author kui.zhang <kui.zhang@amlogic.com>
 * \date 2012-09-03: create the document
 ***************************************************************************/



#ifndef _AM_NTSC_CC_H
#define _AM_NTSC_CC_H


#include <unistd.h>
#include "misc.h"
#include "trigger.h"
#include "format.h"
#include "lang.h"
#include "hamm.h"
#include "tables.h"
#include "vbi.h"
#include <android/log.h>    
#ifdef __cplusplus
extern "C"
{
#endif



enum AM_CC_ErrorCode
{
	AM_CC_ERROR_BASE =0,
	AM_CC_ERR_INVALID_PARAM,   /**< 参数无效*/
	AM_CC_ERR_INVALID_HANDLE,  /**< 句柄无效*/
	AM_CC_ERR_NOT_SUPPORTED,   /**< 不支持的操作*/
	AM_CC_ERR_CREATE_DECODE,   /**< 打开cc解码器失败*/
	AM_CC_ERR_OPEN_PES,        /**< 打开PES通道失败*/
	AM_CC_ERR_SET_BUFFER,      /**< 设置PES 缓冲区失败*/
	AM_CC_ERR_NO_MEM,                  /**< 空闲内存不足*/
	AM_CC_ERR_CANNOT_CREATE_THREAD,    /**< 无法创建线程*/
	AM_CC_ERR_NOT_RUN,            /**< 无法创建线程*/
	AM_CC_INIT_DISPLAY_FAILED,    /**< 初始化显示屏幕失败*/
	AM_CC_ERR_END
};


/****************************************************************************
 * Type definitions
 ***************************************************************************/
 /**\brief Teletext分析器句柄*/
typedef void* AM_NTSC_CC_Handle_t;

/**\brief 开始绘制*/
typedef void (*AM_VBI_CC_DrawBegin_t)(AM_NTSC_CC_Handle_t handle);

/**\brief 结束绘制*/
typedef void (*AM_VBI_CC_DrawEnd_t)(AM_NTSC_CC_Handle_t handle);

 typedef struct
{
	AM_VBI_CC_DrawBegin_t draw_begin;   /**< 开始绘制*/
	AM_VBI_CC_DrawEnd_t   draw_end;     /**< 结束绘制*/
	vbi_bool        is_subtitle;    /**< 是否为字幕*/
	uint8_t         *bitmap;         /**< 绘图缓冲区*/
	int              pitch;          /**< 绘图缓冲区每行字节数*/
	void            *user_data;      /**< 用户定义数据*/
}AM_NTSC_CC_Para_t;


extern vbi_bool AM_NTSC_Create(AM_NTSC_CC_Handle_t *handle, AM_NTSC_CC_Para_t *para);

extern  vbi_bool AM_NTSC_CC_Start(AM_NTSC_CC_Handle_t handle);

extern vbi_bool
decode_vbi		(int dev_no, int fid, const uint8_t *data, int len, void *user_data);

extern void* AM_VBI_CC_GetUserData(AM_NTSC_CC_Handle_t handle);

#ifdef __cplusplus
}
#endif



#endif
