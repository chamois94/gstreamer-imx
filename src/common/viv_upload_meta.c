/*
 * Copyright (C) 2013 Carlos Rafael Giani
 * Copyright (C) 2014 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>
#include "viv_upload_meta.h"
#include "viv_upload.h"
#include "phys_mem_meta.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static gboolean gst_imx_vivante_gl_texture_upload(GstVideoGLTextureUploadMeta *meta, guint texture_id[4])
{
	GstImxPhysMemMeta *phys_mem_meta;
	GstVideoMeta *video_meta;
	GstMapInfo map_info;
	GLvoid *virtual_addr;
	GLuint physical_addr;
	GLenum gl_format;

	phys_mem_meta = GST_IMX_PHYS_MEM_META_GET(meta->buffer);
	video_meta = gst_buffer_get_video_meta(meta->buffer);

	gl_format = gst_imx_viv_upload_get_viv_format(video_meta->format);

	gst_buffer_map(meta->buffer, &map_info, GST_MAP_READ);
	virtual_addr = map_info.data;
	physical_addr = (GLuint)(phys_mem_meta->phys_addr);

	GST_LOG("uploading buffer %p (data virt_addr %p, phys_addr 0x%x) to texture %u"
		" with w/h: %d/%d texture_w/h: %d/%d", meta->buffer, virtual_addr, physical_addr,
		texture_id[0], video_meta->width, video_meta->height, meta->width, meta->height);

	glBindTexture(GL_TEXTURE_2D, texture_id[0]);
// 	if (gl_has_error("glBindTexture"))
// 		return FALSE;

	glTexDirectVIVMap(GL_TEXTURE_2D, meta->width, meta->height, gl_format,
			&virtual_addr, &physical_addr);
// 	if (gl_has_error("glTexDirectVIVMap"))
// 		return FALSE;

	glTexDirectInvalidateVIV(GL_TEXTURE_2D);
// 	if (gl_has_error("glTexDirectInvalidateVIV"))
// 		return FALSE;

	gst_buffer_unmap(meta->buffer, &map_info);

	return TRUE;
}

GstVideoGLTextureUploadMeta *gst_imx_buffer_add_vivante_gl_texture_upload_meta(GstBuffer *buffer)
{
	GstVideoGLTextureUploadMeta *upload_meta;
	GstVideoMeta *video_meta;
	GstImxPhysMemMeta *phys_mem_meta;
	GstVideoGLTextureType texture_types[4] = { GST_VIDEO_GL_TEXTURE_TYPE_RGBA, 0 };
	guint num_extra_lines;

	phys_mem_meta = GST_IMX_PHYS_MEM_META_GET(buffer);
	g_return_val_if_fail(phys_mem_meta != NULL, NULL);

	video_meta = gst_buffer_get_video_meta(buffer);
	g_return_val_if_fail(video_meta != NULL, NULL);

	upload_meta = gst_buffer_add_video_gl_texture_upload_meta(buffer,
			GST_VIDEO_GL_TEXTURE_ORIENTATION_X_NORMAL_Y_NORMAL, 1,
			texture_types, gst_imx_vivante_gl_texture_upload,
			NULL, NULL, NULL);
	upload_meta->format = GST_VIDEO_FORMAT_RGBA;

	/* The VIV direct texture extension has no way of specifying the stride
	 * separately from the width/height and in order to function properly,
	 * it needs to be given the whole buffer area. So, we pass it a fake width &
	 * height that is based on the memory allocated for the buffer and we
	 * crop it later in the shader of the video sink.
	 */
	/* stride is in bytes, we need pixels */
	upload_meta->width = video_meta->stride[0] / gst_imx_viv_upload_get_bpp(video_meta->format);
	num_extra_lines = phys_mem_meta->padding / video_meta->stride[0];
	upload_meta->height = video_meta->height + num_extra_lines;

	return upload_meta;
}
