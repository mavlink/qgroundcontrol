/* 
 gaiamatrix.h -- Gaia common utility functions: affine trasform Matrix
  
 version 4.3, 2015 June 29

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
the Initial Developer. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

/**
 \file gaiamatrix.h

 Auxiliary/helper functions
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef DLL_EXPORT
#define GAIAMATRIX_DECLARE __declspec(dllexport)
#else
#define GAIAMATRIX_DECLARE extern
#endif
#endif

#ifndef _GAIAMATRIX_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIAMATRIX_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Typedef for gaiaAffineTransformMatrix object (opaque, hidden)

 \sa gaiaAffineTransformMatrixPtr
 */
    typedef struct priv_affine_transform gaiaAffineTransformMatrix;
/**
 Typedef for gaiaAffineTransformMatrixPtr object pointer (opaque, hidden)

 \sa gaiaAffineTransformMatrix
 */
    typedef gaiaAffineTransformMatrix *gaiaAffineTransformMatrixPtr;

/* function prototypes */

/**
 Creating a fully initialized BLOB-Matrix
 \param a XX component of the affine transformation.
 \param b XY component of the affine transformation.
 \param c XZ component of the affine transformation.
 \param d YX component of the affine transformation.
 \param e YY component of the affine transformation.
 \param f YZ component of the affine transformation.
 \param g ZX component of the affine transformation.
 \param h ZY component of the affine transformation.
 \param i ZZ component of the affine transformation.
 \param xoff X translation component of the affine transformation.
 \param yoff Y translation component of the affine transformation.
 \param zoff Z translation component of the affine transformation.
 \param blob on completion this variable will contain a BLOB-encoded
  affine transform Matrix
 \param blob_sz on completion this variable will contain the BLOB's size
  (in bytes)

 \return 0 on failure: any other different value on success.

 \sa gaia_matrix_is_valid, gaia_matrix_as_text, 
  gaia_matrix_multiply, gaia_matrix_create_multiply, 
  gaia_matrix_transform_geometry

 \note you are responsible to destroy (before or after) any BLOB
  returned by this function.
 */
    GAIAMATRIX_DECLARE int gaia_matrix_create (double a, double b, double c,
					       double d, double e, double f,
					       double g, double h, double i,
					       double xoff, double yoff,
					       double zoff,
					       unsigned char **blob,
					       int *blob_sz);

/**
 Creating a BLOB-Matrix by multiplying MatrixA by MatrixB
 \param iblob1 pointer to a BLOB-encoded Matrix A
 \param iblob1_sz A BLOB's size (in bytes)
 \param iblob2 pointer to a BLOB-encoded Matrix A
 \param iblob2_sz A BLOB's size (in bytes)
 \param blob on completion this variable will contain a BLOB-encoded
  affine transform Matrix
 \param blob_sz on completion this variable will contain the BLOB's size
  (in bytes)

 \return 0 on failure: any other different value on success.

 \sa gaia_matrix_create, gaia_matrix_is_valid, gaia_matrix_as_text, 
  gaia_matrix_create_multiply, gaia_matrix_transform_geometry,
  gaia_matrix_invert

 \note you are responsible to destroy (before or after) any BLOB
  returned by this function.
 */
    GAIAMATRIX_DECLARE int gaia_matrix_multiply (const unsigned char *iblob1,
						 int iblob1_sz,
						 const unsigned char *iblob2,
						 int iblob2_sz,
						 unsigned char **blob,
						 int *blob_sz);

/**
 Creating a BLOB-Matrix by applying a further trasformation to a previous BLOB-Matrix
 \param iblob pointer to a BLOB-encoded Matrix
 \param iblob_sz BLOB's size (in bytes)
 \param a XX component of the affine transformation.
 \param b XY component of the affine transformation.
 \param c XZ component of the affine transformation.
 \param d YX component of the affine transformation.
 \param e YY component of the affine transformation.
 \param f YZ component of the affine transformation.
 \param g ZX component of the affine transformation.
 \param h ZY component of the affine transformation.
 \param i ZZ component of the affine transformation.
 \param xoff X translation component of the affine transformation.
 \param yoff Y translation component of the affine transformation.
 \param zoff Z translation component of the affine transformation.
 \param blob on completion this variable will contain a BLOB-encoded
  affine transform Matrix
 \param blob_sz on completion this variable will contain the BLOB's size
  (in bytes)

 \return 0 on failure: any other different value on success.

 \sa gaia_matrix_create, gaia_matrix_is_valid, gaia_matrix_as_text, 
  gaia_matrix_multiply, gaia_matrix_transform_geometry

 \note you are responsible to destroy (before or after) any BLOB
  returned by this function.
 */
    GAIAMATRIX_DECLARE int gaia_matrix_create_multiply (const unsigned char
							*iblob, int iblob_sz,
							double a, double b,
							double c, double d,
							double e, double f,
							double g, double h,
							double i, double xoff,
							double yoff,
							double zoff,
							unsigned char **blob,
							int *blob_sz);

/**
 Testing a BLOB-Matrix for validity
 \param blob pointer to a BLOB-encoded Matrix
 \param blob_sz BLOB's size (in bytes)

 \return TRUE if the BLOB really is of the BLOB-Matrix type; FALSE if not.

 \sa gaia_matrix_create, gaia_matrix_as_text, 
  gaia_matrix_multiply, gaia_matrix_create_multiply, 
  gaia_matrix_transform_geometry
 */
    GAIAMATRIX_DECLARE int gaia_matrix_is_valid (const unsigned char *blob,
						 int blob_sz);

/**
 Printing a textual represention from a BLOB-Matrix
 \param blob pointer to a BLOB-encoded Matrix
 \param blob_sz BLOB's size (in bytes)

 \return a text string; NULL on failure.

 \sa gaia_matrix_create, gaia_matrix_is_valid, 
  gaia_matrix_multiply, gaia_matrix_create_multiply, 
  gaia_matrix_transform_geometry
 \note you are responsible to destroy (before or after) any text
  string returned by this function by calling sqlite3_free().
 */
    GAIAMATRIX_DECLARE char *gaia_matrix_as_text (const unsigned char *blob,
						  int blob_sz);

/**
 Transforming a Geometry accordingly to an Affine Transform Matrix
 \param geom the input Geometry
 \param blob pointer to a BLOB-encoded Matrix 
 \param blob_sz BLOB's size (in bytes)

 \return pointer to the transformed Geometry or NULL on failure.

 \sa gaia_matrix_create, gaia_matrix_is_valid, gaia_matrix_as_text, 
  gaia_matrix_multiply, gaia_matrix_create_multiply

 \note you are responsible to destroy (before or after) any Geometry
  returned by this function.
 */
    GAIAMATRIX_DECLARE gaiaGeomCollPtr
	gaia_matrix_transform_geometry (gaiaGeomCollPtr geom,
					const unsigned char *blob, int blob_sz);

/**
 Computing the Determinant from an Affine Transform Matrix
 \param blob pointer to a BLOB-encoded Matrix 
 \param blob_sz BLOB's size (in bytes)

 \return the Determinant of the Matix; 0.0 on invalid args.

 \sa gaia_matrix_create, gaia_matrix_is_valid, gaia_matrix_invert
 \note you are responsible to destroy (before or after) any Geometry
  returned by this function.
 */
    GAIAMATRIX_DECLARE double
	gaia_matrix_determinant (const unsigned char *blob, int blob_sz);

/**
 Creating a BLOB-Matrix by applying a further trasformation to a previous BLOB-Matrix
 \param iblob pointer to a BLOB-encoded Matrix
 \param iblob_sz BLOB's size (in bytes)
 \param blob on completion this variable will contain a BLOB-encoded
  affine transform Matrix (Inverse)
 \param blob_sz on completion this variable will contain the BLOB's size
  (in bytes)

 \return 0 on failure: any other different value on success.
 Note that not all Matrices can be Inverted, only those having
 a valid Determinant.

 \sa gaia_matrix_create, gaia_matrix_is_valid, gaia_matrix_multiply, 
 gaia_matrix_determinant

 \note you are responsible to destroy (before or after) any BLOB
  returned by this function.
 */
    GAIAMATRIX_DECLARE int gaia_matrix_invert (const unsigned char
					       *iblob, int iblob_sz,
					       unsigned char **blob,
					       int *blob_sz);

#ifdef __cplusplus
}
#endif

#endif				/* _GAIAMATRIX_H */
