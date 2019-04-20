/* gpg-error.h - Public interface to libgpg-error.               -*- c -*-
   Copyright (C) 2003, 2004, 2010, 2013, 2014, 2015 g10 Code GmbH

   This file is part of libgpg-error.

   libgpg-error is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   libgpg-error is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.

   Do not edit.  Generated from gpg-error.h.in for arm-unknown-linux-androideabi.
 */


#ifndef GPG_ERROR_H
#define GPG_ERROR_H	1

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __GNUC__
#define GPG_ERR_INLINE __inline__
#elif _MSC_VER >= 1300
#define GPG_ERR_INLINE __inline
#elif __STDC_VERSION__ >= 199901L
#define GPG_ERR_INLINE inline
#else
#ifndef GPG_ERR_INLINE
#define GPG_ERR_INLINE
#endif
#endif

#ifdef __cplusplus
extern "C" {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif
#endif /* __cplusplus */

/* The GnuPG project consists of many components.  Error codes are
   exchanged between all components.  The common error codes and their
   user-presentable descriptions are kept into a shared library to
   allow adding new error codes and components without recompiling any
   of the other components.  The interface will not change in a
   backward incompatible way.

   An error code together with an error source build up an error
   value.  As the error value is been passed from one component to
   another, it preserver the information about the source and nature
   of the error.

   A component of the GnuPG project can define the following macros to
   tune the behaviour of the library:

   GPG_ERR_SOURCE_DEFAULT: Define to an error source of type
   gpg_err_source_t to make that source the default for gpg_error().
   Otherwise GPG_ERR_SOURCE_UNKNOWN is used as default.

   GPG_ERR_ENABLE_GETTEXT_MACROS: Define to provide macros to map the
   internal gettext API to standard names.  This has only an effect on
   Windows platforms.

   GPGRT_ENABLE_ES_MACROS: Define to provide "es_" macros for the
   estream functions.

   In addition to the error codes, Libgpg-error also provides a set of
   functions used by most GnuPG components.  */


/* The error source type gpg_err_source_t.

   Where as the Poo out of a welle small
   Taketh his firste springing and his sours.
					--Chaucer.  */

/* Only use free slots, never change or reorder the existing
   entries.  */
typedef enum
  {
    GPG_ERR_SOURCE_UNKNOWN = 0,
    GPG_ERR_SOURCE_GCRYPT = 1,
    GPG_ERR_SOURCE_GPG = 2,
    GPG_ERR_SOURCE_GPGSM = 3,
    GPG_ERR_SOURCE_GPGAGENT = 4,
    GPG_ERR_SOURCE_PINENTRY = 5,
    GPG_ERR_SOURCE_SCD = 6,
    GPG_ERR_SOURCE_GPGME = 7,
    GPG_ERR_SOURCE_KEYBOX = 8,
    GPG_ERR_SOURCE_KSBA = 9,
    GPG_ERR_SOURCE_DIRMNGR = 10,
    GPG_ERR_SOURCE_GSTI = 11,
    GPG_ERR_SOURCE_GPA = 12,
    GPG_ERR_SOURCE_KLEO = 13,
    GPG_ERR_SOURCE_G13 = 14,
    GPG_ERR_SOURCE_ASSUAN = 15,
    GPG_ERR_SOURCE_TLS = 17,
    GPG_ERR_SOURCE_ANY = 31,
    GPG_ERR_SOURCE_USER_1 = 32,
    GPG_ERR_SOURCE_USER_2 = 33,
    GPG_ERR_SOURCE_USER_3 = 34,
    GPG_ERR_SOURCE_USER_4 = 35,

    /* This is one more than the largest allowed entry.  */
    GPG_ERR_SOURCE_DIM = 128
  } gpg_err_source_t;


/* The error code type gpg_err_code_t.  */

/* Only use free slots, never change or reorder the existing
   entries.  */
typedef enum
  {
    GPG_ERR_NO_ERROR = 0,
    GPG_ERR_GENERAL = 1,
    GPG_ERR_UNKNOWN_PACKET = 2,
    GPG_ERR_UNKNOWN_VERSION = 3,
    GPG_ERR_PUBKEY_ALGO = 4,
    GPG_ERR_DIGEST_ALGO = 5,
    GPG_ERR_BAD_PUBKEY = 6,
    GPG_ERR_BAD_SECKEY = 7,
    GPG_ERR_BAD_SIGNATURE = 8,
    GPG_ERR_NO_PUBKEY = 9,
    GPG_ERR_CHECKSUM = 10,
    GPG_ERR_BAD_PASSPHRASE = 11,
    GPG_ERR_CIPHER_ALGO = 12,
    GPG_ERR_KEYRING_OPEN = 13,
    GPG_ERR_INV_PACKET = 14,
    GPG_ERR_INV_ARMOR = 15,
    GPG_ERR_NO_USER_ID = 16,
    GPG_ERR_NO_SECKEY = 17,
    GPG_ERR_WRONG_SECKEY = 18,
    GPG_ERR_BAD_KEY = 19,
    GPG_ERR_COMPR_ALGO = 20,
    GPG_ERR_NO_PRIME = 21,
    GPG_ERR_NO_ENCODING_METHOD = 22,
    GPG_ERR_NO_ENCRYPTION_SCHEME = 23,
    GPG_ERR_NO_SIGNATURE_SCHEME = 24,
    GPG_ERR_INV_ATTR = 25,
    GPG_ERR_NO_VALUE = 26,
    GPG_ERR_NOT_FOUND = 27,
    GPG_ERR_VALUE_NOT_FOUND = 28,
    GPG_ERR_SYNTAX = 29,
    GPG_ERR_BAD_MPI = 30,
    GPG_ERR_INV_PASSPHRASE = 31,
    GPG_ERR_SIG_CLASS = 32,
    GPG_ERR_RESOURCE_LIMIT = 33,
    GPG_ERR_INV_KEYRING = 34,
    GPG_ERR_TRUSTDB = 35,
    GPG_ERR_BAD_CERT = 36,
    GPG_ERR_INV_USER_ID = 37,
    GPG_ERR_UNEXPECTED = 38,
    GPG_ERR_TIME_CONFLICT = 39,
    GPG_ERR_KEYSERVER = 40,
    GPG_ERR_WRONG_PUBKEY_ALGO = 41,
    GPG_ERR_TRIBUTE_TO_D_A = 42,
    GPG_ERR_WEAK_KEY = 43,
    GPG_ERR_INV_KEYLEN = 44,
    GPG_ERR_INV_ARG = 45,
    GPG_ERR_BAD_URI = 46,
    GPG_ERR_INV_URI = 47,
    GPG_ERR_NETWORK = 48,
    GPG_ERR_UNKNOWN_HOST = 49,
    GPG_ERR_SELFTEST_FAILED = 50,
    GPG_ERR_NOT_ENCRYPTED = 51,
    GPG_ERR_NOT_PROCESSED = 52,
    GPG_ERR_UNUSABLE_PUBKEY = 53,
    GPG_ERR_UNUSABLE_SECKEY = 54,
    GPG_ERR_INV_VALUE = 55,
    GPG_ERR_BAD_CERT_CHAIN = 56,
    GPG_ERR_MISSING_CERT = 57,
    GPG_ERR_NO_DATA = 58,
    GPG_ERR_BUG = 59,
    GPG_ERR_NOT_SUPPORTED = 60,
    GPG_ERR_INV_OP = 61,
    GPG_ERR_TIMEOUT = 62,
    GPG_ERR_INTERNAL = 63,
    GPG_ERR_EOF_GCRYPT = 64,
    GPG_ERR_INV_OBJ = 65,
    GPG_ERR_TOO_SHORT = 66,
    GPG_ERR_TOO_LARGE = 67,
    GPG_ERR_NO_OBJ = 68,
    GPG_ERR_NOT_IMPLEMENTED = 69,
    GPG_ERR_CONFLICT = 70,
    GPG_ERR_INV_CIPHER_MODE = 71,
    GPG_ERR_INV_FLAG = 72,
    GPG_ERR_INV_HANDLE = 73,
    GPG_ERR_TRUNCATED = 74,
    GPG_ERR_INCOMPLETE_LINE = 75,
    GPG_ERR_INV_RESPONSE = 76,
    GPG_ERR_NO_AGENT = 77,
    GPG_ERR_AGENT = 78,
    GPG_ERR_INV_DATA = 79,
    GPG_ERR_ASSUAN_SERVER_FAULT = 80,
    GPG_ERR_ASSUAN = 81,
    GPG_ERR_INV_SESSION_KEY = 82,
    GPG_ERR_INV_SEXP = 83,
    GPG_ERR_UNSUPPORTED_ALGORITHM = 84,
    GPG_ERR_NO_PIN_ENTRY = 85,
    GPG_ERR_PIN_ENTRY = 86,
    GPG_ERR_BAD_PIN = 87,
    GPG_ERR_INV_NAME = 88,
    GPG_ERR_BAD_DATA = 89,
    GPG_ERR_INV_PARAMETER = 90,
    GPG_ERR_WRONG_CARD = 91,
    GPG_ERR_NO_DIRMNGR = 92,
    GPG_ERR_DIRMNGR = 93,
    GPG_ERR_CERT_REVOKED = 94,
    GPG_ERR_NO_CRL_KNOWN = 95,
    GPG_ERR_CRL_TOO_OLD = 96,
    GPG_ERR_LINE_TOO_LONG = 97,
    GPG_ERR_NOT_TRUSTED = 98,
    GPG_ERR_CANCELED = 99,
    GPG_ERR_BAD_CA_CERT = 100,
    GPG_ERR_CERT_EXPIRED = 101,
    GPG_ERR_CERT_TOO_YOUNG = 102,
    GPG_ERR_UNSUPPORTED_CERT = 103,
    GPG_ERR_UNKNOWN_SEXP = 104,
    GPG_ERR_UNSUPPORTED_PROTECTION = 105,
    GPG_ERR_CORRUPTED_PROTECTION = 106,
    GPG_ERR_AMBIGUOUS_NAME = 107,
    GPG_ERR_CARD = 108,
    GPG_ERR_CARD_RESET = 109,
    GPG_ERR_CARD_REMOVED = 110,
    GPG_ERR_INV_CARD = 111,
    GPG_ERR_CARD_NOT_PRESENT = 112,
    GPG_ERR_NO_PKCS15_APP = 113,
    GPG_ERR_NOT_CONFIRMED = 114,
    GPG_ERR_CONFIGURATION = 115,
    GPG_ERR_NO_POLICY_MATCH = 116,
    GPG_ERR_INV_INDEX = 117,
    GPG_ERR_INV_ID = 118,
    GPG_ERR_NO_SCDAEMON = 119,
    GPG_ERR_SCDAEMON = 120,
    GPG_ERR_UNSUPPORTED_PROTOCOL = 121,
    GPG_ERR_BAD_PIN_METHOD = 122,
    GPG_ERR_CARD_NOT_INITIALIZED = 123,
    GPG_ERR_UNSUPPORTED_OPERATION = 124,
    GPG_ERR_WRONG_KEY_USAGE = 125,
    GPG_ERR_NOTHING_FOUND = 126,
    GPG_ERR_WRONG_BLOB_TYPE = 127,
    GPG_ERR_MISSING_VALUE = 128,
    GPG_ERR_HARDWARE = 129,
    GPG_ERR_PIN_BLOCKED = 130,
    GPG_ERR_USE_CONDITIONS = 131,
    GPG_ERR_PIN_NOT_SYNCED = 132,
    GPG_ERR_INV_CRL = 133,
    GPG_ERR_BAD_BER = 134,
    GPG_ERR_INV_BER = 135,
    GPG_ERR_ELEMENT_NOT_FOUND = 136,
    GPG_ERR_IDENTIFIER_NOT_FOUND = 137,
    GPG_ERR_INV_TAG = 138,
    GPG_ERR_INV_LENGTH = 139,
    GPG_ERR_INV_KEYINFO = 140,
    GPG_ERR_UNEXPECTED_TAG = 141,
    GPG_ERR_NOT_DER_ENCODED = 142,
    GPG_ERR_NO_CMS_OBJ = 143,
    GPG_ERR_INV_CMS_OBJ = 144,
    GPG_ERR_UNKNOWN_CMS_OBJ = 145,
    GPG_ERR_UNSUPPORTED_CMS_OBJ = 146,
    GPG_ERR_UNSUPPORTED_ENCODING = 147,
    GPG_ERR_UNSUPPORTED_CMS_VERSION = 148,
    GPG_ERR_UNKNOWN_ALGORITHM = 149,
    GPG_ERR_INV_ENGINE = 150,
    GPG_ERR_PUBKEY_NOT_TRUSTED = 151,
    GPG_ERR_DECRYPT_FAILED = 152,
    GPG_ERR_KEY_EXPIRED = 153,
    GPG_ERR_SIG_EXPIRED = 154,
    GPG_ERR_ENCODING_PROBLEM = 155,
    GPG_ERR_INV_STATE = 156,
    GPG_ERR_DUP_VALUE = 157,
    GPG_ERR_MISSING_ACTION = 158,
    GPG_ERR_MODULE_NOT_FOUND = 159,
    GPG_ERR_INV_OID_STRING = 160,
    GPG_ERR_INV_TIME = 161,
    GPG_ERR_INV_CRL_OBJ = 162,
    GPG_ERR_UNSUPPORTED_CRL_VERSION = 163,
    GPG_ERR_INV_CERT_OBJ = 164,
    GPG_ERR_UNKNOWN_NAME = 165,
    GPG_ERR_LOCALE_PROBLEM = 166,
    GPG_ERR_NOT_LOCKED = 167,
    GPG_ERR_PROTOCOL_VIOLATION = 168,
    GPG_ERR_INV_MAC = 169,
    GPG_ERR_INV_REQUEST = 170,
    GPG_ERR_UNKNOWN_EXTN = 171,
    GPG_ERR_UNKNOWN_CRIT_EXTN = 172,
    GPG_ERR_LOCKED = 173,
    GPG_ERR_UNKNOWN_OPTION = 174,
    GPG_ERR_UNKNOWN_COMMAND = 175,
    GPG_ERR_NOT_OPERATIONAL = 176,
    GPG_ERR_NO_PASSPHRASE = 177,
    GPG_ERR_NO_PIN = 178,
    GPG_ERR_NOT_ENABLED = 179,
    GPG_ERR_NO_ENGINE = 180,
    GPG_ERR_MISSING_KEY = 181,
    GPG_ERR_TOO_MANY = 182,
    GPG_ERR_LIMIT_REACHED = 183,
    GPG_ERR_NOT_INITIALIZED = 184,
    GPG_ERR_MISSING_ISSUER_CERT = 185,
    GPG_ERR_NO_KEYSERVER = 186,
    GPG_ERR_INV_CURVE = 187,
    GPG_ERR_UNKNOWN_CURVE = 188,
    GPG_ERR_DUP_KEY = 189,
    GPG_ERR_AMBIGUOUS = 190,
    GPG_ERR_NO_CRYPT_CTX = 191,
    GPG_ERR_WRONG_CRYPT_CTX = 192,
    GPG_ERR_BAD_CRYPT_CTX = 193,
    GPG_ERR_CRYPT_CTX_CONFLICT = 194,
    GPG_ERR_BROKEN_PUBKEY = 195,
    GPG_ERR_BROKEN_SECKEY = 196,
    GPG_ERR_MAC_ALGO = 197,
    GPG_ERR_FULLY_CANCELED = 198,
    GPG_ERR_UNFINISHED = 199,
    GPG_ERR_BUFFER_TOO_SHORT = 200,
    GPG_ERR_SEXP_INV_LEN_SPEC = 201,
    GPG_ERR_SEXP_STRING_TOO_LONG = 202,
    GPG_ERR_SEXP_UNMATCHED_PAREN = 203,
    GPG_ERR_SEXP_NOT_CANONICAL = 204,
    GPG_ERR_SEXP_BAD_CHARACTER = 205,
    GPG_ERR_SEXP_BAD_QUOTATION = 206,
    GPG_ERR_SEXP_ZERO_PREFIX = 207,
    GPG_ERR_SEXP_NESTED_DH = 208,
    GPG_ERR_SEXP_UNMATCHED_DH = 209,
    GPG_ERR_SEXP_UNEXPECTED_PUNC = 210,
    GPG_ERR_SEXP_BAD_HEX_CHAR = 211,
    GPG_ERR_SEXP_ODD_HEX_NUMBERS = 212,
    GPG_ERR_SEXP_BAD_OCT_CHAR = 213,
    GPG_ERR_LEGACY_KEY = 222,
    GPG_ERR_REQUEST_TOO_SHORT = 223,
    GPG_ERR_REQUEST_TOO_LONG = 224,
    GPG_ERR_OBJ_TERM_STATE = 225,
    GPG_ERR_NO_CERT_CHAIN = 226,
    GPG_ERR_CERT_TOO_LARGE = 227,
    GPG_ERR_INV_RECORD = 228,
    GPG_ERR_BAD_MAC = 229,
    GPG_ERR_UNEXPECTED_MSG = 230,
    GPG_ERR_COMPR_FAILED = 231,
    GPG_ERR_WOULD_WRAP = 232,
    GPG_ERR_FATAL_ALERT = 233,
    GPG_ERR_NO_CIPHER = 234,
    GPG_ERR_MISSING_CLIENT_CERT = 235,
    GPG_ERR_CLOSE_NOTIFY = 236,
    GPG_ERR_TICKET_EXPIRED = 237,
    GPG_ERR_BAD_TICKET = 238,
    GPG_ERR_UNKNOWN_IDENTITY = 239,
    GPG_ERR_BAD_HS_CERT = 240,
    GPG_ERR_BAD_HS_CERT_REQ = 241,
    GPG_ERR_BAD_HS_CERT_VER = 242,
    GPG_ERR_BAD_HS_CHANGE_CIPHER = 243,
    GPG_ERR_BAD_HS_CLIENT_HELLO = 244,
    GPG_ERR_BAD_HS_SERVER_HELLO = 245,
    GPG_ERR_BAD_HS_SERVER_HELLO_DONE = 246,
    GPG_ERR_BAD_HS_FINISHED = 247,
    GPG_ERR_BAD_HS_SERVER_KEX = 248,
    GPG_ERR_BAD_HS_CLIENT_KEX = 249,
    GPG_ERR_BOGUS_STRING = 250,
    GPG_ERR_FORBIDDEN = 251,
    GPG_ERR_KEY_DISABLED = 252,
    GPG_ERR_KEY_ON_CARD = 253,
    GPG_ERR_INV_LOCK_OBJ = 254,
    GPG_ERR_ASS_GENERAL = 257,
    GPG_ERR_ASS_ACCEPT_FAILED = 258,
    GPG_ERR_ASS_CONNECT_FAILED = 259,
    GPG_ERR_ASS_INV_RESPONSE = 260,
    GPG_ERR_ASS_INV_VALUE = 261,
    GPG_ERR_ASS_INCOMPLETE_LINE = 262,
    GPG_ERR_ASS_LINE_TOO_LONG = 263,
    GPG_ERR_ASS_NESTED_COMMANDS = 264,
    GPG_ERR_ASS_NO_DATA_CB = 265,
    GPG_ERR_ASS_NO_INQUIRE_CB = 266,
    GPG_ERR_ASS_NOT_A_SERVER = 267,
    GPG_ERR_ASS_NOT_A_CLIENT = 268,
    GPG_ERR_ASS_SERVER_START = 269,
    GPG_ERR_ASS_READ_ERROR = 270,
    GPG_ERR_ASS_WRITE_ERROR = 271,
    GPG_ERR_ASS_TOO_MUCH_DATA = 273,
    GPG_ERR_ASS_UNEXPECTED_CMD = 274,
    GPG_ERR_ASS_UNKNOWN_CMD = 275,
    GPG_ERR_ASS_SYNTAX = 276,
    GPG_ERR_ASS_CANCELED = 277,
    GPG_ERR_ASS_NO_INPUT = 278,
    GPG_ERR_ASS_NO_OUTPUT = 279,
    GPG_ERR_ASS_PARAMETER = 280,
    GPG_ERR_ASS_UNKNOWN_INQUIRE = 281,
    GPG_ERR_USER_1 = 1024,
    GPG_ERR_USER_2 = 1025,
    GPG_ERR_USER_3 = 1026,
    GPG_ERR_USER_4 = 1027,
    GPG_ERR_USER_5 = 1028,
    GPG_ERR_USER_6 = 1029,
    GPG_ERR_USER_7 = 1030,
    GPG_ERR_USER_8 = 1031,
    GPG_ERR_USER_9 = 1032,
    GPG_ERR_USER_10 = 1033,
    GPG_ERR_USER_11 = 1034,
    GPG_ERR_USER_12 = 1035,
    GPG_ERR_USER_13 = 1036,
    GPG_ERR_USER_14 = 1037,
    GPG_ERR_USER_15 = 1038,
    GPG_ERR_USER_16 = 1039,
    GPG_ERR_MISSING_ERRNO = 16381,
    GPG_ERR_UNKNOWN_ERRNO = 16382,
    GPG_ERR_EOF = 16383,

    /* The following error codes are used to map system errors.  */
#define GPG_ERR_SYSTEM_ERROR	(1 << 15)
    GPG_ERR_E2BIG = GPG_ERR_SYSTEM_ERROR | 0,
    GPG_ERR_EACCES = GPG_ERR_SYSTEM_ERROR | 1,
    GPG_ERR_EADDRINUSE = GPG_ERR_SYSTEM_ERROR | 2,
    GPG_ERR_EADDRNOTAVAIL = GPG_ERR_SYSTEM_ERROR | 3,
    GPG_ERR_EADV = GPG_ERR_SYSTEM_ERROR | 4,
    GPG_ERR_EAFNOSUPPORT = GPG_ERR_SYSTEM_ERROR | 5,
    GPG_ERR_EAGAIN = GPG_ERR_SYSTEM_ERROR | 6,
    GPG_ERR_EALREADY = GPG_ERR_SYSTEM_ERROR | 7,
    GPG_ERR_EAUTH = GPG_ERR_SYSTEM_ERROR | 8,
    GPG_ERR_EBACKGROUND = GPG_ERR_SYSTEM_ERROR | 9,
    GPG_ERR_EBADE = GPG_ERR_SYSTEM_ERROR | 10,
    GPG_ERR_EBADF = GPG_ERR_SYSTEM_ERROR | 11,
    GPG_ERR_EBADFD = GPG_ERR_SYSTEM_ERROR | 12,
    GPG_ERR_EBADMSG = GPG_ERR_SYSTEM_ERROR | 13,
    GPG_ERR_EBADR = GPG_ERR_SYSTEM_ERROR | 14,
    GPG_ERR_EBADRPC = GPG_ERR_SYSTEM_ERROR | 15,
    GPG_ERR_EBADRQC = GPG_ERR_SYSTEM_ERROR | 16,
    GPG_ERR_EBADSLT = GPG_ERR_SYSTEM_ERROR | 17,
    GPG_ERR_EBFONT = GPG_ERR_SYSTEM_ERROR | 18,
    GPG_ERR_EBUSY = GPG_ERR_SYSTEM_ERROR | 19,
    GPG_ERR_ECANCELED = GPG_ERR_SYSTEM_ERROR | 20,
    GPG_ERR_ECHILD = GPG_ERR_SYSTEM_ERROR | 21,
    GPG_ERR_ECHRNG = GPG_ERR_SYSTEM_ERROR | 22,
    GPG_ERR_ECOMM = GPG_ERR_SYSTEM_ERROR | 23,
    GPG_ERR_ECONNABORTED = GPG_ERR_SYSTEM_ERROR | 24,
    GPG_ERR_ECONNREFUSED = GPG_ERR_SYSTEM_ERROR | 25,
    GPG_ERR_ECONNRESET = GPG_ERR_SYSTEM_ERROR | 26,
    GPG_ERR_ED = GPG_ERR_SYSTEM_ERROR | 27,
    GPG_ERR_EDEADLK = GPG_ERR_SYSTEM_ERROR | 28,
    GPG_ERR_EDEADLOCK = GPG_ERR_SYSTEM_ERROR | 29,
    GPG_ERR_EDESTADDRREQ = GPG_ERR_SYSTEM_ERROR | 30,
    GPG_ERR_EDIED = GPG_ERR_SYSTEM_ERROR | 31,
    GPG_ERR_EDOM = GPG_ERR_SYSTEM_ERROR | 32,
    GPG_ERR_EDOTDOT = GPG_ERR_SYSTEM_ERROR | 33,
    GPG_ERR_EDQUOT = GPG_ERR_SYSTEM_ERROR | 34,
    GPG_ERR_EEXIST = GPG_ERR_SYSTEM_ERROR | 35,
    GPG_ERR_EFAULT = GPG_ERR_SYSTEM_ERROR | 36,
    GPG_ERR_EFBIG = GPG_ERR_SYSTEM_ERROR | 37,
    GPG_ERR_EFTYPE = GPG_ERR_SYSTEM_ERROR | 38,
    GPG_ERR_EGRATUITOUS = GPG_ERR_SYSTEM_ERROR | 39,
    GPG_ERR_EGREGIOUS = GPG_ERR_SYSTEM_ERROR | 40,
    GPG_ERR_EHOSTDOWN = GPG_ERR_SYSTEM_ERROR | 41,
    GPG_ERR_EHOSTUNREACH = GPG_ERR_SYSTEM_ERROR | 42,
    GPG_ERR_EIDRM = GPG_ERR_SYSTEM_ERROR | 43,
    GPG_ERR_EIEIO = GPG_ERR_SYSTEM_ERROR | 44,
    GPG_ERR_EILSEQ = GPG_ERR_SYSTEM_ERROR | 45,
    GPG_ERR_EINPROGRESS = GPG_ERR_SYSTEM_ERROR | 46,
    GPG_ERR_EINTR = GPG_ERR_SYSTEM_ERROR | 47,
    GPG_ERR_EINVAL = GPG_ERR_SYSTEM_ERROR | 48,
    GPG_ERR_EIO = GPG_ERR_SYSTEM_ERROR | 49,
    GPG_ERR_EISCONN = GPG_ERR_SYSTEM_ERROR | 50,
    GPG_ERR_EISDIR = GPG_ERR_SYSTEM_ERROR | 51,
    GPG_ERR_EISNAM = GPG_ERR_SYSTEM_ERROR | 52,
    GPG_ERR_EL2HLT = GPG_ERR_SYSTEM_ERROR | 53,
    GPG_ERR_EL2NSYNC = GPG_ERR_SYSTEM_ERROR | 54,
    GPG_ERR_EL3HLT = GPG_ERR_SYSTEM_ERROR | 55,
    GPG_ERR_EL3RST = GPG_ERR_SYSTEM_ERROR | 56,
    GPG_ERR_ELIBACC = GPG_ERR_SYSTEM_ERROR | 57,
    GPG_ERR_ELIBBAD = GPG_ERR_SYSTEM_ERROR | 58,
    GPG_ERR_ELIBEXEC = GPG_ERR_SYSTEM_ERROR | 59,
    GPG_ERR_ELIBMAX = GPG_ERR_SYSTEM_ERROR | 60,
    GPG_ERR_ELIBSCN = GPG_ERR_SYSTEM_ERROR | 61,
    GPG_ERR_ELNRNG = GPG_ERR_SYSTEM_ERROR | 62,
    GPG_ERR_ELOOP = GPG_ERR_SYSTEM_ERROR | 63,
    GPG_ERR_EMEDIUMTYPE = GPG_ERR_SYSTEM_ERROR | 64,
    GPG_ERR_EMFILE = GPG_ERR_SYSTEM_ERROR | 65,
    GPG_ERR_EMLINK = GPG_ERR_SYSTEM_ERROR | 66,
    GPG_ERR_EMSGSIZE = GPG_ERR_SYSTEM_ERROR | 67,
    GPG_ERR_EMULTIHOP = GPG_ERR_SYSTEM_ERROR | 68,
    GPG_ERR_ENAMETOOLONG = GPG_ERR_SYSTEM_ERROR | 69,
    GPG_ERR_ENAVAIL = GPG_ERR_SYSTEM_ERROR | 70,
    GPG_ERR_ENEEDAUTH = GPG_ERR_SYSTEM_ERROR | 71,
    GPG_ERR_ENETDOWN = GPG_ERR_SYSTEM_ERROR | 72,
    GPG_ERR_ENETRESET = GPG_ERR_SYSTEM_ERROR | 73,
    GPG_ERR_ENETUNREACH = GPG_ERR_SYSTEM_ERROR | 74,
    GPG_ERR_ENFILE = GPG_ERR_SYSTEM_ERROR | 75,
    GPG_ERR_ENOANO = GPG_ERR_SYSTEM_ERROR | 76,
    GPG_ERR_ENOBUFS = GPG_ERR_SYSTEM_ERROR | 77,
    GPG_ERR_ENOCSI = GPG_ERR_SYSTEM_ERROR | 78,
    GPG_ERR_ENODATA = GPG_ERR_SYSTEM_ERROR | 79,
    GPG_ERR_ENODEV = GPG_ERR_SYSTEM_ERROR | 80,
    GPG_ERR_ENOENT = GPG_ERR_SYSTEM_ERROR | 81,
    GPG_ERR_ENOEXEC = GPG_ERR_SYSTEM_ERROR | 82,
    GPG_ERR_ENOLCK = GPG_ERR_SYSTEM_ERROR | 83,
    GPG_ERR_ENOLINK = GPG_ERR_SYSTEM_ERROR | 84,
    GPG_ERR_ENOMEDIUM = GPG_ERR_SYSTEM_ERROR | 85,
    GPG_ERR_ENOMEM = GPG_ERR_SYSTEM_ERROR | 86,
    GPG_ERR_ENOMSG = GPG_ERR_SYSTEM_ERROR | 87,
    GPG_ERR_ENONET = GPG_ERR_SYSTEM_ERROR | 88,
    GPG_ERR_ENOPKG = GPG_ERR_SYSTEM_ERROR | 89,
    GPG_ERR_ENOPROTOOPT = GPG_ERR_SYSTEM_ERROR | 90,
    GPG_ERR_ENOSPC = GPG_ERR_SYSTEM_ERROR | 91,
    GPG_ERR_ENOSR = GPG_ERR_SYSTEM_ERROR | 92,
    GPG_ERR_ENOSTR = GPG_ERR_SYSTEM_ERROR | 93,
    GPG_ERR_ENOSYS = GPG_ERR_SYSTEM_ERROR | 94,
    GPG_ERR_ENOTBLK = GPG_ERR_SYSTEM_ERROR | 95,
    GPG_ERR_ENOTCONN = GPG_ERR_SYSTEM_ERROR | 96,
    GPG_ERR_ENOTDIR = GPG_ERR_SYSTEM_ERROR | 97,
    GPG_ERR_ENOTEMPTY = GPG_ERR_SYSTEM_ERROR | 98,
    GPG_ERR_ENOTNAM = GPG_ERR_SYSTEM_ERROR | 99,
    GPG_ERR_ENOTSOCK = GPG_ERR_SYSTEM_ERROR | 100,
    GPG_ERR_ENOTSUP = GPG_ERR_SYSTEM_ERROR | 101,
    GPG_ERR_ENOTTY = GPG_ERR_SYSTEM_ERROR | 102,
    GPG_ERR_ENOTUNIQ = GPG_ERR_SYSTEM_ERROR | 103,
    GPG_ERR_ENXIO = GPG_ERR_SYSTEM_ERROR | 104,
    GPG_ERR_EOPNOTSUPP = GPG_ERR_SYSTEM_ERROR | 105,
    GPG_ERR_EOVERFLOW = GPG_ERR_SYSTEM_ERROR | 106,
    GPG_ERR_EPERM = GPG_ERR_SYSTEM_ERROR | 107,
    GPG_ERR_EPFNOSUPPORT = GPG_ERR_SYSTEM_ERROR | 108,
    GPG_ERR_EPIPE = GPG_ERR_SYSTEM_ERROR | 109,
    GPG_ERR_EPROCLIM = GPG_ERR_SYSTEM_ERROR | 110,
    GPG_ERR_EPROCUNAVAIL = GPG_ERR_SYSTEM_ERROR | 111,
    GPG_ERR_EPROGMISMATCH = GPG_ERR_SYSTEM_ERROR | 112,
    GPG_ERR_EPROGUNAVAIL = GPG_ERR_SYSTEM_ERROR | 113,
    GPG_ERR_EPROTO = GPG_ERR_SYSTEM_ERROR | 114,
    GPG_ERR_EPROTONOSUPPORT = GPG_ERR_SYSTEM_ERROR | 115,
    GPG_ERR_EPROTOTYPE = GPG_ERR_SYSTEM_ERROR | 116,
    GPG_ERR_ERANGE = GPG_ERR_SYSTEM_ERROR | 117,
    GPG_ERR_EREMCHG = GPG_ERR_SYSTEM_ERROR | 118,
    GPG_ERR_EREMOTE = GPG_ERR_SYSTEM_ERROR | 119,
    GPG_ERR_EREMOTEIO = GPG_ERR_SYSTEM_ERROR | 120,
    GPG_ERR_ERESTART = GPG_ERR_SYSTEM_ERROR | 121,
    GPG_ERR_EROFS = GPG_ERR_SYSTEM_ERROR | 122,
    GPG_ERR_ERPCMISMATCH = GPG_ERR_SYSTEM_ERROR | 123,
    GPG_ERR_ESHUTDOWN = GPG_ERR_SYSTEM_ERROR | 124,
    GPG_ERR_ESOCKTNOSUPPORT = GPG_ERR_SYSTEM_ERROR | 125,
    GPG_ERR_ESPIPE = GPG_ERR_SYSTEM_ERROR | 126,
    GPG_ERR_ESRCH = GPG_ERR_SYSTEM_ERROR | 127,
    GPG_ERR_ESRMNT = GPG_ERR_SYSTEM_ERROR | 128,
    GPG_ERR_ESTALE = GPG_ERR_SYSTEM_ERROR | 129,
    GPG_ERR_ESTRPIPE = GPG_ERR_SYSTEM_ERROR | 130,
    GPG_ERR_ETIME = GPG_ERR_SYSTEM_ERROR | 131,
    GPG_ERR_ETIMEDOUT = GPG_ERR_SYSTEM_ERROR | 132,
    GPG_ERR_ETOOMANYREFS = GPG_ERR_SYSTEM_ERROR | 133,
    GPG_ERR_ETXTBSY = GPG_ERR_SYSTEM_ERROR | 134,
    GPG_ERR_EUCLEAN = GPG_ERR_SYSTEM_ERROR | 135,
    GPG_ERR_EUNATCH = GPG_ERR_SYSTEM_ERROR | 136,
    GPG_ERR_EUSERS = GPG_ERR_SYSTEM_ERROR | 137,
    GPG_ERR_EWOULDBLOCK = GPG_ERR_SYSTEM_ERROR | 138,
    GPG_ERR_EXDEV = GPG_ERR_SYSTEM_ERROR | 139,
    GPG_ERR_EXFULL = GPG_ERR_SYSTEM_ERROR | 140,

    /* This is one more than the largest allowed entry.  */
    GPG_ERR_CODE_DIM = 65536
  } gpg_err_code_t;


/* The error value type gpg_error_t.  */

/* We would really like to use bit-fields in a struct, but using
   structs as return values can cause binary compatibility issues, in
   particular if you want to do it effeciently (also see
   -freg-struct-return option to GCC).  */
typedef unsigned int gpg_error_t;

/* We use the lowest 16 bits of gpg_error_t for error codes.  The 16th
   bit indicates system errors.  */
#define GPG_ERR_CODE_MASK	(GPG_ERR_CODE_DIM - 1)

/* Bits 17 to 24 are reserved.  */

/* We use the upper 7 bits of gpg_error_t for error sources.  */
#define GPG_ERR_SOURCE_MASK	(GPG_ERR_SOURCE_DIM - 1)
#define GPG_ERR_SOURCE_SHIFT	24

/* The highest bit is reserved.  It shouldn't be used to prevent
   potential negative numbers when transmitting error values as
   text.  */


/* GCC feature test.  */
#if __GNUC__
# define _GPG_ERR_GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
#else
# define _GPG_ERR_GCC_VERSION 0
#endif

#undef _GPG_ERR_HAVE_CONSTRUCTOR
#if _GPG_ERR_GCC_VERSION > 30100
# define _GPG_ERR_CONSTRUCTOR	__attribute__ ((__constructor__))
# define _GPG_ERR_HAVE_CONSTRUCTOR
#else
# define _GPG_ERR_CONSTRUCTOR
#endif

#if _GPG_ERR_GCC_VERSION >= 40400
# define _GPGRT_GCC_A_PRINTF(f, a) __attribute__ ((format(__gnu_printf__,f,a)))
#elif _GPG_ERR_GCC_VERSION >= 20500
# define _GPGRT_GCC_A_PRINTF(f, a) __attribute__ ((format(printf,f,a)))
#else
# define _GPGRT_GCC_A_PRINTF(f, a)
#endif

#if _GPG_ERR_GCC_VERSION >= 29200
# define _GPGRT__RESTRICT __restrict__
#else
# define _GPGRT__RESTRICT
#endif



/* Initialization function.  */

/* Initialize the library.  This function should be run early.  */
gpg_error_t gpg_err_init (void) _GPG_ERR_CONSTRUCTOR;

/* If this is defined, the library is already initialized by the
   constructor and does not need to be initialized explicitely.  */
#undef GPG_ERR_INITIALIZED
#ifdef _GPG_ERR_HAVE_CONSTRUCTOR
# define GPG_ERR_INITIALIZED	1
# define gpgrt_init() do { gpg_err_init (); } while (0)
#else
# define gpgrt_init() do { ; } while (0)
#endif

/* See the source on how to use the deinit function; it is usually not
   required.  */
void gpg_err_deinit (int mode);

/* Register blocking system I/O clamping functions.  */
void gpgrt_set_syscall_clamp (void (*pre)(void), void (*post)(void));

/* Register a custom malloc/realloc/free function.  */
void gpgrt_set_alloc_func  (void *(*f)(void *a, size_t n));



/* Constructor and accessor functions.  */

/* Construct an error value from an error code and source.  Within a
   subsystem, use gpg_error.  */
static GPG_ERR_INLINE gpg_error_t
gpg_err_make (gpg_err_source_t source, gpg_err_code_t code)
{
  return code == GPG_ERR_NO_ERROR ? GPG_ERR_NO_ERROR
    : (((source & GPG_ERR_SOURCE_MASK) << GPG_ERR_SOURCE_SHIFT)
       | (code & GPG_ERR_CODE_MASK));
}


/* The user should define GPG_ERR_SOURCE_DEFAULT before including this
   file to specify a default source for gpg_error.  */
#ifndef GPG_ERR_SOURCE_DEFAULT
#define GPG_ERR_SOURCE_DEFAULT	GPG_ERR_SOURCE_UNKNOWN
#endif

static GPG_ERR_INLINE gpg_error_t
gpg_error (gpg_err_code_t code)
{
  return gpg_err_make (GPG_ERR_SOURCE_DEFAULT, code);
}


/* Retrieve the error code from an error value.  */
static GPG_ERR_INLINE gpg_err_code_t
gpg_err_code (gpg_error_t err)
{
  return (gpg_err_code_t) (err & GPG_ERR_CODE_MASK);
}


/* Retrieve the error source from an error value.  */
static GPG_ERR_INLINE gpg_err_source_t
gpg_err_source (gpg_error_t err)
{
  return (gpg_err_source_t) ((err >> GPG_ERR_SOURCE_SHIFT)
			     & GPG_ERR_SOURCE_MASK);
}


/* String functions.  */

/* Return a pointer to a string containing a description of the error
   code in the error value ERR.  This function is not thread-safe.  */
const char *gpg_strerror (gpg_error_t err);

/* Return the error string for ERR in the user-supplied buffer BUF of
   size BUFLEN.  This function is, in contrast to gpg_strerror,
   thread-safe if a thread-safe strerror_r() function is provided by
   the system.  If the function succeeds, 0 is returned and BUF
   contains the string describing the error.  If the buffer was not
   large enough, ERANGE is returned and BUF contains as much of the
   beginning of the error string as fits into the buffer.  */
int gpg_strerror_r (gpg_error_t err, char *buf, size_t buflen);

/* Return a pointer to a string containing a description of the error
   source in the error value ERR.  */
const char *gpg_strsource (gpg_error_t err);


/* Mapping of system errors (errno).  */

/* Retrieve the error code for the system error ERR.  This returns
   GPG_ERR_UNKNOWN_ERRNO if the system error is not mapped (report
   this). */
gpg_err_code_t gpg_err_code_from_errno (int err);


/* Retrieve the system error for the error code CODE.  This returns 0
   if CODE is not a system error code.  */
int gpg_err_code_to_errno (gpg_err_code_t code);


/* Retrieve the error code directly from the ERRNO variable.  This
   returns GPG_ERR_UNKNOWN_ERRNO if the system error is not mapped
   (report this) and GPG_ERR_MISSING_ERRNO if ERRNO has the value 0. */
gpg_err_code_t gpg_err_code_from_syserror (void);


/* Set the ERRNO variable.  This function is the preferred way to set
   ERRNO due to peculiarities on WindowsCE.  */
void gpg_err_set_errno (int err);

/* Return or check the version.  Both functions are identical.  */
const char *gpgrt_check_version (const char *req_version);
const char *gpg_error_check_version (const char *req_version);

/* The version string of this header. */
#define GPG_ERROR_VERSION "1.18"

/* The version number of this header. */
#define GPG_ERROR_VERSION_NUMBER 0x011200

/* System specific type definitions.  */
#include <sys/types.h>
typedef ssize_t gpgrt_ssize_t;

#include <stdint.h>

typedef int64_t gpgrt_off_t;




/* Self-documenting convenience functions.  */

static GPG_ERR_INLINE gpg_error_t
gpg_err_make_from_errno (gpg_err_source_t source, int err)
{
  return gpg_err_make (source, gpg_err_code_from_errno (err));
}


static GPG_ERR_INLINE gpg_error_t
gpg_error_from_errno (int err)
{
  return gpg_error (gpg_err_code_from_errno (err));
}

static GPG_ERR_INLINE gpg_error_t
gpg_error_from_syserror (void)
{
  return gpg_error (gpg_err_code_from_syserror ());
}



/* Lock functions.  */


typedef struct
{
  long _vers;
  union {
    volatile char _priv[4];
    long _x_align;
    long *_xp_align;
  } u;
} gpgrt_lock_t;

#define GPGRT_LOCK_INITIALIZER {1,{{0,0,0,0}}}


#define GPGRT_LOCK_DEFINE(name) \
  static gpgrt_lock_t name  = GPGRT_LOCK_INITIALIZER

/* NB: If GPGRT_LOCK_DEFINE is not used, zero out the lock variable
   before passing it to gpgrt_lock_init.  */
gpg_err_code_t gpgrt_lock_init (gpgrt_lock_t *lockhd);
gpg_err_code_t gpgrt_lock_lock (gpgrt_lock_t *lockhd);
gpg_err_code_t gpgrt_lock_trylock (gpgrt_lock_t *lockhd);
gpg_err_code_t gpgrt_lock_unlock (gpgrt_lock_t *lockhd);
gpg_err_code_t gpgrt_lock_destroy (gpgrt_lock_t *lockhd);



/* Thread functions.  */

gpg_err_code_t gpgrt_yield (void);




/* Estream */

/* The definition of this struct is entirely private.  You must not
   use it for anything.  It is only here so some functions can be
   implemented as macros.  */
struct _gpgrt_stream_internal;
struct _gpgrt__stream
{
  /* The layout of this struct must never change.  It may be grown,
     but only if all functions which access the new members are
     versioned.  */

  /* Various flags.  */
  struct {
    unsigned int magic: 16;
    unsigned int writing: 1;
    unsigned int reserved: 15;
  } flags;

  /* A pointer to the stream buffer.  */
  unsigned char *buffer;

  /* The size of the buffer in bytes.  */
  size_t buffer_size;

  /* The length of the usable data in the buffer, only valid when in
     read mode (see flags).  */
  size_t data_len;

  /* The current position of the offset pointer, valid in read and
     write mode.  */
  size_t data_offset;

  size_t data_flushed;
  unsigned char *unread_buffer;
  size_t unread_buffer_size;

  /* The number of unread bytes.  */
  size_t unread_data_len;

  /* A pointer to our internal data for this stream.  */
  struct _gpgrt_stream_internal *intern;
};

/* The opaque type for an estream.  */
typedef struct _gpgrt__stream *gpgrt_stream_t;
#ifdef GPGRT_ENABLE_ES_MACROS
typedef struct _gpgrt__stream *estream_t;
#endif

typedef ssize_t (*gpgrt_cookie_read_function_t) (void *cookie,
                                                 void *buffer, size_t size);
typedef ssize_t (*gpgrt_cookie_write_function_t) (void *cookie,
                                                  const void *buffer,
                                                  size_t size);
typedef int (*gpgrt_cookie_seek_function_t) (void *cookie,
                                             gpgrt_off_t *pos, int whence);
typedef int (*gpgrt_cookie_close_function_t) (void *cookie);

struct _gpgrt_cookie_io_functions
{
  gpgrt_cookie_read_function_t func_read;
  gpgrt_cookie_write_function_t func_write;
  gpgrt_cookie_seek_function_t func_seek;
  gpgrt_cookie_close_function_t func_close;
};
typedef struct _gpgrt_cookie_io_functions gpgrt_cookie_io_functions_t;
#ifdef GPGRT_ENABLE_ES_MACROS
typedef struct _gpgrt_cookie_io_functions  es_cookie_io_functions_t;
#define es_cookie_read_function_t  gpgrt_cookie_read_function_t
#define es_cookie_write_function_t gpgrt_cookie_read_function_t
#define es_cookie_seek_function_t  gpgrt_cookie_read_function_t
#define es_cookie_close_function_t gpgrt_cookie_read_function_t
#endif

enum gpgrt_syshd_types
  {
    GPGRT_SYSHD_NONE = 0,  /* No system handle available.                   */
    GPGRT_SYSHD_FD = 1,    /* A file descriptor as returned by open().      */
    GPGRT_SYSHD_SOCK = 2,  /* A socket as returned by socket().             */
    GPGRT_SYSHD_RVID = 3,  /* A rendevous id (see libassuan's gpgcedev.c).  */
    GPGRT_SYSHD_HANDLE = 4 /* A HANDLE object (Windows).                    */
  };

struct _gpgrt_syshd
{
  enum gpgrt_syshd_types type;
  union {
    int fd;
    int sock;
    int rvid;
    void *handle;
  } u;
};
typedef struct _gpgrt_syshd gpgrt_syshd_t;
#ifdef GPGRT_ENABLE_ES_MACROS
typedef struct _gpgrt_syshd es_syshd_t;
#define ES_SYSHD_NONE   GPGRT_SYSHD_NONE
#define ES_SYSHD_FD     GPGRT_SYSHD_FD
#define ES_SYSHD_SOCK   GPGRT_SYSHD_SOCK
#define ES_SYSHD_RVID   GPGRT_SYSHD_RVID
#define ES_SYSHD_HANDLE GPGRT_SYSHD_HANDLE
#endif

gpgrt_stream_t gpgrt_fopen (const char *_GPGRT__RESTRICT path,
                            const char *_GPGRT__RESTRICT mode);
gpgrt_stream_t gpgrt_mopen (void *_GPGRT__RESTRICT data,
                            size_t data_n, size_t data_len,
                            unsigned int grow,
                            void *(*func_realloc) (void *mem, size_t size),
                            void (*func_free) (void *mem),
                            const char *_GPGRT__RESTRICT mode);
gpgrt_stream_t gpgrt_fopenmem (size_t memlimit,
                               const char *_GPGRT__RESTRICT mode);
gpgrt_stream_t gpgrt_fopenmem_init (size_t memlimit,
                                    const char *_GPGRT__RESTRICT mode,
                                    const void *data, size_t datalen);
gpgrt_stream_t gpgrt_fdopen    (int filedes, const char *mode);
gpgrt_stream_t gpgrt_fdopen_nc (int filedes, const char *mode);
gpgrt_stream_t gpgrt_sysopen    (gpgrt_syshd_t *syshd, const char *mode);
gpgrt_stream_t gpgrt_sysopen_nc (gpgrt_syshd_t *syshd, const char *mode);
gpgrt_stream_t gpgrt_fpopen    (FILE *fp, const char *mode);
gpgrt_stream_t gpgrt_fpopen_nc (FILE *fp, const char *mode);
gpgrt_stream_t gpgrt_freopen (const char *_GPGRT__RESTRICT path,
                              const char *_GPGRT__RESTRICT mode,
                              gpgrt_stream_t _GPGRT__RESTRICT stream);
gpgrt_stream_t gpgrt_fopencookie (void *_GPGRT__RESTRICT cookie,
                                  const char *_GPGRT__RESTRICT mode,
                                  gpgrt_cookie_io_functions_t functions);
int gpgrt_fclose (gpgrt_stream_t stream);
int gpgrt_fclose_snatch (gpgrt_stream_t stream,
                         void **r_buffer, size_t *r_buflen);
int gpgrt_onclose (gpgrt_stream_t stream, int mode,
                   void (*fnc) (gpgrt_stream_t, void*), void *fnc_value);
int gpgrt_fileno (gpgrt_stream_t stream);
int gpgrt_fileno_unlocked (gpgrt_stream_t stream);
int gpgrt_syshd (gpgrt_stream_t stream, gpgrt_syshd_t *syshd);
int gpgrt_syshd_unlocked (gpgrt_stream_t stream, gpgrt_syshd_t *syshd);

void _gpgrt_set_std_fd (int no, int fd);
gpgrt_stream_t _gpgrt_get_std_stream (int fd);

#define gpgrt_stdin  _gpgrt_get_std_stream (0)
#define gpgrt_stdout _gpgrt_get_std_stream (1)
#define gpgrt_stderr _gpgrt_get_std_stream (2)


void gpgrt_flockfile (gpgrt_stream_t stream);
int  gpgrt_ftrylockfile (gpgrt_stream_t stream);
void gpgrt_funlockfile (gpgrt_stream_t stream);

int gpgrt_feof (gpgrt_stream_t stream);
int gpgrt_feof_unlocked (gpgrt_stream_t stream);
int gpgrt_ferror (gpgrt_stream_t stream);
int gpgrt_ferror_unlocked (gpgrt_stream_t stream);
void gpgrt_clearerr (gpgrt_stream_t stream);
void gpgrt_clearerr_unlocked (gpgrt_stream_t stream);

int _gpgrt_pending (gpgrt_stream_t stream);          /* (private) */
int _gpgrt_pending_unlocked (gpgrt_stream_t stream); /* (private) */

#define gpgrt_pending(stream) _gpgrt_pending (stream)

#define gpgrt_pending_unlocked(stream)				\
  (((!(stream)->flags.writing)					\
    && (((stream)->data_offset < (stream)->data_len)		\
        || ((stream)->unread_data_len)))                        \
   ? 1 : _gpgrt_pending_unlocked ((stream)))

int gpgrt_fflush (gpgrt_stream_t stream);
int gpgrt_fseek (gpgrt_stream_t stream, long int offset, int whence);
int gpgrt_fseeko (gpgrt_stream_t stream, gpgrt_off_t offset, int whence);
long int gpgrt_ftell (gpgrt_stream_t stream);
gpgrt_off_t gpgrt_ftello (gpgrt_stream_t stream);
void gpgrt_rewind (gpgrt_stream_t stream);

int gpgrt_fgetc (gpgrt_stream_t stream);
int gpgrt_fputc (int c, gpgrt_stream_t stream);

int _gpgrt_getc_underflow (gpgrt_stream_t stream);       /* (private) */
int _gpgrt_putc_overflow (int c, gpgrt_stream_t stream); /* (private) */

#define gpgrt_getc_unlocked(stream)				\
  (((!(stream)->flags.writing)					\
    && ((stream)->data_offset < (stream)->data_len)		\
    && (! (stream)->unread_data_len))				\
  ? ((int) (stream)->buffer[((stream)->data_offset)++])		\
  : _gpgrt_getc_underflow ((stream)))

#define gpgrt_putc_unlocked(c, stream)				\
  (((stream)->flags.writing					\
    && ((stream)->data_offset < (stream)->buffer_size)		\
    && (c != '\n'))						\
  ? ((int) ((stream)->buffer[((stream)->data_offset)++] = (c)))	\
  : _gpgrt_putc_overflow ((c), (stream)))

#define gpgrt_getc(stream)    gpgrt_fgetc (stream)
#define gpgrt_putc(c, stream) gpgrt_fputc (c, stream)

int gpgrt_ungetc (int c, gpgrt_stream_t stream);

int gpgrt_read (gpgrt_stream_t _GPGRT__RESTRICT stream,
                void *_GPGRT__RESTRICT buffer, size_t bytes_to_read,
                size_t *_GPGRT__RESTRICT bytes_read);
int gpgrt_write (gpgrt_stream_t _GPGRT__RESTRICT stream,
                 const void *_GPGRT__RESTRICT buffer, size_t bytes_to_write,
                 size_t *_GPGRT__RESTRICT bytes_written);
int gpgrt_write_sanitized (gpgrt_stream_t _GPGRT__RESTRICT stream,
                           const void *_GPGRT__RESTRICT buffer, size_t length,
                           const char *delimiters,
                           size_t *_GPGRT__RESTRICT bytes_written);
int gpgrt_write_hexstring (gpgrt_stream_t _GPGRT__RESTRICT stream,
                           const void *_GPGRT__RESTRICT buffer, size_t length,
                           int reserved,
                           size_t *_GPGRT__RESTRICT bytes_written);

size_t gpgrt_fread (void *_GPGRT__RESTRICT ptr, size_t size, size_t nitems,
                    gpgrt_stream_t _GPGRT__RESTRICT stream);
size_t gpgrt_fwrite (const void *_GPGRT__RESTRICT ptr, size_t size, size_t memb,
                     gpgrt_stream_t _GPGRT__RESTRICT stream);

char *gpgrt_fgets (char *_GPGRT__RESTRICT s, int n,
                   gpgrt_stream_t _GPGRT__RESTRICT stream);
int gpgrt_fputs (const char *_GPGRT__RESTRICT s,
                 gpgrt_stream_t _GPGRT__RESTRICT stream);
int gpgrt_fputs_unlocked (const char *_GPGRT__RESTRICT s,
                          gpgrt_stream_t _GPGRT__RESTRICT stream);

ssize_t gpgrt_getline (char *_GPGRT__RESTRICT *_GPGRT__RESTRICT lineptr,
                       size_t *_GPGRT__RESTRICT n,
                       gpgrt_stream_t stream);
ssize_t gpgrt_read_line (gpgrt_stream_t stream,
                         char **addr_of_buffer, size_t *length_of_buffer,
                         size_t *max_length);
void gpgrt_free (void *a);

int gpgrt_fprintf (gpgrt_stream_t _GPGRT__RESTRICT stream,
                   const char *_GPGRT__RESTRICT format, ...)
                   _GPGRT_GCC_A_PRINTF(2,3);
int gpgrt_fprintf_unlocked (gpgrt_stream_t _GPGRT__RESTRICT stream,
                            const char *_GPGRT__RESTRICT format, ...)
                            _GPGRT_GCC_A_PRINTF(2,3);

int gpgrt_printf (const char *_GPGRT__RESTRICT format, ...)
                  _GPGRT_GCC_A_PRINTF(1,2);
int gpgrt_printf_unlocked (const char *_GPGRT__RESTRICT format, ...)
                           _GPGRT_GCC_A_PRINTF(1,2);

int gpgrt_vfprintf (gpgrt_stream_t _GPGRT__RESTRICT stream,
                    const char *_GPGRT__RESTRICT format, va_list ap)
                    _GPGRT_GCC_A_PRINTF(2,0);
int gpgrt_vfprintf_unlocked (gpgrt_stream_t _GPGRT__RESTRICT stream,
                             const char *_GPGRT__RESTRICT format, va_list ap)
                             _GPGRT_GCC_A_PRINTF(2,0);

int gpgrt_setvbuf (gpgrt_stream_t _GPGRT__RESTRICT stream,
                   char *_GPGRT__RESTRICT buf, int mode, size_t size);
void gpgrt_setbuf (gpgrt_stream_t _GPGRT__RESTRICT stream,
                   char *_GPGRT__RESTRICT buf);

void gpgrt_set_binary (gpgrt_stream_t stream);

gpgrt_stream_t gpgrt_tmpfile (void);

void gpgrt_opaque_set (gpgrt_stream_t _GPGRT__RESTRICT stream,
                       void *_GPGRT__RESTRICT opaque);
void *gpgrt_opaque_get (gpgrt_stream_t stream);

void gpgrt_fname_set (gpgrt_stream_t stream, const char *fname);
const char *gpgrt_fname_get (gpgrt_stream_t stream);

int gpgrt_asprintf (char **r_buf, const char * _GPGRT__RESTRICT format, ...)
                    _GPGRT_GCC_A_PRINTF(2,3);
int gpgrt_vasprintf (char **r_buf, const char * _GPGRT__RESTRICT format,
                     va_list ap)
                     _GPGRT_GCC_A_PRINTF(2,0);
char *gpgrt_bsprintf (const char * _GPGRT__RESTRICT format, ...)
                      _GPGRT_GCC_A_PRINTF(1,2);
char *gpgrt_vbsprintf (const char * _GPGRT__RESTRICT format, va_list ap)
                       _GPGRT_GCC_A_PRINTF(1,0);
int gpgrt_snprintf (char *buf, size_t bufsize,
                    const char * _GPGRT__RESTRICT format, ...)
                    _GPGRT_GCC_A_PRINTF(3,4);
int gpgrt_vsnprintf (char *buf,size_t bufsize,
                     const char * _GPGRT__RESTRICT format, va_list arg_ptr)
                     _GPGRT_GCC_A_PRINTF(3,0);


#ifdef GPGRT_ENABLE_ES_MACROS
# define es_fopen             gpgrt_fopen
# define es_mopen             gpgrt_mopen
# define es_fopenmem          gpgrt_fopenmem
# define es_fopenmem_init     gpgrt_fopenmem_init
# define es_fdopen            gpgrt_fdopen
# define es_fdopen_nc         gpgrt_fdopen_nc
# define es_sysopen           gpgrt_sysopen
# define es_sysopen_nc        gpgrt_sysopen_nc
# define es_fpopen            gpgrt_fpopen
# define es_fpopen_nc         gpgrt_fpopen_nc
# define es_freopen           gpgrt_freopen
# define es_fopencookie       gpgrt_fopencookie
# define es_fclose            gpgrt_fclose
# define es_fclose_snatch     gpgrt_fclose_snatch
# define es_onclose           gpgrt_onclose
# define es_fileno            gpgrt_fileno
# define es_fileno_unlocked   gpgrt_fileno_unlocked
# define es_syshd             gpgrt_syshd
# define es_syshd_unlocked    gpgrt_syshd_unlocked
# define es_stdin             _gpgrt_get_std_stream (0)
# define es_stdout            _gpgrt_get_std_stream (1)
# define es_stderr            _gpgrt_get_std_stream (2)
# define es_flockfile         gpgrt_flockfile
# define es_ftrylockfile      gpgrt_ftrylockfile
# define es_funlockfile       gpgrt_funlockfile
# define es_feof              gpgrt_feof
# define es_feof_unlocked     gpgrt_feof_unlocked
# define es_ferror            gpgrt_ferror
# define es_ferror_unlocked   gpgrt_ferror_unlocked
# define es_clearerr          gpgrt_clearerr
# define es_clearerr_unlocked gpgrt_clearerr_unlocked
# define es_pending           gpgrt_pending
# define es_pending_unlocked  gpgrt_pending_unlocked
# define es_fflush            gpgrt_fflush
# define es_fseek             gpgrt_fseek
# define es_fseeko            gpgrt_fseeko
# define es_ftell             gpgrt_ftell
# define es_ftello            gpgrt_ftello
# define es_rewind            gpgrt_rewind
# define es_fgetc             gpgrt_fgetc
# define es_fputc             gpgrt_fputc
# define es_getc_unlocked     gpgrt_getc_unlocked
# define es_putc_unlocked     gpgrt_putc_unlocked
# define es_getc              gpgrt_getc
# define es_putc              gpgrt_putc
# define es_ungetc            gpgrt_ungetc
# define es_read              gpgrt_read
# define es_write             gpgrt_write
# define es_write_sanitized   gpgrt_write_sanitized
# define es_write_hexstring   gpgrt_write_hexstring
# define es_fread             gpgrt_fread
# define es_fwrite            gpgrt_fwrite
# define es_fgets             gpgrt_fgets
# define es_fputs             gpgrt_fputs
# define es_fputs_unlocked    gpgrt_fputs_unlocked
# define es_getline           gpgrt_getline
# define es_read_line         gpgrt_read_line
# define es_free              gpgrt_free
# define es_fprintf           gpgrt_fprintf
# define es_fprintf_unlocked  gpgrt_fprintf_unlocked
# define es_printf            gpgrt_printf
# define es_printf_unlocked   gpgrt_printf_unlocked
# define es_vfprintf          gpgrt_vfprintf
# define es_vfprintf_unlocked gpgrt_vfprintf_unlocked
# define es_setvbuf           gpgrt_setvbuf
# define es_setbuf            gpgrt_setbuf
# define es_set_binary        gpgrt_set_binary
# define es_tmpfile           gpgrt_tmpfile
# define es_opaque_set        gpgrt_opaque_set
# define es_opaque_get        gpgrt_opaque_get
# define es_fname_set         gpgrt_fname_set
# define es_fname_get         gpgrt_fname_get
# define es_asprintf          gpgrt_asprintf
# define es_vasprintf         gpgrt_vasprintf
# define es_bsprintf          gpgrt_bsprintf
# define es_vbsprintf         gpgrt_vbsprintf
#endif /*GPGRT_ENABLE_ES_MACROS*/

#ifdef __cplusplus
}
#endif
#endif	/* GPG_ERROR_H */
/*
Local Variables:
buffer-read-only: t
End:
*/
