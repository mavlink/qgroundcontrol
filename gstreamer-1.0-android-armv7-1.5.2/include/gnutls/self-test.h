/*
 * Copyright (C) 2014 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifndef __GNUTLS_SELF_H
#define __GNUTLS_SELF_H

#include <gnutls/gnutls.h>

 /* Self checking functions */
 
 /* The functions are not part of the main API, and are conditionally
  * enabled. */

int gnutls_cipher_self_test(unsigned all, gnutls_cipher_algorithm_t cipher);
int gnutls_mac_self_test(unsigned all, gnutls_mac_algorithm_t mac);
int gnutls_digest_self_test(unsigned all, gnutls_digest_algorithm_t digest);
int gnutls_pk_self_test(unsigned all, gnutls_pk_algorithm_t pk);

#endif
