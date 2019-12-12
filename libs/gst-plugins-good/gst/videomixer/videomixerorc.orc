.function video_mixer_orc_splat_u32
.dest 4 d1 guint32
.param 4 p1 guint32

copyl d1, p1

.function video_mixer_orc_memcpy_u32
.dest 4 d1 guint32
.source 4 s1 guint32

copyl d1, s1

.function video_mixer_orc_blend_u8
.flags 2d
.dest 1 d1 guint8
.source 1 s1 guint8
.param 2 p1
.temp 2 t1
.temp 2 t2
.const 1 c1 8 

convubw t1, d1
convubw t2, s1
subw t2, t2, t1
mullw t2, t2, p1
shlw t1, t1, c1
addw t2, t1, t2
shruw t2, t2, c1
convsuswb d1, t2


.function video_mixer_orc_blend_argb
.flags 2d
.dest 4 d guint8
.source 4 s guint8
.param 2 alpha
.temp 4 t
.temp 2 tw
.temp 1 tb
.temp 4 a
.temp 8 d_wide
.temp 8 s_wide
.temp 8 a_wide
.const 4 a_alpha 0x000000ff

loadl t, s
convlw tw, t
convwb tb, tw
splatbl a, tb
x4 convubw a_wide, a
x4 mullw a_wide, a_wide, alpha
x4 shruw a_wide, a_wide, 8
x4 convubw s_wide, t
loadl t, d
x4 convubw d_wide, t
x4 subw s_wide, s_wide, d_wide
x4 mullw s_wide, s_wide, a_wide
x4 div255w s_wide, s_wide
x4 addw d_wide, d_wide, s_wide
x4 convwb t, d_wide
orl t, t, a_alpha
storel d, t

.function video_mixer_orc_blend_bgra
.flags 2d
.dest 4 d guint8
.source 4 s guint8
.param 2 alpha
.temp 4 t
.temp 4 t2
.temp 2 tw
.temp 1 tb
.temp 4 a
.temp 8 d_wide
.temp 8 s_wide
.temp 8 a_wide
.const 4 a_alpha 0xff000000

loadl t, s
shrul t2, t, 24
convlw tw, t2
convwb tb, tw
splatbl a, tb
x4 convubw a_wide, a
x4 mullw a_wide, a_wide, alpha
x4 shruw a_wide, a_wide, 8
x4 convubw s_wide, t
loadl t, d
x4 convubw d_wide, t
x4 subw s_wide, s_wide, d_wide
x4 mullw s_wide, s_wide, a_wide
x4 div255w s_wide, s_wide
x4 addw d_wide, d_wide, s_wide
x4 convwb t, d_wide
orl t, t, a_alpha
storel d, t


.function video_mixer_orc_overlay_argb
.flags 2d
.dest 4 d guint8
.source 4 s guint8
.param 2 alpha
.temp 4 t
.temp 2 tw
.temp 1 tb
.temp 8 alpha_s
.temp 8 alpha_s_inv
.temp 8 alpha_d
.temp 4 a
.temp 8 d_wide
.temp 8 s_wide
.const 4 xfs 0xffffffff
.const 4 a_alpha 0x000000ff
.const 4 a_alpha_inv 0xffffff00

# calc source alpha as alpha_s = alpha_s * alpha / 256
loadl t, s
convlw tw, t
convwb tb, tw
splatbl a, tb
x4 convubw alpha_s, a
x4 mullw alpha_s, alpha_s, alpha
x4 shruw alpha_s, alpha_s, 8
x4 convubw s_wide, t
x4 mullw s_wide, s_wide, alpha_s

# calc destination alpha as alpha_d = (255-alpha_s) * alpha_d / 255
loadpl a, xfs
x4 convubw alpha_s_inv, a
x4 subw alpha_s_inv, alpha_s_inv, alpha_s
loadl t, d
convlw tw, t
convwb tb, tw
splatbl a, tb
x4 convubw alpha_d, a
x4 mullw alpha_d, alpha_d, alpha_s_inv
x4 div255w alpha_d, alpha_d
x4 convubw d_wide, t
x4 mullw d_wide, d_wide, alpha_d

# calc final pixel as pix_d = pix_s*alpha_s + pix_d*alpha_d*(255-alpha_s)/255
x4 addw d_wide, d_wide, s_wide

# calc the final destination alpha_d = alpha_s + alpha_d * (255-alpha_s)/255
x4 addw alpha_d, alpha_d, alpha_s

# now normalize the pix_d by the final alpha to make it associative
x4 divluw, d_wide, d_wide, alpha_d

# pack the new alpha into the correct spot
x4 convwb t, d_wide
andl t, t, a_alpha_inv
x4 convwb a, alpha_d
andl a, a, a_alpha
orl  t, t, a
storel d, t

.function video_mixer_orc_overlay_bgra
.flags 2d
.dest 4 d guint8
.source 4 s guint8
.param 2 alpha
.temp 4 t
.temp 4 t2
.temp 2 tw
.temp 1 tb
.temp 8 alpha_s
.temp 8 alpha_s_inv
.temp 8 alpha_d
.temp 4 a
.temp 8 d_wide
.temp 8 s_wide
.const 4 xfs 0xffffffff
.const 4 a_alpha 0xff000000
.const 4 a_alpha_inv 0x00ffffff

# calc source alpha as alpha_s = alpha_s * alpha / 256
loadl t, s
shrul t2, t, 24
convlw tw, t2
convwb tb, tw
splatbl a, tb
x4 convubw alpha_s, a
x4 mullw alpha_s, alpha_s, alpha
x4 shruw alpha_s, alpha_s, 8
x4 convubw s_wide, t
x4 mullw s_wide, s_wide, alpha_s

# calc destination alpha as alpha_d = (255-alpha_s) * alpha_d / 255
loadpl a, xfs
x4 convubw alpha_s_inv, a
x4 subw alpha_s_inv, alpha_s_inv, alpha_s
loadl t, d
shrul t2, t, 24
convlw tw, t2
convwb tb, tw
splatbl a, tb
x4 convubw alpha_d, a
x4 mullw alpha_d, alpha_d, alpha_s_inv
x4 div255w alpha_d, alpha_d
x4 convubw d_wide, t
x4 mullw d_wide, d_wide, alpha_d

# calc final pixel as pix_d = pix_s*alpha_s + pix_d*alpha_d*(255-alpha_s)/255
x4 addw d_wide, d_wide, s_wide

# calc the final destination alpha_d = alpha_s + alpha_d * (255-alpha_s)/255
x4 addw alpha_d, alpha_d, alpha_s

# now normalize the pix_d by the final alpha to make it associative
x4 divluw, d_wide, d_wide, alpha_d

# pack the new alpha into the correct spot
x4 convwb t, d_wide
andl t, t, a_alpha_inv
x4 convwb a, alpha_d
andl a, a, a_alpha
orl  t, t, a
storel d, t

