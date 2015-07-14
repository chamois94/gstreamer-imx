/* GStreamer buffer pool for wrapped VPU framebuffers
 * Copyright (C) 2013  Carlos Rafael Giani
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <vpu_wrapper.h>
#include <string.h>
#include "../common/phys_mem_meta.h"
#include "fb_buffer_pool.h"
#include "utils.h"
#include "vpu_buffer_meta.h"

#ifdef HAVE_VIV_UPLOAD
# include "../common/viv_upload_meta.h"
#endif

GST_DEBUG_CATEGORY_STATIC(imx_vpu_fb_buffer_pool_debug);
#define GST_CAT_DEFAULT imx_vpu_fb_buffer_pool_debug


static void gst_imx_vpu_fb_buffer_pool_finalize(GObject *object);
static const gchar ** gst_imx_vpu_fb_buffer_pool_get_options(GstBufferPool *pool);
static gboolean gst_imx_vpu_fb_buffer_pool_set_config(GstBufferPool *pool, GstStructure *config);
static GstFlowReturn gst_imx_vpu_fb_buffer_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **buffer, GstBufferPoolAcquireParams *params);
static void gst_imx_vpu_fb_buffer_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer);


G_DEFINE_TYPE(GstImxVpuFbBufferPool, gst_imx_vpu_fb_buffer_pool, GST_TYPE_BUFFER_POOL)




static void gst_imx_vpu_fb_buffer_pool_finalize(GObject *object)
{
	GstImxVpuFbBufferPool *vpu_pool = GST_IMX_VPU_FB_BUFFER_POOL(object);

	if (vpu_pool->framebuffers != NULL)
		gst_object_unref(vpu_pool->framebuffers);

	GST_TRACE_OBJECT(vpu_pool, "shutting down buffer pool");

	G_OBJECT_CLASS(gst_imx_vpu_fb_buffer_pool_parent_class)->finalize(object);
}


static const gchar ** gst_imx_vpu_fb_buffer_pool_get_options(G_GNUC_UNUSED GstBufferPool *pool)
{
	static const gchar *options[] =
	{
		GST_BUFFER_POOL_OPTION_VIDEO_META,
		GST_BUFFER_POOL_OPTION_IMX_VPU_FRAMEBUFFER,
#ifdef HAVE_VIV_UPLOAD
		GST_BUFFER_POOL_OPTION_IMX_VIV_UPLOAD_META,
#endif
		NULL
	};

	return options;
}


static gboolean gst_imx_vpu_fb_buffer_pool_set_config(GstBufferPool *pool, GstStructure *config)
{
	GstImxVpuFbBufferPool *vpu_pool;
	GstVideoInfo info;
	GstCaps *caps;
	gsize size;
	guint min, max;

	vpu_pool = GST_IMX_VPU_FB_BUFFER_POOL(pool);

	if (!gst_buffer_pool_config_get_params(config, &caps, &size, &min, &max))
	{
		GST_ERROR_OBJECT(pool, "pool configuration invalid");
		return FALSE;
	}

	if (caps == NULL)
	{
		GST_ERROR_OBJECT(pool, "configuration contains no caps");
		return FALSE;
	}

	if (!gst_video_info_from_caps(&info, caps))
	{
		GST_ERROR_OBJECT(pool, "caps cannot be parsed for video info");
		return FALSE;
	}

	vpu_pool->video_info = info;

	vpu_pool->video_info.stride[0] = vpu_pool->framebuffers->y_stride;
	vpu_pool->video_info.stride[1] = vpu_pool->framebuffers->uv_stride;
	vpu_pool->video_info.stride[2] = vpu_pool->framebuffers->uv_stride;
	vpu_pool->video_info.offset[0] = 0;
	vpu_pool->video_info.offset[1] = vpu_pool->framebuffers->y_size;
	vpu_pool->video_info.offset[2] = vpu_pool->framebuffers->y_size + vpu_pool->framebuffers->u_size;
	vpu_pool->video_info.size = vpu_pool->framebuffers->total_size;

	vpu_pool->add_videometa = gst_buffer_pool_config_has_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
#ifdef HAVE_VIV_UPLOAD
	vpu_pool->add_vivuploadmeta = gst_buffer_pool_config_has_option(config, GST_BUFFER_POOL_OPTION_IMX_VIV_UPLOAD_META);
	if (vpu_pool->add_vivuploadmeta)
		vpu_pool->add_videometa = TRUE; /* we need GstVideoMeta for the upload meta */
#endif

	return GST_BUFFER_POOL_CLASS(gst_imx_vpu_fb_buffer_pool_parent_class)->set_config(pool, config);
}


static GstFlowReturn gst_imx_vpu_fb_buffer_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **buffer, G_GNUC_UNUSED GstBufferPoolAcquireParams *params)
{
	GstBuffer *buf;
	GstImxVpuFbBufferPool *vpu_pool;
	GstVideoInfo *info;

	vpu_pool = GST_IMX_VPU_FB_BUFFER_POOL(pool);

	info = &(vpu_pool->video_info);

	buf = gst_buffer_new();
	if (buf == NULL)
	{
		GST_ERROR_OBJECT(pool, "could not create new buffer");
		return GST_FLOW_ERROR;
	}

	GST_IMX_VPU_BUFFER_META_ADD(buf);
	GST_IMX_PHYS_MEM_META_ADD(buf);

	if (vpu_pool->add_videometa)
	{
		gst_buffer_add_video_meta_full(
			buf,
			GST_VIDEO_FRAME_FLAG_NONE,
			GST_VIDEO_INFO_FORMAT(info),
			GST_VIDEO_INFO_WIDTH(info), GST_VIDEO_INFO_HEIGHT(info),
			GST_VIDEO_INFO_N_PLANES(info),
			info->offset,
			info->stride
		);
	}

#ifdef HAVE_VIV_UPLOAD
	if (vpu_pool->add_vivuploadmeta)
	{
		gst_imx_buffer_add_vivante_gl_texture_upload_meta(buf);
	}
#endif

	*buffer = buf;

	return GST_FLOW_OK;
}


static void gst_imx_vpu_fb_buffer_pool_release_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
	GstImxVpuFbBufferPool *vpu_pool;

	vpu_pool = GST_IMX_VPU_FB_BUFFER_POOL(pool);
	g_assert(vpu_pool->framebuffers != NULL);

	if (vpu_pool->framebuffers->registration_state == GST_IMX_VPU_FRAMEBUFFERS_DECODER_REGISTERED)
	{
		VpuDecRetCode dec_ret;
		GstImxVpuBufferMeta *vpu_meta;
		GstImxPhysMemMeta *phys_mem_meta;

		vpu_meta = GST_IMX_VPU_BUFFER_META_GET(buffer);
		phys_mem_meta = GST_IMX_PHYS_MEM_META_GET(buffer);

		GST_IMX_VPU_FRAMEBUFFERS_LOCK(vpu_pool->framebuffers);

		if ((vpu_meta->framebuffer != NULL) && (phys_mem_meta != NULL) && (phys_mem_meta->phys_addr != 0))
		{
			if (vpu_meta->not_displayed_yet && vpu_pool->framebuffers->decenc_states.dec.decoder_open)
			{
				dec_ret = VPU_DecOutFrameDisplayed(vpu_pool->framebuffers->decenc_states.dec.handle, vpu_meta->framebuffer);
				if (dec_ret != VPU_DEC_RET_SUCCESS)
					GST_ERROR_OBJECT(pool, "clearing display framebuffer failed: %s", gst_imx_vpu_strerror(dec_ret));
				else
				{
					vpu_meta->not_displayed_yet = FALSE;
					if (vpu_pool->framebuffers->decremented_availbuf_counter > 0)
					{
						vpu_pool->framebuffers->num_available_framebuffers++;
						vpu_pool->framebuffers->decremented_availbuf_counter--;
						vpu_pool->framebuffers->num_framebuffers_in_buffers--;
						GST_LOG_OBJECT(pool, "number of available buffers: %d -> %d", vpu_pool->framebuffers->num_available_framebuffers - 1, vpu_pool->framebuffers->num_available_framebuffers);
					}
					GST_LOG_OBJECT(pool, "cleared buffer %p", (gpointer)buffer);
				}
			}
			else if (!vpu_pool->framebuffers->decenc_states.dec.decoder_open)
				GST_DEBUG_OBJECT(pool, "not clearing buffer %p, since VPU decoder is closed", (gpointer)buffer);
			else
				GST_DEBUG_OBJECT(pool, "buffer %p already cleared", (gpointer)buffer);
		}
		else
		{
			GST_DEBUG_OBJECT(pool, "buffer %p does not contain physical memory and/or a VPU framebuffer pointer, and does not need to be cleared", (gpointer)buffer);
		}

		/* Clear out old memory blocks ; the decoder always fills empty buffers with new memory
		 * blocks when it needs to push a newly decoded frame downstream anyway
		 * (see gst_imx_vpu_set_buffer_contents() below)
		 * removing the now-unused memory blocks immediately avoids buildup of unused but
		 * still allocated memory */
		gst_buffer_remove_all_memory(buffer);

		g_cond_signal(&(vpu_pool->framebuffers->cond));

		GST_IMX_VPU_FRAMEBUFFERS_UNLOCK(vpu_pool->framebuffers);
	}

	GST_BUFFER_POOL_CLASS(gst_imx_vpu_fb_buffer_pool_parent_class)->release_buffer(pool, buffer);
}


static void gst_imx_vpu_fb_buffer_pool_class_init(GstImxVpuFbBufferPoolClass *klass)
{
	GObjectClass *object_class;
	GstBufferPoolClass *parent_class;
	
	object_class = G_OBJECT_CLASS(klass);
	parent_class = GST_BUFFER_POOL_CLASS(klass);

	GST_DEBUG_CATEGORY_INIT(imx_vpu_fb_buffer_pool_debug, "imxvpufbbufferpool", 0, "Freescale i.MX VPU framebuffers buffer pool");

	object_class->finalize       = GST_DEBUG_FUNCPTR(gst_imx_vpu_fb_buffer_pool_finalize);
	parent_class->get_options    = GST_DEBUG_FUNCPTR(gst_imx_vpu_fb_buffer_pool_get_options);
	parent_class->set_config     = GST_DEBUG_FUNCPTR(gst_imx_vpu_fb_buffer_pool_set_config);
	parent_class->alloc_buffer   = GST_DEBUG_FUNCPTR(gst_imx_vpu_fb_buffer_pool_alloc_buffer);
	parent_class->release_buffer = GST_DEBUG_FUNCPTR(gst_imx_vpu_fb_buffer_pool_release_buffer);
}


static void gst_imx_vpu_fb_buffer_pool_init(GstImxVpuFbBufferPool *pool)
{
	pool->framebuffers = NULL;
	pool->add_videometa = FALSE;

	GST_INFO_OBJECT(pool, "initializing VPU buffer pool");
}


GstBufferPool *gst_imx_vpu_fb_buffer_pool_new(GstImxVpuFramebuffers *framebuffers)
{
	GstImxVpuFbBufferPool *vpu_pool;

	g_assert(framebuffers != NULL);

	vpu_pool = g_object_new(gst_imx_vpu_fb_buffer_pool_get_type(), NULL);
	vpu_pool->framebuffers = gst_object_ref(framebuffers);

	return GST_BUFFER_POOL_CAST(vpu_pool);
}


void gst_imx_vpu_fb_buffer_pool_set_framebuffers(GstBufferPool *pool, GstImxVpuFramebuffers *framebuffers)
{
	GstImxVpuFbBufferPool *vpu_pool = GST_IMX_VPU_FB_BUFFER_POOL(pool);

	g_assert(framebuffers != NULL);

	if (framebuffers == vpu_pool->framebuffers)
		return;

	/* it is good practice to first ref the new, then unref the old object
	 * even though the case of identical pointers is caught above */
	gst_object_ref(framebuffers);

	if (vpu_pool->framebuffers != NULL)
		gst_object_unref(vpu_pool->framebuffers);

	vpu_pool->framebuffers = framebuffers;
}


gboolean gst_imx_vpu_set_buffer_contents(GstBuffer *buffer, GstImxVpuFramebuffers *framebuffers, VpuFrameBuffer *framebuffer)
{
	GstVideoMeta *video_meta;
	GstImxVpuBufferMeta *vpu_meta;
	GstImxPhysMemMeta *phys_mem_meta;
	GstMemory *memory;

	video_meta = gst_buffer_get_video_meta(buffer);
	if (video_meta == NULL)
	{
		GST_ERROR("buffer with pointer %p has no video metadata", (gpointer)buffer);
		return FALSE;
	}

	vpu_meta = GST_IMX_VPU_BUFFER_META_GET(buffer);
	if (vpu_meta == NULL)
	{
		GST_ERROR("buffer with pointer %p has no VPU metadata", (gpointer)buffer);
		return FALSE;
	}

	phys_mem_meta = GST_IMX_PHYS_MEM_META_GET(buffer);
	if (phys_mem_meta == NULL)
	{
		GST_ERROR("buffer with pointer %p has no phys mem metadata", (gpointer)buffer);
		return FALSE;
	}

	{
		gsize x_padding = 0, y_padding = 0;

		if (framebuffers->pic_width > video_meta->width)
			x_padding = framebuffers->pic_width - video_meta->width;
		if (framebuffers->pic_height > video_meta->height)
			y_padding = framebuffers->pic_height - video_meta->height;

		vpu_meta->framebuffer = framebuffer;

		phys_mem_meta->phys_addr = (guintptr)(framebuffer->pbufY);
		phys_mem_meta->x_padding = x_padding;
		phys_mem_meta->y_padding = y_padding;

		GST_LOG("setting phys mem meta for buffer with pointer %p: phys addr %" GST_IMX_PHYS_ADDR_FORMAT " x/y padding %" G_GSIZE_FORMAT "/%" G_GSIZE_FORMAT, (gpointer)buffer, phys_mem_meta->phys_addr, phys_mem_meta->x_padding, phys_mem_meta->y_padding);

		memory = gst_memory_new_wrapped(
			GST_MEMORY_FLAG_NO_SHARE,
			framebuffer->pbufVirtY,
			framebuffers->total_size,
			0,
			framebuffers->total_size,
			NULL,
			NULL
		);
	}

	GST_IMX_VPU_FRAMEBUFFERS_LOCK(framebuffers);
	framebuffers->num_framebuffers_in_buffers++;
	GST_IMX_VPU_FRAMEBUFFERS_UNLOCK(framebuffers);

	/* remove any existing memory blocks */
	gst_buffer_remove_all_memory(buffer);
	/* and append the new memory block */
	gst_buffer_append_memory(buffer, memory);

	return TRUE;
}


void gst_imx_vpu_mark_buf_as_not_displayed(GstBuffer *buffer)
{
	GstImxVpuBufferMeta *vpu_meta = GST_IMX_VPU_BUFFER_META_GET(buffer);
	g_assert(vpu_meta != NULL);
	vpu_meta->not_displayed_yet = TRUE;
}

