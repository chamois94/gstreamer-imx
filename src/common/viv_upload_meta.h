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

#ifndef GST_IMX_COMMON_VIV_UPLOAD_META_H
#define GST_IMX_COMMON_VIV_UPLOAD_META_H

#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_BUFFER_POOL_OPTION_IMX_VIV_UPLOAD_META "GstBufferPoolOptionImxVivUploadMeta"

GstVideoGLTextureUploadMeta *gst_imx_buffer_add_vivante_gl_texture_upload_meta(GstBuffer *buffer);

G_END_DECLS

#endif
