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

#ifndef GST_IMX_COMMON_VIV_UPLOAD_H
#define GST_IMX_COMMON_VIV_UPLOAD_H

#include <gst/video/video.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

G_BEGIN_DECLS

static inline GLenum gst_imx_viv_upload_get_viv_format(GstVideoFormat format)
{
    switch (format)
    {
#ifdef HAVE_VIV_I420
        case GST_VIDEO_FORMAT_I420:  return GL_VIV_I420;
#endif
#ifdef HAVE_VIV_YV12
        case GST_VIDEO_FORMAT_YV12:  return GL_VIV_YV12;
#endif
#ifdef HAVE_VIV_NV12
        case GST_VIDEO_FORMAT_NV12:  return GL_VIV_NV12;
#endif
#ifdef HAVE_VIV_NV21
        case GST_VIDEO_FORMAT_NV21:  return GL_VIV_NV21;
#endif
#ifdef HAVE_VIV_YUY2
        case GST_VIDEO_FORMAT_YUY2:  return GL_VIV_YUY2;
#endif
#ifdef HAVE_VIV_UYVY
        case GST_VIDEO_FORMAT_UYVY:  return GL_VIV_UYVY;
#endif
        case GST_VIDEO_FORMAT_RGB16: return GL_RGB565;
        case GST_VIDEO_FORMAT_RGBA:  return GL_RGBA;
        case GST_VIDEO_FORMAT_BGRA:  return GL_BGRA_EXT;
        case GST_VIDEO_FORMAT_RGBx:  return GL_RGBA;
        case GST_VIDEO_FORMAT_BGRx:  return GL_BGRA_EXT;
        default: return 0;
    }
}

static inline gint gst_imx_viv_upload_get_bpp(GstVideoFormat fmt)
{
    switch (fmt)
    {
        case GST_VIDEO_FORMAT_RGB16: return 2;
        case GST_VIDEO_FORMAT_RGB: return 3;
        case GST_VIDEO_FORMAT_RGBA: return 4;
        case GST_VIDEO_FORMAT_BGRA: return 4;
        case GST_VIDEO_FORMAT_RGBx: return 4;
        case GST_VIDEO_FORMAT_BGRx: return 4;
        case GST_VIDEO_FORMAT_YUY2: return 2;
        case GST_VIDEO_FORMAT_UYVY: return 2;
        default: return 1;
    }
}

G_END_DECLS

#endif
