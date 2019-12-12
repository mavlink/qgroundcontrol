# pass through functions

.function audiopanoramam_orc_process_s16_ch1_none
.source 2 s1 gint16
.dest 4 d1 gint16

mergewl d1 s1 s1


.function audiopanoramam_orc_process_f32_ch1_none
.source 4 s1 gfloat
.dest 8 d1 gfloat

mergelq d1 s1 s1


.function audiopanoramam_orc_process_s16_ch2_none
.source 4 s1 gint16
.dest 4 d1 gint16

x2 copyw d1 s1


.function audiopanoramam_orc_process_f32_ch2_none
.source 8 s1 gfloat
.dest 8 d1 gfloat

x2 copyl d1 s1


# psychoacoustic processing function

.function audiopanoramam_orc_process_s16_ch1_psy
.source 2 s1 gint16
.dest 4 d1 gint16
.floatparam 4 lpan
.floatparam 4 rpan
.temp 8 t1
.temp 4 left
.temp 4 right

convswl left s1
convlf left left
mulf right left rpan
mulf left left lpan
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_f32_ch1_psy
.source 4 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 lpan
.floatparam 4 rpan
.temp 4 left
.temp 4 right

mulf right s1 rpan
mulf left s1 lpan
mergelq d1 left right


.function audiopanoramam_orc_process_s16_ch2_psy_right
.source 4 s1 gint16
.dest 4 d1 gint16
.floatparam 4 llpan
.floatparam 4 rlpan
.temp 8 t1
.temp 4 left
.temp 4 right
.temp 4 right1

x2 convswl t1 s1
x2 convlf t1 t1
select0ql left t1
select1ql right t1
mulf right1 left rlpan
mulf left left llpan
addf right right1 right
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_s16_ch2_psy_left
.source 4 s1 gint16
.dest 4 d1 gint16
.floatparam 4 lrpan
.floatparam 4 rrpan
.temp 8 t1
.temp 4 left
.temp 4 left1
.temp 4 right

x2 convswl t1 s1
x2 convlf t1 t1
select0ql left t1
select1ql right t1
mulf left1 right lrpan
mulf right right rrpan
addf left left1 left
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_f32_ch2_psy_right
.source 8 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 llpan
.floatparam 4 rlpan
.temp 4 left
.temp 4 right
.temp 4 right1

select0ql left s1
select1ql right s1
mulf right1 left rlpan
mulf left left llpan
addf right right1 right
mergelq d1 left right


.function audiopanoramam_orc_process_f32_ch2_psy_left
.source 8 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 lrpan
.floatparam 4 rrpan
.temp 4 left
.temp 4 left1
.temp 4 right

select0ql left s1
select1ql right s1
mulf left1 right lrpan
mulf right right rrpan
addf left left1 left
mergelq d1 left right

# simple processing functions

.function audiopanoramam_orc_process_s16_ch1_sim_right
.source 2 s1 gint16
.dest 4 d1 gint16
.floatparam 4 rpan
.temp 8 t1
.temp 4 left
.temp 4 right

convswl left s1
convlf left left
mulf right left rpan
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_s16_ch1_sim_left
.source 2 s1 gint16
.dest 4 d1 gint16
.floatparam 4 lpan
.temp 8 t1
.temp 4 left
.temp 4 right

convswl right s1
convlf right right
mulf left right lpan
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_s16_ch2_sim_right
.source 4 s1 gint16
.dest 4 d1 gint16
.floatparam 4 rpan
.temp 8 t1
.temp 4 left
.temp 4 right

x2 convswl t1 s1
x2 convlf t1 t1
select0ql left t1
select1ql right t1
mulf right right rpan
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_s16_ch2_sim_left
.source 4 s1 gint16
.dest 4 d1 gint16
.floatparam 4 lpan
.temp 8 t1
.temp 4 left
.temp 4 right

x2 convswl t1 s1
x2 convlf t1 t1
select0ql left t1
select1ql right t1
mulf left left lpan
mergelq t1 left right
x2 convfl t1 t1
x2 convssslw d1 t1


.function audiopanoramam_orc_process_f32_ch1_sim_right
.source 4 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 rpan
.temp 4 left
.temp 4 right

copyl left s1
mulf right s1 rpan
mergelq d1 left right


.function audiopanoramam_orc_process_f32_ch1_sim_left
.source 4 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 lpan
.temp 4 left
.temp 4 right

mulf left s1 lpan
copyl right s1
mergelq d1 left right


.function audiopanoramam_orc_process_f32_ch2_sim_right
.source 8 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 rpan
.temp 4 left
.temp 4 right

select0ql left s1
select1ql right s1
mulf right right rpan
mergelq d1 left right

.function audiopanoramam_orc_process_f32_ch2_sim_left
.source 8 s1 gfloat
.dest 8 d1 gfloat
.floatparam 4 lpan
.temp 4 left
.temp 4 right

select0ql left s1
select1ql right s1
mulf left left lpan
mergelq d1 left right

