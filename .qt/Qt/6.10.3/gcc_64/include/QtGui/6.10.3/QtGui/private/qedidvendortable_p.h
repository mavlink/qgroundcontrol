// Copyright (C) 2017 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/*
 * This lookup table was generated from https://github.com/vcrhonek/hwdata/raw/master/pnp.ids
 *
 * Do not change this file directly, instead edit the
 * qtbase/util/edid/qedidvendortable.py script and regenerate this file.
 */

#ifndef QEDIDVENDORTABLE_P_H
#define QEDIDVENDORTABLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qtypes.h>

#include <iterator>

QT_BEGIN_NAMESPACE

struct QEdidVendorId {
    const char id[4];
};

static constexpr QEdidVendorId q_edidVendorIds[] = {
    { "AAA" },
    { "AAE" },
    { "AAM" },
    { "AAN" },
    { "AAT" },
    { "ABA" },
    { "ABC" },
    { "ABD" },
    { "ABE" },
    { "ABO" },
    { "ABS" },
    { "ABT" },
    { "ABV" },
    { "ACA" },
    { "ACB" },
    { "ACC" },
    { "ACD" },
    { "ACE" },
    { "ACG" },
    { "ACH" },
    { "ACI" },
    { "ACK" },
    { "ACL" },
    { "ACM" },
    { "ACO" },
    { "ACP" },
    { "ACR" },
    { "ACS" },
    { "ACT" },
    { "ACU" },
    { "ACV" },
    { "ADA" },
    { "ADB" },
    { "ADC" },
    { "ADD" },
    { "ADE" },
    { "ADG" },
    { "ADH" },
    { "ADI" },
    { "ADK" },
    { "ADL" },
    { "ADM" },
    { "ADN" },
    { "ADP" },
    { "ADR" },
    { "ADS" },
    { "ADT" },
    { "ADV" },
    { "ADX" },
    { "ADZ" },
    { "AEC" },
    { "AED" },
    { "AEI" },
    { "AEJ" },
    { "AEM" },
    { "AEN" },
    { "AEP" },
    { "AET" },
    { "AFA" },
    { "AGC" },
    { "AGI" },
    { "AGL" },
    { "AGM" },
    { "AGO" },
    { "AGT" },
    { "AHC" },
    { "AHQ" },
    { "AHS" },
    { "AIC" },
    { "AIE" },
    { "AII" },
    { "AIK" },
    { "AIL" },
    { "AIM" },
    { "AIR" },
    { "AIS" },
    { "AIW" },
    { "AIX" },
    { "AJA" },
    { "AKB" },
    { "AKE" },
    { "AKI" },
    { "AKL" },
    { "AKM" },
    { "AKP" },
    { "AKR" },
    { "AKY" },
    { "ALA" },
    { "ALC" },
    { "ALD" },
    { "ALE" },
    { "ALG" },
    { "ALH" },
    { "ALI" },
    { "ALJ" },
    { "ALK" },
    { "ALL" },
    { "ALM" },
    { "ALN" },
    { "ALO" },
    { "ALP" },
    { "ALR" },
    { "ALS" },
    { "ALT" },
    { "ALV" },
    { "ALX" },
    { "AMA" },
    { "AMB" },
    { "AMC" },
    { "AMD" },
    { "AMI" },
    { "AML" },
    { "AMN" },
    { "AMO" },
    { "AMP" },
    { "AMR" },
    { "AMS" },
    { "AMT" },
    { "AMX" },
    { "ANA" },
    { "ANC" },
    { "AND" },
    { "ANI" },
    { "ANK" },
    { "ANL" },
    { "ANO" },
    { "ANP" },
    { "ANR" },
    { "ANS" },
    { "ANT" },
    { "ANV" },
    { "ANW" },
    { "ANX" },
    { "AOA" },
    { "AOE" },
    { "AOL" },
    { "AOT" },
    { "APC" },
    { "APD" },
    { "APE" },
    { "APG" },
    { "API" },
    { "APL" },
    { "APM" },
    { "APN" },
    { "APP" },
    { "APR" },
    { "APS" },
    { "APT" },
    { "APV" },
    { "APX" },
    { "ARC" },
    { "ARD" },
    { "ARE" },
    { "ARG" },
    { "ARI" },
    { "ARK" },
    { "ARL" },
    { "ARM" },
    { "ARO" },
    { "ARR" },
    { "ARS" },
    { "ART" },
    { "ASC" },
    { "ASD" },
    { "ASE" },
    { "ASH" },
    { "ASI" },
    { "ASK" },
    { "ASL" },
    { "ASM" },
    { "ASN" },
    { "ASP" },
    { "AST" },
    { "ASU" },
    { "ASX" },
    { "ASY" },
    { "ATA" },
    { "ATC" },
    { "ATD" },
    { "ATE" },
    { "ATH" },
    { "ATI" },
    { "ATJ" },
    { "ATK" },
    { "ATL" },
    { "ATM" },
    { "ATN" },
    { "ATO" },
    { "ATP" },
    { "ATT" },
    { "ATU" },
    { "ATV" },
    { "ATX" },
    { "AUD" },
    { "AUG" },
    { "AUI" },
    { "AUO" },
    { "AUR" },
    { "AUS" },
    { "AUT" },
    { "AUV" },
    { "AVA" },
    { "AVC" },
    { "AVD" },
    { "AVE" },
    { "AVG" },
    { "AVI" },
    { "AVJ" },
    { "AVL" },
    { "AVM" },
    { "AVN" },
    { "AVO" },
    { "AVR" },
    { "AVS" },
    { "AVT" },
    { "AVV" },
    { "AVX" },
    { "AWC" },
    { "AWL" },
    { "AWS" },
    { "AXB" },
    { "AXC" },
    { "AXE" },
    { "AXI" },
    { "AXL" },
    { "AXO" },
    { "AXP" },
    { "AXT" },
    { "AXX" },
    { "AXY" },
    { "AYD" },
    { "AYR" },
    { "AZH" },
    { "AZM" },
    { "AZT" },
    { "BAC" },
    { "BAN" },
    { "BBB" },
    { "BBH" },
    { "BBL" },
    { "BBV" },
    { "BBX" },
    { "BCC" },
    { "BCD" },
    { "BCI" },
    { "BCK" },
    { "BCM" },
    { "BCQ" },
    { "BCS" },
    { "BDO" },
    { "BDR" },
    { "BDS" },
    { "BEC" },
    { "BEI" },
    { "BEK" },
    { "BEL" },
    { "BEO" },
    { "BFE" },
    { "BGB" },
    { "BGT" },
    { "BHZ" },
    { "BIA" },
    { "BIC" },
    { "BIG" },
    { "BII" },
    { "BIL" },
    { "BIO" },
    { "BIT" },
    { "BLD" },
    { "BLI" },
    { "BLN" },
    { "BLP" },
    { "BMD" },
    { "BMI" },
    { "BML" },
    { "BMS" },
    { "BNE" },
    { "BNK" },
    { "BNO" },
    { "BNS" },
    { "BOB" },
    { "BOE" },
    { "BOI" },
    { "BOS" },
    { "BPD" },
    { "BPS" },
    { "BPU" },
    { "BRA" },
    { "BRC" },
    { "BRG" },
    { "BRI" },
    { "BRL" },
    { "BRM" },
    { "BRO" },
    { "BSE" },
    { "BSG" },
    { "BSL" },
    { "BSN" },
    { "BST" },
    { "BTC" },
    { "BTE" },
    { "BTF" },
    { "BTI" },
    { "BTO" },
    { "BUF" },
    { "BUG" },
    { "BUJ" },
    { "BUL" },
    { "BUR" },
    { "BUS" },
    { "BUT" },
    { "BWK" },
    { "BXE" },
    { "BYD" },
    { "CAA" },
    { "CAC" },
    { "CAG" },
    { "CAI" },
    { "CAL" },
    { "CAM" },
    { "CAN" },
    { "CAR" },
    { "CAS" },
    { "CAT" },
    { "CAV" },
    { "CBI" },
    { "CBR" },
    { "CBT" },
    { "CBX" },
    { "CCC" },
    { "CCI" },
    { "CCJ" },
    { "CCL" },
    { "CCP" },
    { "CDC" },
    { "CDD" },
    { "CDE" },
    { "CDG" },
    { "CDI" },
    { "CDK" },
    { "CDN" },
    { "CDP" },
    { "CDS" },
    { "CDT" },
    { "CDV" },
    { "CEA" },
    { "CEC" },
    { "CED" },
    { "CEF" },
    { "CEI" },
    { "CEM" },
    { "CEN" },
    { "CEP" },
    { "CER" },
    { "CET" },
    { "CFG" },
    { "CFR" },
    { "CGA" },
    { "CGS" },
    { "CGT" },
    { "CHA" },
    { "CHD" },
    { "CHE" },
    { "CHG" },
    { "CHI" },
    { "CHL" },
    { "CHM" },
    { "CHO" },
    { "CHP" },
    { "CHR" },
    { "CHS" },
    { "CHT" },
    { "CHY" },
    { "CIC" },
    { "CID" },
    { "CIE" },
    { "CII" },
    { "CIL" },
    { "CIN" },
    { "CIP" },
    { "CIR" },
    { "CIS" },
    { "CIT" },
    { "CKC" },
    { "CKJ" },
    { "CLA" },
    { "CLD" },
    { "CLE" },
    { "CLG" },
    { "CLI" },
    { "CLM" },
    { "CLO" },
    { "CLR" },
    { "CLT" },
    { "CLV" },
    { "CLX" },
    { "CMC" },
    { "CMD" },
    { "CMG" },
    { "CMI" },
    { "CMK" },
    { "CMM" },
    { "CMN" },
    { "CMO" },
    { "CMR" },
    { "CMS" },
    { "CMX" },
    { "CNB" },
    { "CNC" },
    { "CND" },
    { "CNE" },
    { "CNI" },
    { "CNN" },
    { "CNT" },
    { "COB" },
    { "COD" },
    { "COI" },
    { "COL" },
    { "COM" },
    { "CON" },
    { "COO" },
    { "COR" },
    { "COS" },
    { "COT" },
    { "COW" },
    { "COX" },
    { "CPC" },
    { "CPD" },
    { "CPI" },
    { "CPL" },
    { "CPM" },
    { "CPP" },
    { "CPQ" },
    { "CPT" },
    { "CPX" },
    { "CRA" },
    { "CRC" },
    { "CRD" },
    { "CRE" },
    { "CRH" },
    { "CRI" },
    { "CRL" },
    { "CRM" },
    { "CRN" },
    { "CRO" },
    { "CRQ" },
    { "CRS" },
    { "CRV" },
    { "CRW" },
    { "CRX" },
    { "CSB" },
    { "CSC" },
    { "CSD" },
    { "CSE" },
    { "CSI" },
    { "CSL" },
    { "CSM" },
    { "CSO" },
    { "CSS" },
    { "CST" },
    { "CSW" },
    { "CTA" },
    { "CTC" },
    { "CTE" },
    { "CTL" },
    { "CTM" },
    { "CTN" },
    { "CTP" },
    { "CTR" },
    { "CTS" },
    { "CTX" },
    { "CUB" },
    { "CUK" },
    { "CVA" },
    { "CVI" },
    { "CVP" },
    { "CVS" },
    { "CWC" },
    { "CWR" },
    { "CXT" },
    { "CYB" },
    { "CYC" },
    { "CYD" },
    { "CYL" },
    { "CYP" },
    { "CYT" },
    { "CYV" },
    { "CYW" },
    { "CYX" },
    { "CZC" },
    { "CZE" },
    { "DAC" },
    { "DAE" },
    { "DAI" },
    { "DAK" },
    { "DAL" },
    { "DAN" },
    { "DAS" },
    { "DAT" },
    { "DAU" },
    { "DAV" },
    { "DAW" },
    { "DAX" },
    { "DBD" },
    { "DBI" },
    { "DBK" },
    { "DBL" },
    { "DBN" },
    { "DCA" },
    { "DCC" },
    { "DCD" },
    { "DCE" },
    { "DCI" },
    { "DCL" },
    { "DCM" },
    { "DCO" },
    { "DCR" },
    { "DCS" },
    { "DCT" },
    { "DCV" },
    { "DDA" },
    { "DDD" },
    { "DDE" },
    { "DDI" },
    { "DDS" },
    { "DDT" },
    { "DDV" },
    { "DEC" },
    { "DEF" },
    { "DEI" },
    { "DEL" },
    { "DEM" },
    { "DEN" },
    { "DEX" },
    { "DFI" },
    { "DFK" },
    { "DFT" },
    { "DGA" },
    { "DGC" },
    { "DGI" },
    { "DGK" },
    { "DGP" },
    { "DGS" },
    { "DGT" },
    { "DHD" },
    { "DHP" },
    { "DHQ" },
    { "DHT" },
    { "DIA" },
    { "DIG" },
    { "DII" },
    { "DIM" },
    { "DIN" },
    { "DIS" },
    { "DIT" },
    { "DJE" },
    { "DJP" },
    { "DKY" },
    { "DLB" },
    { "DLC" },
    { "DLD" },
    { "DLG" },
    { "DLK" },
    { "DLL" },
    { "DLM" },
    { "DLO" },
    { "DLT" },
    { "DMB" },
    { "DMC" },
    { "DMG" },
    { "DMM" },
    { "DMN" },
    { "DMO" },
    { "DMP" },
    { "DMS" },
    { "DMT" },
    { "DMV" },
    { "DNA" },
    { "DNG" },
    { "DNI" },
    { "DNT" },
    { "DNV" },
    { "DOL" },
    { "DOM" },
    { "DON" },
    { "DOT" },
    { "DPA" },
    { "DPC" },
    { "DPH" },
    { "DPI" },
    { "DPL" },
    { "DPM" },
    { "DPN" },
    { "DPS" },
    { "DPT" },
    { "DPX" },
    { "DQB" },
    { "DRB" },
    { "DRC" },
    { "DRD" },
    { "DRI" },
    { "DRS" },
    { "DSA" },
    { "DSD" },
    { "DSG" },
    { "DSI" },
    { "DSJ" },
    { "DSM" },
    { "DSP" },
    { "DTA" },
    { "DTC" },
    { "DTE" },
    { "DTI" },
    { "DTK" },
    { "DTL" },
    { "DTM" },
    { "DTN" },
    { "DTO" },
    { "DTT" },
    { "DTX" },
    { "DUA" },
    { "DUN" },
    { "DVD" },
    { "DVL" },
    { "DVS" },
    { "DVT" },
    { "DWE" },
    { "DXC" },
    { "DXD" },
    { "DXL" },
    { "DXN" },
    { "DXP" },
    { "DXS" },
    { "DYC" },
    { "DYM" },
    { "DYN" },
    { "DYX" },
    { "EAC" },
    { "EAG" },
    { "EAS" },
    { "EBH" },
    { "EBS" },
    { "EBT" },
    { "ECA" },
    { "ECC" },
    { "ECH" },
    { "ECI" },
    { "ECK" },
    { "ECL" },
    { "ECM" },
    { "ECO" },
    { "ECP" },
    { "ECS" },
    { "ECT" },
    { "EDC" },
    { "EDG" },
    { "EDI" },
    { "EDM" },
    { "EDT" },
    { "EEE" },
    { "EEH" },
    { "EEI" },
    { "EEP" },
    { "EES" },
    { "EGA" },
    { "EGD" },
    { "EGL" },
    { "EGN" },
    { "EGO" },
    { "EHJ" },
    { "EHN" },
    { "EIC" },
    { "EIN" },
    { "EKA" },
    { "EKC" },
    { "EKS" },
    { "ELA" },
    { "ELC" },
    { "ELD" },
    { "ELE" },
    { "ELG" },
    { "ELI" },
    { "ELL" },
    { "ELM" },
    { "ELO" },
    { "ELS" },
    { "ELT" },
    { "ELU" },
    { "ELX" },
    { "EMB" },
    { "EMC" },
    { "EMD" },
    { "EME" },
    { "EMG" },
    { "EMI" },
    { "EMK" },
    { "EMO" },
    { "EMR" },
    { "EMU" },
    { "ENC" },
    { "END" },
    { "ENE" },
    { "ENI" },
    { "ENS" },
    { "ENT" },
    { "EON" },
    { "EPC" },
    { "EPH" },
    { "EPI" },
    { "EPN" },
    { "EPS" },
    { "EQP" },
    { "EQX" },
    { "ERG" },
    { "ERI" },
    { "ERN" },
    { "ERP" },
    { "ERS" },
    { "ERT" },
    { "ESA" },
    { "ESB" },
    { "ESC" },
    { "ESD" },
    { "ESG" },
    { "ESI" },
    { "ESK" },
    { "ESL" },
    { "ESN" },
    { "ESS" },
    { "EST" },
    { "ESY" },
    { "ETC" },
    { "ETD" },
    { "ETG" },
    { "ETH" },
    { "ETI" },
    { "ETK" },
    { "ETL" },
    { "ETS" },
    { "ETT" },
    { "EUT" },
    { "EVE" },
    { "EVI" },
    { "EVP" },
    { "EVX" },
    { "EXA" },
    { "EXC" },
    { "EXI" },
    { "EXN" },
    { "EXP" },
    { "EXR" },
    { "EXT" },
    { "EXX" },
    { "EXY" },
    { "EYE" },
    { "EYF" },
    { "EZE" },
    { "EZP" },
    { "FAN" },
    { "FAR" },
    { "FBI" },
    { "FCB" },
    { "FCG" },
    { "FCS" },
    { "FDC" },
    { "FDD" },
    { "FDI" },
    { "FDT" },
    { "FDX" },
    { "FEC" },
    { "FEL" },
    { "FEN" },
    { "FER" },
    { "FFC" },
    { "FFI" },
    { "FGD" },
    { "FGL" },
    { "FHL" },
    { "FIC" },
    { "FIL" },
    { "FIN" },
    { "FIR" },
    { "FIS" },
    { "FIT" },
    { "FJC" },
    { "FJS" },
    { "FJT" },
    { "FLE" },
    { "FLI" },
    { "FLY" },
    { "FMA" },
    { "FMC" },
    { "FMI" },
    { "FML" },
    { "FMZ" },
    { "FNC" },
    { "FNI" },
    { "FOA" },
    { "FOK" },
    { "FOS" },
    { "FOV" },
    { "FOX" },
    { "FPC" },
    { "FPE" },
    { "FPS" },
    { "FPX" },
    { "FRC" },
    { "FRD" },
    { "FRE" },
    { "FRI" },
    { "FRO" },
    { "FRS" },
    { "FSC" },
    { "FSI" },
    { "FST" },
    { "FTC" },
    { "FTE" },
    { "FTG" },
    { "FTI" },
    { "FTL" },
    { "FTN" },
    { "FTR" },
    { "FTS" },
    { "FTW" },
    { "FUJ" },
    { "FUL" },
    { "FUN" },
    { "FUS" },
    { "FVC" },
    { "FVX" },
    { "FWA" },
    { "FWR" },
    { "FXX" },
    { "FZC" },
    { "FZI" },
    { "GAC" },
    { "GAG" },
    { "GAL" },
    { "GAU" },
    { "GBT" },
    { "GCC" },
    { "GCI" },
    { "GCS" },
    { "GDC" },
    { "GDI" },
    { "GDS" },
    { "GDT" },
    { "GEC" },
    { "GED" },
    { "GEF" },
    { "GEH" },
    { "GEM" },
    { "GEN" },
    { "GEO" },
    { "GER" },
    { "GES" },
    { "GET" },
    { "GFM" },
    { "GFN" },
    { "GGL" },
    { "GGT" },
    { "GIC" },
    { "GIM" },
    { "GIP" },
    { "GIS" },
    { "GJN" },
    { "GLD" },
    { "GLE" },
    { "GLM" },
    { "GLS" },
    { "GMK" },
    { "GML" },
    { "GMM" },
    { "GMN" },
    { "GMX" },
    { "GND" },
    { "GNN" },
    { "GNZ" },
    { "GOE" },
    { "GPR" },
    { "GRA" },
    { "GRE" },
    { "GRH" },
    { "GRM" },
    { "GRV" },
    { "GRY" },
    { "GSB" },
    { "GSC" },
    { "GSM" },
    { "GSN" },
    { "GST" },
    { "GSY" },
    { "GTC" },
    { "GTI" },
    { "GTK" },
    { "GTM" },
    { "GTS" },
    { "GTT" },
    { "GUD" },
    { "GUP" },
    { "GUZ" },
    { "GVC" },
    { "GVL" },
    { "GVS" },
    { "GWI" },
    { "GWK" },
    { "GWY" },
    { "GXL" },
    { "GZE" },
    { "HAE" },
    { "HAI" },
    { "HAL" },
    { "HAN" },
    { "HAR" },
    { "HAY" },
    { "HCA" },
    { "HCE" },
    { "HCL" },
    { "HCM" },
    { "HCP" },
    { "HCW" },
    { "HDC" },
    { "HDI" },
    { "HDV" },
    { "HEC" },
    { "HEL" },
    { "HER" },
    { "HET" },
    { "HHC" },
    { "HHI" },
    { "HHT" },
    { "HIB" },
    { "HIC" },
    { "HII" },
    { "HIK" },
    { "HIL" },
    { "HIQ" },
    { "HIS" },
    { "HIT" },
    { "HJI" },
    { "HKA" },
    { "HKC" },
    { "HKG" },
    { "HLG" },
    { "HMC" },
    { "HMK" },
    { "HMX" },
    { "HNM" },
    { "HNS" },
    { "HOB" },
    { "HOE" },
    { "HOL" },
    { "HON" },
    { "HPA" },
    { "HPC" },
    { "HPD" },
    { "HPE" },
    { "HPI" },
    { "HPK" },
    { "HPN" },
    { "HPQ" },
    { "HPR" },
    { "HRC" },
    { "HRE" },
    { "HRI" },
    { "HRL" },
    { "HRS" },
    { "HRT" },
    { "HSC" },
    { "HSD" },
    { "HSM" },
    { "HSN" },
    { "HSP" },
    { "HST" },
    { "HTC" },
    { "HTI" },
    { "HTK" },
    { "HTL" },
    { "HTR" },
    { "HTX" },
    { "HUB" },
    { "HUK" },
    { "HUM" },
    { "HVR" },
    { "HWA" },
    { "HWC" },
    { "HWD" },
    { "HWP" },
    { "HWV" },
    { "HXM" },
    { "HYC" },
    { "HYD" },
    { "HYL" },
    { "HYO" },
    { "HYP" },
    { "HYR" },
    { "HYT" },
    { "HYV" },
    { "IAD" },
    { "IAF" },
    { "IAI" },
    { "IAT" },
    { "IBC" },
    { "IBI" },
    { "IBM" },
    { "IBP" },
    { "IBR" },
    { "ICA" },
    { "ICC" },
    { "ICD" },
    { "ICE" },
    { "ICI" },
    { "ICM" },
    { "ICN" },
    { "ICO" },
    { "ICP" },
    { "ICR" },
    { "ICS" },
    { "ICV" },
    { "ICX" },
    { "IDC" },
    { "IDE" },
    { "IDK" },
    { "IDN" },
    { "IDO" },
    { "IDP" },
    { "IDS" },
    { "IDT" },
    { "IDX" },
    { "IEC" },
    { "IEE" },
    { "IEI" },
    { "IFS" },
    { "IFT" },
    { "IFX" },
    { "IFZ" },
    { "IGC" },
    { "IGM" },
    { "IHE" },
    { "IIC" },
    { "III" },
    { "IIN" },
    { "IIT" },
    { "IKE" },
    { "IKS" },
    { "ILC" },
    { "ILS" },
    { "IMA" },
    { "IMB" },
    { "IMC" },
    { "IMD" },
    { "IME" },
    { "IMF" },
    { "IMG" },
    { "IMI" },
    { "IMM" },
    { "IMN" },
    { "IMP" },
    { "IMT" },
    { "IMX" },
    { "INA" },
    { "INC" },
    { "IND" },
    { "INE" },
    { "INF" },
    { "ING" },
    { "INI" },
    { "INK" },
    { "INL" },
    { "INM" },
    { "INN" },
    { "INO" },
    { "INP" },
    { "INS" },
    { "INT" },
    { "INU" },
    { "INV" },
    { "INX" },
    { "INZ" },
    { "IOA" },
    { "IOC" },
    { "IOD" },
    { "IOM" },
    { "ION" },
    { "IOS" },
    { "IOT" },
    { "IPC" },
    { "IPD" },
    { "IPI" },
    { "IPM" },
    { "IPN" },
    { "IPP" },
    { "IPQ" },
    { "IPR" },
    { "IPS" },
    { "IPT" },
    { "IPW" },
    { "IQI" },
    { "IQT" },
    { "IRD" },
    { "ISA" },
    { "ISC" },
    { "ISG" },
    { "ISI" },
    { "ISL" },
    { "ISM" },
    { "ISP" },
    { "ISR" },
    { "ISS" },
    { "IST" },
    { "ISY" },
    { "ITA" },
    { "ITC" },
    { "ITD" },
    { "ITE" },
    { "ITI" },
    { "ITK" },
    { "ITL" },
    { "ITM" },
    { "ITN" },
    { "ITP" },
    { "ITR" },
    { "ITS" },
    { "ITT" },
    { "ITX" },
    { "IUC" },
    { "IVI" },
    { "IVM" },
    { "IVO" },
    { "IVR" },
    { "IVS" },
    { "IWR" },
    { "IWX" },
    { "IXD" },
    { "IXN" },
    { "JAC" },
    { "JAE" },
    { "JAS" },
    { "JAT" },
    { "JAZ" },
    { "JCE" },
    { "JDI" },
    { "JDL" },
    { "JEM" },
    { "JEN" },
    { "JET" },
    { "JFX" },
    { "JGD" },
    { "JIC" },
    { "JKC" },
    { "JLK" },
    { "JMT" },
    { "JPC" },
    { "JPW" },
    { "JQE" },
    { "JSD" },
    { "JSI" },
    { "JSK" },
    { "JTS" },
    { "JTY" },
    { "JUK" },
    { "JUP" },
    { "JVC" },
    { "JWD" },
    { "JWL" },
    { "JWS" },
    { "JWY" },
    { "KAR" },
    { "KBI" },
    { "KBL" },
    { "KCD" },
    { "KCL" },
    { "KDE" },
    { "KDK" },
    { "KDM" },
    { "KDS" },
    { "KDT" },
    { "KEC" },
    { "KEM" },
    { "KES" },
    { "KEU" },
    { "KEY" },
    { "KFC" },
    { "KFE" },
    { "KFX" },
    { "KGI" },
    { "KGL" },
    { "KGN" },
    { "KIO" },
    { "KIS" },
    { "KLT" },
    { "KMC" },
    { "KME" },
    { "KML" },
    { "KMR" },
    { "KNC" },
    { "KNX" },
    { "KOB" },
    { "KOD" },
    { "KOE" },
    { "KOL" },
    { "KOM" },
    { "KOP" },
    { "KOU" },
    { "KOW" },
    { "KPC" },
    { "KPT" },
    { "KRL" },
    { "KRM" },
    { "KRY" },
    { "KSC" },
    { "KSG" },
    { "KSL" },
    { "KSX" },
    { "KTC" },
    { "KTD" },
    { "KTE" },
    { "KTG" },
    { "KTI" },
    { "KTK" },
    { "KTN" },
    { "KTS" },
    { "KUR" },
    { "KVA" },
    { "KVX" },
    { "KWD" },
    { "KYC" },
    { "KYE" },
    { "KYK" },
    { "KYN" },
    { "KZI" },
    { "KZN" },
    { "LAB" },
    { "LAC" },
    { "LAF" },
    { "LAG" },
    { "LAN" },
    { "LAS" },
    { "LAV" },
    { "LBC" },
    { "LBO" },
    { "LCC" },
    { "LCD" },
    { "LCE" },
    { "LCI" },
    { "LCM" },
    { "LCN" },
    { "LCP" },
    { "LCS" },
    { "LCT" },
    { "LDN" },
    { "LDT" },
    { "LEC" },
    { "LED" },
    { "LEG" },
    { "LEN" },
    { "LEO" },
    { "LEX" },
    { "LGC" },
    { "LGD" },
    { "LGI" },
    { "LGS" },
    { "LGX" },
    { "LHA" },
    { "LHC" },
    { "LHE" },
    { "LHT" },
    { "LIN" },
    { "LIP" },
    { "LIS" },
    { "LIT" },
    { "LJX" },
    { "LKM" },
    { "LLL" },
    { "LLT" },
    { "LMG" },
    { "LMI" },
    { "LMP" },
    { "LMS" },
    { "LMT" },
    { "LNC" },
    { "LND" },
    { "LNK" },
    { "LNR" },
    { "LNT" },
    { "LNV" },
    { "LNX" },
    { "LOC" },
    { "LOE" },
    { "LOG" },
    { "LOL" },
    { "LPE" },
    { "LPI" },
    { "LPL" },
    { "LSC" },
    { "LSD" },
    { "LSI" },
    { "LSJ" },
    { "LSL" },
    { "LSP" },
    { "LSY" },
    { "LTC" },
    { "LTI" },
    { "LTK" },
    { "LTN" },
    { "LTS" },
    { "LTV" },
    { "LTW" },
    { "LUC" },
    { "LUM" },
    { "LUX" },
    { "LVI" },
    { "LWC" },
    { "LWR" },
    { "LWW" },
    { "LXC" },
    { "LXN" },
    { "LXS" },
    { "LZX" },
    { "MAC" },
    { "MAD" },
    { "MAE" },
    { "MAG" },
    { "MAI" },
    { "MAL" },
    { "MAN" },
    { "MAS" },
    { "MAT" },
    { "MAX" },
    { "MAY" },
    { "MAZ" },
    { "MBC" },
    { "MBD" },
    { "MBM" },
    { "MBV" },
    { "MCA" },
    { "MCC" },
    { "MCD" },
    { "MCE" },
    { "MCG" },
    { "MCI" },
    { "MCJ" },
    { "MCL" },
    { "MCM" },
    { "MCN" },
    { "MCO" },
    { "MCP" },
    { "MCQ" },
    { "MCR" },
    { "MCS" },
    { "MCT" },
    { "MCX" },
    { "MDA" },
    { "MDC" },
    { "MDD" },
    { "MDF" },
    { "MDG" },
    { "MDI" },
    { "MDK" },
    { "MDO" },
    { "MDR" },
    { "MDS" },
    { "MDT" },
    { "MDV" },
    { "MDX" },
    { "MDY" },
    { "MEC" },
    { "MED" },
    { "MEE" },
    { "MEG" },
    { "MEI" },
    { "MEJ" },
    { "MEK" },
    { "MEL" },
    { "MEN" },
    { "MEP" },
    { "MEQ" },
    { "MET" },
    { "MEU" },
    { "MEX" },
    { "MFG" },
    { "MFI" },
    { "MFR" },
    { "MGA" },
    { "MGC" },
    { "MGE" },
    { "MGL" },
    { "MGT" },
    { "MHQ" },
    { "MIC" },
    { "MID" },
    { "MII" },
    { "MIL" },
    { "MIM" },
    { "MIN" },
    { "MIP" },
    { "MIR" },
    { "MIS" },
    { "MIT" },
    { "MIV" },
    { "MJI" },
    { "MJS" },
    { "MKC" },
    { "MKS" },
    { "MKT" },
    { "MKV" },
    { "MLC" },
    { "MLD" },
    { "MLG" },
    { "MLI" },
    { "MLL" },
    { "MLM" },
    { "MLN" },
    { "MLP" },
    { "MLS" },
    { "MLT" },
    { "MLX" },
    { "MMA" },
    { "MMD" },
    { "MMF" },
    { "MMI" },
    { "MMM" },
    { "MMN" },
    { "MMS" },
    { "MMT" },
    { "MNC" },
    { "MNI" },
    { "MNL" },
    { "MNP" },
    { "MNS" },
    { "MOC" },
    { "MOD" },
    { "MOK" },
    { "MOM" },
    { "MOS" },
    { "MOT" },
    { "MPC" },
    { "MPI" },
    { "MPJ" },
    { "MPL" },
    { "MPN" },
    { "MPS" },
    { "MPV" },
    { "MPX" },
    { "MQP" },
    { "MRA" },
    { "MRC" },
    { "MRD" },
    { "MRG" },
    { "MRK" },
    { "MRL" },
    { "MRO" },
    { "MRT" },
    { "MSA" },
    { "MSC" },
    { "MSD" },
    { "MSF" },
    { "MSG" },
    { "MSH" },
    { "MSI" },
    { "MSK" },
    { "MSL" },
    { "MSM" },
    { "MSP" },
    { "MSR" },
    { "MST" },
    { "MSU" },
    { "MSV" },
    { "MSX" },
    { "MSY" },
    { "MTA" },
    { "MTB" },
    { "MTC" },
    { "MTD" },
    { "MTE" },
    { "MTH" },
    { "MTI" },
    { "MTJ" },
    { "MTK" },
    { "MTL" },
    { "MTM" },
    { "MTN" },
    { "MTR" },
    { "MTS" },
    { "MTT" },
    { "MTU" },
    { "MTX" },
    { "MUD" },
    { "MUK" },
    { "MVD" },
    { "MVI" },
    { "MVM" },
    { "MVN" },
    { "MVR" },
    { "MVS" },
    { "MVX" },
    { "MWI" },
    { "MWR" },
    { "MWY" },
    { "MXD" },
    { "MXI" },
    { "MXL" },
    { "MXM" },
    { "MXP" },
    { "MXT" },
    { "MXV" },
    { "MYA" },
    { "MYR" },
    { "MYX" },
    { "NAC" },
    { "NAD" },
    { "NAF" },
    { "NAK" },
    { "NAL" },
    { "NAT" },
    { "NAV" },
    { "NAX" },
    { "NBL" },
    { "NBS" },
    { "NBT" },
    { "NCA" },
    { "NCC" },
    { "NCE" },
    { "NCI" },
    { "NCL" },
    { "NCP" },
    { "NCR" },
    { "NCS" },
    { "NCT" },
    { "NCV" },
    { "NDC" },
    { "NDF" },
    { "NDI" },
    { "NDK" },
    { "NDL" },
    { "NDS" },
    { "NEC" },
    { "NEO" },
    { "NES" },
    { "NET" },
    { "NEU" },
    { "NEX" },
    { "NFC" },
    { "NFS" },
    { "NGC" },
    { "NGS" },
    { "NHC" },
    { "NHT" },
    { "NIC" },
    { "NIS" },
    { "NIT" },
    { "NIX" },
    { "NLC" },
    { "NME" },
    { "NMP" },
    { "NMS" },
    { "NMV" },
    { "NMX" },
    { "NNC" },
    { "NOD" },
    { "NOE" },
    { "NOI" },
    { "NOK" },
    { "NOR" },
    { "NOT" },
    { "NPA" },
    { "NPI" },
    { "NRI" },
    { "NRL" },
    { "NRT" },
    { "NRV" },
    { "NSA" },
    { "NSC" },
    { "NSI" },
    { "NSP" },
    { "NSS" },
    { "NST" },
    { "NTC" },
    { "NTI" },
    { "NTK" },
    { "NTL" },
    { "NTN" },
    { "NTR" },
    { "NTS" },
    { "NTT" },
    { "NTW" },
    { "NTX" },
    { "NUG" },
    { "NUI" },
    { "NVC" },
    { "NVD" },
    { "NVI" },
    { "NVL" },
    { "NVO" },
    { "NVR" },
    { "NVT" },
    { "NWC" },
    { "NWL" },
    { "NWP" },
    { "NWS" },
    { "NXC" },
    { "NXE" },
    { "NXG" },
    { "NXP" },
    { "NXQ" },
    { "NXR" },
    { "NXS" },
    { "NXT" },
    { "NYC" },
    { "OAK" },
    { "OAS" },
    { "OBS" },
    { "OCD" },
    { "OCN" },
    { "OCS" },
    { "ODM" },
    { "ODR" },
    { "OEC" },
    { "OEI" },
    { "OFI" },
    { "OHW" },
    { "OIC" },
    { "OIM" },
    { "OIN" },
    { "OKI" },
    { "OLC" },
    { "OLD" },
    { "OLI" },
    { "OLT" },
    { "OLV" },
    { "OLY" },
    { "OMC" },
    { "OMG" },
    { "OMN" },
    { "OMR" },
    { "ONE" },
    { "ONK" },
    { "ONL" },
    { "ONS" },
    { "ONW" },
    { "ONX" },
    { "OOS" },
    { "OPC" },
    { "OPI" },
    { "OPP" },
    { "OPT" },
    { "OPV" },
    { "OQI" },
    { "ORG" },
    { "ORI" },
    { "ORN" },
    { "OSA" },
    { "OSD" },
    { "OSI" },
    { "OSP" },
    { "OSR" },
    { "OTB" },
    { "OTI" },
    { "OTK" },
    { "OTM" },
    { "OTT" },
    { "OUK" },
    { "OVR" },
    { "OWL" },
    { "OXU" },
    { "OYO" },
    { "OZC" },
    { "OZD" },
    { "OZO" },
    { "PAC" },
    { "PAD" },
    { "PAE" },
    { "PAK" },
    { "PAM" },
    { "PAN" },
    { "PAR" },
    { "PBI" },
    { "PBL" },
    { "PBN" },
    { "PBV" },
    { "PCA" },
    { "PCB" },
    { "PCC" },
    { "PCG" },
    { "PCI" },
    { "PCK" },
    { "PCL" },
    { "PCM" },
    { "PCO" },
    { "PCP" },
    { "PCS" },
    { "PCT" },
    { "PCW" },
    { "PCX" },
    { "PDM" },
    { "PDN" },
    { "PDR" },
    { "PDS" },
    { "PDT" },
    { "PDV" },
    { "PEC" },
    { "PEG" },
    { "PEI" },
    { "PEL" },
    { "PEN" },
    { "PEP" },
    { "PER" },
    { "PET" },
    { "PFT" },
    { "PGI" },
    { "PGM" },
    { "PGP" },
    { "PGS" },
    { "PHC" },
    { "PHE" },
    { "PHI" },
    { "PHL" },
    { "PHO" },
    { "PHS" },
    { "PHY" },
    { "PIC" },
    { "PIE" },
    { "PIM" },
    { "PIO" },
    { "PIR" },
    { "PIS" },
    { "PIX" },
    { "PJA" },
    { "PJD" },
    { "PJT" },
    { "PKA" },
    { "PLC" },
    { "PLF" },
    { "PLM" },
    { "PLT" },
    { "PLV" },
    { "PLX" },
    { "PLY" },
    { "PMC" },
    { "PMD" },
    { "PMM" },
    { "PMS" },
    { "PMT" },
    { "PMX" },
    { "PNG" },
    { "PNL" },
    { "PNP" },
    { "PNR" },
    { "PNS" },
    { "PNT" },
    { "PNX" },
    { "POL" },
    { "PON" },
    { "POR" },
    { "POS" },
    { "POT" },
    { "PPC" },
    { "PPD" },
    { "PPI" },
    { "PPM" },
    { "PPP" },
    { "PPR" },
    { "PPX" },
    { "PQI" },
    { "PRA" },
    { "PRC" },
    { "PRD" },
    { "PRF" },
    { "PRG" },
    { "PRI" },
    { "PRM" },
    { "PRO" },
    { "PRP" },
    { "PRS" },
    { "PRT" },
    { "PRX" },
    { "PSA" },
    { "PSC" },
    { "PSD" },
    { "PSE" },
    { "PSI" },
    { "PSL" },
    { "PSM" },
    { "PST" },
    { "PSY" },
    { "PTA" },
    { "PTC" },
    { "PTG" },
    { "PTH" },
    { "PTI" },
    { "PTL" },
    { "PTS" },
    { "PTW" },
    { "PTX" },
    { "PUL" },
    { "PVC" },
    { "PVG" },
    { "PVI" },
    { "PVM" },
    { "PVN" },
    { "PVP" },
    { "PVR" },
    { "PXC" },
    { "PXE" },
    { "PXL" },
    { "PXM" },
    { "PXN" },
    { "PXO" },
    { "QCC" },
    { "QCH" },
    { "QCI" },
    { "QCK" },
    { "QCL" },
    { "QCP" },
    { "QDI" },
    { "QDL" },
    { "QDM" },
    { "QDS" },
    { "QFF" },
    { "QFI" },
    { "QLC" },
    { "QQQ" },
    { "QSC" },
    { "QSI" },
    { "QTD" },
    { "QTH" },
    { "QTI" },
    { "QTM" },
    { "QTR" },
    { "QUA" },
    { "QUE" },
    { "QVU" },
    { "RAC" },
    { "RAD" },
    { "RAI" },
    { "RAN" },
    { "RAR" },
    { "RAS" },
    { "RAT" },
    { "RAY" },
    { "RCE" },
    { "RCH" },
    { "RCI" },
    { "RCN" },
    { "RCO" },
    { "RDI" },
    { "RDL" },
    { "RDM" },
    { "RDN" },
    { "RDS" },
    { "REA" },
    { "REC" },
    { "RED" },
    { "REF" },
    { "REH" },
    { "REL" },
    { "REM" },
    { "REN" },
    { "RES" },
    { "RET" },
    { "REV" },
    { "REX" },
    { "RFI" },
    { "RFX" },
    { "RGB" },
    { "RGL" },
    { "RHD" },
    { "RHM" },
    { "RHT" },
    { "RIC" },
    { "RII" },
    { "RIO" },
    { "RIT" },
    { "RIV" },
    { "RJA" },
    { "RJS" },
    { "RKC" },
    { "RLD" },
    { "RLN" },
    { "RMC" },
    { "RMP" },
    { "RMS" },
    { "RMT" },
    { "RNB" },
    { "RNL" },
    { "ROB" },
    { "ROH" },
    { "ROK" },
    { "ROP" },
    { "ROS" },
    { "RPI" },
    { "RPL" },
    { "RPT" },
    { "RRI" },
    { "RRO" },
    { "RSC" },
    { "RSH" },
    { "RSI" },
    { "RSN" },
    { "RSQ" },
    { "RSR" },
    { "RSS" },
    { "RSV" },
    { "RSX" },
    { "RTC" },
    { "RTI" },
    { "RTK" },
    { "RTL" },
    { "RTS" },
    { "RUN" },
    { "RUP" },
    { "RVC" },
    { "RVI" },
    { "RVL" },
    { "RWC" },
    { "RXT" },
    { "RZR" },
    { "RZS" },
    { "SAA" },
    { "SAE" },
    { "SAG" },
    { "SAI" },
    { "SAK" },
    { "SAM" },
    { "SAN" },
    { "SAS" },
    { "SAT" },
    { "SBC" },
    { "SBD" },
    { "SBI" },
    { "SBS" },
    { "SBT" },
    { "SCA" },
    { "SCB" },
    { "SCC" },
    { "SCD" },
    { "SCE" },
    { "SCG" },
    { "SCH" },
    { "SCI" },
    { "SCL" },
    { "SCM" },
    { "SCN" },
    { "SCO" },
    { "SCP" },
    { "SCR" },
    { "SCS" },
    { "SCT" },
    { "SCX" },
    { "SDA" },
    { "SDC" },
    { "SDD" },
    { "SDE" },
    { "SDF" },
    { "SDH" },
    { "SDI" },
    { "SDK" },
    { "SDR" },
    { "SDS" },
    { "SDT" },
    { "SDX" },
    { "SEA" },
    { "SEB" },
    { "SEC" },
    { "SEE" },
    { "SEG" },
    { "SEI" },
    { "SEL" },
    { "SEM" },
    { "SEN" },
    { "SEO" },
    { "SEP" },
    { "SER" },
    { "SES" },
    { "SET" },
    { "SFL" },
    { "SFM" },
    { "SFT" },
    { "SGC" },
    { "SGD" },
    { "SGE" },
    { "SGI" },
    { "SGL" },
    { "SGM" },
    { "SGN" },
    { "SGO" },
    { "SGT" },
    { "SGW" },
    { "SGX" },
    { "SGZ" },
    { "SHC" },
    { "SHG" },
    { "SHI" },
    { "SHP" },
    { "SHR" },
    { "SHT" },
    { "SHU" },
    { "SIA" },
    { "SIB" },
    { "SIC" },
    { "SID" },
    { "SIE" },
    { "SIG" },
    { "SII" },
    { "SIL" },
    { "SIM" },
    { "SIN" },
    { "SIR" },
    { "SIS" },
    { "SIT" },
    { "SIU" },
    { "SIX" },
    { "SJE" },
    { "SKD" },
    { "SKG" },
    { "SKI" },
    { "SKM" },
    { "SKT" },
    { "SKW" },
    { "SKY" },
    { "SLA" },
    { "SLB" },
    { "SLC" },
    { "SLF" },
    { "SLH" },
    { "SLI" },
    { "SLK" },
    { "SLM" },
    { "SLR" },
    { "SLS" },
    { "SLT" },
    { "SLX" },
    { "SMA" },
    { "SMB" },
    { "SMC" },
    { "SME" },
    { "SMI" },
    { "SMK" },
    { "SML" },
    { "SMM" },
    { "SMN" },
    { "SMO" },
    { "SMP" },
    { "SMR" },
    { "SMS" },
    { "SMT" },
    { "SNC" },
    { "SNI" },
    { "SNK" },
    { "SNN" },
    { "SNO" },
    { "SNP" },
    { "SNS" },
    { "SNT" },
    { "SNV" },
    { "SNW" },
    { "SNX" },
    { "SNY" },
    { "SOC" },
    { "SOI" },
    { "SOL" },
    { "SON" },
    { "SOR" },
    { "SOT" },
    { "SOY" },
    { "SPC" },
    { "SPE" },
    { "SPH" },
    { "SPI" },
    { "SPK" },
    { "SPL" },
    { "SPN" },
    { "SPO" },
    { "SPR" },
    { "SPS" },
    { "SPT" },
    { "SPU" },
    { "SPX" },
    { "SQT" },
    { "SRC" },
    { "SRD" },
    { "SRF" },
    { "SRG" },
    { "SRS" },
    { "SRT" },
    { "SSC" },
    { "SSD" },
    { "SSE" },
    { "SSG" },
    { "SSI" },
    { "SSJ" },
    { "SSL" },
    { "SSP" },
    { "SSS" },
    { "SST" },
    { "STA" },
    { "STB" },
    { "STC" },
    { "STD" },
    { "STE" },
    { "STF" },
    { "STG" },
    { "STH" },
    { "STI" },
    { "STK" },
    { "STL" },
    { "STM" },
    { "STN" },
    { "STO" },
    { "STP" },
    { "STQ" },
    { "STR" },
    { "STS" },
    { "STT" },
    { "STU" },
    { "STV" },
    { "STW" },
    { "STX" },
    { "STY" },
    { "SUB" },
    { "SUM" },
    { "SUN" },
    { "SUP" },
    { "SUR" },
    { "SVA" },
    { "SVC" },
    { "SVD" },
    { "SVI" },
    { "SVR" },
    { "SVS" },
    { "SVT" },
    { "SWC" },
    { "SWI" },
    { "SWL" },
    { "SWO" },
    { "SWS" },
    { "SWT" },
    { "SXB" },
    { "SXD" },
    { "SXG" },
    { "SXI" },
    { "SXL" },
    { "SXT" },
    { "SYC" },
    { "SYE" },
    { "SYK" },
    { "SYL" },
    { "SYM" },
    { "SYN" },
    { "SYP" },
    { "SYS" },
    { "SYT" },
    { "SYV" },
    { "SYX" },
    { "SZM" },
    { "TAA" },
    { "TAB" },
    { "TAG" },
    { "TAI" },
    { "TAM" },
    { "TAS" },
    { "TAT" },
    { "TAV" },
    { "TAX" },
    { "TBB" },
    { "TBC" },
    { "TBS" },
    { "TCC" },
    { "TCD" },
    { "TCE" },
    { "TCF" },
    { "TCH" },
    { "TCI" },
    { "TCJ" },
    { "TCL" },
    { "TCM" },
    { "TCN" },
    { "TCO" },
    { "TCR" },
    { "TCS" },
    { "TCT" },
    { "TCX" },
    { "TDC" },
    { "TDD" },
    { "TDG" },
    { "TDM" },
    { "TDP" },
    { "TDS" },
    { "TDT" },
    { "TDV" },
    { "TDY" },
    { "TEA" },
    { "TEC" },
    { "TEK" },
    { "TEL" },
    { "TEN" },
    { "TER" },
    { "TET" },
    { "TEV" },
    { "TEZ" },
    { "TGC" },
    { "TGI" },
    { "TGM" },
    { "TGS" },
    { "TGV" },
    { "TGW" },
    { "THN" },
    { "TIC" },
    { "TIL" },
    { "TIP" },
    { "TIV" },
    { "TIX" },
    { "TKC" },
    { "TKG" },
    { "TKN" },
    { "TKO" },
    { "TKS" },
    { "TLA" },
    { "TLD" },
    { "TLE" },
    { "TLF" },
    { "TLI" },
    { "TLK" },
    { "TLL" },
    { "TLN" },
    { "TLS" },
    { "TLT" },
    { "TLV" },
    { "TLX" },
    { "TLY" },
    { "TMA" },
    { "TMC" },
    { "TME" },
    { "TMI" },
    { "TMM" },
    { "TMO" },
    { "TMR" },
    { "TMS" },
    { "TMT" },
    { "TMV" },
    { "TMX" },
    { "TNC" },
    { "TNJ" },
    { "TNM" },
    { "TNY" },
    { "TOE" },
    { "TOG" },
    { "TOL" },
    { "TOM" },
    { "TON" },
    { "TOP" },
    { "TOS" },
    { "TOU" },
    { "TPC" },
    { "TPD" },
    { "TPE" },
    { "TPJ" },
    { "TPK" },
    { "TPR" },
    { "TPS" },
    { "TPT" },
    { "TPV" },
    { "TPZ" },
    { "TRA" },
    { "TRB" },
    { "TRC" },
    { "TRD" },
    { "TRE" },
    { "TRI" },
    { "TRL" },
    { "TRM" },
    { "TRN" },
    { "TRP" },
    { "TRS" },
    { "TRT" },
    { "TRU" },
    { "TRV" },
    { "TRX" },
    { "TSB" },
    { "TSC" },
    { "TSD" },
    { "TSE" },
    { "TSF" },
    { "TSG" },
    { "TSH" },
    { "TSI" },
    { "TSL" },
    { "TSP" },
    { "TST" },
    { "TSV" },
    { "TSW" },
    { "TSY" },
    { "TTA" },
    { "TTB" },
    { "TTC" },
    { "TTE" },
    { "TTI" },
    { "TTK" },
    { "TTL" },
    { "TTP" },
    { "TTR" },
    { "TTS" },
    { "TTX" },
    { "TTY" },
    { "TUA" },
    { "TUT" },
    { "TVD" },
    { "TVI" },
    { "TVL" },
    { "TVM" },
    { "TVO" },
    { "TVR" },
    { "TVS" },
    { "TVV" },
    { "TWA" },
    { "TWE" },
    { "TWH" },
    { "TWI" },
    { "TWK" },
    { "TWX" },
    { "TXL" },
    { "TXN" },
    { "TXT" },
    { "TYN" },
    { "UAS" },
    { "UBI" },
    { "UBL" },
    { "UBU" },
    { "UDN" },
    { "UEC" },
    { "UEG" },
    { "UEI" },
    { "UET" },
    { "UFG" },
    { "UFO" },
    { "UHB" },
    { "UIC" },
    { "UJR" },
    { "ULT" },
    { "UMC" },
    { "UMG" },
    { "UMM" },
    { "UMT" },
    { "UNA" },
    { "UNB" },
    { "UNC" },
    { "UND" },
    { "UNE" },
    { "UNF" },
    { "UNI" },
    { "UNM" },
    { "UNO" },
    { "UNP" },
    { "UNS" },
    { "UNT" },
    { "UNY" },
    { "UPP" },
    { "UPS" },
    { "URD" },
    { "USA" },
    { "USD" },
    { "USE" },
    { "USI" },
    { "USR" },
    { "UTC" },
    { "UTD" },
    { "UWC" },
    { "VAD" },
    { "VAI" },
    { "VAL" },
    { "VAR" },
    { "VAT" },
    { "VAV" },
    { "VBR" },
    { "VBT" },
    { "VCC" },
    { "VCE" },
    { "VCI" },
    { "VCJ" },
    { "VCM" },
    { "VCX" },
    { "VDA" },
    { "VDC" },
    { "VDM" },
    { "VDO" },
    { "VDS" },
    { "VDT" },
    { "VEC" },
    { "VEK" },
    { "VES" },
    { "VFI" },
    { "VHI" },
    { "VIA" },
    { "VIB" },
    { "VIC" },
    { "VID" },
    { "VIK" },
    { "VIM" },
    { "VIN" },
    { "VIO" },
    { "VIR" },
    { "VIS" },
    { "VIT" },
    { "VIZ" },
    { "VLB" },
    { "VLC" },
    { "VLK" },
    { "VLM" },
    { "VLT" },
    { "VLV" },
    { "VMI" },
    { "VML" },
    { "VMW" },
    { "VNC" },
    { "VNX" },
    { "VOB" },
    { "VPI" },
    { "VPR" },
    { "VPX" },
    { "VQ@" },
    { "VRC" },
    { "VRG" },
    { "VRM" },
    { "VRS" },
    { "VRT" },
    { "VSC" },
    { "VSD" },
    { "VSI" },
    { "VSN" },
    { "VSP" },
    { "VSR" },
    { "VTB" },
    { "VTC" },
    { "VTG" },
    { "VTI" },
    { "VTK" },
    { "VTL" },
    { "VTM" },
    { "VTN" },
    { "VTS" },
    { "VTV" },
    { "VTX" },
    { "VUT" },
    { "VVI" },
    { "VWB" },
    { "WAC" },
    { "WAL" },
    { "WAN" },
    { "WAV" },
    { "WBN" },
    { "WBS" },
    { "WCI" },
    { "WCS" },
    { "WDC" },
    { "WDE" },
    { "WEB" },
    { "WEC" },
    { "WEL" },
    { "WEY" },
    { "WHI" },
    { "WII" },
    { "WIL" },
    { "WIN" },
    { "WIP" },
    { "WKH" },
    { "WLD" },
    { "WLF" },
    { "WMI" },
    { "WML" },
    { "WMO" },
    { "WMT" },
    { "WNI" },
    { "WNV" },
    { "WNX" },
    { "WPA" },
    { "WPI" },
    { "WRC" },
    { "WSC" },
    { "WSP" },
    { "WST" },
    { "WTC" },
    { "WTI" },
    { "WTK" },
    { "WTS" },
    { "WVM" },
    { "WVV" },
    { "WWP" },
    { "WWV" },
    { "WXT" },
    { "WYR" },
    { "WYS" },
    { "WYT" },
    { "XAC" },
    { "XAD" },
    { "XDM" },
    { "XER" },
    { "XES" },
    { "XFG" },
    { "XFO" },
    { "XIN" },
    { "XIO" },
    { "XIR" },
    { "XIT" },
    { "XLX" },
    { "XMI" },
    { "XMM" },
    { "XNT" },
    { "XOC" },
    { "XQU" },
    { "XRC" },
    { "XRO" },
    { "XSN" },
    { "XST" },
    { "XSY" },
    { "XTD" },
    { "XTE" },
    { "XTL" },
    { "XTN" },
    { "XYC" },
    { "XYE" },
    { "YED" },
    { "YHQ" },
    { "YHW" },
    { "YMH" },
    { "YOW" },
    { "ZAN" },
    { "ZAX" },
    { "ZAZ" },
    { "ZBR" },
    { "ZBX" },
    { "ZCT" },
    { "ZDS" },
    { "ZEN" },
    { "ZGT" },
    { "ZIC" },
    { "ZMC" },
    { "ZMT" },
    { "ZMZ" },
    { "ZNI" },
    { "ZNX" },
    { "ZOW" },
    { "ZRN" },
    { "ZSE" },
    { "ZTC" },
    { "ZTE" },
    { "ZTI" },
    { "ZTM" },
    { "ZTT" },
    { "ZWE" },
    { "ZYD" },
    { "ZYP" },
    { "ZYT" },
    { "ZYX" },
    { "ZZZ" }
};

static constexpr char q_edidVendorNames[] = {
    "Avolites Ltd\0"
    "Anatek Electronics Inc.\0"
    "Aava Mobile Oy\0"
    "AAEON Technology Inc.\0"
    "Ann Arbor Technologies\0"
    "ABBAHOME INC.\0"
    "AboCom System Inc.\0"
    "Allen Bradley Company\0"
    "Alcatel Bell\0"
    "D-Link Systems Inc\0"
    "Abaco Systems, Inc.\0"
    "Anchor Bay Technologies, Inc.\0"
    "Advanced Research Technology\0"
    "Ariel Corporation\0"
    "Aculab Ltd\0"
    "Accton Technology Corporation\0"
    "AWETA BV\0"
    "Actek Engineering Pty Ltd\0"
    "A&R Cambridge Ltd.\0"
    "Archtek Telecom Corporation\0"
    "Ancor Communications Inc\0"
    "Acksys\0"
    "Apricot Computers\0"
    "Acroloop Motion Control Systems Inc\0"
    "Allion Computer Inc.\0"
    "Aspen Tech Inc\0"
    "Acer Technologies\0"
    "Altos Computer Systems\0"
    "Applied Creative Technology\0"
    "Acculogic\0"
    "ActivCard S.A\0"
    "Addi-Data GmbH\0"
    "Aldebbaron\0"
    "Acnhor Datacomm\0"
    "Advanced Peripheral Devices Inc\0"
    "Arithmos, Inc.\0"
    "Airdrop Gaming LLC\0"
    "Aerodata Holdings Ltd\0"
    "ADI Systems Inc\0"
    "Adtek System Science Company Ltd\0"
    "ASTRA Security Products Ltd\0"
    "Ad Lib MultiMedia Inc\0"
    "Analog & Digital Devices Tel. Inc\0"
    "Adaptec Inc\0"
    "Nasa Ames Research Center\0"
    "Analog Devices Inc\0"
    "Adtek\0"
    "Advanced Micro Devices Inc\0"
    "Adax Inc\0"
    "ADDER TECHNOLOGY LTD\0"
    "Antex Electronics Corporation\0"
    "Advanced Electronic Designs, Inc.\0"
    "Actiontec Electric Inc\0"
    "Alpha Electronics Company\0"
    "ASEM S.p.A.\0"
    "Avencall\0"
    "Aetas Peripheral International\0"
    "Aethra Telecomunicazioni S.r.l.\0"
    "Alfa Inc\0"
    "Beijing Aerospace Golden Card Electronic Engineering Co.,Ltd.\0"
    "Artish Graphics Inc\0"
    "Argolis\0"
    "Advan Int'l Corporation\0"
    "AlgolTek, Inc.\0"
    "Agilent Technologies\0"
    "Advantech Co., Ltd.\0"
    "Astro HQ LLC\0"
    "Beijing AnHeng SecoTech Information Technology Co., Ltd.\0"
    "Arnos Insturments & Computer Systems\0"
    "Altmann Industrieelektronik\0"
    "Amptron International Inc.\0"
    "Dongguan Alllike Electronics Co., Ltd.\0"
    "Altos India Ltd\0"
    "AIMS Lab Inc\0"
    "Advanced Integ. Research Inc\0"
    "Alien Internet Services\0"
    "Aiwa Company Ltd\0"
    "ALTINEX, INC.\0"
    "AJA Video Systems, Inc.\0"
    "Akebia Ltd\0"
    "AKAMI Electric Co.,Ltd\0"
    "AKIA Corporation\0"
    "AMiT Ltd\0"
    "Asahi Kasei Microsystems Company Ltd\0"
    "Atom Komplex Prylad\0"
    "Anker Innovations Limited\0"
    "Askey Computer Corporation\0"
    "Alacron Inc\0"
    "Altec Corporation\0"
    "In4S Inc\0"
    "Alenco BV\0"
    "Realtek Semiconductor Corp.\0"
    "AL Systems\0"
    "Acer Labs\0"
    "Altec Lansing\0"
    "Acrolink Inc\0"
    "Alliance Semiconductor Corporation\0"
    "Acutec Ltd.\0"
    "Alana Technologies\0"
    "Algolith Inc.\0"
    "ALPS ALPINE CO., LTD.\0"
    "Advanced Logic\0"
    "Avance Logic Inc\0"
    "Altra\0"
    "AlphaView LCD\0"
    "ALEXON Co.,Ltd.\0"
    "Asia Microelectronic Development Inc\0"
    "Ambient Technologies, Inc.\0"
    "Attachmate Corporation\0"
    "Amdek Corporation\0"
    "American Megatrends Inc\0"
    "Anderson Multimedia Communications (HK) Limited\0"
    "Amimon LTD.\0"
    "Amino Technologies PLC and Amino Communications Limited\0"
    "AMP Inc\0"
    "AmTRAN Technology Co., Ltd.\0"
    "ARMSTEL, Inc.\0"
    "AMT International Industry\0"
    "AMX LLC\0"
    "Anakron\0"
    "Ancot\0"
    "Adtran Inc\0"
    "Anigma Inc\0"
    "Anko Electronic Company Ltd\0"
    "Analogix Semiconductor, Inc\0"
    "Anorad Corporation\0"
    "Andrew Network Production\0"
    "ANR Ltd\0"
    "Ansel Communication Company\0"
    "Ace CAD Enterprise Company Ltd\0"
    "Beijing ANTVR Technology Co., Ltd.\0"
    "Analog Way SAS\0"
    "Acer Netxus Inc\0"
    "AOpen Inc.\0"
    "Advanced Optics Electronics, Inc.\0"
    "America OnLine\0"
    "Alcatel\0"
    "American Power Conversion\0"
    "AppliAdata\0"
    "ALPS ALPINE CO., LTD.\0"
    "Horner Electric Inc\0"
    "A Plus Info Corporation\0"
    "Aplicom Oy\0"
    "Applied Memory Tech\0"
    "Appian Tech Inc\0"
    "Apple Computer Inc\0"
    "Aprilia s.p.a.\0"
    "Autologic Inc\0"
    "Audio Processing Technology  Ltd\0"
    "A+V Link\0"
    "AP Designs Ltd\0"
    "Alta Research Corporation\0"
    "AREC Inc.\0"
    "ICET S.p.A.\0"
    "Argus Electronics Co., LTD\0"
    "Argosy Research Inc\0"
    "Ark Logic Inc\0"
    "Arlotto Comnet Inc\0"
    "Arima\0"
    "Poso International B.V.\0"
    "ARRIS Group, Inc.\0"
    "Arescom Inc\0"
    "Corion Industrial Corporation\0"
    "Ascom Strategic Technology Unit\0"
    "USC Information Sciences Institute\0"
    "AseV Display Labs\0"
    "Ashton Bentley Concepts\0"
    "Ahead Systems\0"
    "Ask A/S\0"
    "AccuScene Corporation Ltd\0"
    "ASEM S.p.A.\0"
    "Asante Tech Inc\0"
    "ASP Microelectronics Ltd\0"
    "AST Research Inc\0"
    "Asuscom Network Inc\0"
    "AudioScience\0"
    "Rockwell Collins / Airshow Systems\0"
    "Allied Telesyn International (Asia) Pte Ltd\0"
    "Ably-Tech Corporation\0"
    "Alpha Telecom Inc\0"
    "Innovate Ltd\0"
    "Athena Informatica S.R.L.\0"
    "Allied Telesis KK\0"
    "ArchiTek Corporation\0"
    "Allied Telesyn Int'l\0"
    "Arcus Technology Ltd\0"
    "ATM Ltd\0"
    "Athena Smartcard Solutions Ltd.\0"
    "ASTRO DESIGN, INC.\0"
    "Alpha-Top Corporation\0"
    "AT&T\0"
    "Avocor Technologies USA, Inc\0"
    "Office Depot, Inc.\0"
    "Athenix Corporation\0"
    "AudioControl\0"
    "August Home, Inc.\0"
    "ALPS ALPINE CO., LTD.\0"
    "AU Optronics\0"
    "Aureal Semiconductor\0"
    "ASUSTek COMPUTER INC\0"
    "Autotime Corporation\0"
    "Auvidea GmbH\0"
    "Avaya Communication\0"
    "Auravision Corporation\0"
    "Avid Electronics Corporation\0"
    "Add Value Enterpises (Asia) Pte Ltd\0"
    "Avegant Corporation\0"
    "Nippon Avionics Co.,Ltd\0"
    "Atelier Vision Corporation\0"
    "Avalue Technology Inc.\0"
    "AVM GmbH\0"
    "Advance Computer Corporation\0"
    "Avocent Corporation\0"
    "AVer Information Inc.\0"
    "Avatron Software Inc.\0"
    "Avtek (Electronics) Pty Ltd\0"
    "SBS Technologies (Canada), Inc. (was Avvida Systems, Inc.)\0"
    "A/Vaux Electronics\0"
    "Access Works Comm Inc\0"
    "Aironet Wireless Communications, Inc\0"
    "Wave Systems\0"
    "Adrienne Electronics Corporation\0"
    "AXIOMTEK CO., LTD.\0"
    "Axell Corporation\0"
    "American Magnetics\0"
    "Axel\0"
    "Axonic Labs LLC\0"
    "American Express\0"
    "Axtend Technologies Inc\0"
    "Axxon Computer Corporation\0"
    "AXYZ Automation Services, Inc\0"
    "Aydin Displays\0"
    "Airlib, Inc\0"
    "Shenzhen three Connaught Information Technology Co., Ltd. (3nod Group)\0"
    "AZ Middelheim - Radiotherapy\0"
    "Aztech Systems Ltd\0"
    "Biometric Access Corporation\0"
    "Banyan\0"
    "an-najah university\0"
    "B&Bh\0"
    "Brain Boxes Limited\0"
    "BlueBox Video Limited\0"
    "Black Box Corporation\0"
    "Beaver Computer Corporaton\0"
    "Barco GmbH\0"
    "Broadata Communications Inc.\0"
    "Beck GmbH & Co. Elektronik Bauelemente KG\0"
    "Broadcom\0"
    "Deutsche Telekom Berkom GmbH\0"
    "Booria CAD/CAM systems\0"
    "Brahler ICS\0"
    "Blonder Tongue Labs, Inc.\0"
    "Barco Display Systems\0"
    "Beckhoff Automation\0"
    "Beckworth Enterprises Inc\0"
    "Beko Elektronik A.S.\0"
    "Beltronic Industrieelektronik GmbH\0"
    "Baug & Olufsen\0"
    "B.F. Engineering Corporation\0"
    "Barco Graphics N.V\0"
    "Budzetron Inc\0"
    "BitHeadz, Inc.\0"
    "Biamp Systems Corporation\0"
    "Big Island Communications\0"
    "Bigscreen, Inc.\0"
    "Boeckeler Instruments Inc\0"
    "Billion Electric Company Ltd\0"
    "BioLink Technologies International, Inc.\0"
    "Bit 3 Computer\0"
    "BILD INNOVATIVE TECHNOLOGY LLC\0"
    "Busicom\0"
    "BioLink Technologies\0"
    "Bloomberg L.P.\0"
    "Blackmagic Design\0"
    "Benson Medical Instruments Company\0"
    "BIOMED Lab\0"
    "BIOMEDISYS\0"
    "Bull AB\0"
    "Banksia Tech Pty Ltd\0"
    "Bang & Olufsen\0"
    "Boulder Nonlinear Systems\0"
    "Rainy Orchard\0"
    "BOE\0"
    "NINGBO BOIGLE DIGITAL TECHNOLOGY CO.,LTD\0"
    "BOS\0"
    "Micro Solutions, Inc.\0"
    "Barco, N.V.\0"
    "Best Power\0"
    "Braemac Pty Ltd\0"
    "BARC\0"
    "Bridge Information Co., Ltd\0"
    "Boca Research Inc\0"
    "Brainlab AG\0"
    "Braemar Inc\0"
    "BROTHER INDUSTRIES,LTD.\0"
    "Bose Corporation\0"
    "Robert Bosch GmbH\0"
    "Biomedical Systems Laboratory\0"
    "BRIGHTSIGN, LLC\0"
    "BodySound Technologies, Inc.\0"
    "Bit 3 Computer\0"
    "Brilliant Technology\0"
    "Bitfield Oy\0"
    "BusTech Inc\0"
    "BioTao Ltd\0"
    "Yasuhiko Shirai Melco Inc\0"
    "B.U.G., Inc.\0"
    "ATI Tech Inc\0"
    "Bull\0"
    "B&R Industrial Automation GmbH\0"
    "BusTek\0"
    "21ST CENTURY ENTERTAINMENT\0"
    "Bitworks Inc.\0"
    "Buxco Electronics\0"
    "byd:sign corporation\0"
    "Castles Automation Co., Ltd\0"
    "CA & F Elettronica\0"
    "CalComp\0"
    "Canon Inc.\0"
    "Acon\0"
    "Cambridge Audio\0"
    "Canopus Company Ltd\0"
    "Cardinal Company Ltd\0"
    "CASIO COMPUTER CO.,LTD\0"
    "Consultancy in Advanced Technology\0"
    "Cavium Networks, Inc\0"
    "ComputerBoards Inc\0"
    "Cebra Tech A/S\0"
    "Cabletime Ltd\0"
    "Cybex Computer Products Corporation\0"
    "C-Cube Microsystems\0"
    "Cache\0"
    "CONTEC CO.,LTD.\0"
    "CCL/ITRI\0"
    "Capetronic USA Inc\0"
    "Core Dynamics Corporation\0"
    "Convergent Data Devices\0"
    "Colin.de\0"
    "Christie Digital Systems Inc\0"
    "Concept Development Inc\0"
    "Cray Communications\0"
    "Codenoll Technical Corporation\0"
    "CalComp\0"
    "Computer Diagnostic Systems\0"
    "IBM Corporation\0"
    "Convergent Design Inc.\0"
    "Consumer Electronics Association\0"
    "Chicony Electronics Company Ltd\0"
    "Cambridge Electronic Design Ltd\0"
    "Cefar Digital Vision\0"
    "Crestron Electronics, Inc.\0"
    "MEC Electronics GmbH\0"
    "Centurion Technologies P/L\0"
    "C-DAC\0"
    "Ceronix\0"
    "TEC CORPORATION\0"
    "Atlantis\0"
    "Meta View, Inc.\0"
    "Chunghwa Picture Tubes, LTD\0"
    "Chyron Corp\0"
    "congatec AG\0"
    "Chase Research PLC\0"
    "ChangHong Electric Co.,Ltd\0"
    "Acer Inc\0"
    "Sichuan Changhong Electric CO, LTD.\0"
    "Chrontel Inc\0"
    "Chloride-R&D\0"
    "CHIC TECHNOLOGY CORP.\0"
    "Sichuang Changhong Corporation\0"
    "CH Products\0"
    "christmann informationstechnik + medien GmbH & Co. KG\0"
    "Agentur Chairos\0"
    "Chunghwa Picture Tubes,LTD.\0"
    "Cherry GmbH\0"
    "Comm. Intelligence Corporation\0"
    "Indicates an identity defined by CTS/DID Standards other than EDID\0"
    "Convergent Engineering, Inc.\0"
    "Cromack Industries Inc\0"
    "Citicom Infotech Private Limited\0"
    "Citron GmbH\0"
    "Ciprico Inc\0"
    "Cirrus Logic Inc\0"
    "Cisco Systems Inc\0"
    "Citifax Limited\0"
    "The Concept Keyboard Company Ltd\0"
    "Carina System Co., Ltd.\0"
    "Clarion Company Ltd\0"
    "COMMAT L.t.d.\0"
    "Classe Audio\0"
    "CoreLogic\0"
    "Cirrus Logic Inc\0"
    "CrystaLake Multimedia\0"
    "Clone Computers\0"
    "Clover Electronics\0"
    "automated computer control systems\0"
    "Clevo Company\0"
    "CardLogix\0"
    "CMC Ltd\0"
    "Colorado MicroDisplay, Inc.\0"
    "Chenming Mold Ind. Corp.\0"
    "C-Media Electronics\0"
    "Comark LLC\0"
    "Comtime GmbH\0"
    "Chimei Innolux Corporation\0"
    "Chi Mei Optoelectronics corp.\0"
    "Cambridge Research Systems Ltd\0"
    "CompuMaster Srl\0"
    "Comex Electronics AB\0"
    "American Power Conversion\0"
    "Alvedon Computers Ltd\0"
    "Micro-Star Int'l Co., Ltd.\0"
    "Cine-tal\0"
    "Connect Int'l A/S\0"
    "Canon Inc\0"
    "COINT Multimedia Systems\0"
    "COBY Electronics Co., Ltd\0"
    "CODAN Pty. Ltd.\0"
    "Codec Inc.\0"
    "Rockwell Collins, Inc.\0"
    "Comtrol Corporation\0"
    "Contec Company Ltd\0"
    "coolux GmbH\0"
    "Corollary Inc\0"
    "CoStar Corporation\0"
    "Core Technology Inc\0"
    "Polycow Productions\0"
    "Comrex\0"
    "Ciprico Inc\0"
    "CompuAdd\0"
    "Computer Peripherals Inc\0"
    "Compal Electronics Inc\0"
    "Capella Microsystems Inc.\0"
    "Compound Photonics\0"
    "Compaq Computer Company\0"
    "cPATH\0"
    "Powermatic Data Systems\0"
    "CRALTECH ELECTRONICA, S.L.\0"
    "CONRAC GmbH\0"
    "Cardinal Technical Inc\0"
    "Creative Labs Inc\0"
    "Contemporary Research Corp.\0"
    "Crio Inc.\0"
    "Creative Logic\0"
    "CORSAIR MEMORY Inc.\0"
    "Cornerstone Imaging\0"
    "Extraordinary Technologies PTY Limited\0"
    "Cirque Corporation\0"
    "Crescendo Communication Inc\0"
    "Cerevo Inc.\0"
    "Cammegh Limited\0"
    "Cyrix Corporation\0"
    "Transtex SA\0"
    "Crystal Semiconductor\0"
    "Cresta Systems Inc\0"
    "Concept Solutions & Engineering\0"
    "Cabletron System Inc\0"
    "Cloudium Systems Ltd.\0"
    "Cosmic Engineering Inc.\0"
    "California Institute of Technology\0"
    "CSS Laboratories\0"
    "CSTI Inc\0"
    "China Star Optoelectronics Technology Co., Ltd\0"
    "CoSystems Inc\0"
    "CTC Communication Development Company Ltd\0"
    "Chunghwa Telecom Co., Ltd.\0"
    "Creative Technology Ltd\0"
    "Computerm Corporation\0"
    "Computone Products\0"
    "Computer Technology Corporation\0"
    "Control4 Corporation\0"
    "Comtec Systems Co., Ltd.\0"
    "Creatix Polymedia GmbH\0"
    "Cubix Corporation\0"
    "Calibre UK Ltd\0"
    "Covia Inc.\0"
    "Colorado Video, Inc.\0"
    "Chromatec Video Products Ltd\0"
    "Clarity Visual Systems\0"
    "Curtiss-Wright Controls, Inc.\0"
    "Connectware Inc\0"
    "Conexant Systems\0"
    "CyberVision\0"
    "Cylink Corporation\0"
    "Cyclades Corporation\0"
    "Cyberlabs\0"
    "CYPRESS SEMICONDUCTOR CORPORATION\0"
    "Cytechinfo Inc\0"
    "Cyviz AS\0"
    "Cyberware\0"
    "Cyrix Corporation\0"
    "Shenzhen ChuangZhiCheng Technology Co., Ltd.\0"
    "Carl Zeiss AG\0"
    "Digital Acoustics Corporation\0"
    "Digatron Industrie Elektronik GmbH\0"
    "DAIS SET Ltd.\0"
    "Daktronics\0"
    "Digital Audio Labs Inc\0"
    "Danelec Marine A/S\0"
    "DAVIS AS\0"
    "Datel Inc\0"
    "Daou Tech Inc\0"
    "Davicom Semiconductor Inc\0"
    "DA2 Technologies Inc\0"
    "Data Apex Ltd\0"
    "Diebold Inc.\0"
    "DigiBoard Inc\0"
    "Databook Inc\0"
    "Doble Engineering Company\0"
    "DB Networks Inc\0"
    "Digital Communications Association\0"
    "Dale Computer Corporation\0"
    "Datacast LLC\0"
    "dSPACE GmbH\0"
    "Concepts Inc\0"
    "Dynamic Controls Ltd\0"
    "DCM Data Products\0"
    "Dialogue Technology Corporation\0"
    "Decros Ltd\0"
    "Diamond Computer Systems Inc\0"
    "Dancall Telecom A/S\0"
    "Datatronics Technology Inc\0"
    "DA2 Technologies Corporation\0"
    "Danka Data Devices\0"
    "Datasat Digital Entertainment\0"
    "Data Display AG\0"
    "Barco, N.V.\0"
    "Datadesk Technologies Inc\0"
    "Delta Information Systems, Inc\0"
    "Digital Equipment Corporation\0"
    "DEIF A/S\0"
    "Deico Electronics\0"
    "Dell Inc.\0"
    "DemoPad Software Ltd\0"
    "Densitron Computers Ltd\0"
    "idex displays\0"
    "DFI\0"
    "SharkTec A/S\0"
    "DEI Holdings dba Definitive Technology\0"
    "Digiital Arts Inc\0"
    "Data General Corporation\0"
    "DIGI International\0"
    "DugoTech Co., LTD\0"
    "Digicorp European sales S.A.\0"
    "Diagsoft Inc\0"
    "Dearborn Group Technology\0"
    "Dension Audio Systems\0"
    "DH Print\0"
    "Quadram\0"
    "Projectavision Inc\0"
    "Diadem\0"
    "Digicom S.p.A.\0"
    "Dataq Instruments Inc\0"
    "dPict Imaging, Inc.\0"
    "Daintelecom Co., Ltd\0"
    "Diseda S.A.\0"
    "Dragon Information Technology\0"
    "Capstone Visual Product Development\0"
    "Maygay Machines, Ltd\0"
    "Datakey Inc\0"
    "Dolby Laboratories Inc.\0"
    "Diamond Lane Comm. Corporation\0"
    "Delem\0"
    "Digital-Logic GmbH\0"
    "D-Link Systems Inc\0"
    "Dell Inc\0"
    "DLOGIC Ltd.\0"
    "Shenzhen Dlodlo Technologies Co., Ltd.\0"
    "Digitelec Informatique Park Cadera\0"
    "Digicom Systems Inc\0"
    "Dune Microsystems Corporation\0"
    "Monoprice.Inc\0"
    "Dimond Multimedia Systems Inc\0"
    "Dimension Engineering LLC\0"
    "Data Modul AG\0"
    "D&M Holdings Inc, Professional Business Company\0"
    "DOME imaging systems\0"
    "Distributed Management Task Force, Inc. (DMTF)\0"
    "NDS Ltd\0"
    "DNA Enterprises, Inc.\0"
    "Apache Micro Peripherals Inc\0"
    "Deterministic Networks Inc.\0"
    "Dr. Neuhous Telekommunikation GmbH\0"
    "DiCon\0"
    "Dolman Technologies Group Inc\0"
    "Dome Imaging Systems\0"
    "DENON, Ltd.\0"
    "Dotronic Mikroelektronik GmbH\0"
    "DigiTalk Pro AV\0"
    "Delta Electronics Inc\0"
    "Delphi Automotive LLP\0"
    "DocuPoint\0"
    "Digital Projection Limited\0"
    "ADPM Synthesis sas\0"
    "Shanghai Lexiang Technology Limited\0"
    "Digital Processing Systems\0"
    "DPT\0"
    "DpiX, Inc.\0"
    "Datacube Inc\0"
    "Dr. Bott KG\0"
    "Data Ray Corp.\0"
    "DIGITAL REFLECTION INC.\0"
    "Data Race Inc\0"
    "DRS Defense Solutions, LLC\0"
    "Display Solution AG\0"
    "DS Multimedia Pte Ltd\0"
    "Disguise Technologies\0"
    "Digitan Systems Inc\0"
    "VR Technology Holdings Limited\0"
    "DSM Digital Services GmbH\0"
    "Domain Technology Inc\0"
    "DELTATEC\0"
    "DTC Tech Corporation\0"
    "Dimension Technologies, Inc.\0"
    "Diversified Technology, Inc.\0"
    "Dynax Electronics (HK) Ltd\0"
    "e-Net Inc\0"
    "Daten Tecnologia\0"
    "Datang  Telephone Co\0"
    "Deutsche Thomson OHG\0"
    "Design & Test Technology, Inc.\0"
    "Data Translation\0"
    "Dosch & Amand GmbH & Company KG\0"
    "NCR Corporation\0"
    "Dictaphone Corporation\0"
    "Devolo AG\0"
    "Digital Video System\0"
    "Data Video\0"
    "Daewoo Electronics Company Ltd\0"
    "Digipronix Control Systems\0"
    "DECIMATOR DESIGN PTY LTD\0"
    "Dextera Labs Inc\0"
    "Dixon Technologies (India) Limited\0"
    "Data Expert Corporation\0"
    "Signet\0"
    "Dycam Inc\0"
    "Dymo-CoStar Corporation\0"
    "Askey Computer Corporation\0"
    "Dynax Electronics (HK) Ltd\0"
    "Emotiva Audio Corp.\0"
    "ELTEC Elektronik AG\0"
    "Evans and Sutherland Computer\0"
    "Data Price Informatica\0"
    "EBS Euchner Büro- und Schulsysteme GmbH\0"
    "HUALONG TECHNOLOGY CO., LTD\0"
    "Electro Cam Corp.\0"
    "ESSential Comm. Corporation\0"
    "EchoStar Corporation\0"
    "Enciris Technologies\0"
    "Eugene Chukhlomin Sole Proprietorship, d.b.a.\0"
    "Excel Company Ltd\0"
    "E-Cmos Tech Corporation\0"
    "Echo Speech Corporation\0"
    "Elecom Company Ltd\0"
    "Elitegroup Computer Systems Company Ltd\0"
    "Enciris Technologies\0"
    "e.Digital Corporation\0"
    "Electronic-Design GmbH\0"
    "Edimax Tech. Company Ltd\0"
    "EDMI\0"
    "Emerging Display Technologies Corp\0"
    "ET&T Technology Company Ltd\0"
    "EEH Datalink GmbH\0"
    "ELARABY COMPANY FOR ENGINEERING INDUSTRIES\0"
    "E.E.P.D. GmbH\0"
    "EE Solutions, Inc.\0"
    "Elgato Systems LLC\0"
    "EIZO GmbH Display Technologies\0"
    "Eagle Technology\0"
    "Egenera, Inc.\0"
    "Ergo Electronics\0"
    "Epson Research\0"
    "Enhansoft\0"
    "Eicon Technology Corporation\0"
    "Elegant Invention\0"
    "MagTek Inc.\0"
    "Eastman Kodak Company\0"
    "EKSEN YAZILIM\0"
    "ELAD srl\0"
    "Electro Scientific Ind\0"
    "Express Luck, Inc.\0"
    "Elecom Company Ltd\0"
    "Elmeg GmbH Kommunikationstechnik\0"
    "Edsun Laboratories\0"
    "Electrosonic Ltd\0"
    "Elmic Systems Inc\0"
    "Elo TouchSystems Inc\0"
    "ELSA GmbH\0"
    "Element Labs, Inc.\0"
    "Express Industrial, Ltd.\0"
    "Elonex PLC\0"
    "Embedded computing inc ltd\0"
    "eMicro Corporation\0"
    "Embrionix Design Inc.\0"
    "EMiNE TECHNOLOGY COMPANY, LTD.\0"
    "EMG Consultants Inc\0"
    "Ex Machina Inc\0"
    "Emcore Corporation\0"
    "ELMO COMPANY, LIMITED\0"
    "ICC Intelligent Platforms GmbH\0"
    "Emulex Corporation\0"
    "Eizo Nanao Corporation\0"
    "ENIDAN Technologies Ltd\0"
    "ENE Technology Inc.\0"
    "Efficient Networks\0"
    "Ensoniq Corporation\0"
    "Enterprise Comm. & Computing Inc\0"
    "Eon Instrumentation, Inc.\0"
    "Empac\0"
    "Epiphan Systems Inc.\0"
    "Envision Peripherals, Inc\0"
    "EPiCON Inc.\0"
    "KEPS\0"
    "Equipe Electronics Ltd.\0"
    "Equinox Systems Inc\0"
    "Ergo System\0"
    "Ericsson Mobile Communications AB\0"
    "Ericsson, Inc.\0"
    "Euraplan GmbH\0"
    "Eizo Rugged Solutions\0"
    "Escort Insturments Corporation\0"
    "Elbit Systems of America\0"
    "ScioTeq\0"
    "Eden Sistemas de Computacao S/A\0"
    "Ensemble Designs, Inc\0"
    "ELCON Systemtechnik GmbH\0"
    "Extended Systems, Inc.\0"
    "ES&S\0"
    "Esterline Technologies\0"
    "eSATURNUS\0"
    "ESS Technology Inc\0"
    "Embedded Solution Technology\0"
    "E-Systems Inc\0"
    "Everton Technology Company Ltd\0"
    "ELAN MICROELECTRONICS CORPORATION\0"
    "Eizo Technologies GmbH\0"
    "Etherboot Project\0"
    "Eclipse Tech Inc\0"
    "eTEK Labs Inc.\0"
    "Evertz Microsystems Ltd.\0"
    "Electronic Trade Solutions Ltd\0"
    "E-Tech Inc\0"
    "Ericsson Mobile Networks B.V.\0"
    "Advanced Micro Peripherals Ltd\0"
    "eviateg GmbH\0"
    "EverPro Technologies Company Limited\0"
    "Everex\0"
    "Exabyte\0"
    "Excession Audio\0"
    "Exide Electronics\0"
    "RGB Systems, Inc. dba Extron Electronics\0"
    "Data Export Corporation\0"
    "Explorer Inc.\0"
    "Exatech Computadores & Servicos Ltda\0"
    "Exxact GmbH\0"
    "Exterity Ltd\0"
    "eyevis GmbH\0"
    "eyefactive Gmbh\0"
    "EzE Technologies\0"
    "Storm Technology\0"
    "Fantalooks Co., Ltd.\0"
    "Farallon Computing\0"
    "Interface Corporation\0"
    "Furukawa Electric Company Ltd\0"
    "First International Computer Ltd\0"
    "Focus Enhancements, Inc.\0"
    "Future Domain\0"
    "Forth Dimension Displays Ltd\0"
    "Future Designs, Inc.\0"
    "Fujitsu Display Technologies Corp.\0"
    "Findex, Inc.\0"
    "FURUNO ELECTRIC CO., LTD.\0"
    "Fellowes & Questec\0"
    "Fen Systems Ltd.\0"
    "Ferranti Int'L\0"
    "FUJIFILM Corporation\0"
    "Fairfield Industries\0"
    "Lisa Draexlmaier GmbH\0"
    "Fujitsu General Limited.\0"
    "FHLP\0"
    "Formosa Industrial Computing Inc\0"
    "Forefront Int'l Ltd\0"
    "Finecom Co., Ltd.\0"
    "Chaplet Systems Inc\0"
    "FLY-IT Simulators\0"
    "Feature Integration Technology Inc.\0"
    "FCL COMPONENTS LIMITED\0"
    "Fujitsu Spain\0"
    "F.J. Tieman BV\0"
    "ADTI Media, Inc\0"
    "Faroudja Laboratories\0"
    "Butterfly Communications\0"
    "Fast Multimedia AG\0"
    "Ford Microelectronics Inc\0"
    "Fellowes, Inc.\0"
    "Fujitsu Microelect Ltd\0"
    "Formoza-Altair\0"
    "Fanuc LTD\0"
    "Funai Electric Co., Ltd.\0"
    "FOR-A Company Limited\0"
    "Fokus Technologies GmbH\0"
    "Foss Tecator\0"
    "FOVE INC\0"
    "HON HAI PRECISION IND.CO.,LTD.\0"
    "Fingerprint Cards AB\0"
    "Fujitsu Peripherals Ltd\0"
    "Deltec Corporation\0"
    "Cirel Systemes\0"
    "Force Computers\0"
    "Freedom Scientific BLV\0"
    "Forvus Research Inc\0"
    "Fibernet Research Inc\0"
    "FARO Technologies\0"
    "South Mountain Technologies, LTD\0"
    "Future Systems Consulting KK\0"
    "Fore Systems Inc\0"
    "Modesto PC Inc\0"
    "Futuretouch Corporation\0"
    "Frontline Test Equipment Inc.\0"
    "FTG Data Systems\0"
    "FastPoint Technologies, Inc.\0"
    "FUJITSU TEN LIMITED\0"
    "Fountain Technologies Inc\0"
    "Mediasonic\0"
    "FocalTech Systems Co., Ltd.\0"
    "MindTribe Product Engineering, Inc.\0"
    "Fujitsu Ltd\0"
    "Fun Technology Innovation INC.\0"
    "sisel muhendislik\0"
    "Fujitsu Siemens Computers GmbH\0"
    "First Virtual Corporation\0"
    "C-C-C Group Plc\0"
    "Attero Tech, LLC\0"
    "Flat Connections Inc\0"
    "Fuji Xerox\0"
    "Founder Group Shenzhen Co.\0"
    "FZI Forschungszentrum Informatik\0"
    "GreenArrays, Inc.\0"
    "Gage Applied Sciences Inc\0"
    "Galil Motion Control\0"
    "Gaudi Co., Ltd.\0"
    "GIGA-BYTE TECHNOLOGY CO., LTD.\0"
    "GCC Technologies Inc\0"
    "Gateway Comm. Inc\0"
    "Grey Cell Systems Ltd\0"
    "General Datacom\0"
    "G. Diehl ISDN GmbH\0"
    "GDS\0"
    "Vortex Computersysteme GmbH\0"
    "Gechic Corporation\0"
    "General Dynamics C4 Systems\0"
    "GE Fanuc Embedded Systems\0"
    "Abaco Systems, Inc.\0"
    "Gem Plus\0"
    "Genesys ATE Inc\0"
    "GEO Sense\0"
    "GERMANEERS GmbH\0"
    "GES Singapore Pte Ltd\0"
    "Getac Technology Corporation\0"
    "GFMesstechnik GmbH\0"
    "Gefen Inc.\0"
    "Google Inc.\0"
    "G2TOUCH KOREA\0"
    "General Inst. Corporation\0"
    "Guillemont International\0"
    "GI Provision Ltd\0"
    "AT&T Global Info Solutions\0"
    "Grand Junction Networks\0"
    "Goldmund - Digital Audio SA\0"
    "AD electronics\0"
    "Genesys Logic\0"
    "Gadget Labs LLC\0"
    "GMK Electronic Design GmbH\0"
    "General Information Systems\0"
    "GMM Research Inc\0"
    "GEMINI 2000 Ltd\0"
    "GMX Inc\0"
    "Gennum Corporation\0"
    "GN Nettest Inc\0"
    "Gunze Ltd\0"
    "GOEPEL electronic GmbH\0"
    "GoPro, Inc.\0"
    "Graphica Computer\0"
    "GOLD RAIN ENTERPRISES CORP.\0"
    "Granch Ltd\0"
    "Garmin International\0"
    "Advanced Gravis\0"
    "Robert Gray Company\0"
    "NIPPONDENCHI CO,.LTD\0"
    "General Standards Corporation\0"
    "LG Electronics\0"
    "Grandstream Networks, Inc.\0"
    "Graphic SystemTechnology\0"
    "Grossenbacher Systeme AG\0"
    "Graphtec Corporation\0"
    "Goldtouch\0"
    "G-Tech Corporation\0"
    "Garnet System Company Ltd\0"
    "Geotest Marvin Test Systems Inc\0"
    "General Touch Technology Co., Ltd.\0"
    "Guntermann & Drunck GmbH\0"
    "GoUp Co.,Ltd\0"
    "Guzik Technical Enterprises\0"
    "GVC Corporation\0"
    "Global Village Communication\0"
    "G.VISION\0"
    "GW Instruments\0"
    "Gateworks Corporation\0"
    "Gateway 2000\0"
    "Galaxy Microsystems Ltd.\0"
    "GUNZE Limited\0"
    "Haider electronics\0"
    "Haivision Systems Inc.\0"
    "Halberthal\0"
    "Hanchang System Corporation\0"
    "Harris Corporation\0"
    "Hayes Microcomputer Products Inc\0"
    "DAT\0"
    "Hitachi Consumer Electronics Co., Ltd\0"
    "HCL America Inc\0"
    "HCL Peripherals\0"
    "Hitachi Computer Products Inc\0"
    "Hauppauge Computer Works Inc\0"
    "HardCom Elektronik & Datateknik\0"
    "HD-INFO d.o.o.\0"
    "Holografika kft.\0"
    "Hisense Electric Co., Ltd.\0"
    "Hitachi Micro Systems Europe Ltd\0"
    "Ascom Business Systems\0"
    "HETEC Datensysteme GmbH\0"
    "HIRAKAWA HEWTECH CORP.\0"
    "Fraunhofer Heinrich-Hertz-Institute\0"
    "Hitevision Group\0"
    "Hibino Corporation\0"
    "Hitachi Information Technology Co., Ltd.\0"
    "Harman International Industries, Inc\0"
    "Hikom Co., Ltd.\0"
    "Hilevel Technology\0"
    "Kaohsiung Opto Electronics Americas, Inc.\0"
    "Hope Industrial Systems, Inc.\0"
    "Hitachi America Ltd\0"
    "Harris & Jeffries Inc\0"
    "HONKO MFG. CO., LTD.\0"
    "HKC OVERSEAS LIMITED\0"
    "Josef Heim KG\0"
    "China Hualu Group Co., Ltd.\0"
    "Hualon Microelectric Corporation\0"
    "hmk Daten-System-Technik BmbH\0"
    "HUMAX Co., Ltd.\0"
    "HONOR Device Co., Ltd.\0"
    "Hughes Network Systems\0"
    "HOB Electronic GmbH\0"
    "Hosiden Corporation\0"
    "Holoeye Photonics AG\0"
    "Sonitronix\0"
    "Zytor Communications\0"
    "Hewlett-Packard Co.\0"
    "Hewlett Packard\0"
    "Hewlett Packard Enterprise\0"
    "Headplay, Inc.\0"
    "HAMAMATSU PHOTONICS K.K.\0"
    "HP Inc.\0"
    "Hewlett-Packard Co.\0"
    "H.P.R. Electronics GmbH\0"
    "Hercules\0"
    "Qingdao Haier Electronics Co., Ltd.\0"
    "Hall Research\0"
    "Herolab GmbH\0"
    "Harris Semiconductor\0"
    "HERCULES\0"
    "Hagiwara Sys-Com Company Ltd\0"
    "HannStar Display Corp\0"
    "AT&T Microelectronics\0"
    "Hansung Co., Ltd\0"
    "HannStar Display Corp\0"
    "Horsent Technology Co., Ltd.\0"
    "Hitachi Ltd\0"
    "Hampshire Company, Inc.\0"
    "Holtek Microelectronics Inc\0"
    "HTBLuVA Mödling\0"
    "Shenzhen ZhuoYi HengTong Computer Technology Limited\0"
    "Hitex Systementwicklung GmbH\0"
    "GAI-Tronics, A Hubbell Company\0"
    "Hoffmann + Krippner GmbH\0"
    "IMP Electronics Ltd.\0"
    "HTC Corportation\0"
    "Harris Canada Inc\0"
    "DBA Hans Wedemeyer\0"
    "Highwater Designs Ltd\0"
    "Hewlett Packard\0"
    "Huawei Technologies Co., Inc.\0"
    "Hexium Ltd.\0"
    "Hypercope Gmbh Aachen\0"
    "Hydis Technologies.Co.,LTD\0"
    "Shanghai Chai Ming Huang Info&Tech Co, Ltd\0"
    "HYC CO., LTD.\0"
    "Hyphen Ltd\0"
    "Hypertec Pty Ltd\0"
    "Heng Yu Technology (HK) Limited\0"
    "Hynix Semiconductor\0"
    "IAdea Corporation\0"
    "Institut f r angewandte Funksystemtechnik GmbH\0"
    "Integration Associates, Inc.\0"
    "IAT Germany GmbH\0"
    "Integrated Business Systems\0"
    "INBINE.CO.LTD\0"
    "IBM Brasil\0"
    "IBP Instruments GmbH\0"
    "IBR GmbH\0"
    "ICA Inc\0"
    "BICC Data Networks Ltd\0"
    "ICD Inc\0"
    "IC Ensemble\0"
    "Infotek Communication Inc\0"
    "Intracom SA\0"
    "Sanyo Icon\0"
    "Intel Corp\0"
    "ICP Electronics, Inc./iEi Technology Corp.\0"
    "Icron\0"
    "Integrated Circuit Systems\0"
    "Inside Contactless\0"
    "ICCC A/S\0"
    "International Datacasting Corporation\0"
    "IDE Associates\0"
    "IDK Corporation\0"
    "Idneo Technologies\0"
    "IDEO Product Development\0"
    "Integrated Device Technology, Inc.\0"
    "Interdigital Sistemas de Informacao\0"
    "International Display Technology\0"
    "IDEXX Labs\0"
    "Interlace Engineering Corporation\0"
    "IEE\0"
    "Interlink Electronics\0"
    "In Focus Systems Inc\0"
    "Informtech\0"
    "Infineon Technologies AG\0"
    "Infinite Z\0"
    "Intergate Pty Ltd\0"
    "IGM Communi\0"
    "InHand Electronics\0"
    "ISIC Innoscan Industrial Computers A/S\0"
    "Intelligent Instrumentation\0"
    "IINFRA Co., Ltd\0"
    "Informatik Information Technologies\0"
    "Ikegami Tsushinki Co. Ltd.\0"
    "Ikos Systems Inc\0"
    "Image Logic Corporation\0"
    "Innotech Corporation\0"
    "Imagraph\0"
    "ART s.r.l.\0"
    "IMC Networks\0"
    "ImasDe Canarias S.A.\0"
    "Imagraph\0"
    "Immersive Audio Technologies France\0"
    "IMAGENICS Co., Ltd.\0"
    "International Microsystems Inc\0"
    "Immersion Corporation\0"
    "Impossible Production\0"
    "Impinj\0"
    "Inmax Technology Corporation\0"
    "arpara Technology Co., Ltd.\0"
    "Inventec Corporation\0"
    "Home Row Inc\0"
    "ILC\0"
    "Inventec Electronics (M) Sdn. Bhd.\0"
    "Inframetrics Inc\0"
    "Integraph Corporation\0"
    "Initio Corporation\0"
    "Indtek Co., Ltd.\0"
    "InnoLux Display Corporation\0"
    "InnoMedia Inc\0"
    "Innovent Systems, Inc.\0"
    "Innolab Pte Ltd\0"
    "Interphase Corporation\0"
    "Ines GmbH\0"
    "Interphase Corporation\0"
    "Inovatec S.p.A.\0"
    "Inviso, Inc.\0"
    "Communications Supply Corporation (A division of WESCO)\0"
    "Best Buy\0"
    "CRE Technology Corporation\0"
    "Guangxi Century Innovation Display Electronics Co., Ltd\0"
    "I-O Data Device Inc\0"
    "Iomega\0"
    "Inside Out Networks\0"
    "i-O Display System\0"
    "I/OTech Inc\0"
    "IPC Corporation\0"
    "Industrial Products Design, Inc.\0"
    "Intelligent Platform Management Interface (IPMI) forum (Intel, HP, NEC, Dell)\0"
    "IPM Industria Politecnica Meridionale SpA\0"
    "Performance Technologies\0"
    "IP Power Technologies GmbH\0"
    "IP3 Technology Ltd.\0"
    "Ithaca Peripherals\0"
    "IPS, Inc. (Intellectual Property Solutions, Inc.)\0"
    "International Power Technologies\0"
    "IPWireless, Inc\0"
    "IneoQuest Technologies, Inc\0"
    "IMAGEQUEST Co., Ltd\0"
    "Irdata\0"
    "Symbol Technologies\0"
    "Id3 Semiconductors\0"
    "Insignia Solutions Inc\0"
    "Interface Solutions\0"
    "Isolation Systems\0"
    "Image Stream Medical\0"
    "IntreSource Systems Pte Ltd\0"
    "INSIS Co., LTD.\0"
    "ISS Inc\0"
    "Intersolve Technologies\0"
    "International Integrated Systems,Inc.(IISI)\0"
    "Itausa Export North America\0"
    "Intercom Inc\0"
    "Internet Technology Corporation\0"
    "Integrated Tech Express Inc\0"
    "VanErum Group\0"
    "ITK Telekommunikation AG\0"
    "Inter-Tel\0"
    "ITM inc.\0"
    "The NTI Group\0"
    "IT-PRO Consulting und Systemhaus GmbH\0"
    "Infotronic America, Inc.\0"
    "IDTECH\0"
    "I&T Telecom.\0"
    "integrated Technology Express Inc\0"
    "ICSL\0"
    "Intervoice Inc\0"
    "Iiyama North America\0"
    "InfoVision Optoelectronics (Kunshan) Co.,Ltd China\0"
    "Inlife-Handnet Co., Ltd.\0"
    "Intevac Photonics Inc.\0"
    "Icuiti Corporation\0"
    "Intelliworxx, Inc.\0"
    "Intertex Data AB\0"
    "Shenzhen Inet Mobile Internet Technology Co., LTD\0"
    "Astec Inc\0"
    "Japan Aviation Electronics Industry, Limited\0"
    "Janz Automationssysteme AG\0"
    "Jaton Corporation\0"
    "Carrera Computer Inc\0"
    "Jace Tech Inc\0"
    "Japan Display Inc.\0"
    "Japan Digital Laboratory Co.,Ltd.\0"
    "Japan E.M.Solutions Co., Ltd.\0"
    "N-Vision\0"
    "JET POWER TECHNOLOGY CO., LTD.\0"
    "Jones Futurex Inc\0"
    "University College\0"
    "Jaeik Information & Communication Co., Ltd.\0"
    "JVC KENWOOD Corporation\0"
    "UNIONMAN TECHNOLOGY CO., LTD\0"
    "Micro Technical Company Ltd\0"
    "JPC Technology Limited\0"
    "Wallis Hamilton Industries\0"
    "CNet Technical Inc\0"
    "JS DigiTech, Inc\0"
    "Jupiter Systems, Inc.\0"
    "SANKEN ELECTRIC CO., LTD\0"
    "JS Motorsports\0"
    "jetway security micro,inc\0"
    "Janich & Klass Computertechnik GmbH\0"
    "Jupiter Systems\0"
    "JVC\0"
    "Video International Inc.\0"
    "Jewell Instruments, LLC\0"
    "JWSpencer & Co.\0"
    "Jetway Information Co., Ltd\0"
    "Karna\0"
    "Kidboard Inc\0"
    "Kobil Systems GmbH\0"
    "Chunichi Denshi Co.,LTD.\0"
    "Keycorp Ltd\0"
    "KDE\0"
    "Kodiak Tech\0"
    "Korea Data Systems Co., Ltd.\0"
    "KDS USA\0"
    "KDDI Technology Corporation\0"
    "Kyushu Electronics Systems Inc\0"
    "Kontron Embedded Modules GmbH\0"
    "Kesa Corporation\0"
    "Kontron Europe GmbH\0"
    "Key Tech Inc\0"
    "SCD Tech\0"
    "Komatsu Forest\0"
    "Kofax Image Products\0"
    "Klipsch Group, Inc\0"
    "KEISOKU GIKEN Co.,Ltd.\0"
    "KOGAN AUSTRALIA PTY LTD\0"
    "Kionix, Inc.\0"
    "KiSS Technology A/S\0"
    "Colorlight\0"
    "Mitsumi Company Ltd\0"
    "KIMIN Electronics Co., Ltd.\0"
    "Kensington Microware Ltd\0"
    "Kramer Electronics Ltd. International\0"
    "Konica corporation\0"
    "Nutech Marketing PTL\0"
    "Kobil Systems GmbH\0"
    "Eastman Kodak Company\0"
    "KOLTER ELECTRONIC\0"
    "Kollmorgen Motion Technologies Group\0"
    "Kontron GmbH\0"
    "Kopin Corporation\0"
    "KOUZIRO Co.,Ltd.\0"
    "KOWA Company,LTD.\0"
    "King Phoenix Company\0"
    "TPK Holding Co., Ltd\0"
    "Krell Industries Inc.\0"
    "Kroma Telecom\0"
    "Kroy LLC\0"
    "Kinetic Systems Corporation\0"
    "KUPA China Shenzhen Micro Technology Co., Ltd. Gold Institute\0"
    "Karn Solutions Ltd.\0"
    "King Tester Corporation\0"
    "Kingston Tech Corporation\0"
    "Takahata Electronics Co.,Ltd.\0"
    "K-Tech\0"
    "Kayser-Threde GmbH\0"
    "Konica Technical Inc\0"
    "Key Tronic Corporation\0"
    "Katron Tech Inc\0"
    "Kyokko Communication System Co., Ltd.\0"
    "Kurta Corporation\0"
    "Kvaser AB\0"
    "KeyView\0"
    "Kenwood Corporation\0"
    "Kyocera Corporation\0"
    "KYE Syst Corporation\0"
    "Samsung Electronics America Inc\0"
    "KEYENCE CORPORATION\0"
    "K-Zone International co. Ltd.\0"
    "K-Zone International\0"
    "ACT Labs Ltd\0"
    "LaCie\0"
    "Microline\0"
    "Laguna Systems\0"
    "Sodeman Lancom Inc\0"
    "LASAT Comm. A/S\0"
    "Lava Computer MFG Inc\0"
    "LABAU Technology Corp.\0"
    "Lubosoft\0"
    "LCI\0"
    "Toshiba Matsushita Display Technology Co., Ltd\0"
    "La Commande Electronique\0"
    "Lite-On Communication Inc\0"
    "Latitude Comm.\0"
    "LEXICON\0"
    "Silent Power Electronics GmbH\0"
    "Longshine Electronics Company\0"
    "Labcal Technologies\0"
    "Laserdyne Technologies\0"
    "LogiDataTech Electronic GmbH\0"
    "Lectron Company Ltd\0"
    "Long Engineering Design Inc\0"
    "Legerity, Inc\0"
    "Lenovo Group Limited\0"
    "First International Computer Inc\0"
    "Lexical Ltd\0"
    "Logic Ltd\0"
    "LG Display\0"
    "Logitech Inc\0"
    "LG Semicom Company Ltd\0"
    "Lasergraphics, Inc.\0"
    "Lars Haagh ApS\0"
    "Beihai Century Joint Innovation Technology Co.,Ltd\0"
    "Lung Hwa Electronics Company Ltd\0"
    "Lighthouse Technologies Limited\0"
    "Lenovo Beijing Co. Ltd.\0"
    "Linked IP GmbH\0"
    "Life is Style Inc.\0"
    "Lithics Silicon Technology\0"
    "Datalogic Corporation\0"
    "Likom Technology Sdn. Bhd.\0"
    "L-3 Communications\0"
    "LUMINO Licht Elektronik GmbH\0"
    "Lucent Technologies\0"
    "Lexmark Int'l Inc\0"
    "Leda Media Products\0"
    "Lumens Digital Optics Inc.\0"
    "Laser Master\0"
    "Lincoln Technology Solutions\0"
    "Land Computer Company Ltd\0"
    "Link Tech Inc\0"
    "Linear Systems Ltd.\0"
    "LANETCO International\0"
    "Lenovo\0"
    "The Linux Foundation\0"
    "Locamation B.V.\0"
    "Loewe Opta GmbH\0"
    "Logicode Technology Inc\0"
    "Litelogic Operations Ltd\0"
    "El-PUSK Co., Ltd.\0"
    "Design Technology\0"
    "LG Philips\0"
    "LifeSize Communications\0"
    "Intersil Corporation\0"
    "Loughborough Sound Images\0"
    "LSI Japan Company Ltd\0"
    "Logical Solutions\0"
    "Lightspace Technologies\0"
    "LSI Systems Inc\0"
    "Labtec Inc\0"
    "Jongshine Tech Inc\0"
    "Lucidity Technology Company Ltd\0"
    "Litronic Inc\0"
    "LTS Scale LLC\0"
    "Leitch Technology International Inc.\0"
    "Lightware, Inc\0"
    "Lucent Technologies\0"
    "Lumagen, Inc.\0"
    "Luxxell Research Inc\0"
    "LVI Low Vision International AB\0"
    "Labway Corporation\0"
    "Lightware Visual Engineering\0"
    "Lanier Worldwide\0"
    "LXCO Technologies AG\0"
    "Luxeon\0"
    "ELEA CardWare\0"
    "Lightwell Company Ltd\0"
    "MAC System Company Ltd\0"
    "Xedia Corporation\0"
    "Maestro Pty Ltd\0"
    "MAG InnoVision\0"
    "Mutoh America Inc\0"
    "Meridian Audio Ltd\0"
    "LGIC\0"
    "Mass Inc.\0"
    "Panasonic Connect Co.,Ltd.\0"
    "Rogen Tech Distribution Inc\0"
    "Maynard Electronics\0"
    "MAZeT GmbH\0"
    "MBC\0"
    "Microbus PLC\0"
    "Marshall Electronics\0"
    "Moreton Bay\0"
    "American Nuclear Systems Inc\0"
    "Micro Industries\0"
    "McDATA Corporation\0"
    "Metz-Werke GmbH & Co KG\0"
    "Motorola Computer Group\0"
    "Micronics Computers\0"
    "Medicaroid Corporation\0"
    "Motorola Communications Israel\0"
    "Metricom Inc\0"
    "Micron Electronics Inc\0"
    "Motion Computing Inc.\0"
    "Magni Systems Inc\0"
    "Mat's Computers\0"
    "Marina Communicaitons\0"
    "Micro Computer Systems\0"
    "Microtec\0"
    "Millson Custom Solutions Inc.\0"
    "Media4 Inc\0"
    "Midori Electronics\0"
    "MODIS\0"
    "MILDEF AB\0"
    "Madge Networks\0"
    "Micro Design Inc\0"
    "Mediatek Corporation\0"
    "Panasonic\0"
    "Medar Inc\0"
    "Micro Display Systems Inc\0"
    "Magus Data Tech\0"
    "MET Development Inc\0"
    "MicroDatec GmbH\0"
    "Microdyne Inc\0"
    "Mega System Technologies Inc\0"
    "Messeltronik Dresden GmbH\0"
    "Mitsubishi Electric Engineering Co., Ltd.\0"
    "Abeam Tech Ltd.\0"
    "Panasonic Industry Company\0"
    "Mac-Eight Co., LTD.\0"
    "Mediaedge Corporation\0"
    "Mitsubishi Electric Corporation\0"
    "MEN Mikroelectronik Nueruberg GmbH\0"
    "Meld Technology\0"
    "Matelect Ltd.\0"
    "Metheus Corporation\0"
    "MPL AG, Elektronik-Unternehmen\0"
    "MSC Vertriebs GmbH\0"
    "MicroField Graphics Inc\0"
    "Micro Firmware\0"
    "MediaFire Corp.\0"
    "Mega System Technologies, Inc.\0"
    "Mentor Graphics Corporation\0"
    "Schneider Electric S.A.\0"
    "M-G Technology Ltd\0"
    "Megatech R & D Company\0"
    "Moxa Inc.\0"
    "Micom Communications Inc\0"
    "miro Displays\0"
    "Mitec Inc\0"
    "Marconi Instruments Ltd\0"
    "Mimio – A Newell Rubbermaid Company\0"
    "Minicom Digital Signage\0"
    "micronpc.com\0"
    "Miro Computer Prod.\0"
    "Modular Industrial Solutions Inc\0"
    "MCM Industrial Technology GmbH\0"
    "MicroImage Video Systems\0"
    "MARANTZ JAPAN, INC.\0"
    "MJS Designs\0"
    "Media Tek Inc.\0"
    "MK Seiko Co., Ltd.\0"
    "MICROTEK Inc.\0"
    "Trtheim Technology\0"
    "MILCOTS\0"
    "Deep Video Imaging Ltd\0"
    "Micrologica AG\0"
    "McIntosh Laboratory Inc.\0"
    "Millogic Ltd.\0"
    "Millennium Engineering Inc\0"
    "Mark Levinson\0"
    "Magic Leap\0"
    "Milestone EPE\0"
    "Wanlida Group Co., Ltd.\0"
    "Mylex Corporation\0"
    "Micromedia AG\0"
    "Micromed Biotecnologia Ltd\0"
    "Minnesota Mining and Manufacturing\0"
    "Multimax\0"
    "Electronic Measurements\0"
    "MiniMan Inc\0"
    "MMS Electronics\0"
    "MIMO Monitors\0"
    "Mini Micro Methods Ltd\0"
    "Marseille, Inc.\0"
    "Monorail Inc\0"
    "Microcom\0"
    "Maxnerva Technology Services Limited\0"
    "Matrix Orbital Corporation\0"
    "Modular Technology\0"
    "Moka International limited\0"
    "Momentum Data Systems\0"
    "Moses Corporation\0"
    "Motorola UDS\0"
    "M-Pact Inc\0"
    "Mediatrix Peripherals Inc\0"
    "Microlab\0"
    "Maple Research Inst. Company Ltd\0"
    "Mainpine Limited\0"
    "mps Software GmbH\0"
    "Megapixel Visual Realty\0"
    "Micropix Technologies, Ltd.\0"
    "MultiQ Products AB\0"
    "Miranda Technologies Inc\0"
    "Marconi Simulation & Ty-Coch Way Training\0"
    "MicroDisplay Corporation\0"
    "Nreal\0"
    "Maruko & Company Ltd\0"
    "Miratel\0"
    "Medikro Oy\0"
    "Merging Technologies\0"
    "Micro Systemation AB\0"
    "Mouse Systems Corporation\0"
    "Datenerfassungs- und Informationssysteme\0"
    "M-Systems Flash Disk Pioneers\0"
    "MSI GmbH\0"
    "Microsoft\0"
    "Microstep\0"
    "Megasoft Inc\0"
    "MicroSlate Inc.\0"
    "Advanced Digital Systems\0"
    "Mistral Solutions [P] Ltd.\0"
    "MASPRO DENKOH Corp.\0"
    "MS Telematica\0"
    "motorola\0"
    "Mosgi Corporation\0"
    "Micomsoft Co., Ltd.\0"
    "MicroTouch Systems Inc\0"
    "Meta Watch Ltd\0"
    "Media Technologies Ltd.\0"
    "Mars-Tech Corporation\0"
    "MindTech Display Co. Ltd\0"
    "MediaTec GmbH\0"
    "Micro-Tech Hearing Instruments\0"
    "MaxCom Technical Inc\0"
    "MicroTechnica Co.,Ltd.\0"
    "Microtek International Inc.\0"
    "Mitel Corporation\0"
    "Motium\0"
    "Mtron Storage Technology Co., Ltd.\0"
    "Mitron computer Inc\0"
    "Multi-Tech Systems\0"
    "Moore Threads Virtual Display\0"
    "Mark of the Unicorn Inc\0"
    "Matrox\0"
    "Multi-Dimension Institute\0"
    "Mainpine Limited\0"
    "Microvitec PLC\0"
    "Media Vision Inc\0"
    "SOBO VISION\0"
    "Meta Company\0"
    "MediCapture, Inc.\0"
    "Microvision\0"
    "COM 1\0"
    "Multiwave Innovation Pte Ltd\0"
    "mware\0"
    "Microway Inc\0"
    "MaxData Computer GmbH & Co.KG\0"
    "Macronix Inc\0"
    "Hitachi Maxell, Ltd.\0"
    "C&T Solution Inc.\0"
    "Maxpeed Corporation\0"
    "Maxtech Corporation\0"
    "MaxVision Corporation\0"
    "Monydata\0"
    "Myriad Solutions Ltd\0"
    "Micronyx Inc\0"
    "Ncast Corporation\0"
    "NAD Electronics\0"
    "NAFASAE INDIA Pvt. Ltd\0"
    "Nakano Engineering Co.,Ltd.\0"
    "Network Alchemy\0"
    "NaturalPoint Inc.\0"
    "Navigation Corporation\0"
    "Naxos Tecnologia\0"
    "N*Able Technologies Inc\0"
    "National Key Lab. on ISN\0"
    "NingBo Bestwinning Technology CO., Ltd\0"
    "Nixdorf Company\0"
    "NCR Corporation\0"
    "Norcent Technology, Inc.\0"
    "NewCom Inc\0"
    "NetComm Ltd\0"
    "Najing CEC Panda FPD Technology CO. ltd\0"
    "NCR Electronics\0"
    "Northgate Computer Systems\0"
    "NEC CustomTechnica, Ltd.\0"
    "NewCoSemi (Beijing) Technology CO.,Ltd.\0"
    "National DataComm Corporaiton\0"
    "NDF Special Light Products B.V.\0"
    "National Display Systems\0"
    "Naitoh Densei CO., LTD.\0"
    "Network Designers\0"
    "Nokia Data\0"
    "NEC Corporation\0"
    "NEO TELECOM CO.,LTD.\0"
    "INNES\0"
    "Mettler Toledo\0"
    "NEUROTEC - EMPRESA DE PESQUISA E DESENVOLVIMENTO EM BIOMEDICINA\0"
    "Nexgen Mediatech Inc.,\0"
    "BTC Korea Co., Ltd\0"
    "Number Five Software\0"
    "Network General\0"
    "A D S Exports\0"
    "New H3C Technology Co., Ltd.\0"
    "Vinci Labs\0"
    "National Instruments Corporation\0"
    "Nissei Electric Company\0"
    "Network Info Technology\0"
    "Seanix Technology Inc\0"
    "Next Level Communications\0"
    "Navico, Inc.\0"
    "Nokia Mobile Phones\0"
    "Natural Micro System\0"
    "NEC-Mitsubishi Electric Visual Systems Corporation\0"
    "Neomagic\0"
    "NNC\0"
    "3NOD Digital Technology Co. Ltd.\0"
    "NordicEye AB\0"
    "North Invent A/S\0"
    "Nokia Display Products\0"
    "Norand Corporation\0"
    "Not Limited Inc\0"
    "Arvanics\0"
    "Network Peripherals Inc\0"
    "Noritake Itron Corporation\0"
    "U.S. Naval Research Lab\0"
    "Beijing Northern Radiantelecom Co.\0"
    "Taugagreining hf\0"
    "NeuroSky, Inc.\0"
    "National Semiconductor Corporation\0"
    "NISSEI ELECTRIC CO.,LTD\0"
    "Nspire System Inc.\0"
    "Newport Systems Solutions\0"
    "Network Security Technology Co\0"
    "NeoTech S.R.L\0"
    "New Tech Int'l Company\0"
    "NewTek\0"
    "National Transcomm. Ltd\0"
    "Nuvoton Technology Corporation\0"
    "N-trig Innovative Technologies, Inc.\0"
    "Nits Technology Inc.\0"
    "NTT Advanced Technology Corporation\0"
    "Networth Inc\0"
    "Netaccess Inc\0"
    "NU Technology, Inc.\0"
    "NU Inc.\0"
    "NetVision Corporation\0"
    "Nvidia\0"
    "NuVision US, Inc.\0"
    "Novell Inc\0"
    "Netvio Ltd.\0"
    "NOLO CO., LTD.\0"
    "Navatek Engineering Corporation\0"
    "NW Computer Engineering\0"
    "Newline Interactive Inc.\0"
    "NovaWeb Technologies Inc\0"
    "Newisys, Inc.\0"
    "NextCom K.K.\0"
    "Norxe AS\0"
    "Nexgen\0"
    "NXP Semiconductors bv.\0"
    "Nexiq Technologies, Inc.\0"
    "Nextorage Corporation\0"
    "Technology Nexus Secure Open Systems AB\0"
    "NZXT (PNP same EDID)_\0"
    "Nakayo Relecommunications, Inc.\0"
    "Oak Tech Inc\0"
    "Oasys Technology Company\0"
    "Optibase Technologies\0"
    "Macraigor Systems Inc\0"
    "Olfan\0"
    "Open Connect Solutions\0"
    "ODME Inc.\0"
    "Odrac\0"
    "ORION ELECTRIC CO.,LTD\0"
    "Optum Engineering Inc.\0"
    "Jiangxi Jinghao Optical Co., Ltd.\0"
    "M-Labs Limited\0"
    "Option Industrial Computers\0"
    "Option International\0"
    "Option International\0"
    "OKI Electric Industrial Company Ltd\0"
    "Olicom A/S\0"
    "Olidata S.p.A.\0"
    "Olivetti\0"
    "Olitec S.A.\0"
    "Olitec S.A.\0"
    "OLYMPUS CORPORATION\0"
    "OBJIX Multimedia Corporation\0"
    "RODE\0"
    "Omnitel\0"
    "Omron Corporation\0"
    "Oneac Corporation\0"
    "ONKYO Corporation\0"
    "OnLive, Inc\0"
    "On Systems Inc\0"
    "OPEN Networks Ltd\0"
    "SOMELEC Z.I. Du Vert Galanta\0"
    "OSRAM\0"
    "Opcode Inc\0"
    "D.N.S. Corporation\0"
    "OPPO Digital, Inc.\0"
    "OPTi Inc\0"
    "Optivision Inc\0"
    "Oksori Company Ltd\0"
    "ORGA Kartensysteme GmbH\0"
    "OSR Open Systems Resources, Inc.\0"
    "ORION ELECTRIC CO., LTD.\0"
    "OSAKA Micro Computer, Inc.\0"
    "Optical Systems Design Pty Ltd\0"
    "Open Stack, Inc.\0"
    "OPTI-UPS Corporation\0"
    "Oksori Company Ltd\0"
    "outsidetheboxstuff.com\0"
    "Orchid Technology\0"
    "OmniTek\0"
    "Optoma Corporation\0"
    "OPTO22, Inc.\0"
    "OUK Company Ltd\0"
    "Oculus VR, Inc.\0"
    "Mediacom Technologies Pte Ltd\0"
    "Oxus Research S.A.\0"
    "Shadow Systems\0"
    "OZ Corporation\0"
    "OZO Co.Ltd\0"
    "Tribe Computer Works Inc\0"
    "Pacific Avionics Corporation\0"
    "Promotion and Display Technology Ltd.\0"
    "PreSonus Audio Electronics\0"
    "Many CNC System Co., Ltd.\0"
    "Peter Antesberger Messtechnik\0"
    "The Panda Project\0"
    "Parallan Comp Inc\0"
    "Pitney Bowes\0"
    "Packard Bell Electronics\0"
    "Packard Bell NEC\0"
    "Pitney Bowes\0"
    "Philips BU Add On Card\0"
    "OCTAL S.A.\0"
    "PowerCom Technology Company Ltd\0"
    "First Industrial Computer Inc\0"
    "Pioneer Computer Inc\0"
    "PCBANK21\0"
    "pentel.co.,ltd\0"
    "PCM Systems Corporation\0"
    "Performance Concepts Inc.,\0"
    "Procomp USA Inc\0"
    "TOSHIBA PERSONAL COMPUTER SYSTEM CORPRATION\0"
    "PC-Tel Inc\0"
    "Pacific CommWare Inc\0"
    "PC Xperten\0"
    "Psion Dacom Plc.\0"
    "AT&T Paradyne\0"
    "Pure Data Inc\0"
    "PD Systems International Ltd\0"
    "PDTS - Prozessdatentechnik und Systeme\0"
    "Prodrive B.V.\0"
    "POTRANS Electrical Corp.\0"
    "Pegatron Corporation\0"
    "PEI Electronics Inc\0"
    "Primax Electric Ltd\0"
    "Interactive Computer Products Inc\0"
    "Peppercon AG\0"
    "Perceptive Signal Technologies\0"
    "Practical Electronic Tools\0"
    "Telia ProSoft AB\0"
    "PACSGEAR, Inc.\0"
    "Paradigm Advanced Research Centre\0"
    "propagamma kommunikation\0"
    "Princeton Graphic Systems\0"
    "Pijnenburg Beheer N.V.\0"
    "Philips Medical Systems Boeblingen GmbH\0"
    "Invalid Vendor Codename - PHI\0"
    "Philips Consumer Electronics Company\0"
    "Photonics Systems Inc.\0"
    "Philips Communication Systems\0"
    "Phylon Communications\0"
    "Picturall Ltd.\0"
    "Pacific Image Electronics Company Ltd\0"
    "Prism, LLC\0"
    "Pioneer Electronic Corporation\0"
    "Pico Technology Inc.\0"
    "TECNART CO.,LTD.\0"
    "Pixie Tech Inc\0"
    "Projecta\0"
    "Projectiondesign AS\0"
    "Pan Jit International Inc.\0"
    "Acco UK Ltd.\0"
    "Pro-Log Corporation\0"
    "Panasonic Avionics Corporation\0"
    "PROLINK Microsystems Corp.\0"
    "PT Hartono Istana Teknologi\0"
    "PLUS Vision Corp.\0"
    "Parallax Graphics\0"
    "Polycom Inc.\0"
    "PMC Consumer Electronics Ltd\0"
    "TDK USA Corporation\0"
    "Point Multimedia System\0"
    "Pabian Embedded Systems\0"
    "Promate Electronic Co., Ltd.\0"
    "Photomatrix\0"
    "Microsoft\0"
    "Panelview, Inc.\0"
    "Microsoft\0"
    "Planar Systems, Inc.\0"
    "PanaScope\0"
    "HOYA Corporation PENTAX Lifecare Division\0"
    "Phoenix Technologies, Ltd.\0"
    "PolyComp (PTY) Ltd.\0"
    "Perpetual Technologies, LLC\0"
    "Portalis LC\0"
    "Positivo Tecnologia S.A.\0"
    "Parrot\0"
    "Phoenixtec Power Company Ltd\0"
    "MEPhI\0"
    "Practical Peripherals\0"
    "Clinton Electronics Corp.\0"
    "Purup Prepress AS\0"
    "PicPro\0"
    "Perceptive Pixel Inc.\0"
    "Pixel Qi\0"
    "PRO/AUTOMATION\0"
    "PerComm\0"
    "Praim S.R.L.\0"
    "Schneider Electric Japan Holdings, Ltd.\0"
    "The Phoenix Research Group Inc\0"
    "Priva Hortimation BV\0"
    "Prometheus\0"
    "Proteon\0"
    "UEFI Forum\0"
    "Leutron Vision\0"
    "Parade Technologies, Ltd.\0"
    "Proxima Corporation\0"
    "Advanced Signal Processing Technologies\0"
    "Philips Semiconductors\0"
    "Peus-Systems GmbH\0"
    "Practical Solutions Pte., Ltd.\0"
    "PSI-Perceptive Solutions Inc\0"
    "Perle Systems Limited\0"
    "Prosum\0"
    "Global Data SA\0"
    "Prodea Systems Inc.\0"
    "PAR Tech Inc.\0"
    "PS Technology Corporation\0"
    "Cipher Systems Inc\0"
    "Pathlight Technology Inc\0"
    "Promise Technology Inc\0"
    "Pantel Inc\0"
    "Plain Tree Systems Inc\0"
    "Invalid Vendor Codename - PTW\0"
    "Printronix LLC\0"
    "Pulse-Eight Ltd\0"
    "Invalid Vendor Codename - PVC\0"
    "Proview Global Co., Ltd\0"
    "Prime view international Co., Ltd\0"
    "Penta Studiotechnik GmbH\0"
    "Pixel Vision\0"
    "Klos Technologies, Inc.\0"
    "Pimax Tech. CO., LTD\0"
    "Phoenix Contact\0"
    "PIXELA CORPORATION\0"
    "The Moving Pixel Company\0"
    "Proxim Inc\0"
    "PixelNext Inc\0"
    "Pixio USA\0"
    "QuakeCom Company Ltd\0"
    "Metronics Inc\0"
    "Quanta Computer Inc\0"
    "Quick Corporation\0"
    "Quadrant Components Inc\0"
    "Qualcomm Inc\0"
    "Quantum Data Incorporated\0"
    "QD Laser, Inc.\0"
    "Quadram\0"
    "Quanta Display Inc.\0"
    "Padix Co., Inc.\0"
    "Quickflex, Inc\0"
    "Q-Logic\0"
    "Chuomusen Co., Ltd.\0"
    "QSC, LLC\0"
    "Quantum Solutions, Inc.\0"
    "Quantum 3D Inc\0"
    "Questech Ltd\0"
    "Quicknet Technologies Inc\0"
    "Quantum\0"
    "Qtronix Corporation\0"
    "Quatographic AG\0"
    "Questra Consulting\0"
    "Quartics\0"
    "Racore Computer Products Inc\0"
    "Radisys Corporation\0"
    "Rockwell Automation/Intecolor\0"
    "Rancho Tech Inc\0"
    "Raritan, Inc.\0"
    "RAScom Inc\0"
    "Rent-A-Tech\0"
    "Raylar Design, Inc.\0"
    "Parc d'Activite des Bellevues\0"
    "Reach Technology Inc\0"
    "RC International\0"
    "Radio Consult SRL\0"
    "Rockwell Collins\0"
    "Rainbow Displays, Inc.\0"
    "Riedel Communications Canada Inc.\0"
    "Tremon Enterprises Company Ltd\0"
    "RADIODATA GmbH\0"
    "Radius Inc\0"
    "Real D\0"
    "ReCom\0"
    "Research Electronics Development Inc\0"
    "Reflectivity, Inc.\0"
    "Rehan Electronics Ltd.\0"
    "Reliance Electric Ind Corporation\0"
    "SCI Systems Inc.\0"
    "Renesas Technology Corp.\0"
    "ResMed Pty Ltd\0"
    "Resonance Technology, Inc.\0"
    "Revolution Display, Inc.\0"
    "RATOC Systems, Inc.\0"
    "RAFI GmbH & Co. KG\0"
    "Redfox Technologies Inc.\0"
    "RGB Spectrum\0"
    "Robertson Geologging Ltd\0"
    "RightHand Technologies\0"
    "Rohm Company Ltd\0"
    "Red Hat, Inc.\0"
    "RICOH COMPANY, LTD.\0"
    "Racal Interlan Inc\0"
    "Rios Systems Company Ltd\0"
    "Ritech Inc\0"
    "Rivulet Communications\0"
    "Roland Corporation\0"
    "Advanced Engineering\0"
    "Reakin Technolohy Corporation\0"
    "MEPCO\0"
    "RadioLAN Inc\0"
    "Raritan Computer, Inc\0"
    "Research Machines\0"
    "Shenzhen Ramos Digital Technology Co., Ltd\0"
    "Roper Mobile\0"
    "Rainbow Technologies\0"
    "Reonel Oy\0"
    "Robust Electronics GmbH\0"
    "Rohm Co., Ltd.\0"
    "Rockwell International\0"
    "Roper International Ltd\0"
    "Rohde & Schwarz\0"
    "RoomPro Technologies\0"
    "Raspberry PI\0"
    "R.P.T.Intergroups\0"
    "Radicom Research Inc\0"
    "AVARRO\0"
    "PhotoTelesis\0"
    "ADC-Centre\0"
    "Rampage Systems Inc\0"
    "Radiospire Networks, Inc.\0"
    "R Squared\0"
    "Zhong Shan City Richsound Electronic Industrial Ltd.\0"
    "Rockwell Semiconductor Systems\0"
    "Ross Video Ltd\0"
    "Rapid Tech Corporation\0"
    "Relia Technologies\0"
    "Rancho Tech Inc\0"
    "Invalid Vendor Codename - RTK\0"
    "Realtek Semiconductor Company Ltd\0"
    "Raintree Systems\0"
    "RUNCO International\0"
    "Ups Manufactoring s.r.l.\0"
    "RSI Systems Inc\0"
    "Realvision Inc\0"
    "Reveal Computer Prod\0"
    "Red Wing Corporation\0"
    "Tectona SoftSolutions (P) Ltd.,\0"
    "Razer Taiwan Co. Ltd.\0"
    "Rozsnyó, s.r.o.\0"
    "Sanritz Automation Co.,Ltd.\0"
    "Saab Aerotech\0"
    "Sedlbauer\0"
    "Sage Inc\0"
    "Saitek Ltd\0"
    "Samsung Electric Company\0"
    "Sanyo Electric Co.,Ltd.\0"
    "Stores Automated Systems Inc\0"
    "Shuttle Tech\0"
    "Shanghai Bell Telephone Equip Mfg Co\0"
    "Softbed - Consulting & Development Ltd\0"
    "SMART Technologies Inc.\0"
    "SBS-or Industrial Computers GmbH\0"
    "Senseboard Technologies AB\0"
    "Schneider Consumer Group\0"
    "SeeCubic B.V.\0"
    "SORD Computer Corporation\0"
    "Sanyo Electric Company Ltd\0"
    "Sun Corporation\0"
    "Seco S.p.A.\0"
    "Schlumberger Cards\0"
    "System Craft\0"
    "Sigmacom Co., Ltd.\0"
    "SCM Microsystems Inc\0"
    "Scanport, Inc.\0"
    "SORCUS Computer GmbH\0"
    "Scriptel Corporation\0"
    "Systran Corporation\0"
    "Nanomach Anstalt\0"
    "Smart Card Technology\0"
    "Socionext Inc.\0"
    "SAT (Societe Anonyme)\0"
    "Samsung Display Corp.\0"
    "Intrada-SDD Ltd\0"
    "Sherwood Digital Electronics Corporation\0"
    "SODIFF E&T CO., Ltd.\0"
    "Communications Specialies, Inc.\0"
    "Samtron Displays Inc\0"
    "SAIT-Devlonics\0"
    "SDR Systems\0"
    "SunRiver Data System\0"
    "Siemens AG\0"
    "SDX Business Systems Ltd\0"
    "Seanix Technology Inc.\0"
    "system elektronik GmbH\0"
    "Seiko Epson Corporation\0"
    "SeeColor Corporation\0"
    "Invalid Vendor Codename - SEG\0"
    "Seitz & Associates Inc\0"
    "Way2Call Communications\0"
    "Samsung Electronics Company Ltd\0"
    "Sencore\0"
    "SEOS Ltd\0"
    "SEP Eletronica Ltda.\0"
    "Sony Ericsson Mobile Communications Inc.\0"
    "Session Control LLC\0"
    "SendTek Corporation\0"
    "Shiftall Inc.\0"
    "TORNADO Company\0"
    "Mikroforum Ring 3\0"
    "Spectragraphics Corporation\0"
    "Sigma Designs, Inc.\0"
    "Kansai Electric Company Ltd\0"
    "Scan Group Ltd\0"
    "Super Gate Technology Company Ltd\0"
    "SAGEM\0"
    "Shenzhen Soogeen Electronics Co., LTD.\0"
    "Logos Design A/S\0"
    "Stargate Technology\0"
    "Shanghai Guowei Science and Technology Co., Ltd.\0"
    "Silicon Graphics Inc\0"
    "Systec Computer GmbH\0"
    "ShibaSoku Co., Ltd.\0"
    "Soft & Hardware development Goldammer GmbH\0"
    "Jiangsu Shinco Electronic Group Co., Ltd\0"
    "Sharp Corporation\0"
    "Digital Discovery\0"
    "Shin Ho Tech\0"
    "Shure Inc.\0"
    "SIEMENS AG\0"
    "Sanyo Electric Company Ltd\0"
    "Sysmate Corporation\0"
    "Seiko Instruments Information Devices Inc\0"
    "Siemens\0"
    "Sigma Designs Inc\0"
    "Silicon Image, Inc.\0"
    "Silicon Laboratories, Inc\0"
    "S3 Inc\0"
    "Singular Technology Co., Ltd.\0"
    "Sirius Technologies Pty Ltd\0"
    "Silicon Integrated Systems Corporation\0"
    "Sitintel\0"
    "Seiko Instruments USA Inc\0"
    "Zuniq Data Corporation\0"
    "Sejin Electron Inc\0"
    "Schneider & Koch\0"
    "Shenzhen KTC Technology Group\0"
    "LLC SKTB “SKIT”\0"
    "Guangzhou Teclast Information Technology Limited\0"
    "Samsung Electro-Mechanics Company Ltd\0"
    "Skyworth\0"
    "SKYDATA S.P.A.\0"
    "Systeme Lauer GmbH&Co KG\0"
    "Shlumberger Ltd\0"
    "Syslogic Datentechnik AG\0"
    "StarLeaf\0"
    "Silicon Library Inc.\0"
    "Symbios Logic Inc\0"
    "Silitek Corporation\0"
    "Solomon Technology Corporation\0"
    "Schlumberger Technology Corporate\0"
    "Schnick-Schnack-Systems GmbH\0"
    "Salt Internatioinal Corp.\0"
    "Specialix\0"
    "SMART Modular Technologies\0"
    "Schlumberger\0"
    "Standard Microsystems Corporation\0"
    "Sysmate Company\0"
    "SpaceLabs Medical Inc\0"
    "SMK CORPORATION\0"
    "Sumitomo Metal Industries, Ltd.\0"
    "Shark Multimedia Inc\0"
    "Somnium Space Ltd.\0"
    "STMicroelectronics\0"
    "Simple Computing\0"
    "B.& V. s.r.l.\0"
    "Silicom Multimedia Systems Inc\0"
    "Silcom Manufacturing Tech Inc\0"
    "Sentronic International Corp.\0"
    "Siemens Microdesign GmbH\0"
    "S&K Electronics\0"
    "SUNNY ELEKTRONIK\0"
    "SINOSUN TECHNOLOGY CO., LTD\0"
    "Siemens Nixdorf Info Systems\0"
    "Cirtech (UK) Ltd\0"
    "SuperNet Inc\0"
    "SONOVE GmbH\0"
    "Snell & Wilcox\0"
    "Sonix Comm. Ltd\0"
    "Sony\0"
    "Santec Corporation\0"
    "Silicon Optix Corporation\0"
    "Solitron Technologies Inc\0"
    "Sony\0"
    "Sorcus Computer GmbH\0"
    "Sotec Company Ltd\0"
    "SOYO Group, Inc\0"
    "SpinCore Technologies, Inc\0"
    "SPEA Software AG\0"
    "G&W Instruments GmbH\0"
    "SPACE-I Co., Ltd.\0"
    "SpeakerCraft\0"
    "Smart Silicon Systems Pty Ltd\0"
    "Sapience Corporation\0"
    "SAMPO CORPORATION\0"
    "pmns GmbH\0"
    "Synopsys Inc\0"
    "Sceptre Tech Inc\0"
    "SIM2 Multimedia S.P.A.\0"
    "Simplex Time Recorder Co.\0"
    "Sequent Computer Systems Inc\0"
    "Integrated Tech Express Inc\0"
    "Setred\0"
    "Surf Communication Solutions Ltd\0"
    "Intuitive Surgical, Inc.\0"
    "SR-Systems e.K.\0"
    "SeeReal Technologies GmbH\0"
    "Sierra Semiconductor Inc\0"
    "FlightSafety International\0"
    "Samsung Electronic Co.\0"
    "Steelseries ApS\0"
    "S-S Technology Inc\0"
    "Sankyo Seiki Mfg.co., Ltd\0"
    "Shenzhen South-Top Computer Co., Ltd.\0"
    "Spectrum Signal Proecessing Inc\0"
    "S3 Inc\0"
    "SystemSoft Corporation\0"
    "ST Electronics Systems Assembly Pte Ltd\0"
    "STB Systems Inc\0"
    "STAC Electronics\0"
    "STD Computer Inc\0"
    "SII Ido-Tsushin Inc\0"
    "Starflight Electronics\0"
    "StereoGraphics Corp.\0"
    "Semtech Corporation\0"
    "Smart Tech Inc\0"
    "SANTAK CORP.\0"
    "SigmaTel Inc\0"
    "SGS Thomson Microelectronics\0"
    "Samsung Electronics America\0"
    "Stollmann E+V GmbH\0"
    "StreamPlay Ltd\0"
    "Synthetel Corporation\0"
    "Starlight Networks Inc\0"
    "SITECSYSTEM CO., LTD.\0"
    "Star Paging Telecom Tech (Shenzhen) Co. Ltd.\0"
    "Sentelic Corporation\0"
    "Beijing Guochengwantong Information Technology Co., Ltd.\0"
    "Starwin Inc.\0"
    "ST-Ericsson\0"
    "SDS Technologies\0"
    "Subspace Comm. Inc\0"
    "Summagraphics Corporation\0"
    "Sun Electronics Corporation\0"
    "Supra Corporation\0"
    "Surenam Computer Corporation\0"
    "SGEG\0"
    "Intellix Corp.\0"
    "SVD Computer\0"
    "Sun Microsystems\0"
    "Sensics, Inc.\0"
    "SVSI\0"
    "SEVIT Co., Ltd.\0"
    "Software Café\0"
    "Sierra Wireless Inc.\0"
    "Sharedware Ltd\0"
    "Guangzhou Shirui Electronics Co., Ltd.\0"
    "Static\0"
    "Software Technologies Group,Inc.\0"
    "Syntax-Brillian\0"
    "Silex technology, Inc.\0"
    "SELEX GALILEO\0"
    "Silex Inside\0"
    "SolutionInside\0"
    "SHARP TAKAYA ELECTRONIC INDUSTRY CO.,LTD.\0"
    "Sysmic\0"
    "SY Electronics Ltd\0"
    "Stryker Communications\0"
    "Sylvania Computer Products\0"
    "Symicron Computer Communications Ltd.\0"
    "Synaptics Inc\0"
    "SYPRO Co Ltd\0"
    "Sysgration Ltd\0"
    "Seyeon Tech Company Ltd\0"
    "SYVAX Inc\0"
    "Prime Systems, Inc.\0"
    "Shenzhen MTC Co., Ltd\0"
    "Tandberg\0"
    "Todos Data System AB\0"
    "Teles AG\0"
    "Toshiba America Info Systems Inc\0"
    "Tamura Seisakusyo Ltd\0"
    "Taskit Rechnertechnik GmbH\0"
    "Teleliaison Inc\0"
    "Thales Avionics\0"
    "Taxan (Europe) Ltd\0"
    "Triple S Engineering Inc\0"
    "Turbo Communication, Inc\0"
    "Turtle Beach System\0"
    "Tandon Corporation\0"
    "Taicom Data Systems Co., Ltd.\0"
    "Century Corporation\0"
    "Televic Conference\0"
    "Interaction Systems, Inc\0"
    "Tulip Computers Int'l B.V.\0"
    "TEAC America Inc\0"
    "Technical Concepts Ltd\0"
    "3Com Corporation\0"
    "Tecnetics (PTY) Ltd\0"
    "Thomas-Conrad Corporation\0"
    "Thomson Consumer Electronics\0"
    "Tatung Company of America Inc\0"
    "Telecom Technology Centre Co. Ltd.\0"
    "FREEMARS Heavy Industries\0"
    "Teradici\0"
    "Tandberg Data Display AS\0"
    "Six15 Technologies\0"
    "Tandem Computer Europe Inc\0"
    "3D Perception\0"
    "Tri-Data Systems Inc\0"
    "TDT\0"
    "TDVision Systems, Inc.\0"
    "Tandy Electronics\0"
    "TEAC System Corporation\0"
    "Tecmar Inc\0"
    "Tektronix Inc\0"
    "Promotion and Display Technology Ltd.\0"
    "Tencent\0"
    "TerraTec Electronic GmbH\0"
    "TETRADYNE CO., LTD.\0"
    "Televés, S.A.\0"
    "Tech Source Inc.\0"
    "Toshiba Global Commerce Solutions, Inc.\0"
    "TriGem Computer Inc\0"
    "TriGem Computer,Inc.\0"
    "Torus Systems Ltd\0"
    "Grass Valley Germany GmbH\0"
    "TECHNOGYM S.p.A.\0"
    "Thundercom Holdings Sdn. Bhd.\0"
    "Trigem KinfoComm\0"
    "Technical Illusions Inc.\0"
    "TIPTEL AG\0"
    "OOO Technoinvest\0"
    "Tixi.Com GmbH\0"
    "Taiko Electric Works.LTD\0"
    "Tek Gear\0"
    "Teknor Microsystem Inc\0"
    "TouchKo, Inc.\0"
    "TimeKeeping Systems, Inc.\0"
    "Ferrari Electronic GmbH\0"
    "Telindus\0"
    "Zhejiang Tianle Digital Electric Co., Ltd.\0"
    "Teleforce.,co,ltd\0"
    "TOSHIBA TELI CORPORATION\0"
    "Telelink AG\0"
    "Thinklogical\0"
    "Techlogix Networx\0"
    "Teleste Educational OY\0"
    "Dai Telecom S.p.A.\0"
    "S3 Inc\0"
    "Telxon Corporation\0"
    "Truly Semiconductors Ltd.\0"
    "Tianma Microelectronics Ltd.\0"
    "Techmedia Computer Systems Corporation\0"
    "AT&T Microelectronics\0"
    "Texas Microsystem\0"
    "Time Management, Inc.\0"
    "Terumo Corporation\0"
    "Taicom International Inc\0"
    "Trident Microsystems Ltd\0"
    "T-Metrics Inc.\0"
    "TeamViewer Germany GmbH\0"
    "Thermotrex Corporation\0"
    "TNC Industrial Company Ltd\0"
    "Invalid Vendor Codename - TNJ\0"
    "TECNIMAGEN SA\0"
    "Tennyson Tech Pty Ltd\0"
    "TOEI Electronics Co., Ltd.\0"
    "The OPEN Group\0"
    "TCL Corporation\0"
    "Ceton Corporation\0"
    "TONNA\0"
    "Orion Communications Co., Ltd.\0"
    "Dynabook Inc.\0"
    "Touchstone Technology\0"
    "Touch Panel Systems Corporation\0"
    "Times (Shanghai) Computer Co., Ltd.\0"
    "Technology Power Enterprises Inc\0"
    "Junnila\0"
    "TOPRE CORPORATION\0"
    "Topro Technology Inc\0"
    "Teleprocessing Systeme GmbH\0"
    "Thruput Ltd\0"
    "Top Victory Electronics ( Fujian ) Company Ltd\0"
    "Ypoaz Systems Inc\0"
    "TriTech Microelectronics International\0"
    "Triumph Board a.s.\0"
    "Trioc AB\0"
    "Trident Microsystem Inc\0"
    "Tremetrics\0"
    "Tricord Systems\0"
    "Royal Information\0"
    "Tekram Technology Company Ltd\0"
    "Datacommunicatie Tron B.V.\0"
    "TRAPEZE GROUP\0"
    "Torus Systems Ltd\0"
    "Tritec Electronic AG\0"
    "Aashima Technology B.V.\0"
    "Trivisio Prototyping GmbH\0"
    "Trex Enterprises\0"
    "Toshiba America Info Systems Inc\0"
    "Sanyo Electric Company Ltd\0"
    "TechniSat Digital GmbH\0"
    "Tottori Sanyo Electric\0"
    "Racal-Airtech Software Forge Ltd\0"
    "The Software Group Ltd\0"
    "ELAN MICROELECTRONICS CORPORATION\0"
    "TeleVideo Systems\0"
    "Tottori SANYO Electric Co., Ltd.\0"
    "U.S. Navy\0"
    "Transtream Inc\0"
    "TRANSVIDEO\0"
    "VRSHOW Technology Limited\0"
    "TouchSystems\0"
    "Topson Technology Co., Ltd.\0"
    "National Semiconductor Japan Ltd\0"
    "Telecommunications Techniques Corporation\0"
    "TTE, Inc.\0"
    "Trenton Terminals Inc\0"
    "Totoku Electric Company Ltd\0"
    "2-Tel B.V\0"
    "Toshiba Corporation\0"
    "Hubei Century Joint Innovation Technology Co.Ltd\0"
    "TechnoTrend Systemtechnik GmbH\0"
    "Taitex Corporation\0"
    "TRIDELITY Display Solutions GmbH\0"
    "T+A elektroakustik GmbH\0"
    "Tut Systems\0"
    "Tecnovision\0"
    "Truevision\0"
    "Total Vision LTD\0"
    "Taiwan Video & Monitor Corporation\0"
    "TV One Ltd\0"
    "TV Interactive Corporation\0"
    "TVS Electronics Limited\0"
    "TV1 GmbH\0"
    "Tidewater Association\0"
    "Kontron Electronik\0"
    "Twinhead International Corporation\0"
    "Easytel oy\0"
    "TOWITOKO electronics GmbH\0"
    "TEKWorx Limited\0"
    "Trixel Ltd\0"
    "Texas Insturments\0"
    "Textron Defense System\0"
    "Tyan Computer Corporation\0"
    "Ultima Associates Pte Ltd\0"
    "Ungermann-Bass Inc\0"
    "Ubinetics Ltd.\0"
    "Canonical Ltd.\0"
    "Uniden Corporation\0"
    "Ultima Electronics Corporation\0"
    "Elitegroup Computer Systems Company Ltd\0"
    "Universal Electronics Inc\0"
    "Universal Empowering Technologies\0"
    "UNIGRAF-USA\0"
    "UFO Systems Inc\0"
    "XOCECO\0"
    "Uniform Industrial Corporation\0"
    "Ueda Japan Radio Co., Ltd.\0"
    "Ultra Network Tech\0"
    "United Microelectr Corporation\0"
    "Umezawa Giken Co.,Ltd\0"
    "Universal Multimedia\0"
    "UltiMachine\0"
    "Unisys DSD\0"
    "Unisys Corporation\0"
    "Unisys Corporation\0"
    "Unisys Corporation\0"
    "Unisys Corporation\0"
    "Unisys Corporation\0"
    "Uniform Industry Corp.\0"
    "Unisys Corporation\0"
    "Unisys Corporation\0"
    "Unitop\0"
    "Unisys Corporation\0"
    "Unisys Corporation\0"
    "Unicate\0"
    "UPPI\0"
    "Systems Enhancement\0"
    "Video Computer S.p.A.\0"
    "Utimaco Safeware AG\0"
    "U.S. Digital Corporation\0"
    "U. S. Electronics Inc.\0"
    "Universal Scientific Industrial Co., Ltd.\0"
    "U.S. Robotics Inc\0"
    "Unicompute Technology Co., Ltd.\0"
    "Up to Date Tech\0"
    "Uniwill Computer Corp.\0"
    "Vaddio, LLC\0"
    "VAIO Corporation\0"
    "Valence Computing Corporation\0"
    "Varian Australia Pty Ltd\0"
    "VADATECH INC\0"
    "aviica\0"
    "VBrick Systems Inc.\0"
    "Valley Board Ltda\0"
    "Virtual Computer Corporation\0"
    "VARCem\0"
    "VistaCom Inc\0"
    "Victor Company of Japan, Limited\0"
    "Vector Magnetics, LLC\0"
    "VCONEX\0"
    "Victor Data Systems\0"
    "VDC Display Systems\0"
    "Vadem\0"
    "Video & Display Oriented Corporation\0"
    "Vidisys GmbH & Company\0"
    "Viditec, Inc.\0"
    "Vector Informatik GmbH\0"
    "Vektrex\0"
    "Vestel Elektronik Sanayi ve Ticaret A. S.\0"
    "VeriFone Inc\0"
    "Macrocad Development Inc.\0"
    "VIA Tech Inc\0"
    "Tatung UK Ltd\0"
    "Victron B.V.\0"
    "Ingram Macrotron Germany\0"
    "Viking Connectors\0"
    "Via Mons Ltd.\0"
    "Vine Micros Ltd\0"
    "Zake IP Holdings LLC (3B tech)\0"
    "Visual Interface, Inc\0"
    "Visioneer\0"
    "Visitech AS\0"
    "VIZIO, Inc\0"
    "ValleyBoard Ltda.\0"
    "VersaLogic Corporation\0"
    "Vislink International Ltd\0"
    "LENOVO BEIJING CO. LTD.\0"
    "VideoLan Technologies\0"
    "Valve Corporation\0"
    "Vermont Microsystems\0"
    "Vine Micros Limited\0"
    "VMware Inc.,\0"
    "Vinca Corporation\0"
    "Venetex Corporation\0"
    "MaxData Computer AG\0"
    "Video Products Inc\0"
    "Best Buy\0"
    "VPixx Technologies Inc.\0"
    "Vision Quest\0"
    "Virtual Resources Corporation\0"
    "VRgineers, Inc.\0"
    "VRmagic Holding AG\0"
    "VRstudios, Inc.\0"
    "Varjo Technologies\0"
    "ViewSonic Corporation\0"
    "3M\0"
    "VideoServer\0"
    "Ingram Macrotron\0"
    "Vision Systems GmbH\0"
    "V-Star Electronics Inc.\0"
    "Videotechnik Breithaupt\0"
    "VTel Corporation\0"
    "Voice Technologies Group Inc\0"
    "VLSI Tech Inc\0"
    "Viewteck Co., Ltd.\0"
    "Vivid Technology Pte Ltd\0"
    "Miltope Corporation\0"
    "VIDEOTRON CORP.\0"
    "VTech Computers Ltd\0"
    "VATIV Technologies\0"
    "Vestax Corporation\0"
    "Vutrix (UK) Ltd\0"
    "VITEC\0"
    "Vweb Corp.\0"
    "Wacom Tech\0"
    "Wave Access\0"
    "Invalid Vendor Codename - WAN\0"
    "Wavephore\0"
    "MicroSoftWare\0"
    "WB Systemtechnik GmbH\0"
    "Wisecom Inc\0"
    "Woodwind Communications Systems Inc\0"
    "Western Digital\0"
    "Westinghouse Digital Electronics\0"
    "WebGear Inc\0"
    "Winbond Electronics Corporation\0"
    "W-DEV\0"
    "WEY Design AG\0"
    "Whistle Communications\0"
    "Innoware Inc\0"
    "WIPRO Information Technology Ltd\0"
    "Wintop Technology Inc\0"
    "Wipro Infotech\0"
    "Uni-Take Int'l Inc.\0"
    "Wildfire Communications Inc\0"
    "WOLF Advanced Technology\0"
    "Weidmuller Interface GmbH & Co. KG\0"
    "Wolfson Microelectronics Ltd\0"
    "Westermo Teleindustri AB\0"
    "Winmate Communication Inc\0"
    "WillNet Inc.\0"
    "Winnov L.P.\0"
    "Diebold Nixdorf Systems GmbH\0"
    "Matsushita Communication Industrial Co., Ltd.\0"
    "Wearnes Peripherals International (Pte) Ltd\0"
    "WiNRADiO Communications\0"
    "CIS Technology Inc\0"
    "Wireless And Smart Products Inc.\0"
    "Wistron Corporation\0"
    "ACC Microelectronics\0"
    "WorkStation Tech\0"
    "Wearnes Thakral Pte\0"
    "Restek Electric Company Ltd\0"
    "Wave Systems Corporation\0"
    "WolfVision GmbH\0"
    "Wipotec Wiege- und Positioniersysteme GmbH\0"
    "World Wide Video, Inc.\0"
    "Woxter Technology Co. Ltd\0"
    "WyreStorm Technologies LLC\0"
    "Wyse Technology\0"
    "Wooyoung Image & Information Co.,Ltd.\0"
    "XAC Automation Corp\0"
    "Alpha Data\0"
    "XDM Ltd.\0"
    "Invalid Vendor Codename - XER\0"
    "Extreme Engineering Solutions, Inc.\0"
    "Jan Strapko - FOTO\0"
    "EXFO Electro Optical Engineering\0"
    "Xinex Networks Inc\0"
    "Xiotech Corporation\0"
    "Xirocm Inc\0"
    "Xitel Pty ltd\0"
    "Xilinx, Inc.\0"
    "Xiaomi Corporation\0"
    "C3PO S.L.\0"
    "XN Technologies, Inc.\0"
    "Invalid Vendor Codename - XOC\0"
    "SHANGHAI SVA-DAV ELECTRONICS CO., LTD\0"
    "Xircom Inc\0"
    "XORO ELECTRONICS (CHENGDU) LIMITED\0"
    "Xscreen AS\0"
    "XS Technologies Inc\0"
    "XSYS\0"
    "Icuiti Corporation\0"
    "X2E GmbH\0"
    "Crystal Computer\0"
    "X-10 (USA) Inc\0"
    "Xycotec Computer GmbH\0"
    "Shenzhen Zhuona Technology Co., Ltd.\0"
    "Y-E Data Inc\0"
    "Yokogawa Electric Corporation\0"
    "Exacom SA\0"
    "Yamaha Corporation\0"
    "American Biometric Company\0"
    "Zandar Technologies plc\0"
    "Zefiro Acoustics\0"
    "ZeeVee, Inc.\0"
    "Zebra Technologies International, LLC\0"
    "Zebax Technologies\0"
    "ZeitControl cardsystems GmbH\0"
    "Zenith Data Systems\0"
    "ZENIC Inc.\0"
    "Zenith Data Systems\0"
    "Nationz Technologies Inc.\0"
    "HangZhou ZMCHIVIN\0"
    "Zalman Tech Co., Ltd.\0"
    "Z Microsystems\0"
    "Zetinet Inc\0"
    "Znyx Adv. Systems\0"
    "Zowie Intertainment, Inc\0"
    "Zoran Corporation\0"
    "Zenith Data Systems\0"
    "ZyDAS Technology Corporation\0"
    "ZTE Corporation\0"
    "Zoom Telephonics Inc\0"
    "ZT Group Int'l Inc.\0"
    "Z3 Technology\0"
    "Shenzhen Zowee Technology Co., LTD\0"
    "Zydacron Inc\0"
    "Zypcom Inc\0"
    "Zytex Computers\0"
    "Zyxel\0"
    "Boca Research Inc\0"
};

static constexpr quint16 q_edidVendorNamesOffsets[] = {
    0,
    13,
    37,
    52,
    74,
    97,
    111,
    130,
    152,
    165,
    184,
    204,
    234,
    263,
    281,
    292,
    322,
    331,
    357,
    376,
    404,
    429,
    436,
    454,
    490,
    511,
    526,
    544,
    567,
    595,
    605,
    619,
    634,
    645,
    661,
    693,
    708,
    727,
    749,
    765,
    798,
    826,
    848,
    882,
    894,
    920,
    939,
    945,
    972,
    981,
    1002,
    1032,
    1066,
    1089,
    1115,
    1127,
    1136,
    1167,
    1199,
    1208,
    1270,
    1290,
    1298,
    1322,
    1337,
    1358,
    1378,
    1391,
    1448,
    1485,
    1513,
    1540,
    1579,
    1595,
    1608,
    1637,
    1661,
    1678,
    1692,
    1716,
    1727,
    1750,
    1767,
    1776,
    1813,
    1833,
    1859,
    1886,
    1898,
    1916,
    1925,
    1935,
    1963,
    1974,
    1984,
    1998,
    2011,
    2046,
    2058,
    2077,
    2091,
    2113,
    2128,
    2145,
    2151,
    2165,
    2181,
    2218,
    2245,
    2268,
    2286,
    2310,
    2358,
    2370,
    2426,
    2434,
    2462,
    2476,
    2503,
    2511,
    2519,
    2525,
    2536,
    2547,
    2575,
    2603,
    2622,
    2648,
    2656,
    2684,
    2715,
    2750,
    2765,
    2781,
    2792,
    2826,
    2841,
    2849,
    2875,
    2886,
    2908,
    2928,
    2952,
    2963,
    2983,
    2999,
    3018,
    3033,
    3047,
    3080,
    3089,
    3104,
    3130,
    3140,
    3152,
    3179,
    3199,
    3213,
    3232,
    3238,
    3262,
    3280,
    3292,
    3322,
    3354,
    3389,
    3407,
    3431,
    3445,
    3453,
    3479,
    3491,
    3507,
    3532,
    3549,
    3569,
    3582,
    3617,
    3661,
    3683,
    3701,
    3714,
    3740,
    3758,
    3779,
    3800,
    3821,
    3829,
    3861,
    3880,
    3902,
    3907,
    3936,
    3955,
    3975,
    3988,
    4006,
    4028,
    4041,
    4062,
    4083,
    4104,
    4117,
    4137,
    4160,
    4189,
    4225,
    4245,
    4269,
    4296,
    4319,
    4328,
    4357,
    4377,
    4399,
    4421,
    4449,
    4508,
    4527,
    4549,
    4586,
    4599,
    4632,
    4651,
    4669,
    4688,
    4693,
    4709,
    4726,
    4750,
    4777,
    4807,
    4822,
    4834,
    4905,
    4934,
    4953,
    4982,
    4989,
    5009,
    5014,
    5034,
    5056,
    5078,
    5105,
    5116,
    5145,
    5187,
    5196,
    5225,
    5248,
    5260,
    5286,
    5308,
    5328,
    5354,
    5375,
    5410,
    5425,
    5454,
    5473,
    5487,
    5502,
    5528,
    5554,
    5570,
    5596,
    5625,
    5666,
    5681,
    5712,
    5720,
    5741,
    5756,
    5774,
    5809,
    5820,
    5831,
    5839,
    5860,
    5875,
    5901,
    5915,
    5919,
    5960,
    5964,
    5986,
    5998,
    6009,
    6025,
    6030,
    6058,
    6076,
    6088,
    6100,
    6124,
    6141,
    6159,
    6189,
    6205,
    6234,
    6249,
    6270,
    6282,
    6294,
    6305,
    6331,
    6344,
    6357,
    6362,
    6393,
    6400,
    6427,
    6441,
    6459,
    6480,
    6508,
    6527,
    6535,
    6546,
    6551,
    6567,
    6587,
    6608,
    6631,
    6666,
    6687,
    6706,
    6721,
    6735,
    6771,
    6791,
    6797,
    6813,
    6822,
    6841,
    6867,
    6891,
    6900,
    6929,
    6953,
    6973,
    7004,
    7012,
    7040,
    7056,
    7079,
    7112,
    7144,
    7176,
    7197,
    7224,
    7245,
    7272,
    7278,
    7286,
    7302,
    7311,
    7327,
    7355,
    7367,
    7379,
    7398,
    7425,
    7434,
    7470,
    7483,
    7496,
    7518,
    7549,
    7561,
    7615,
    7631,
    7659,
    7671,
    7702,
    7769,
    7798,
    7821,
    7854,
    7866,
    7878,
    7895,
    7913,
    7929,
    7962,
    7986,
    8006,
    8020,
    8033,
    8043,
    8060,
    8082,
    8098,
    8117,
    8152,
    8166,
    8176,
    8184,
    8212,
    8237,
    8257,
    8268,
    8281,
    8308,
    8338,
    8369,
    8385,
    8406,
    8432,
    8454,
    8481,
    8490,
    8508,
    8518,
    8543,
    8569,
    8585,
    8596,
    8619,
    8639,
    8658,
    8670,
    8684,
    8703,
    8723,
    8743,
    8750,
    8762,
    8771,
    8796,
    8819,
    8845,
    8864,
    8888,
    8894,
    8918,
    8945,
    8957,
    8980,
    8998,
    9026,
    9036,
    9051,
    9071,
    9091,
    9130,
    9149,
    9177,
    9189,
    9205,
    9223,
    9235,
    9257,
    9276,
    9308,
    9329,
    9351,
    9375,
    9410,
    9427,
    9436,
    9483,
    9497,
    9539,
    9566,
    9590,
    9612,
    9631,
    9663,
    9684,
    9709,
    9732,
    9750,
    9765,
    9776,
    9797,
    9826,
    9849,
    9879,
    9895,
    9912,
    9924,
    9943,
    9964,
    9974,
    10008,
    10023,
    10032,
    10042,
    10060,
    10105,
    10119,
    10149,
    10184,
    10198,
    10209,
    10232,
    10251,
    10260,
    10270,
    10284,
    10310,
    10331,
    10345,
    10358,
    10372,
    10385,
    10411,
    10427,
    10462,
    10488,
    10501,
    10513,
    10526,
    10547,
    10565,
    10597,
    10608,
    10637,
    10657,
    10684,
    10713,
    10732,
    10762,
    10778,
    10790,
    10816,
    10847,
    10877,
    10886,
    10904,
    10914,
    10935,
    10959,
    10973,
    10977,
    10990,
    11029,
    11047,
    11072,
    11091,
    11109,
    11138,
    11151,
    11177,
    11199,
    11208,
    11216,
    11235,
    11242,
    11257,
    11279,
    11299,
    11320,
    11332,
    11362,
    11398,
    11419,
    11431,
    11455,
    11486,
    11492,
    11511,
    11530,
    11539,
    11551,
    11590,
    11625,
    11645,
    11675,
    11689,
    11719,
    11745,
    11759,
    11807,
    11828,
    11875,
    11883,
    11905,
    11934,
    11962,
    11997,
    12003,
    12033,
    12054,
    12066,
    12096,
    12112,
    12134,
    12156,
    12166,
    12193,
    12212,
    12248,
    12275,
    12279,
    12290,
    12303,
    12315,
    12330,
    12354,
    12368,
    12395,
    12415,
    12437,
    12459,
    12479,
    12510,
    12536,
    12558,
    12567,
    12588,
    12617,
    12646,
    12673,
    12683,
    12700,
    12721,
    12742,
    12773,
    12790,
    12822,
    12838,
    12861,
    12871,
    12892,
    12903,
    12934,
    12961,
    12986,
    13003,
    13038,
    13062,
    13069,
    13079,
    13103,
    13130,
    13157,
    13177,
    13197,
    13227,
    13250,
    13290,
    13318,
    13336,
    13364,
    13385,
    13406,
    13452,
    13470,
    13494,
    13518,
    13537,
    13577,
    13598,
    13620,
    13643,
    13668,
    13673,
    13708,
    13736,
    13754,
    13797,
    13811,
    13830,
    13849,
    13880,
    13897,
    13911,
    13928,
    13943,
    13953,
    13982,
    14000,
    14012,
    14034,
    14048,
    14057,
    14080,
    14099,
    14118,
    14151,
    14170,
    14187,
    14205,
    14226,
    14236,
    14255,
    14280,
    14291,
    14318,
    14337,
    14359,
    14390,
    14410,
    14425,
    14444,
    14466,
    14497,
    14516,
    14539,
    14563,
    14583,
    14602,
    14622,
    14655,
    14681,
    14687,
    14708,
    14734,
    14746,
    14751,
    14775,
    14795,
    14807,
    14841,
    14856,
    14870,
    14892,
    14923,
    14948,
    14956,
    14988,
    15010,
    15035,
    15058,
    15063,
    15086,
    15096,
    15115,
    15144,
    15158,
    15189,
    15223,
    15246,
    15264,
    15281,
    15296,
    15321,
    15352,
    15363,
    15393,
    15424,
    15437,
    15474,
    15481,
    15489,
    15505,
    15523,
    15564,
    15588,
    15602,
    15639,
    15651,
    15664,
    15676,
    15692,
    15709,
    15726,
    15747,
    15766,
    15788,
    15818,
    15851,
    15876,
    15890,
    15919,
    15940,
    15975,
    15988,
    16014,
    16033,
    16050,
    16065,
    16086,
    16107,
    16129,
    16154,
    16159,
    16192,
    16212,
    16230,
    16250,
    16268,
    16304,
    16327,
    16341,
    16356,
    16372,
    16394,
    16419,
    16438,
    16464,
    16479,
    16502,
    16517,
    16527,
    16552,
    16574,
    16598,
    16611,
    16620,
    16651,
    16672,
    16696,
    16715,
    16730,
    16746,
    16769,
    16789,
    16811,
    16829,
    16862,
    16891,
    16908,
    16923,
    16947,
    16977,
    16994,
    17023,
    17043,
    17069,
    17080,
    17108,
    17144,
    17156,
    17187,
    17205,
    17236,
    17262,
    17278,
    17295,
    17316,
    17327,
    17354,
    17387,
    17405,
    17431,
    17452,
    17468,
    17499,
    17520,
    17538,
    17560,
    17576,
    17595,
    17599,
    17627,
    17646,
    17674,
    17700,
    17720,
    17729,
    17745,
    17755,
    17771,
    17793,
    17822,
    17841,
    17852,
    17864,
    17878,
    17904,
    17929,
    17946,
    17973,
    17997,
    18025,
    18040,
    18054,
    18070,
    18097,
    18125,
    18142,
    18158,
    18166,
    18185,
    18200,
    18210,
    18233,
    18245,
    18263,
    18291,
    18302,
    18323,
    18339,
    18359,
    18380,
    18410,
    18425,
    18452,
    18477,
    18502,
    18523,
    18533,
    18552,
    18578,
    18610,
    18645,
    18670,
    18683,
    18711,
    18727,
    18756,
    18765,
    18780,
    18802,
    18815,
    18840,
    18854,
    18873,
    18896,
    18907,
    18935,
    18954,
    18987,
    18991,
    19029,
    19045,
    19061,
    19091,
    19120,
    19152,
    19167,
    19184,
    19211,
    19244,
    19267,
    19291,
    19314,
    19350,
    19367,
    19386,
    19427,
    19464,
    19480,
    19499,
    19541,
    19571,
    19591,
    19613,
    19634,
    19655,
    19669,
    19697,
    19730,
    19760,
    19776,
    19799,
    19822,
    19842,
    19862,
    19883,
    19894,
    19915,
    19935,
    19951,
    19978,
    19993,
    20018,
    20026,
    20046,
    20070,
    20079,
    20115,
    20129,
    20142,
    20163,
    20172,
    20201,
    20223,
    20245,
    20262,
    20284,
    20313,
    20325,
    20349,
    20377,
    20393,
    20446,
    20475,
    20506,
    20531,
    20552,
    20569,
    20587,
    20606,
    20628,
    20644,
    20674,
    20686,
    20708,
    20735,
    20778,
    20792,
    20803,
    20820,
    20852,
    20872,
    20890,
    20937,
    20966,
    20983,
    21011,
    21025,
    21036,
    21057,
    21066,
    21074,
    21097,
    21105,
    21117,
    21143,
    21155,
    21166,
    21177,
    21220,
    21226,
    21253,
    21272,
    21281,
    21319,
    21334,
    21350,
    21369,
    21394,
    21429,
    21465,
    21498,
    21509,
    21543,
    21547,
    21569,
    21590,
    21601,
    21626,
    21637,
    21655,
    21667,
    21686,
    21725,
    21753,
    21769,
    21805,
    21832,
    21849,
    21873,
    21894,
    21903,
    21914,
    21927,
    21948,
    21957,
    21993,
    22013,
    22044,
    22066,
    22088,
    22095,
    22124,
    22152,
    22173,
    22186,
    22190,
    22225,
    22242,
    22264,
    22283,
    22300,
    22328,
    22342,
    22365,
    22381,
    22404,
    22414,
    22437,
    22453,
    22466,
    22522,
    22531,
    22558,
    22614,
    22634,
    22641,
    22661,
    22680,
    22692,
    22708,
    22741,
    22819,
    22861,
    22886,
    22913,
    22933,
    22952,
    23002,
    23035,
    23051,
    23079,
    23099,
    23106,
    23126,
    23145,
    23168,
    23188,
    23206,
    23227,
    23255,
    23271,
    23279,
    23303,
    23347,
    23375,
    23388,
    23420,
    23448,
    23462,
    23487,
    23497,
    23506,
    23520,
    23558,
    23583,
    23590,
    23603,
    23637,
    23642,
    23657,
    23678,
    23729,
    23754,
    23777,
    23796,
    23815,
    23832,
    23882,
    23892,
    23937,
    23964,
    23982,
    24003,
    24017,
    24036,
    24070,
    24100,
    24109,
    24140,
    24158,
    24177,
    24221,
    24245,
    24274,
    24302,
    24325,
    24352,
    24371,
    24388,
    24410,
    24435,
    24450,
    24476,
    24512,
    24528,
    24532,
    24557,
    24581,
    24597,
    24625,
    24631,
    24644,
    24663,
    24688,
    24700,
    24704,
    24716,
    24745,
    24753,
    24781,
    24812,
    24842,
    24859,
    24879,
    24892,
    24901,
    24916,
    24937,
    24956,
    24979,
    25003,
    25016,
    25036,
    25047,
    25067,
    25095,
    25120,
    25158,
    25177,
    25198,
    25217,
    25239,
    25257,
    25294,
    25307,
    25325,
    25342,
    25360,
    25381,
    25402,
    25424,
    25438,
    25447,
    25475,
    25537,
    25557,
    25581,
    25607,
    25637,
    25644,
    25663,
    25684,
    25707,
    25723,
    25761,
    25779,
    25789,
    25797,
    25817,
    25837,
    25858,
    25890,
    25910,
    25940,
    25961,
    25974,
    25980,
    25990,
    26005,
    26024,
    26040,
    26062,
    26085,
    26094,
    26098,
    26145,
    26170,
    26196,
    26211,
    26219,
    26249,
    26279,
    26299,
    26322,
    26351,
    26371,
    26399,
    26413,
    26434,
    26467,
    26479,
    26489,
    26500,
    26513,
    26536,
    26556,
    26571,
    26622,
    26655,
    26687,
    26711,
    26726,
    26745,
    26772,
    26794,
    26821,
    26840,
    26869,
    26889,
    26907,
    26927,
    26954,
    26967,
    26996,
    27022,
    27036,
    27056,
    27078,
    27085,
    27106,
    27122,
    27138,
    27162,
    27187,
    27205,
    27223,
    27234,
    27258,
    27279,
    27305,
    27327,
    27345,
    27369,
    27385,
    27396,
    27415,
    27447,
    27460,
    27474,
    27511,
    27526,
    27546,
    27560,
    27581,
    27613,
    27632,
    27661,
    27678,
    27699,
    27706,
    27720,
    27742,
    27765,
    27783,
    27799,
    27814,
    27832,
    27851,
    27856,
    27866,
    27893,
    27921,
    27941,
    27952,
    27956,
    27969,
    27990,
    28002,
    28031,
    28048,
    28067,
    28091,
    28115,
    28135,
    28158,
    28189,
    28202,
    28225,
    28247,
    28265,
    28281,
    28303,
    28326,
    28335,
    28365,
    28376,
    28395,
    28401,
    28411,
    28426,
    28443,
    28464,
    28474,
    28484,
    28510,
    28526,
    28546,
    28562,
    28576,
    28605,
    28631,
    28673,
    28689,
    28716,
    28736,
    28758,
    28790,
    28825,
    28841,
    28855,
    28875,
    28906,
    28925,
    28949,
    28964,
    28980,
    29011,
    29039,
    29063,
    29082,
    29105,
    29115,
    29140,
    29154,
    29164,
    29188,
    29224,
    29248,
    29261,
    29281,
    29314,
    29345,
    29370,
    29390,
    29402,
    29417,
    29436,
    29450,
    29469,
    29477,
    29500,
    29515,
    29540,
    29554,
    29581,
    29595,
    29606,
    29620,
    29644,
    29662,
    29676,
    29703,
    29738,
    29747,
    29771,
    29783,
    29799,
    29813,
    29836,
    29852,
    29865,
    29874,
    29911,
    29938,
    29957,
    29984,
    30006,
    30024,
    30037,
    30048,
    30074,
    30083,
    30116,
    30133,
    30151,
    30175,
    30203,
    30222,
    30247,
    30289,
    30314,
    30320,
    30341,
    30349,
    30360,
    30381,
    30402,
    30428,
    30469,
    30499,
    30508,
    30518,
    30528,
    30541,
    30557,
    30582,
    30609,
    30629,
    30643,
    30652,
    30670,
    30690,
    30713,
    30728,
    30752,
    30774,
    30799,
    30813,
    30844,
    30865,
    30888,
    30916,
    30934,
    30941,
    30976,
    30996,
    31015,
    31045,
    31069,
    31076,
    31102,
    31119,
    31134,
    31151,
    31163,
    31176,
    31194,
    31206,
    31212,
    31241,
    31247,
    31260,
    31290,
    31303,
    31324,
    31342,
    31362,
    31382,
    31404,
    31413,
    31434,
    31447,
    31465,
    31481,
    31504,
    31532,
    31548,
    31566,
    31589,
    31606,
    31630,
    31655,
    31694,
    31710,
    31726,
    31751,
    31762,
    31774,
    31814,
    31830,
    31857,
    31882,
    31922,
    31952,
    31984,
    32009,
    32033,
    32051,
    32062,
    32078,
    32099,
    32105,
    32120,
    32184,
    32207,
    32226,
    32247,
    32263,
    32277,
    32306,
    32317,
    32350,
    32374,
    32398,
    32420,
    32446,
    32459,
    32479,
    32500,
    32551,
    32560,
    32564,
    32597,
    32610,
    32627,
    32650,
    32669,
    32685,
    32694,
    32718,
    32745,
    32769,
    32804,
    32821,
    32836,
    32871,
    32895,
    32914,
    32940,
    32971,
    32985,
    33008,
    33015,
    33039,
    33070,
    33107,
    33128,
    33164,
    33177,
    33191,
    33211,
    33219,
    33241,
    33248,
    33266,
    33277,
    33289,
    33304,
    33336,
    33360,
    33385,
    33410,
    33424,
    33437,
    33446,
    33453,
    33476,
    33501,
    33523,
    33563,
    33585,
    33617,
    33630,
    33655,
    33677,
    33699,
    33705,
    33728,
    33738,
    33744,
    33767,
    33790,
    33824,
    33839,
    33867,
    33888,
    33909,
    33945,
    33956,
    33971,
    33980,
    33992,
    34004,
    34024,
    34053,
    34058,
    34066,
    34084,
    34102,
    34120,
    34132,
    34147,
    34165,
    34194,
    34200,
    34211,
    34230,
    34249,
    34258,
    34273,
    34292,
    34316,
    34349,
    34374,
    34401,
    34432,
    34449,
    34470,
    34489,
    34512,
    34530,
    34538,
    34557,
    34570,
    34586,
    34602,
    34632,
    34651,
    34666,
    34681,
    34692,
    34717,
    34746,
    34784,
    34811,
    34837,
    34867,
    34885,
    34903,
    34916,
    34941,
    34958,
    34971,
    34994,
    35005,
    35037,
    35067,
    35088,
    35097,
    35112,
    35136,
    35163,
    35179,
    35223,
    35234,
    35255,
    35266,
    35283,
    35297,
    35311,
    35340,
    35379,
    35393,
    35418,
    35439,
    35459,
    35479,
    35513,
    35526,
    35557,
    35584,
    35601,
    35616,
    35650,
    35675,
    35701,
    35724,
    35764,
    35794,
    35831,
    35854,
    35884,
    35906,
    35921,
    35959,
    35970,
    36001,
    36022,
    36039,
    36054,
    36063,
    36083,
    36110,
    36123,
    36143,
    36174,
    36201,
    36229,
    36247,
    36265,
    36278,
    36307,
    36327,
    36351,
    36375,
    36404,
    36416,
    36426,
    36442,
    36452,
    36473,
    36483,
    36525,
    36552,
    36572,
    36600,
    36612,
    36637,
    36644,
    36673,
    36679,
    36701,
    36727,
    36745,
    36752,
    36774,
    36783,
    36798,
    36806,
    36819,
    36859,
    36890,
    36911,
    36922,
    36930,
    36941,
    36956,
    36982,
    37002,
    37042,
    37065,
    37083,
    37114,
    37143,
    37165,
    37172,
    37187,
    37207,
    37221,
    37247,
    37266,
    37291,
    37314,
    37325,
    37348,
    37378,
    37393,
    37409,
    37439,
    37463,
    37497,
    37522,
    37535,
    37559,
    37580,
    37596,
    37615,
    37640,
    37651,
    37665,
    37675,
    37696,
    37710,
    37730,
    37748,
    37772,
    37785,
    37811,
    37826,
    37834,
    37854,
    37870,
    37885,
    37893,
    37913,
    37922,
    37946,
    37961,
    37974,
    38000,
    38008,
    38028,
    38044,
    38063,
    38072,
    38101,
    38121,
    38151,
    38167,
    38181,
    38192,
    38204,
    38224,
    38254,
    38275,
    38292,
    38310,
    38327,
    38350,
    38384,
    38415,
    38430,
    38441,
    38448,
    38454,
    38491,
    38510,
    38533,
    38567,
    38584,
    38609,
    38624,
    38651,
    38676,
    38696,
    38715,
    38740,
    38753,
    38778,
    38801,
    38818,
    38832,
    38852,
    38871,
    38896,
    38907,
    38930,
    38949,
    38970,
    39000,
    39006,
    39019,
    39041,
    39059,
    39102,
    39115,
    39136,
    39146,
    39170,
    39185,
    39208,
    39232,
    39248,
    39269,
    39282,
    39300,
    39321,
    39328,
    39341,
    39352,
    39372,
    39398,
    39408,
    39461,
    39492,
    39507,
    39530,
    39549,
    39565,
    39595,
    39629,
    39646,
    39666,
    39691,
    39707,
    39722,
    39743,
    39764,
    39796,
    39818,
    39834,
    39862,
    39876,
    39886,
    39895,
    39906,
    39931,
    39955,
    39984,
    39997,
    40034,
    40073,
    40097,
    40130,
    40157,
    40182,
    40196,
    40222,
    40249,
    40265,
    40277,
    40296,
    40309,
    40328,
    40349,
    40364,
    40385,
    40406,
    40426,
    40443,
    40465,
    40480,
    40502,
    40524,
    40540,
    40581,
    40602,
    40634,
    40655,
    40670,
    40682,
    40703,
    40714,
    40739,
    40762,
    40785,
    40809,
    40830,
    40860,
    40883,
    40907,
    40939,
    40947,
    40956,
    40977,
    41018,
    41038,
    41058,
    41072,
    41088,
    41106,
    41134,
    41154,
    41182,
    41197,
    41231,
    41237,
    41276,
    41293,
    41313,
    41362,
    41383,
    41404,
    41424,
    41467,
    41508,
    41526,
    41544,
    41557,
    41568,
    41579,
    41606,
    41626,
    41668,
    41676,
    41694,
    41714,
    41740,
    41747,
    41777,
    41805,
    41844,
    41853,
    41879,
    41902,
    41921,
    41938,
    41968,
    41984,
    42033,
    42071,
    42080,
    42095,
    42120,
    42136,
    42161,
    42170,
    42191,
    42209,
    42229,
    42260,
    42294,
    42323,
    42349,
    42359,
    42386,
    42399,
    42433,
    42449,
    42471,
    42487,
    42519,
    42540,
    42559,
    42578,
    42595,
    42609,
    42640,
    42670,
    42700,
    42725,
    42741,
    42758,
    42786,
    42815,
    42832,
    42845,
    42857,
    42872,
    42888,
    42893,
    42912,
    42938,
    42964,
    42969,
    42990,
    43008,
    43024,
    43051,
    43068,
    43089,
    43107,
    43120,
    43150,
    43171,
    43189,
    43199,
    43212,
    43229,
    43252,
    43278,
    43307,
    43335,
    43342,
    43375,
    43400,
    43416,
    43442,
    43467,
    43494,
    43517,
    43533,
    43552,
    43578,
    43616,
    43648,
    43655,
    43678,
    43718,
    43734,
    43751,
    43768,
    43788,
    43811,
    43832,
    43852,
    43867,
    43880,
    43893,
    43922,
    43950,
    43969,
    43984,
    44006,
    44029,
    44051,
    44096,
    44117,
    44174,
    44187,
    44199,
    44216,
    44235,
    44261,
    44289,
    44307,
    44336,
    44341,
    44356,
    44369,
    44386,
    44400,
    44405,
    44421,
    44435,
    44456,
    44471,
    44510,
    44517,
    44550,
    44566,
    44589,
    44603,
    44616,
    44631,
    44673,
    44680,
    44699,
    44722,
    44749,
    44787,
    44801,
    44814,
    44829,
    44853,
    44863,
    44883,
    44905,
    44914,
    44935,
    44944,
    44977,
    44999,
    45026,
    45042,
    45058,
    45077,
    45102,
    45127,
    45147,
    45166,
    45196,
    45216,
    45235,
    45260,
    45287,
    45304,
    45327,
    45344,
    45364,
    45390,
    45419,
    45449,
    45484,
    45510,
    45519,
    45544,
    45563,
    45590,
    45604,
    45625,
    45629,
    45652,
    45670,
    45694,
    45705,
    45719,
    45757,
    45765,
    45790,
    45810,
    45824,
    45841,
    45881,
    45901,
    45922,
    45940,
    45966,
    45983,
    46013,
    46030,
    46055,
    46065,
    46082,
    46096,
    46121,
    46130,
    46153,
    46167,
    46193,
    46217,
    46226,
    46269,
    46287,
    46312,
    46324,
    46337,
    46355,
    46378,
    46397,
    46404,
    46423,
    46449,
    46478,
    46517,
    46539,
    46557,
    46579,
    46598,
    46623,
    46648,
    46663,
    46687,
    46710,
    46737,
    46767,
    46781,
    46803,
    46830,
    46845,
    46861,
    46879,
    46885,
    46916,
    46930,
    46952,
    46984,
    47020,
    47053,
    47061,
    47079,
    47100,
    47128,
    47140,
    47187,
    47205,
    47244,
    47263,
    47272,
    47296,
    47307,
    47323,
    47341,
    47371,
    47398,
    47412,
    47430,
    47451,
    47475,
    47501,
    47518,
    47551,
    47578,
    47601,
    47624,
    47657,
    47680,
    47714,
    47732,
    47765,
    47775,
    47790,
    47801,
    47827,
    47840,
    47868,
    47901,
    47943,
    47953,
    47975,
    48003,
    48013,
    48033,
    48082,
    48113,
    48132,
    48165,
    48189,
    48201,
    48213,
    48224,
    48241,
    48276,
    48287,
    48314,
    48338,
    48347,
    48369,
    48388,
    48423,
    48434,
    48460,
    48476,
    48487,
    48505,
    48528,
    48554,
    48580,
    48599,
    48614,
    48629,
    48648,
    48679,
    48719,
    48745,
    48779,
    48791,
    48807,
    48814,
    48845,
    48872,
    48891,
    48922,
    48944,
    48965,
    48977,
    48988,
    49007,
    49026,
    49045,
    49064,
    49083,
    49106,
    49125,
    49144,
    49151,
    49170,
    49189,
    49197,
    49202,
    49222,
    49244,
    49264,
    49289,
    49312,
    49354,
    49372,
    49404,
    49420,
    49443,
    49455,
    49472,
    49502,
    49527,
    49540,
    49547,
    49567,
    49585,
    49614,
    49621,
    49634,
    49667,
    49689,
    49696,
    49716,
    49736,
    49742,
    49779,
    49802,
    49816,
    49839,
    49847,
    49889,
    49902,
    49928,
    49941,
    49955,
    49968,
    49993,
    50011,
    50025,
    50041,
    50072,
    50094,
    50104,
    50116,
    50127,
    50145,
    50168,
    50194,
    50218,
    50240,
    50258,
    50279,
    50299,
    50312,
    50330,
    50350,
    50370,
    50389,
    50398,
    50422,
    50435,
    50465,
    50481,
    50500,
    50516,
    50535,
    50557,
    50560,
    50572,
    50589,
    50609,
    50633,
    50657,
    50674,
    50703,
    50717,
    50736,
    50761,
    50781,
    50797,
    50817,
    50836,
    50855,
    50871,
    50877,
    50888,
    50899,
    50911,
    50941,
    50951,
    50965,
    50987,
    50999,
    51035,
    51051,
    51084,
    51096,
    51128,
    51134,
    51148,
    51171,
    51184,
    51217,
    51239,
    51254,
    51274,
    51302,
    51327,
    51362,
    51391,
    51416,
    51442,
    51455,
    51467,
    51496,
    51542,
    51586,
    51610,
    51629,
    51662,
    51682,
    51703,
    51720,
    51740,
    51768,
    51793,
    51809,
    51852,
    51875,
    51901,
    51928,
    51944,
    51982,
    52002,
    52013,
    52022,
    52052,
    52088,
    52107,
    52140,
    52159,
    52179,
    52190,
    52204,
    52217,
    52236,
    52246,
    52268,
    52298,
    52336,
    52347,
    52382,
    52393,
    52413,
    52418,
    52437,
    52446,
    52463,
    52478,
    52500,
    52537,
    52550,
    52580,
    52590,
    52609,
    52636,
    52660,
    52677,
    52690,
    52728,
    52747,
    52776,
    52796,
    52807,
    52827,
    52853,
    52871,
    52893,
    52908,
    52920,
    52938,
    52963,
    52981,
    53001,
    53030,
    53046,
    53067,
    53087,
    53101,
    53136,
    53149,
    53160,
    53176,
    53182,
};

static_assert(std::size(q_edidVendorIds) == std::size(q_edidVendorNamesOffsets));

QT_END_NAMESPACE

#endif // QEDIDVENDORTABLE_P_H
