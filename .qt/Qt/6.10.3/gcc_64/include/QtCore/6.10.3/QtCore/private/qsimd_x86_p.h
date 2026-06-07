// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// This is a generated file. DO NOT EDIT.
// Please see util/x86simdgen/README.md

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// This is a generated file. DO NOT EDIT.
// Please see util/x86simdgen/README.md
#ifndef QSIMD_X86_P_H
#define QSIMD_X86_P_H

#include <stdint.h>

// in CPUID Leaf 1, EDX:
#define cpu_feature_sse2                            (UINT64_C(1) << 0)

// in CPUID Leaf 1, ECX:
#define cpu_feature_sse3                            (UINT64_C(1) << 1)
#define cpu_feature_ssse3                           (UINT64_C(1) << 2)
#define cpu_feature_fma                             (UINT64_C(1) << 3)
#define cpu_feature_sse4_1                          (UINT64_C(1) << 4)
#define cpu_feature_sse4_2                          (UINT64_C(1) << 5)
#define cpu_feature_movbe                           (UINT64_C(1) << 6)
#define cpu_feature_popcnt                          (UINT64_C(1) << 7)
#define cpu_feature_aes                             (UINT64_C(1) << 8)
#define cpu_feature_avx                             (UINT64_C(1) << 9)
#define cpu_feature_f16c                            (UINT64_C(1) << 10)

// in CPUID Leaf 7, Sub-leaf 0, EBX:
#define cpu_feature_bmi                             (UINT64_C(1) << 11)
#define cpu_feature_avx2                            (UINT64_C(1) << 12)
#define cpu_feature_bmi2                            (UINT64_C(1) << 13)
#define cpu_feature_avx512f                         (UINT64_C(1) << 14)
#define cpu_feature_avx512dq                        (UINT64_C(1) << 15)
#define cpu_feature_avx512ifma                      (UINT64_C(1) << 16)
#define cpu_feature_avx512cd                        (UINT64_C(1) << 17)
#define cpu_feature_sha                             (UINT64_C(1) << 18)
#define cpu_feature_avx512bw                        (UINT64_C(1) << 19)
#define cpu_feature_avx512vl                        (UINT64_C(1) << 20)

// in CPUID Leaf 7, Sub-leaf 0, ECX:
#define cpu_feature_avx512vbmi                      (UINT64_C(1) << 21)
#define cpu_feature_waitpkg                         (UINT64_C(1) << 22)
#define cpu_feature_avx512vbmi2                     (UINT64_C(1) << 23)
#define cpu_feature_shstk                           (UINT64_C(1) << 24)
#define cpu_feature_gfni                            (UINT64_C(1) << 25)
#define cpu_feature_vaes                            (UINT64_C(1) << 26)
#define cpu_feature_avx512bitalg                    (UINT64_C(1) << 27)
#define cpu_feature_avx512vpopcntdq                 (UINT64_C(1) << 28)

// in CPUID Leaf 7, Sub-leaf 0, EDX:
#define cpu_feature_hybrid                          (UINT64_C(1) << 29)
#define cpu_feature_ibt                             (UINT64_C(1) << 30)
#define cpu_feature_avx512fp16                      (UINT64_C(1) << 31)

// in CPUID Leaf 7, Sub-leaf 1, EAX:
#define cpu_feature_raoint                          (UINT64_C(1) << 32)
#define cpu_feature_cmpccxadd                       (UINT64_C(1) << 33)
#define cpu_feature_avxifma                         (UINT64_C(1) << 34)
#define cpu_feature_lam                             (UINT64_C(1) << 35)

// CPU architectures
#define cpu_x86_64              (0 \
                                 | cpu_feature_sse2)
#define cpu_core2               (cpu_x86_64 \
                                 | cpu_feature_sse3 \
                                 | cpu_feature_ssse3)
#define cpu_nhm                 (cpu_core2 \
                                 | cpu_feature_sse4_1 \
                                 | cpu_feature_sse4_2 \
                                 | cpu_feature_popcnt)
#define cpu_wsm                 (cpu_nhm)
#define cpu_snb                 (cpu_wsm \
                                 | cpu_feature_avx)
#define cpu_ivb                 (cpu_snb \
                                 | cpu_feature_f16c)
#define cpu_hsw                 (cpu_ivb \
                                 | cpu_feature_avx2 \
                                 | cpu_feature_fma \
                                 | cpu_feature_bmi \
                                 | cpu_feature_bmi2 \
                                 | cpu_feature_movbe)
#define cpu_bdw                 (cpu_hsw)
#define cpu_bdx                 (cpu_bdw)
#define cpu_skl                 (cpu_bdw)
#define cpu_skx                 (cpu_skl \
                                 | cpu_feature_avx512f \
                                 | cpu_feature_avx512dq \
                                 | cpu_feature_avx512cd \
                                 | cpu_feature_avx512bw \
                                 | cpu_feature_avx512vl)
#define cpu_clx                 (cpu_skx)
#define cpu_cpx                 (cpu_clx)
#define cpu_plc                 (cpu_skx \
                                 | cpu_feature_avx512ifma \
                                 | cpu_feature_avx512vbmi)
#define cpu_snc                 (cpu_plc \
                                 | cpu_feature_avx512vbmi2 \
                                 | cpu_feature_gfni \
                                 | cpu_feature_vaes \
                                 | cpu_feature_avx512bitalg \
                                 | cpu_feature_avx512vpopcntdq)
#define cpu_wlc                 (cpu_snc \
                                 | cpu_feature_shstk \
                                 | cpu_feature_ibt)
#define cpu_glc                 (cpu_wlc \
                                 | cpu_feature_waitpkg)
#define cpu_rpc                 (cpu_glc)
#define cpu_rwc                 (cpu_rpc)
#define cpu_slm                 (cpu_wsm \
                                 | cpu_feature_movbe)
#define cpu_glm                 (cpu_slm)
#define cpu_tnt                 (cpu_glm \
                                 | cpu_feature_gfni \
                                 | cpu_feature_waitpkg)
#define cpu_grt                 (cpu_skl \
                                 | cpu_feature_gfni \
                                 | cpu_feature_vaes \
                                 | cpu_feature_shstk \
                                 | cpu_feature_ibt \
                                 | cpu_feature_waitpkg)
#define cpu_cmt                 (cpu_grt \
                                 | cpu_feature_cmpccxadd \
                                 | cpu_feature_avxifma)
#define cpu_cnl                 (cpu_plc)
#define cpu_icl                 (cpu_snc)
#define cpu_tgl                 (cpu_wlc)
#define cpu_adl                 (cpu_grt)
#define cpu_rpl                 (cpu_grt)
#define cpu_mtl                 (cpu_cmt)
#define cpu_arl                 (cpu_cmt)
#define cpu_lnl                 (cpu_cmt)
#define cpu_icx                 (cpu_snc)
#define cpu_spr                 (cpu_glc)
#define cpu_emr                 (cpu_spr)
#define cpu_gnr                 (cpu_glc)
#define cpu_srf                 (cpu_cmt \
                                 | cpu_feature_cmpccxadd \
                                 | cpu_feature_avxifma)
#define cpu_grr                 (cpu_srf \
                                 | cpu_feature_raoint)
#define cpu_cwf                 (cpu_srf)
#define cpu_nehalem             (cpu_nhm)
#define cpu_westmere            (cpu_wsm)
#define cpu_sandybridge         (cpu_snb)
#define cpu_ivybridge           (cpu_ivb)
#define cpu_haswell             (cpu_hsw)
#define cpu_broadwell           (cpu_bdw)
#define cpu_skylake             (cpu_skl)
#define cpu_skylake_avx512      (cpu_skx)
#define cpu_cascadelake         (cpu_clx)
#define cpu_cooperlake          (cpu_cpx)
#define cpu_palmcove            (cpu_plc)
#define cpu_cannonlake          (cpu_cnl)
#define cpu_sunnycove           (cpu_snc)
#define cpu_icelake_client      (cpu_icl)
#define cpu_icelake_server      (cpu_icx)
#define cpu_willowcove          (cpu_wlc)
#define cpu_tigerlake           (cpu_tgl)
#define cpu_goldencove          (cpu_glc)
#define cpu_alderlake           (cpu_adl)
#define cpu_raptorcove          (cpu_rpc)
#define cpu_raptorlake          (cpu_rpl)
#define cpu_redwoodcove         (cpu_rwc)
#define cpu_meteorlake          (cpu_mtl)
#define cpu_arrowlake           (cpu_arl)
#define cpu_lunarlake           (cpu_lnl)
#define cpu_sapphirerapids      (cpu_spr)
#define cpu_emeraldrapids       (cpu_emr)
#define cpu_graniterapids       (cpu_gnr)
#define cpu_silvermont          (cpu_slm)
#define cpu_goldmont            (cpu_glm)
#define cpu_tremont             (cpu_tnt)
#define cpu_gracemont           (cpu_grt)
#define cpu_crestmont           (cpu_cmt)
#define cpu_grandridge          (cpu_grr)
#define cpu_sierraforest        (cpu_srf)
#define cpu_clearwaterforest    (cpu_cwf)

// __attribute__ target strings for GCC and Clang
#define QT_FUNCTION_TARGET_STRING_SSE2              "sse2"
#define QT_FUNCTION_TARGET_STRING_SSE3              "sse3"
#define QT_FUNCTION_TARGET_STRING_SSSE3             "ssse3"
#define QT_FUNCTION_TARGET_STRING_FMA               "fma"
#define QT_FUNCTION_TARGET_STRING_SSE4_1            "sse4.1"
#define QT_FUNCTION_TARGET_STRING_SSE4_2            "sse4.2"
#define QT_FUNCTION_TARGET_STRING_MOVBE             "movbe"
#define QT_FUNCTION_TARGET_STRING_POPCNT            "popcnt"
#define QT_FUNCTION_TARGET_STRING_AES               "aes,sse4.2"
#define QT_FUNCTION_TARGET_STRING_AVX               "avx"
#define QT_FUNCTION_TARGET_STRING_F16C              "f16c,avx"
#define QT_FUNCTION_TARGET_STRING_BMI               "bmi"
#define QT_FUNCTION_TARGET_STRING_AVX2              "avx2,avx"
#define QT_FUNCTION_TARGET_STRING_BMI2              "bmi2"
#define QT_FUNCTION_TARGET_STRING_AVX512F           "avx512f,avx"
#define QT_FUNCTION_TARGET_STRING_AVX512DQ          "avx512dq,avx512f"
#define QT_FUNCTION_TARGET_STRING_AVX512IFMA        "avx512ifma,avx512f"
#define QT_FUNCTION_TARGET_STRING_AVX512CD          "avx512cd,avx512f"
#define QT_FUNCTION_TARGET_STRING_SHA               "sha"
#define QT_FUNCTION_TARGET_STRING_AVX512BW          "avx512bw,avx512f"
#define QT_FUNCTION_TARGET_STRING_AVX512VL          "avx512vl,avx512f"
#define QT_FUNCTION_TARGET_STRING_AVX512VBMI        "avx512vbmi,avx512f"
#define QT_FUNCTION_TARGET_STRING_WAITPKG           "waitpkg"
#define QT_FUNCTION_TARGET_STRING_AVX512VBMI2       "avx512vbmi2,avx512f"
#define QT_FUNCTION_TARGET_STRING_SHSTK             "shstk"
#define QT_FUNCTION_TARGET_STRING_GFNI              "gfni"
#define QT_FUNCTION_TARGET_STRING_VAES              "vaes,avx2,avx,aes"
#define QT_FUNCTION_TARGET_STRING_AVX512BITALG      "avx512bitalg,avx512f"
#define QT_FUNCTION_TARGET_STRING_AVX512VPOPCNTDQ   "avx512vpopcntdq,avx512f"
#define QT_FUNCTION_TARGET_STRING_HYBRID            "hybrid"
#define QT_FUNCTION_TARGET_STRING_IBT               "ibt"
#define QT_FUNCTION_TARGET_STRING_AVX512FP16        "avx512fp16,avx512f,f16c"
#define QT_FUNCTION_TARGET_STRING_RAOINT            "raoint"
#define QT_FUNCTION_TARGET_STRING_CMPCCXADD         "cmpccxadd"
#define QT_FUNCTION_TARGET_STRING_AVXIFMA           "avxifma,avx"
#define QT_FUNCTION_TARGET_STRING_LAM               "lam"
#define QT_FUNCTION_TARGET_STRING_ARCH_X86_64       "sse2"
#define QT_FUNCTION_TARGET_STRING_ARCH_CORE2        QT_FUNCTION_TARGET_STRING_ARCH_X86_64 ",sse3,ssse3,cx16"
#define QT_FUNCTION_TARGET_STRING_ARCH_NHM          QT_FUNCTION_TARGET_STRING_ARCH_CORE2 ",sse4.1,sse4.2,popcnt"
#define QT_FUNCTION_TARGET_STRING_ARCH_WSM          QT_FUNCTION_TARGET_STRING_ARCH_NHM
#define QT_FUNCTION_TARGET_STRING_ARCH_SNB          QT_FUNCTION_TARGET_STRING_ARCH_WSM ",avx"
#define QT_FUNCTION_TARGET_STRING_ARCH_IVB          QT_FUNCTION_TARGET_STRING_ARCH_SNB ",f16c,fsgsbase"
#define QT_FUNCTION_TARGET_STRING_ARCH_HSW          QT_FUNCTION_TARGET_STRING_ARCH_IVB ",avx2,fma,bmi,bmi2,lzcnt,movbe"
#define QT_FUNCTION_TARGET_STRING_ARCH_BDW          QT_FUNCTION_TARGET_STRING_ARCH_HSW ",adx"
#define QT_FUNCTION_TARGET_STRING_ARCH_BDX          QT_FUNCTION_TARGET_STRING_ARCH_BDW
#define QT_FUNCTION_TARGET_STRING_ARCH_SKL          QT_FUNCTION_TARGET_STRING_ARCH_BDW ",xsavec,xsaves"
#define QT_FUNCTION_TARGET_STRING_ARCH_SKX          QT_FUNCTION_TARGET_STRING_ARCH_SKL ",avx512f,avx512dq,avx512cd,avx512bw,avx512vl"
#define QT_FUNCTION_TARGET_STRING_ARCH_CLX          QT_FUNCTION_TARGET_STRING_ARCH_SKX ",avx512vnni"
#define QT_FUNCTION_TARGET_STRING_ARCH_CPX          QT_FUNCTION_TARGET_STRING_ARCH_CLX ",avx512bf16"
#define QT_FUNCTION_TARGET_STRING_ARCH_PLC          QT_FUNCTION_TARGET_STRING_ARCH_SKX ",avx512ifma,avx512vbmi"
#define QT_FUNCTION_TARGET_STRING_ARCH_SNC          QT_FUNCTION_TARGET_STRING_ARCH_PLC ",avx512vbmi2,gfni,vaes,vpclmulqdq,avx512vnni,avx512bitalg,avx512vpopcntdq"
#define QT_FUNCTION_TARGET_STRING_ARCH_WLC          QT_FUNCTION_TARGET_STRING_ARCH_SNC ",shstk,movdiri,movdir64b,ibt,keylocker"
#define QT_FUNCTION_TARGET_STRING_ARCH_GLC          QT_FUNCTION_TARGET_STRING_ARCH_WLC ",avx512bf16,avxvnni,cldemote,waitpkg,serialize,uintr"
#define QT_FUNCTION_TARGET_STRING_ARCH_RPC          QT_FUNCTION_TARGET_STRING_ARCH_GLC
#define QT_FUNCTION_TARGET_STRING_ARCH_RWC          QT_FUNCTION_TARGET_STRING_ARCH_RPC ",prefetchiti"
#define QT_FUNCTION_TARGET_STRING_ARCH_SLM          QT_FUNCTION_TARGET_STRING_ARCH_WSM ",rdrnd,movbe"
#define QT_FUNCTION_TARGET_STRING_ARCH_GLM          QT_FUNCTION_TARGET_STRING_ARCH_SLM ",fsgsbase,rdseed,lzcnt,xsavec,xsaves"
#define QT_FUNCTION_TARGET_STRING_ARCH_TNT          QT_FUNCTION_TARGET_STRING_ARCH_GLM ",clwb,gfni,cldemote,waitpkg,movdiri,movdir64b"
#define QT_FUNCTION_TARGET_STRING_ARCH_GRT          QT_FUNCTION_TARGET_STRING_ARCH_SKL ",avxvnni,gfni,vaes,vpclmulqdq,serialize,shstk,cldemote,movdiri,movdir64b,ibt,waitpkg,keylocker"
#define QT_FUNCTION_TARGET_STRING_ARCH_CMT          QT_FUNCTION_TARGET_STRING_ARCH_GRT ",cmpccxadd,avxifma,avxneconvert,avxvnniint8"
#define QT_FUNCTION_TARGET_STRING_ARCH_CNL          QT_FUNCTION_TARGET_STRING_ARCH_PLC
#define QT_FUNCTION_TARGET_STRING_ARCH_ICL          QT_FUNCTION_TARGET_STRING_ARCH_SNC
#define QT_FUNCTION_TARGET_STRING_ARCH_TGL          QT_FUNCTION_TARGET_STRING_ARCH_WLC
#define QT_FUNCTION_TARGET_STRING_ARCH_ADL          QT_FUNCTION_TARGET_STRING_ARCH_GRT
#define QT_FUNCTION_TARGET_STRING_ARCH_RPL          QT_FUNCTION_TARGET_STRING_ARCH_GRT
#define QT_FUNCTION_TARGET_STRING_ARCH_MTL          QT_FUNCTION_TARGET_STRING_ARCH_CMT
#define QT_FUNCTION_TARGET_STRING_ARCH_ARL          QT_FUNCTION_TARGET_STRING_ARCH_CMT
#define QT_FUNCTION_TARGET_STRING_ARCH_LNL          QT_FUNCTION_TARGET_STRING_ARCH_CMT
#define QT_FUNCTION_TARGET_STRING_ARCH_ICX          QT_FUNCTION_TARGET_STRING_ARCH_SNC ",pconfig"
#define QT_FUNCTION_TARGET_STRING_ARCH_SPR          QT_FUNCTION_TARGET_STRING_ARCH_GLC ",pconfig,amx-tile,amx-bf16,amx-int8"
#define QT_FUNCTION_TARGET_STRING_ARCH_EMR          QT_FUNCTION_TARGET_STRING_ARCH_SPR
#define QT_FUNCTION_TARGET_STRING_ARCH_GNR          QT_FUNCTION_TARGET_STRING_ARCH_GLC ",pconfig,amx-tile,amx-bf16,amx-int8,amx-fp16,amx-complex"
#define QT_FUNCTION_TARGET_STRING_ARCH_SRF          QT_FUNCTION_TARGET_STRING_ARCH_CMT ",cmpccxadd,avxifma,avxneconvert,avxvnniint8"
#define QT_FUNCTION_TARGET_STRING_ARCH_GRR          QT_FUNCTION_TARGET_STRING_ARCH_SRF ",raoint"
#define QT_FUNCTION_TARGET_STRING_ARCH_CWF          QT_FUNCTION_TARGET_STRING_ARCH_SRF
#define QT_FUNCTION_TARGET_STRING_ARCH_NEHALEM      QT_FUNCTION_TARGET_STRING_ARCH_NHM
#define QT_FUNCTION_TARGET_STRING_ARCH_WESTMERE     QT_FUNCTION_TARGET_STRING_ARCH_WSM
#define QT_FUNCTION_TARGET_STRING_ARCH_SANDYBRIDGE  QT_FUNCTION_TARGET_STRING_ARCH_SNB
#define QT_FUNCTION_TARGET_STRING_ARCH_IVYBRIDGE    QT_FUNCTION_TARGET_STRING_ARCH_IVB
#define QT_FUNCTION_TARGET_STRING_ARCH_HASWELL      QT_FUNCTION_TARGET_STRING_ARCH_HSW
#define QT_FUNCTION_TARGET_STRING_ARCH_BROADWELL    QT_FUNCTION_TARGET_STRING_ARCH_BDW
#define QT_FUNCTION_TARGET_STRING_ARCH_SKYLAKE      QT_FUNCTION_TARGET_STRING_ARCH_SKL
#define QT_FUNCTION_TARGET_STRING_ARCH_SKYLAKE_AVX512 QT_FUNCTION_TARGET_STRING_ARCH_SKX
#define QT_FUNCTION_TARGET_STRING_ARCH_CASCADELAKE  QT_FUNCTION_TARGET_STRING_ARCH_CLX
#define QT_FUNCTION_TARGET_STRING_ARCH_COOPERLAKE   QT_FUNCTION_TARGET_STRING_ARCH_CPX
#define QT_FUNCTION_TARGET_STRING_ARCH_PALMCOVE     QT_FUNCTION_TARGET_STRING_ARCH_PLC
#define QT_FUNCTION_TARGET_STRING_ARCH_CANNONLAKE   QT_FUNCTION_TARGET_STRING_ARCH_CNL
#define QT_FUNCTION_TARGET_STRING_ARCH_SUNNYCOVE    QT_FUNCTION_TARGET_STRING_ARCH_SNC
#define QT_FUNCTION_TARGET_STRING_ARCH_ICELAKE_CLIENT QT_FUNCTION_TARGET_STRING_ARCH_ICL
#define QT_FUNCTION_TARGET_STRING_ARCH_ICELAKE_SERVER QT_FUNCTION_TARGET_STRING_ARCH_ICX
#define QT_FUNCTION_TARGET_STRING_ARCH_WILLOWCOVE   QT_FUNCTION_TARGET_STRING_ARCH_WLC
#define QT_FUNCTION_TARGET_STRING_ARCH_TIGERLAKE    QT_FUNCTION_TARGET_STRING_ARCH_TGL
#define QT_FUNCTION_TARGET_STRING_ARCH_GOLDENCOVE   QT_FUNCTION_TARGET_STRING_ARCH_GLC
#define QT_FUNCTION_TARGET_STRING_ARCH_ALDERLAKE    QT_FUNCTION_TARGET_STRING_ARCH_ADL
#define QT_FUNCTION_TARGET_STRING_ARCH_RAPTORCOVE   QT_FUNCTION_TARGET_STRING_ARCH_RPC
#define QT_FUNCTION_TARGET_STRING_ARCH_RAPTORLAKE   QT_FUNCTION_TARGET_STRING_ARCH_RPL
#define QT_FUNCTION_TARGET_STRING_ARCH_REDWOODCOVE  QT_FUNCTION_TARGET_STRING_ARCH_RWC
#define QT_FUNCTION_TARGET_STRING_ARCH_METEORLAKE   QT_FUNCTION_TARGET_STRING_ARCH_MTL
#define QT_FUNCTION_TARGET_STRING_ARCH_ARROWLAKE    QT_FUNCTION_TARGET_STRING_ARCH_ARL
#define QT_FUNCTION_TARGET_STRING_ARCH_LUNARLAKE    QT_FUNCTION_TARGET_STRING_ARCH_LNL
#define QT_FUNCTION_TARGET_STRING_ARCH_SAPPHIRERAPIDS QT_FUNCTION_TARGET_STRING_ARCH_SPR
#define QT_FUNCTION_TARGET_STRING_ARCH_EMERALDRAPIDS QT_FUNCTION_TARGET_STRING_ARCH_EMR
#define QT_FUNCTION_TARGET_STRING_ARCH_GRANITERAPIDS QT_FUNCTION_TARGET_STRING_ARCH_GNR
#define QT_FUNCTION_TARGET_STRING_ARCH_SILVERMONT   QT_FUNCTION_TARGET_STRING_ARCH_SLM
#define QT_FUNCTION_TARGET_STRING_ARCH_GOLDMONT     QT_FUNCTION_TARGET_STRING_ARCH_GLM
#define QT_FUNCTION_TARGET_STRING_ARCH_TREMONT      QT_FUNCTION_TARGET_STRING_ARCH_TNT
#define QT_FUNCTION_TARGET_STRING_ARCH_GRACEMONT    QT_FUNCTION_TARGET_STRING_ARCH_GRT
#define QT_FUNCTION_TARGET_STRING_ARCH_CRESTMONT    QT_FUNCTION_TARGET_STRING_ARCH_CMT
#define QT_FUNCTION_TARGET_STRING_ARCH_GRANDRIDGE   QT_FUNCTION_TARGET_STRING_ARCH_GRR
#define QT_FUNCTION_TARGET_STRING_ARCH_SIERRAFOREST QT_FUNCTION_TARGET_STRING_ARCH_SRF
#define QT_FUNCTION_TARGET_STRING_ARCH_CLEARWATERFOREST QT_FUNCTION_TARGET_STRING_ARCH_CWF

static const uint64_t _compilerCpuFeatures = 0
#ifdef __SSE2__
         | cpu_feature_sse2
#endif
#ifdef __SSE3__
         | cpu_feature_sse3
#endif
#ifdef __SSSE3__
         | cpu_feature_ssse3
#endif
#ifdef __FMA__
         | cpu_feature_fma
#endif
#ifdef __SSE4_1__
         | cpu_feature_sse4_1
#endif
#ifdef __SSE4_2__
         | cpu_feature_sse4_2
#endif
#ifdef __MOVBE__
         | cpu_feature_movbe
#endif
#ifdef __POPCNT__
         | cpu_feature_popcnt
#endif
#ifdef __AES__
         | cpu_feature_aes
#endif
#ifdef __AVX__
         | cpu_feature_avx
#endif
#ifdef __F16C__
         | cpu_feature_f16c
#endif
#ifdef __BMI__
         | cpu_feature_bmi
#endif
#ifdef __AVX2__
         | cpu_feature_avx2
#endif
#ifdef __BMI2__
         | cpu_feature_bmi2
#endif
#ifdef __AVX512F__
         | cpu_feature_avx512f
#endif
#ifdef __AVX512DQ__
         | cpu_feature_avx512dq
#endif
#ifdef __AVX512IFMA__
         | cpu_feature_avx512ifma
#endif
#ifdef __AVX512CD__
         | cpu_feature_avx512cd
#endif
#ifdef __SHA__
         | cpu_feature_sha
#endif
#ifdef __AVX512BW__
         | cpu_feature_avx512bw
#endif
#ifdef __AVX512VL__
         | cpu_feature_avx512vl
#endif
#ifdef __AVX512VBMI__
         | cpu_feature_avx512vbmi
#endif
#ifdef __WAITPKG__
         | cpu_feature_waitpkg
#endif
#ifdef __AVX512VBMI2__
         | cpu_feature_avx512vbmi2
#endif
#ifdef __SHSTK__
         | cpu_feature_shstk
#endif
#ifdef __GFNI__
         | cpu_feature_gfni
#endif
#ifdef __VAES__
         | cpu_feature_vaes
#endif
#ifdef __AVX512BITALG__
         | cpu_feature_avx512bitalg
#endif
#ifdef __AVX512VPOPCNTDQ__
         | cpu_feature_avx512vpopcntdq
#endif
#ifdef __HYBRID__
         | cpu_feature_hybrid
#endif
#ifdef __IBT__
         | cpu_feature_ibt
#endif
#ifdef __AVX512FP16__
         | cpu_feature_avx512fp16
#endif
#ifdef __RAOINT__
         | cpu_feature_raoint
#endif
#ifdef __CMPCCXADD__
         | cpu_feature_cmpccxadd
#endif
#ifdef __AVXIFMA__
         | cpu_feature_avxifma
#endif
#ifdef __LAM__
         | cpu_feature_lam
#endif
        ;

#if (defined __cplusplus) && __cplusplus >= 201103L
enum X86CpuFeatures : uint64_t {
    CpuFeatureSSE2 = cpu_feature_sse2,                       ///< Streaming SIMD Extensions 2
    CpuFeatureSSE3 = cpu_feature_sse3,                       ///< Streaming SIMD Extensions 3
    CpuFeatureSSSE3 = cpu_feature_ssse3,                     ///< Supplemental Streaming SIMD Extensions 3
    CpuFeatureFMA = cpu_feature_fma,                         ///< Fused Multiply-Add
    CpuFeatureSSE4_1 = cpu_feature_sse4_1,                   ///< Streaming SIMD Extensions 4.1
    CpuFeatureSSE4_2 = cpu_feature_sse4_2,                   ///< Streaming SIMD Extensions 4.2
    CpuFeatureMOVBE = cpu_feature_movbe,                     ///< MOV Big Endian
    CpuFeaturePOPCNT = cpu_feature_popcnt,                   ///< Population count
    CpuFeatureAES = cpu_feature_aes,                         ///< Advenced Encryption Standard
    CpuFeatureAVX = cpu_feature_avx,                         ///< Advanced Vector Extensions
    CpuFeatureF16C = cpu_feature_f16c,                       ///< 16-bit Float Conversion
    CpuFeatureBMI = cpu_feature_bmi,                         ///< Bit Manipulation Instructions
    CpuFeatureAVX2 = cpu_feature_avx2,                       ///< Advanced Vector Extensions 2
    CpuFeatureBMI2 = cpu_feature_bmi2,                       ///< Bit Manipulation Instructions 2
    CpuFeatureAVX512F = cpu_feature_avx512f,                 ///< AVX512 Foundation
    CpuFeatureAVX512DQ = cpu_feature_avx512dq,               ///< AVX512 Double & Quadword
    CpuFeatureAVX512IFMA = cpu_feature_avx512ifma,           ///< AVX512 Integer Fused Multiply-Add
    CpuFeatureAVX512CD = cpu_feature_avx512cd,               ///< AVX512 Conflict Detection
    CpuFeatureSHA = cpu_feature_sha,                         ///< SHA-1 and SHA-256 instructions
    CpuFeatureAVX512BW = cpu_feature_avx512bw,               ///< AVX512 Byte & Word
    CpuFeatureAVX512VL = cpu_feature_avx512vl,               ///< AVX512 Vector Length
    CpuFeatureAVX512VBMI = cpu_feature_avx512vbmi,           ///< AVX512 Vector Byte Manipulation Instructions
    CpuFeatureWAITPKG = cpu_feature_waitpkg,                 ///< User-Level Monitor / Wait
    CpuFeatureAVX512VBMI2 = cpu_feature_avx512vbmi2,         ///< AVX512 Vector Byte Manipulation Instructions 2
    CpuFeatureSHSTK = cpu_feature_shstk,                     ///< Control Flow Enforcement Technology Shadow Stack
    CpuFeatureGFNI = cpu_feature_gfni,                       ///< Galois Field new instructions
    CpuFeatureVAES = cpu_feature_vaes,                       ///< 256- and 512-bit AES
    CpuFeatureAVX512BITALG = cpu_feature_avx512bitalg,       ///< AVX512 Bit Algorithms
    CpuFeatureAVX512VPOPCNTDQ = cpu_feature_avx512vpopcntdq, ///< AVX512 Population Count
    CpuFeatureHYBRID = cpu_feature_hybrid,                   ///< Hybrid processor
    CpuFeatureIBT = cpu_feature_ibt,                         ///< Control Flow Enforcement Technology Indirect Branch Tracking
    CpuFeatureAVX512FP16 = cpu_feature_avx512fp16,           ///< AVX512 16-bit Floating Point
    CpuFeatureRAOINT = cpu_feature_raoint,                   ///< Remote Atomic Operations, Integer
    CpuFeatureCMPCCXADD = cpu_feature_cmpccxadd,             ///< CMPccXADD instructions
    CpuFeatureAVXIFMA = cpu_feature_avxifma,                 ///< AVX-IFMA instructions
    CpuFeatureLAM = cpu_feature_lam,                         ///< Linear Address Masking
}; // enum X86CpuFeatures

enum X86CpuArchitectures : uint64_t {
    CpuArchx8664 = cpu_x86_64,
    CpuArchCore2 = cpu_core2,
    CpuArchNHM = cpu_nhm,
    CpuArchWSM = cpu_wsm,
    CpuArchSNB = cpu_snb,
    CpuArchIVB = cpu_ivb,                                    ///< rdrnd
    CpuArchHSW = cpu_hsw,                                    ///< hle,rtm
    CpuArchBDW = cpu_bdw,                                    ///< rdseed
    CpuArchBDX = cpu_bdx,
    CpuArchSKL = cpu_skl,
    CpuArchSKX = cpu_skx,                                    ///< clwb
    CpuArchCLX = cpu_clx,
    CpuArchCPX = cpu_cpx,
    CpuArchPLC = cpu_plc,                                    ///< sha
    CpuArchSNC = cpu_snc,                                    ///< fsrm,rdpid
    CpuArchWLC = cpu_wlc,                                    ///< avx512vp2intersect
    CpuArchGLC = cpu_glc,                                    ///< tsxldtrk
    CpuArchRPC = cpu_rpc,
    CpuArchRWC = cpu_rwc,
    CpuArchSLM = cpu_slm,
    CpuArchGLM = cpu_glm,
    CpuArchTNT = cpu_tnt,
    CpuArchGRT = cpu_grt,                                    ///< rdpid
    CpuArchCMT = cpu_cmt,
    CpuArchCNL = cpu_cnl,
    CpuArchICL = cpu_icl,
    CpuArchTGL = cpu_tgl,
    CpuArchADL = cpu_adl,
    CpuArchRPL = cpu_rpl,
    CpuArchMTL = cpu_mtl,
    CpuArchARL = cpu_arl,
    CpuArchLNL = cpu_lnl,
    CpuArchICX = cpu_icx,
    CpuArchSPR = cpu_spr,
    CpuArchEMR = cpu_emr,
    CpuArchGNR = cpu_gnr,
    CpuArchSRF = cpu_srf,
    CpuArchGRR = cpu_grr,
    CpuArchCWF = cpu_cwf,
    CpuArchNehalem = cpu_nehalem,                            ///< Intel Core i3/i5/i7
    CpuArchWestmere = cpu_westmere,                          ///< Intel Core i3/i5/i7
    CpuArchSandyBridge = cpu_sandybridge,                    ///< Second Generation Intel Core i3/i5/i7
    CpuArchIvyBridge = cpu_ivybridge,                        ///< Third Generation Intel Core i3/i5/i7
    CpuArchHaswell = cpu_haswell,                            ///< Fourth Generation Intel Core i3/i5/i7
    CpuArchBroadwell = cpu_broadwell,                        ///< Fifth Generation Intel Core i3/i5/i7
    CpuArchSkylake = cpu_skylake,                            ///< Sixth Generation Intel Core i3/i5/i7
    CpuArchSkylakeAvx512 = cpu_skylake_avx512,               ///< Intel Xeon Scalable
    CpuArchCascadeLake = cpu_cascadelake,                    ///< Second Generation Intel Xeon Scalable
    CpuArchCooperLake = cpu_cooperlake,                      ///< Third Generation Intel Xeon Scalable
    CpuArchPalmCove = cpu_palmcove,
    CpuArchCannonLake = cpu_cannonlake,                      ///< Intel Core i3-8121U
    CpuArchSunnyCove = cpu_sunnycove,
    CpuArchIceLakeClient = cpu_icelake_client,               ///< Tenth Generation Intel Core i3/i5/i7
    CpuArchIceLakeServer = cpu_icelake_server,               ///< Third Generation Intel Xeon Scalable
    CpuArchWillowCove = cpu_willowcove,
    CpuArchTigerLake = cpu_tigerlake,                        ///< Eleventh Generation Intel Core i3/i5/i7
    CpuArchGoldenCove = cpu_goldencove,
    CpuArchAlderLake = cpu_alderlake,                        ///< Twelfth Generation Intel Core
    CpuArchRaptorCove = cpu_raptorcove,
    CpuArchRaptorLake = cpu_raptorlake,                      ///< Thirteenth Generation Intel Core
    CpuArchRedwoodCove = cpu_redwoodcove,
    CpuArchMeteorLake = cpu_meteorlake,
    CpuArchArrowLake = cpu_arrowlake,
    CpuArchLunarLake = cpu_lunarlake,
    CpuArchSapphireRapids = cpu_sapphirerapids,              ///< Fourth Generation Intel Xeon Scalable
    CpuArchEmeraldRapids = cpu_emeraldrapids,                ///< Fifth Generation Intel Xeon Scalable
    CpuArchGraniteRapids = cpu_graniterapids,
    CpuArchSilvermont = cpu_silvermont,
    CpuArchGoldmont = cpu_goldmont,
    CpuArchTremont = cpu_tremont,
    CpuArchGracemont = cpu_gracemont,
    CpuArchCrestmont = cpu_crestmont,
    CpuArchGrandRidge = cpu_grandridge,
    CpuArchSierraForest = cpu_sierraforest,
    CpuArchClearwaterForest = cpu_clearwaterforest,
}; // enum X86cpuArchitectures
#endif /* C++11 */

#endif /* QSIMD_X86_P_H */
