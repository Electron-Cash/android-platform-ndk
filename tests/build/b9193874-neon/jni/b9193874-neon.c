#include <arm_neon.h>

static const int16_t coef[4] = { 89858 / 4, 22014, 45773 / 2, 113618 / 4 };

void UpsampleRgbaLinePairNEON(const uint8_t *top_y, const uint8_t *bottom_y, const uint8_t *top_u, const uint8_t *top_v, const uint8_t *cur_u, const uint8_t *cur_v, uint8_t *top_dst, uint8_t *bottom_dst, int len)
{
    int block;
    uint8_t uv_buf[2 * 32 + 15];
    uint8_t *const r_uv = (uint8_t*)((uintptr_t)(uv_buf + 15) & ~15);
    const int uv_len = (len + 1) >> 1;
    const int num_blocks = (uv_len - 1) >> 3;
    const int leftover = uv_len - num_blocks * 8;
    const int last_pos = 1 + 16 * num_blocks;
    const int u_diag = ((top_u[0] + cur_u[0]) >> 1) + 1;
    const int v_diag = ((top_v[0] + cur_v[0]) >> 1) + 1;
    const int16x4_t cf16 = vld1_s16(coef);
    const int32x2_t cf32 = vmov_n_s32(76283);
    const uint8x8_t u16 = vmov_n_u8(16);
    const uint8x8_t u128 = vmov_n_u8(128);
    for (block = 0; block < num_blocks; ++block) {
        {
            uint8x8_t a = vld1_u8(top_u);
            uint8x8_t b = vld1_u8(top_u + 1);
            uint8x8_t c = vld1_u8(cur_u);
            uint8x8_t d = vld1_u8(cur_u + 1);
            uint16x8_t al = vshll_n_u8(a, 1);
            uint16x8_t bl = vshll_n_u8(b, 1);
            uint16x8_t cl = vshll_n_u8(c, 1);
            uint16x8_t dl = vshll_n_u8(d, 1);
            uint8x8_t diag1, diag2;
            uint16x8_t sl;
            sl = vaddl_u8(a, b);
            sl = vaddw_u8(sl, c);
            sl = vaddw_u8(sl, d);
            al = vaddq_u16(sl, al);
            bl = vaddq_u16(sl, bl);
            al = vaddq_u16(al, dl);
            bl = vaddq_u16(bl, cl);
            diag2 = vshrn_n_u16(al, 3);
            diag1 = vshrn_n_u16(bl, 3);
            a = vrhadd_u8(a, diag1);
            b = vrhadd_u8(b, diag2);
            c = vrhadd_u8(c, diag2);
            d = vrhadd_u8(d, diag1);
            {
                const uint8x8x2_t a_b = {{ a, b }};
                const uint8x8x2_t c_d = {{ c, d }};
                vst2_u8(r_uv, a_b);
                vst2_u8(r_uv + 32, c_d);
            }
        }
        {
            uint8x8_t a = vld1_u8(top_v);
            uint8x8_t b = vld1_u8(top_v + 1);
            uint8x8_t c = vld1_u8(cur_v);
            uint8x8_t d = vld1_u8(cur_v + 1);
            uint16x8_t al = vshll_n_u8(a, 1);
            uint16x8_t bl = vshll_n_u8(b, 1);
            uint16x8_t cl = vshll_n_u8(c, 1);
            uint16x8_t dl = vshll_n_u8(d, 1);
            uint8x8_t diag1, diag2;
            uint16x8_t sl;
            sl = vaddl_u8(a, b);
            sl = vaddw_u8(sl, c);
            sl = vaddw_u8(sl, d);
            al = vaddq_u16(sl, al);
            bl = vaddq_u16(sl, bl);
            al = vaddq_u16(al, dl);
            bl = vaddq_u16(bl, cl);
            diag2 = vshrn_n_u16(al, 3);
            diag1 = vshrn_n_u16(bl, 3);
            a = vrhadd_u8(a, diag1);
            b = vrhadd_u8(b, diag2);
            c = vrhadd_u8(c, diag2);
            d = vrhadd_u8(d, diag1);
            {
                const uint8x8x2_t a_b = {{ a, b }};
                const uint8x8x2_t c_d = {{ c, d }};
                vst2_u8(r_uv + 16, a_b);
                vst2_u8(r_uv + 16 + 32, c_d);
            }
        }
        {
            if (top_y) {
                {
                    int i;
                    for (i = 0; i < 16; i += 8) {
                        int off = ((16 * block + 1) + i) * 4;
                        uint8x8_t y = vld1_u8(top_y + (16 * block + 1) + i);
                        uint8x8_t u = vld1_u8((r_uv) + i);
                        uint8x8_t v = vld1_u8((r_uv) + i + 16);
                        int16x8_t yy = vreinterpretq_s16_u16(vsubl_u8(y, u16));
                        int16x8_t uu = vreinterpretq_s16_u16(vsubl_u8(u, u128));
                        int16x8_t vv = vreinterpretq_s16_u16(vsubl_u8(v, u128));
                        int16x8_t ud = vshlq_n_s16(uu, 1);
                        int16x8_t vd = vshlq_n_s16(vv, 1);
                        int32x4_t vrl = vqdmlal_lane_s16(vshll_n_s16(vget_low_s16(vv), 1), vget_low_s16(vd), cf16, 0);
                        int32x4_t vrh = vqdmlal_lane_s16(vshll_n_s16(vget_high_s16(vv), 1), vget_high_s16(vd), cf16, 0);
                        int16x8_t vr = vcombine_s16(vrshrn_n_s32(vrl, 16), vrshrn_n_s32(vrh, 16));
                        int32x4_t vl = vmovl_s16(vget_low_s16(vv));
                        int32x4_t vh = vmovl_s16(vget_high_s16(vv));
                        int32x4_t ugl = vmlal_lane_s16(vl, vget_low_s16(uu), cf16, 1);
                        int32x4_t ugh = vmlal_lane_s16(vh, vget_high_s16(uu), cf16, 1);
                        int32x4_t gcl = vqdmlal_lane_s16(ugl, vget_low_s16(vv), cf16, 2);
                        int32x4_t gch = vqdmlal_lane_s16(ugh, vget_high_s16(vv), cf16, 2);
                        int16x8_t gc = vcombine_s16(vrshrn_n_s32(gcl, 16), vrshrn_n_s32(gch, 16));
                        int32x4_t ubl = vqdmlal_lane_s16(vshll_n_s16(vget_low_s16(uu), 1), vget_low_s16(ud), cf16, 3);
                        int32x4_t ubh = vqdmlal_lane_s16(vshll_n_s16(vget_high_s16(uu), 1), vget_high_s16(ud), cf16, 3);
                        int16x8_t ub = vcombine_s16(vrshrn_n_s32(ubl, 16), vrshrn_n_s32(ubh, 16));
                        int32x4_t rl = vaddl_s16(vget_low_s16(yy), vget_low_s16(vr));
                        int32x4_t rh = vaddl_s16(vget_high_s16(yy), vget_high_s16(vr));
                        int32x4_t gl = vsubl_s16(vget_low_s16(yy), vget_low_s16(gc));
                        int32x4_t gh = vsubl_s16(vget_high_s16(yy), vget_high_s16(gc));
                        int32x4_t bl = vaddl_s16(vget_low_s16(yy), vget_low_s16(ub));
                        int32x4_t bh = vaddl_s16(vget_high_s16(yy), vget_high_s16(ub));
                        rl = vmulq_lane_s32(rl, cf32, 0);
                        rh = vmulq_lane_s32(rh, cf32, 0);
                        gl = vmulq_lane_s32(gl, cf32, 0);
                        gh = vmulq_lane_s32(gh, cf32, 0);
                        bl = vmulq_lane_s32(bl, cf32, 0);
                        bh = vmulq_lane_s32(bh, cf32, 0);
                        y = vqmovun_s16(vcombine_s16(vrshrn_n_s32(rl, 16), vrshrn_n_s32(rh, 16)));
                        u = vqmovun_s16(vcombine_s16(vrshrn_n_s32(gl, 16), vrshrn_n_s32(gh, 16)));
                        v = vqmovun_s16(vcombine_s16(vrshrn_n_s32(bl, 16), vrshrn_n_s32(bh, 16)));
                        do {
                            const uint8x8x4_t r_g_b_v255 = {{ y, u, v, vmov_n_u8(255) }};
                            vst4_u8(top_dst + off, r_g_b_v255); 
                        } while (0);
                    }
                }
            }
            if (bottom_y) {
                {
                    int i;
                    for (i = 0; i < 16; i += 8) {
                        int off = ((16 * block + 1) + i) * 4;
                        uint8x8_t y = vld1_u8(bottom_y + (16 * block + 1) + i);
                        uint8x8_t u = vld1_u8(((r_uv) + 32) + i);
                        uint8x8_t v = vld1_u8(((r_uv) + 32) + i + 16);
                        int16x8_t yy = vreinterpretq_s16_u16(vsubl_u8(y, u16));
                        int16x8_t uu = vreinterpretq_s16_u16(vsubl_u8(u, u128));
                        int16x8_t vv = vreinterpretq_s16_u16(vsubl_u8(v, u128));
                        int16x8_t ud = vshlq_n_s16(uu, 1);
                        int16x8_t vd = vshlq_n_s16(vv, 1);
                        int32x4_t vrl = vqdmlal_lane_s16(vshll_n_s16(vget_low_s16(vv), 1), vget_low_s16(vd), cf16, 0);
                        int32x4_t vrh = vqdmlal_lane_s16(vshll_n_s16(vget_high_s16(vv), 1), vget_high_s16(vd), cf16, 0);
                        int16x8_t vr = vcombine_s16(vrshrn_n_s32(vrl, 16), vrshrn_n_s32(vrh, 16));
                        int32x4_t vl = vmovl_s16(vget_low_s16(vv));
                        int32x4_t vh = vmovl_s16(vget_high_s16(vv));
                        int32x4_t ugl = vmlal_lane_s16(vl, vget_low_s16(uu), cf16, 1);
                        int32x4_t ugh = vmlal_lane_s16(vh, vget_high_s16(uu), cf16, 1);
                        int32x4_t gcl = vqdmlal_lane_s16(ugl, vget_low_s16(vv), cf16, 2);
                        int32x4_t gch = vqdmlal_lane_s16(ugh, vget_high_s16(vv), cf16, 2);
                        int16x8_t gc = vcombine_s16(vrshrn_n_s32(gcl, 16), vrshrn_n_s32(gch, 16));
                        int32x4_t ubl = vqdmlal_lane_s16(vshll_n_s16(vget_low_s16(uu), 1), vget_low_s16(ud), cf16, 3);
                        int32x4_t ubh = vqdmlal_lane_s16(vshll_n_s16(vget_high_s16(uu), 1), vget_high_s16(ud), cf16, 3);
                        int16x8_t ub = vcombine_s16(vrshrn_n_s32(ubl, 16), vrshrn_n_s32(ubh, 16));
                        int32x4_t rl = vaddl_s16(vget_low_s16(yy), vget_low_s16(vr));
                        int32x4_t rh = vaddl_s16(vget_high_s16(yy), vget_high_s16(vr));
                        int32x4_t gl = vsubl_s16(vget_low_s16(yy), vget_low_s16(gc));
                        int32x4_t gh = vsubl_s16(vget_high_s16(yy), vget_high_s16(gc));
                        int32x4_t bl = vaddl_s16(vget_low_s16(yy), vget_low_s16(ub));
                        int32x4_t bh = vaddl_s16(vget_high_s16(yy), vget_high_s16(ub));
                        rl = vmulq_lane_s32(rl, cf32, 0);
                        rh = vmulq_lane_s32(rh, cf32, 0);
                        gl = vmulq_lane_s32(gl, cf32, 0);
                        gh = vmulq_lane_s32(gh, cf32, 0);
                        bl = vmulq_lane_s32(bl, cf32, 0);
                        bh = vmulq_lane_s32(bh, cf32, 0);
                        y = vqmovun_s16(vcombine_s16(vrshrn_n_s32(rl, 16), vrshrn_n_s32(rh, 16)));
                        u = vqmovun_s16(vcombine_s16(vrshrn_n_s32(gl, 16), vrshrn_n_s32(gh, 16)));
                        v = vqmovun_s16(vcombine_s16(vrshrn_n_s32(bl, 16), vrshrn_n_s32(bh, 16)));
                        do {
                            const uint8x8x4_t r_g_b_v255 = {{ y, u, v, vmov_n_u8(255) }};
                            vst4_u8(bottom_dst + off, r_g_b_v255);
                        } while (0);
                    }
                }
            }
        }
    }
}
