/*****************************************************************************
 * fourcc.c: fourcc helpers functions
 *****************************************************************************
 * Copyright © 2009-2011 Laurent Aimar
 *
 * Authors: Laurent Aimar <fenrir@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_fourcc.h>
#include <vlc_es.h>
#include <assert.h>

#include "fourcc_tables.h"

static int fourcc_cmp(const void *key, const void *ent)
{
    return memcmp(key, ent, 4);
}

static vlc_fourcc_t Lookup(vlc_fourcc_t fourcc, const char **restrict dsc,
                           const struct fourcc_mapping *mapv, size_t mapc,
                           const struct fourcc_desc *dscv, size_t dscc)
{
    const struct fourcc_mapping *mapping;
    const struct fourcc_desc *desc;

    mapping = bsearch(&fourcc, mapv, mapc, sizeof (*mapv), fourcc_cmp);
    if (mapping != NULL)
    {
        if (dsc != NULL)
        {
            desc = bsearch(&fourcc, dscv, dscc, sizeof (*dscv), fourcc_cmp);
            if (desc != NULL)
            {
                *dsc = desc->desc;
                return mapping->fourcc;
            }
        }
        fourcc = mapping->fourcc;
    }

    desc = bsearch(&fourcc, dscv, dscc, sizeof (*dscv), fourcc_cmp);
    if (desc == NULL)
        return 0; /* Unknown FourCC */
    if (dsc != NULL)
        *dsc = desc->desc;
    return fourcc; /* Known FourCC (has a description) */
}

static vlc_fourcc_t LookupVideo(vlc_fourcc_t fourcc, const char **restrict dsc)
{
    return Lookup(fourcc, dsc, mapping_video, ARRAY_SIZE(mapping_video),
                  desc_video, ARRAY_SIZE(desc_video));
}

static vlc_fourcc_t LookupAudio(vlc_fourcc_t fourcc, const char **restrict dsc)
{
    return Lookup(fourcc, dsc, mapping_audio, ARRAY_SIZE(mapping_audio),
                  desc_audio, ARRAY_SIZE(desc_audio));
}

static vlc_fourcc_t LookupSpu(vlc_fourcc_t fourcc, const char **restrict dsc)
{
    return Lookup(fourcc, dsc, mapping_spu, ARRAY_SIZE(mapping_spu),
                  desc_spu, ARRAY_SIZE(desc_spu));
}

static vlc_fourcc_t LookupCat(vlc_fourcc_t fourcc, const char **restrict dsc,
                              int cat)
{
    switch (cat)
    {
        case VIDEO_ES:
            return LookupVideo(fourcc, dsc);
        case AUDIO_ES:
            return LookupAudio(fourcc, dsc);
        case SPU_ES:
            return LookupSpu(fourcc, dsc);
    }

    vlc_fourcc_t ret = LookupVideo(fourcc, dsc);
    if (!ret)
        ret = LookupAudio(fourcc, dsc);
    if (!ret)
        ret = LookupSpu(fourcc, dsc);
    return ret;
}

vlc_fourcc_t vlc_fourcc_GetCodec(int cat, vlc_fourcc_t fourcc)
{
    vlc_fourcc_t codec = LookupCat(fourcc, NULL, cat);
    return codec ? codec : fourcc;
}

vlc_fourcc_t vlc_fourcc_GetCodecFromString( int i_cat, const char *psz_fourcc )
{
    if( psz_fourcc == NULL )
        return 0;
    size_t s = strlen(psz_fourcc);
    if (s > 4)
        return 0;
    return vlc_fourcc_GetCodec( i_cat,
                                VLC_FOURCC( psz_fourcc[0],
                                             s > 1 ? psz_fourcc[1] : ' ',
                                             s > 2 ? psz_fourcc[2] : ' ',
                                             s > 3 ? psz_fourcc[3] : ' ') );
}

vlc_fourcc_t vlc_fourcc_GetCodecAudio( vlc_fourcc_t i_fourcc, int i_bits )
{
    const int i_bytes = ( i_bits + 7 ) / 8;

    if( i_fourcc == VLC_FOURCC( 'a', 'f', 'l', 't' ) )
    {
        switch( i_bytes )
        {
        case 4:
            return VLC_CODEC_FL32;
        case 8:
            return VLC_CODEC_FL64;
        default:
            return 0;
        }
    }
    else if( i_fourcc == VLC_FOURCC( 'a', 'r', 'a', 'w' ) )
    {
        switch( i_bytes )
        {
        case 1:
            return VLC_CODEC_U8;
        case 2:
            return VLC_CODEC_S16L;
        case 3:
            return VLC_CODEC_S24L;
        case 4:
            return VLC_CODEC_S32L;
        default:
            return 0;
        }
    }
    else if( i_fourcc == VLC_FOURCC( 't', 'w', 'o', 's' ) )
    {
        switch( i_bytes )
        {
        case 1:
            return VLC_CODEC_S8;
        case 2:
            return VLC_CODEC_S16B;
        case 3:
            return VLC_CODEC_S24B;
        case 4:
            return VLC_CODEC_S32B;
        default:
            return 0;
        }
    }
    else if( i_fourcc == VLC_FOURCC( 's', 'o', 'w', 't' ) )
    {
        switch( i_bytes )
        {
        case 1:
            return VLC_CODEC_S8;
        case 2:
            return VLC_CODEC_S16L;
        case 3:
            return VLC_CODEC_S24L;
        case 4:
            return VLC_CODEC_S32L;
        default:
            return 0;
        }
    }
    else
    {
        return vlc_fourcc_GetCodec( AUDIO_ES, i_fourcc );
    }
}

const char *vlc_fourcc_GetDescription(int cat, vlc_fourcc_t fourcc)
{
    const char *ret;

    return LookupCat(fourcc, &ret, cat) ? ret : "";
}


/* */
#define VLC_CODEC_YUV_PLANAR_420 \
    VLC_CODEC_I420, VLC_CODEC_YV12

#define VLC_CODEC_YUV_SEMIPLANAR_420 \
    VLC_CODEC_NV12, VLC_CODEC_NV21

#define VLC_CODEC_YUV_PLANAR_420_16 \
    VLC_CODEC_I420_16L, VLC_CODEC_I420_16B, VLC_CODEC_I420_12L, VLC_CODEC_I420_12B, VLC_CODEC_I420_10L, VLC_CODEC_I420_10B, VLC_CODEC_I420_9L, VLC_CODEC_I420_9B

#define VLC_CODEC_YUV_SEMIPLANAR_420_16 \
    VLC_CODEC_P010, VLC_CODEC_P012 ,VLC_CODEC_P016

#define VLC_CODEC_YUV_SEMIPLANAR_422 \
    VLC_CODEC_NV16, VLC_CODEC_NV61

#define VLC_CODEC_YUV_PLANAR_422_16 \
    VLC_CODEC_I422_12L, VLC_CODEC_I422_12B, VLC_CODEC_I422_10L, VLC_CODEC_I422_10B, VLC_CODEC_I422_9L, VLC_CODEC_I422_9B

#define VLC_CODEC_YUV_PLANAR_444_ALPHA \
    VLC_CODEC_YUVA, VLC_CODEC_YUVA_444_10L, VLC_CODEC_YUVA_444_10B, VLC_CODEC_YUVA_444_12L, VLC_CODEC_YUVA_444_12B

#define VLC_CODEC_YUV_SEMIPLANAR_444 \
    VLC_CODEC_NV24, VLC_CODEC_NV42

#define VLC_CODEC_YUV_PLANAR_444_16 \
    VLC_CODEC_I444_10L, VLC_CODEC_I444_10B, VLC_CODEC_I444_9L, VLC_CODEC_I444_9B, \
    VLC_CODEC_I444_16L, VLC_CODEC_I444_16B, VLC_CODEC_I444_12L, VLC_CODEC_I444_12B

#define VLC_CODEC_YUV_PACKED \
    VLC_CODEC_YUYV, VLC_CODEC_YVYU, \
    VLC_CODEC_UYVY, VLC_CODEC_VYUY, \
    VLC_CODEC_VUYA, VLC_CODEC_Y210, \
    VLC_CODEC_Y410

#define VLC_CODEC_FALLBACK_420 \
    VLC_CODEC_I422, VLC_CODEC_YUV_PACKED, \
    VLC_CODEC_I444, VLC_CODEC_I440, \
    VLC_CODEC_I411, VLC_CODEC_I410, VLC_CODEC_Y211

static const vlc_fourcc_t p_I420_fallback[] = {
    VLC_CODEC_I420, VLC_CODEC_YV12, VLC_CODEC_FALLBACK_420, 0
};
static const vlc_fourcc_t p_YV12_fallback[] = {
    VLC_CODEC_YV12, VLC_CODEC_I420, VLC_CODEC_FALLBACK_420, 0
};
static const vlc_fourcc_t p_NV12_fallback[] = {
    VLC_CODEC_NV12, VLC_CODEC_I420, VLC_CODEC_FALLBACK_420, 0
};

#define VLC_CODEC_FALLBACK_420_16 \
    VLC_CODEC_I420, VLC_CODEC_YV12, VLC_CODEC_FALLBACK_420

static const vlc_fourcc_t p_I420_9L_fallback[] = {
    VLC_CODEC_I420_9L, VLC_CODEC_I420_9B, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_9B_fallback[] = {
    VLC_CODEC_I420_9B, VLC_CODEC_I420_9L, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_10L_fallback[] = {
    VLC_CODEC_I420_10L, VLC_CODEC_I420_10B, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_10B_fallback[] = {
    VLC_CODEC_I420_10B, VLC_CODEC_I420_10L, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_12L_fallback[] = {
    VLC_CODEC_I420_12L, VLC_CODEC_I420_12B, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_12B_fallback[] = {
    VLC_CODEC_I420_12B, VLC_CODEC_I420_12L, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_16L_fallback[] = {
    VLC_CODEC_I420_16L, VLC_CODEC_I420_16B, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_I420_16B_fallback[] = {
    VLC_CODEC_I420_16B, VLC_CODEC_I420_16L, VLC_CODEC_FALLBACK_420_16, 0
};
static const vlc_fourcc_t p_P010_fallback[] = {
    VLC_CODEC_P010, VLC_CODEC_FALLBACK_420_16, 0
};


#define VLC_CODEC_FALLBACK_422 \
    VLC_CODEC_YUV_PACKED, VLC_CODEC_YUV_PLANAR_420, \
    VLC_CODEC_I444, VLC_CODEC_I440, \
    VLC_CODEC_I411, VLC_CODEC_I410, VLC_CODEC_Y211

static const vlc_fourcc_t p_I422_fallback[] = {
    VLC_CODEC_I422, VLC_CODEC_FALLBACK_422, 0
};

#define VLC_CODEC_FALLBACK_422_16 \
    VLC_CODEC_I422, VLC_CODEC_FALLBACK_422

static const vlc_fourcc_t p_I422_9L_fallback[] = {
    VLC_CODEC_I422_9L, VLC_CODEC_I422_9B, VLC_CODEC_FALLBACK_422_16, 0
};
static const vlc_fourcc_t p_I422_9B_fallback[] = {
    VLC_CODEC_I422_9B, VLC_CODEC_I422_9L, VLC_CODEC_FALLBACK_422_16, 0
};
static const vlc_fourcc_t p_I422_10L_fallback[] = {
    VLC_CODEC_I422_10L, VLC_CODEC_I422_10B, VLC_CODEC_FALLBACK_422_16, 0
};
static const vlc_fourcc_t p_I422_10B_fallback[] = {
    VLC_CODEC_I422_10B, VLC_CODEC_I422_10L, VLC_CODEC_FALLBACK_422_16, 0
};
static const vlc_fourcc_t p_I422_12L_fallback[] = {
    VLC_CODEC_I422_12L, VLC_CODEC_I422_12B, VLC_CODEC_FALLBACK_422_16, 0
};
static const vlc_fourcc_t p_I422_12B_fallback[] = {
    VLC_CODEC_I422_12B, VLC_CODEC_I422_12L, VLC_CODEC_FALLBACK_422_16, 0
};

#define VLC_CODEC_FALLBACK_444 \
    VLC_CODEC_I422, VLC_CODEC_YUV_PACKED, \
    VLC_CODEC_YUV_PLANAR_420, VLC_CODEC_I440, \
    VLC_CODEC_I411, VLC_CODEC_I410, VLC_CODEC_Y211

static const vlc_fourcc_t p_I444_fallback[] = {
    VLC_CODEC_I444, VLC_CODEC_FALLBACK_444, 0
};

#define VLC_CODEC_FALLBACK_444_16 \
    VLC_CODEC_I444, VLC_CODEC_FALLBACK_444

static const vlc_fourcc_t p_I444_9L_fallback[] = {
    VLC_CODEC_I444_9L, VLC_CODEC_I444_9B, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_9B_fallback[] = {
    VLC_CODEC_I444_9B, VLC_CODEC_I444_9L, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_10L_fallback[] = {
    VLC_CODEC_I444_10L, VLC_CODEC_I444_10B, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_10B_fallback[] = {
    VLC_CODEC_I444_10B, VLC_CODEC_I444_10L, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_12L_fallback[] = {
    VLC_CODEC_I444_12L, VLC_CODEC_I444_12B, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_12B_fallback[] = {
    VLC_CODEC_I444_12B, VLC_CODEC_I444_12L, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_16L_fallback[] = {
    VLC_CODEC_I444_16L, VLC_CODEC_I444_16B, VLC_CODEC_FALLBACK_444_16, 0
};
static const vlc_fourcc_t p_I444_16B_fallback[] = {
    VLC_CODEC_I444_16B, VLC_CODEC_I444_16L, VLC_CODEC_FALLBACK_444_16, 0
};


/* Fallbacks for cvpx */
static const vlc_fourcc_t p_CVPX_VIDEO_NV12_fallback[] = {
    VLC_CODEC_CVPX_NV12, VLC_CODEC_NV12, VLC_CODEC_I420, 0,
};
static const vlc_fourcc_t p_CVPX_VIDEO_UYVY_fallback[] = {
    VLC_CODEC_CVPX_UYVY, VLC_CODEC_UYVY, 0,
};
static const vlc_fourcc_t p_CVPX_VIDEO_I420_fallback[] = {
    VLC_CODEC_CVPX_I420, VLC_CODEC_I420, 0,
};
static const vlc_fourcc_t p_CVPX_VIDEO_BGRA_fallback[] = {
    VLC_CODEC_CVPX_BGRA, VLC_CODEC_BGRA, 0,
};
static const vlc_fourcc_t p_CVPX_VIDEO_P010_fallback[] = {
    VLC_CODEC_CVPX_P010, VLC_CODEC_P010, VLC_CODEC_I420_10L, 0
};

static const vlc_fourcc_t p_VAAPI_420_fallback[] = {
    VLC_CODEC_VAAPI_420, VLC_CODEC_I420, 0,
};

static const vlc_fourcc_t p_VAAPI_420_10BPP_fallback[] = {
    VLC_CODEC_VAAPI_420_10BPP, VLC_CODEC_P010, VLC_CODEC_I420_10L, 0,
};

static const vlc_fourcc_t p_VAAPI_420_12BPP_fallback[] = {
    VLC_CODEC_VAAPI_420_12BPP, VLC_CODEC_P012, VLC_CODEC_I420_12L, 0,
};

static const vlc_fourcc_t p_D3D9_OPAQUE_fallback[] = {
    VLC_CODEC_D3D9_OPAQUE, VLC_CODEC_I420, 0,
};

static const vlc_fourcc_t p_D3D9_OPAQUE_10B_fallback[] = {
    VLC_CODEC_D3D9_OPAQUE_10B, VLC_CODEC_P010, VLC_CODEC_I420_10L, 0,
};

static const vlc_fourcc_t p_D3D11_OPAQUE_fallback[] = {
    VLC_CODEC_D3D11_OPAQUE, VLC_CODEC_NV12, 0,
};

static const vlc_fourcc_t p_D3D11_OPAQUE_10B_fallback[] = {
    VLC_CODEC_D3D11_OPAQUE_10B, VLC_CODEC_P010, VLC_CODEC_I420_10L, 0,
};

static const vlc_fourcc_t p_D3D11_OPAQUE_RGBA_fallback[] = {
    VLC_CODEC_D3D11_OPAQUE_RGBA, VLC_CODEC_RGBA, 0,
};

static const vlc_fourcc_t p_NVDEC_OPAQUE_fallback[] = {
    VLC_CODEC_NVDEC_OPAQUE, VLC_CODEC_NV12, 0,
};

static const vlc_fourcc_t p_NVDEC_OPAQUE_10B_fallback[] = {
    VLC_CODEC_NVDEC_OPAQUE_10B,
    VLC_CODEC_P010,
    VLC_CODEC_I420_10L, 0,
};

static const vlc_fourcc_t p_NVDEC_OPAQUE_16B_fallback[] = {
    VLC_CODEC_NVDEC_OPAQUE_16B,
    VLC_CODEC_P016, VLC_CODEC_P010,
    VLC_CODEC_I420_16L, VLC_CODEC_I420_12L, VLC_CODEC_I420_10L, 0,
};

static const vlc_fourcc_t p_I440_fallback[] = {
    VLC_CODEC_I440,
    VLC_CODEC_YUV_PLANAR_420,
    VLC_CODEC_I422,
    VLC_CODEC_I444,
    VLC_CODEC_YUV_PACKED,
    VLC_CODEC_I411, VLC_CODEC_I410, VLC_CODEC_Y211, 0
};

#define VLC_CODEC_FALLBACK_PACKED \
    VLC_CODEC_I422, VLC_CODEC_YUV_PLANAR_420, \
    VLC_CODEC_I444, VLC_CODEC_I440, \
    VLC_CODEC_I411, VLC_CODEC_I410, VLC_CODEC_Y211

static const vlc_fourcc_t p_YUYV_fallback[] = {
    VLC_CODEC_YUYV,
    VLC_CODEC_YVYU,
    VLC_CODEC_UYVY,
    VLC_CODEC_VYUY,
    VLC_CODEC_FALLBACK_PACKED, 0
};
static const vlc_fourcc_t p_YVYU_fallback[] = {
    VLC_CODEC_YVYU,
    VLC_CODEC_YUYV,
    VLC_CODEC_UYVY,
    VLC_CODEC_VYUY,
    VLC_CODEC_FALLBACK_PACKED, 0
};
static const vlc_fourcc_t p_UYVY_fallback[] = {
    VLC_CODEC_UYVY,
    VLC_CODEC_VYUY,
    VLC_CODEC_YUYV,
    VLC_CODEC_YVYU,
    VLC_CODEC_FALLBACK_PACKED, 0
};
static const vlc_fourcc_t p_VYUY_fallback[] = {
    VLC_CODEC_VYUY,
    VLC_CODEC_UYVY,
    VLC_CODEC_YUYV,
    VLC_CODEC_YVYU,
    VLC_CODEC_FALLBACK_PACKED, 0
};

static const vlc_fourcc_t *const pp_YUV_fallback[] = {
    p_YV12_fallback,
    p_I420_fallback,
    p_I420_9L_fallback,
    p_I420_9B_fallback,
    p_I420_10L_fallback,
    p_I420_10B_fallback,
    p_I420_12L_fallback,
    p_I420_12B_fallback,
    p_I420_16L_fallback,
    p_I420_16B_fallback,
    p_I422_fallback,
    p_I422_9L_fallback,
    p_I422_9B_fallback,
    p_I422_10L_fallback,
    p_I422_10B_fallback,
    p_I422_12L_fallback,
    p_I422_12B_fallback,
    p_I444_fallback,
    p_I444_9L_fallback,
    p_I444_9B_fallback,
    p_I444_10L_fallback,
    p_I444_10B_fallback,
    p_I444_12L_fallback,
    p_I444_12B_fallback,
    p_I444_16L_fallback,
    p_I444_16B_fallback,
    p_I440_fallback,
    p_YUYV_fallback,
    p_YVYU_fallback,
    p_UYVY_fallback,
    p_VYUY_fallback,
    p_NV12_fallback,
    p_P010_fallback,
    p_CVPX_VIDEO_NV12_fallback,
    p_CVPX_VIDEO_UYVY_fallback,
    p_CVPX_VIDEO_I420_fallback,
    p_CVPX_VIDEO_P010_fallback,
    p_VAAPI_420_fallback,
    p_VAAPI_420_10BPP_fallback,
    p_VAAPI_420_12BPP_fallback,
    p_D3D9_OPAQUE_fallback,
    p_D3D9_OPAQUE_10B_fallback,
    p_D3D11_OPAQUE_fallback,
    p_D3D11_OPAQUE_10B_fallback,
    p_NVDEC_OPAQUE_fallback,
    p_NVDEC_OPAQUE_10B_fallback,
    p_NVDEC_OPAQUE_16B_fallback,
    NULL,
};

static const vlc_fourcc_t p_list_YUV[] = {
    VLC_CODEC_YUV_PLANAR_420,
    VLC_CODEC_YUV_SEMIPLANAR_420,
    VLC_CODEC_I422,
    VLC_CODEC_YUV_SEMIPLANAR_422,
    VLC_CODEC_I440,
    VLC_CODEC_I444,
    VLC_CODEC_YUV_PLANAR_444_ALPHA,
    VLC_CODEC_YUV_SEMIPLANAR_444,
    VLC_CODEC_YUV_PACKED,
    VLC_CODEC_I411, VLC_CODEC_I410, VLC_CODEC_Y211,
    VLC_CODEC_YUV_PLANAR_420_16,
    VLC_CODEC_YUV_SEMIPLANAR_420_16,
    VLC_CODEC_YUV_PLANAR_422_16,
    VLC_CODEC_YUV_PLANAR_444_16,
    VLC_CODEC_VDPAU_VIDEO,
    VLC_CODEC_CVPX_NV12,
    VLC_CODEC_CVPX_UYVY,
    VLC_CODEC_CVPX_I420,
    VLC_CODEC_CVPX_P010,
    VLC_CODEC_VAAPI_420,
    VLC_CODEC_VAAPI_420_10BPP,
    VLC_CODEC_D3D9_OPAQUE,
    VLC_CODEC_D3D9_OPAQUE_10B,
    VLC_CODEC_D3D11_OPAQUE,
    VLC_CODEC_D3D11_OPAQUE_10B,
    VLC_CODEC_D3D11_OPAQUE_ALPHA,
    VLC_CODEC_NVDEC_OPAQUE,
    VLC_CODEC_NVDEC_OPAQUE_10B,
    VLC_CODEC_NVDEC_OPAQUE_16B,
    VLC_CODEC_NVDEC_OPAQUE_444,
    VLC_CODEC_NVDEC_OPAQUE_444_16B,
    VLC_CODEC_YUV420A,
    VLC_CODEC_YUV422A,
    0,
};

static const vlc_fourcc_t p_list_YUV_no_fallback[] = {
    VLC_CODEC_V308,
};

/* */
static const vlc_fourcc_t p_RGB32_fallback[] = {
    VLC_CODEC_XRGB,
    VLC_CODEC_BGRX,
    VLC_CODEC_RGB24,
    VLC_CODEC_BGR24,
    VLC_CODEC_RGB565LE,
    VLC_CODEC_RGB555LE,
    VLC_CODEC_RGB233,
    VLC_CODEC_BGR233,
    VLC_CODEC_RGB332,
    0,
};
static const vlc_fourcc_t p_RGB24_fallback[] = {
    VLC_CODEC_RGB24,
    VLC_CODEC_BGR24,
    VLC_CODEC_XRGB,
    VLC_CODEC_BGRX,
    VLC_CODEC_RGB565LE,
    VLC_CODEC_RGB555LE,
    VLC_CODEC_RGB233,
    VLC_CODEC_BGR233,
    VLC_CODEC_RGB332,
    0,
};
static const vlc_fourcc_t p_RGB16_fallback[] = {
    VLC_CODEC_RGB565LE,
    VLC_CODEC_RGB24,
    VLC_CODEC_BGR24,
    VLC_CODEC_XRGB,
    VLC_CODEC_BGRX,
    VLC_CODEC_RGB555LE,
    VLC_CODEC_RGB233,
    VLC_CODEC_BGR233,
    VLC_CODEC_RGB332,
    0,
};
static const vlc_fourcc_t p_RGB15_fallback[] = {
    VLC_CODEC_RGB555LE,
    VLC_CODEC_RGB565LE,
    VLC_CODEC_RGB24,
    VLC_CODEC_BGR24,
    VLC_CODEC_XRGB,
    VLC_CODEC_BGRX,
    VLC_CODEC_RGB233,
    VLC_CODEC_BGR233,
    VLC_CODEC_RGB332,
    0,
};
static const vlc_fourcc_t p_RGB8_fallback[] = {
    VLC_CODEC_RGB233,
    VLC_CODEC_BGR233,
    VLC_CODEC_RGB332,
    VLC_CODEC_RGB555LE,
    VLC_CODEC_RGB565LE,
    VLC_CODEC_RGB24,
    VLC_CODEC_BGR24,
    VLC_CODEC_XRGB,
    VLC_CODEC_BGRX,
    0,
};
static const vlc_fourcc_t *const pp_RGB_fallback[] = {
    p_RGB32_fallback,
    p_RGB24_fallback,
    p_RGB16_fallback,
    p_RGB15_fallback,
    p_RGB8_fallback,
    p_CVPX_VIDEO_BGRA_fallback,
    p_D3D11_OPAQUE_RGBA_fallback,

    NULL,
};


/* */
static const vlc_fourcc_t *GetFallback( vlc_fourcc_t i_fourcc,
                                        const vlc_fourcc_t *const *pp_fallback,
                                        const vlc_fourcc_t p_list[] )
{
    for( unsigned i = 0; pp_fallback[i]; i++ )
    {
        if( pp_fallback[i][0] == i_fourcc )
            return pp_fallback[i];
    }
    return p_list;
}

const vlc_fourcc_t *vlc_fourcc_GetYUVFallback( vlc_fourcc_t i_fourcc )
{
    return GetFallback( i_fourcc, pp_YUV_fallback, p_list_YUV );
}
const vlc_fourcc_t *vlc_fourcc_GetRGBFallback( vlc_fourcc_t i_fourcc )
{
    return GetFallback( i_fourcc, pp_RGB_fallback, p_RGB32_fallback );
}

const vlc_fourcc_t *vlc_fourcc_GetFallback( vlc_fourcc_t i_fourcc )
{
    return vlc_fourcc_IsYUV( i_fourcc)
            ? vlc_fourcc_GetYUVFallback( i_fourcc )
            : vlc_fourcc_GetRGBFallback( i_fourcc );
}

bool vlc_fourcc_IsYUV(vlc_fourcc_t fcc)
{
    for( unsigned i = 0; p_list_YUV[i]; i++ )
    {
        if( p_list_YUV[i] == fcc )
            return true;
    }

    for (size_t i = 0; i < ARRAY_SIZE(p_list_YUV_no_fallback); i++)
    {
        if (p_list_YUV_no_fallback[i] == fcc)
            return true;
    }
    return false;
}

#define PLANAR(n, w_den, h_den, bits) \
      .plane_count = n, \
      .p = { {.w = {1,    1}, .h = {1,    1}}, \
             {.w = {1,w_den}, .h = {1,h_den}}, \
             {.w = {1,w_den}, .h = {1,h_den}}, \
             {.w = {1,    1}, .h = {1,    1}} }, \
      .pixel_size = ((bits + 7) / 8), \
      .pixel_bits = bits

#define PLANAR_8(n, w_den, h_den)        PLANAR(n, w_den, h_den, 8)
#define PLANAR_16(n, w_den, h_den, bits) PLANAR(n, w_den, h_den, bits)

#define SEMIPLANAR(w_den, h_den, bits) \
      .plane_count = 2, \
      .p = { {.w = {1,    1}, .h = {1,    1}}, \
             {.w = {2,w_den}, .h = {1,h_den}} }, \
      .pixel_size = ((bits + 7) / 8), \
      .pixel_bits = bits

#define PACKED_FMT(size, bits) \
      .plane_count = 1, \
      .p = { {.w = {1,1}, .h = {1,1}} }, \
      .pixel_size = size, \
      .pixel_bits = bits

/* Zero planes for hardware picture handles. Cannot be manipulated directly. */
#define FAKE_FMT() \
      .plane_count = 0, \
      .p = { {.w = {1,1}, .h = {1,1}} }, \
      .pixel_size = 0, \
      .pixel_bits = 0

static const vlc_chroma_description_t p_list_chroma_description[] = {
    { VLC_CODEC_I411,                  PLANAR_8(3, 4, 1) },
    { VLC_CODEC_I410,                  PLANAR_8(3, 4, 4) },
    { VLC_CODEC_I420,                  PLANAR_8(3, 2, 2) },
    { VLC_CODEC_YV12,                  PLANAR_8(3, 2, 2) },
    { VLC_CODEC_NV12,                  SEMIPLANAR(2, 2, 8) },
    { VLC_CODEC_NV21,                  SEMIPLANAR(2, 2, 8) },
    { VLC_CODEC_I422,                  PLANAR_8(3, 2, 1) },
    { VLC_CODEC_NV16,                  SEMIPLANAR(2, 1, 8) },
    { VLC_CODEC_NV61,                  SEMIPLANAR(2, 1, 8) },
    { VLC_CODEC_I440,                  PLANAR_8(3, 1, 2) },
    { VLC_CODEC_I444,                  PLANAR_8(3, 1, 1) },
    { VLC_CODEC_NV24,                  SEMIPLANAR(1, 1, 8) },
    { VLC_CODEC_NV42,                  SEMIPLANAR(1, 1, 8) },
    { VLC_CODEC_YUVA,                  PLANAR_8(4, 1, 1) },
    { VLC_CODEC_YUV420A,               PLANAR_8(4, 2, 2) },
    { VLC_CODEC_YUV422A,               PLANAR_8(4, 2, 1) },

    { VLC_CODEC_GBR_PLANAR,            PLANAR_8(3, 1, 1) },
    { VLC_CODEC_GBR_PLANAR_9L,         PLANAR_16(3, 1, 1, 9) },
    { VLC_CODEC_GBR_PLANAR_9B,         PLANAR_16(3, 1, 1, 9) },
    { VLC_CODEC_GBR_PLANAR_10L,        PLANAR_16(3, 1, 1, 10) },
    { VLC_CODEC_GBR_PLANAR_10B,        PLANAR_16(3, 1, 1, 10) },
    { VLC_CODEC_GBR_PLANAR_12L,        PLANAR_16(3, 1, 1, 12) },
    { VLC_CODEC_GBR_PLANAR_12B,        PLANAR_16(3, 1, 1, 12) },
    { VLC_CODEC_GBR_PLANAR_14L,        PLANAR_16(3, 1, 1, 14) },
    { VLC_CODEC_GBR_PLANAR_14B,        PLANAR_16(3, 1, 1, 14) },
    { VLC_CODEC_GBR_PLANAR_16L,        PLANAR_16(3, 1, 1, 16) },
    { VLC_CODEC_GBR_PLANAR_16B,        PLANAR_16(3, 1, 1, 16) },
    { VLC_CODEC_GBRA_PLANAR,           PLANAR_8(4, 1, 1) },
    { VLC_CODEC_GBRA_PLANAR_10L,       PLANAR_16(4, 1, 1, 10) },
    { VLC_CODEC_GBRA_PLANAR_10B,       PLANAR_16(4, 1, 1, 10) },
    { VLC_CODEC_GBRA_PLANAR_12L,       PLANAR_16(4, 1, 1, 12) },
    { VLC_CODEC_GBRA_PLANAR_12B,       PLANAR_16(4, 1, 1, 12) },
    { VLC_CODEC_GBRA_PLANAR_16L,       PLANAR_16(4, 1, 1, 16) },
    { VLC_CODEC_GBRA_PLANAR_16B,       PLANAR_16(4, 1, 1, 16) },

    { VLC_CODEC_I420_16L,              PLANAR_16(3, 2, 2, 16) },
    { VLC_CODEC_I420_16B,              PLANAR_16(3, 2, 2, 16) },
    { VLC_CODEC_I420_12L,              PLANAR_16(3, 2, 2, 12) },
    { VLC_CODEC_I420_12B,              PLANAR_16(3, 2, 2, 12) },
    { VLC_CODEC_I420_10L,              PLANAR_16(3, 2, 2, 10) },
    { VLC_CODEC_I420_10B,              PLANAR_16(3, 2, 2, 10) },
    { VLC_CODEC_I420_9L,               PLANAR_16(3, 2, 2,  9) },
    { VLC_CODEC_I420_9B,               PLANAR_16(3, 2, 2,  9) },
    { VLC_CODEC_I422_16L,              PLANAR_16(3, 2, 1, 16) },
    { VLC_CODEC_I422_16B,              PLANAR_16(3, 2, 1, 16) },
    { VLC_CODEC_I422_12L,              PLANAR_16(3, 2, 1, 12) },
    { VLC_CODEC_I422_12B,              PLANAR_16(3, 2, 1, 12) },
    { VLC_CODEC_I422_10L,              PLANAR_16(3, 2, 1, 10) },
    { VLC_CODEC_I422_10B,              PLANAR_16(3, 2, 1, 10) },
    { VLC_CODEC_I422_9L,               PLANAR_16(3, 2, 1,  9) },
    { VLC_CODEC_I422_9B,               PLANAR_16(3, 2, 1,  9) },
    { VLC_CODEC_I444_12L,              PLANAR_16(3, 1, 1, 12) },
    { VLC_CODEC_I444_12B,              PLANAR_16(3, 1, 1, 12) },
    { VLC_CODEC_I444_10L,              PLANAR_16(3, 1, 1, 10) },
    { VLC_CODEC_I444_10B,              PLANAR_16(3, 1, 1, 10) },
    { VLC_CODEC_I444_9L,               PLANAR_16(3, 1, 1,  9) },
    { VLC_CODEC_I444_9B,               PLANAR_16(3, 1, 1,  9) },
    { VLC_CODEC_I444_16L,              PLANAR_16(3, 1, 1, 16) },
    { VLC_CODEC_I444_16B,              PLANAR_16(3, 1, 1, 16) },
    { VLC_CODEC_YUVA_444_10L,          PLANAR_16(4, 1, 1, 10) },
    { VLC_CODEC_YUVA_444_10B,          PLANAR_16(4, 1, 1, 10) },
    { VLC_CODEC_YUVA_444_12L,          PLANAR_16(4, 1, 1, 12) },
    { VLC_CODEC_YUVA_444_12B,          PLANAR_16(4, 1, 1, 12) },
    { VLC_CODEC_P010,                  SEMIPLANAR(2, 2, 10) },
    { VLC_CODEC_P012,                  SEMIPLANAR(2, 2, 12) },
    { VLC_CODEC_P016,                  SEMIPLANAR(2, 2, 16) },

    { VLC_CODEC_V308,                  PACKED_FMT(1, 24) },
    { VLC_CODEC_YUYV,                  PACKED_FMT(2, 16) },
    { VLC_CODEC_YVYU,                  PACKED_FMT(2, 16) },
    { VLC_CODEC_UYVY,                  PACKED_FMT(2, 16) },
    { VLC_CODEC_VYUY,                  PACKED_FMT(2, 16) },
    { VLC_CODEC_YUV2,                  PACKED_FMT(2, 16) },
    { VLC_CODEC_RGB233,                PACKED_FMT(1, 8) },
    { VLC_CODEC_BGR233,                PACKED_FMT(1, 8) },
    { VLC_CODEC_RGB332,                PACKED_FMT(1, 8) },
    { VLC_CODEC_YUVP,                  PACKED_FMT(1, 8) },
    { VLC_CODEC_RGBP,                  PACKED_FMT(1, 8) },
    { VLC_CODEC_GREY,                  PACKED_FMT(1, 8) },
    { VLC_CODEC_GREY_10L,              PACKED_FMT(2, 10) },
    { VLC_CODEC_GREY_10B,              PACKED_FMT(2, 10) },
    { VLC_CODEC_GREY_12L,              PACKED_FMT(2, 12) },
    { VLC_CODEC_GREY_12B,              PACKED_FMT(2, 12) },
    { VLC_CODEC_GREY_16L,              PACKED_FMT(2, 16) },
    { VLC_CODEC_GREY_16B,              PACKED_FMT(2, 16) },

    { VLC_CODEC_RGB555BE,              PACKED_FMT(2, 15) },
    { VLC_CODEC_RGB555LE,              PACKED_FMT(2, 15) },
    { VLC_CODEC_BGR555LE,              PACKED_FMT(2, 15) },
    { VLC_CODEC_BGR555BE,              PACKED_FMT(2, 15) },
    { VLC_CODEC_RGB565LE,              PACKED_FMT(2, 16) },
    { VLC_CODEC_RGB565BE,              PACKED_FMT(2, 16) },
    { VLC_CODEC_BGR565LE,              PACKED_FMT(2, 16) },
    { VLC_CODEC_BGR565BE,              PACKED_FMT(2, 16) },
    { VLC_CODEC_RGB24,                 PACKED_FMT(3, 24) },
    { VLC_CODEC_BGR24,                 PACKED_FMT(3, 24) },
    { VLC_CODEC_RGBX,                  PACKED_FMT(4, 24) },
    { VLC_CODEC_XRGB,                  PACKED_FMT(4, 24) },
    { VLC_CODEC_BGRX,                  PACKED_FMT(4, 24) },
    { VLC_CODEC_XBGR,                  PACKED_FMT(4, 24) },
    { VLC_CODEC_RGBA,                  PACKED_FMT(4, 32) },
    { VLC_CODEC_ARGB,                  PACKED_FMT(4, 32) },
    { VLC_CODEC_BGRA,                  PACKED_FMT(4, 32) },
    { VLC_CODEC_ABGR,                  PACKED_FMT(4, 32) },
    { VLC_CODEC_RGBA10LE,              PACKED_FMT(4, 32) },
    { VLC_CODEC_RGBA64,                PACKED_FMT(8, 64) },
    { VLC_CODEC_VUYA,                  PACKED_FMT(4, 32) },
    { VLC_CODEC_Y210,                  PACKED_FMT(4, 32) },
    { VLC_CODEC_Y410,                  PACKED_FMT(4, 32) },

    { VLC_CODEC_Y211,                 1, { {{1,4}, {1,1}} }, 4, 32 },
    { VLC_CODEC_XYZ_12L,               PACKED_FMT(6, 48) },
    { VLC_CODEC_XYZ_12B,               PACKED_FMT(6, 48) },

    { VLC_CODEC_VDPAU_VIDEO,           FAKE_FMT() },
    { VLC_CODEC_VDPAU_OUTPUT,          FAKE_FMT() },
    { VLC_CODEC_ANDROID_OPAQUE,        FAKE_FMT() },
    { VLC_CODEC_MMAL_OPAQUE,           FAKE_FMT() },
    { VLC_CODEC_D3D9_OPAQUE,           FAKE_FMT() },
    { VLC_CODEC_D3D11_OPAQUE,          FAKE_FMT() },
    { VLC_CODEC_D3D9_OPAQUE_10B,       FAKE_FMT() },
    { VLC_CODEC_D3D11_OPAQUE_10B,      FAKE_FMT() },
    { VLC_CODEC_D3D11_OPAQUE_RGBA,     FAKE_FMT() },
    { VLC_CODEC_D3D11_OPAQUE_BGRA,     FAKE_FMT() },
    { VLC_CODEC_D3D11_OPAQUE_ALPHA,    FAKE_FMT() },

    { VLC_CODEC_NVDEC_OPAQUE_16B,      FAKE_FMT() },
    { VLC_CODEC_NVDEC_OPAQUE_10B,      FAKE_FMT() },
    { VLC_CODEC_NVDEC_OPAQUE,          FAKE_FMT() },

    { VLC_CODEC_NVDEC_OPAQUE_444,      FAKE_FMT() },
    { VLC_CODEC_NVDEC_OPAQUE_444_16B,  FAKE_FMT() },

    { VLC_CODEC_CVPX_NV12,             FAKE_FMT() },
    { VLC_CODEC_CVPX_UYVY,             FAKE_FMT() },
    { VLC_CODEC_CVPX_I420,             FAKE_FMT() },
    { VLC_CODEC_CVPX_BGRA,             FAKE_FMT() },

    { VLC_CODEC_CVPX_P010,             FAKE_FMT() },

    { VLC_CODEC_GST_MEM_OPAQUE,        FAKE_FMT() },

    { VLC_CODEC_VAAPI_420,             FAKE_FMT() },
    { VLC_CODEC_VAAPI_420_10BPP,       FAKE_FMT() },
    { VLC_CODEC_VAAPI_420_12BPP,       FAKE_FMT() },
};

#undef PACKED_FMT
#undef PLANAR_16
#undef PLANAR_8
#undef PLANAR

const vlc_chroma_description_t *vlc_fourcc_GetChromaDescription( vlc_fourcc_t i_fourcc )
{
    for( size_t i = 0; i < ARRAY_SIZE(p_list_chroma_description); i++ )
    {
        if( p_list_chroma_description[i].fcc == i_fourcc )
            return &p_list_chroma_description[i];
    }
    return NULL;
}
