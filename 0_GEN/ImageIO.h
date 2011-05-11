

#pragma once


#include	"GenDefs.h"
#include	"CPoint.h"

#include	<stdio.h>


/* --------------------------------------------------------------- */
/* General ------------------------------------------------------- */
/* --------------------------------------------------------------- */

void* RasterAlloc( unsigned long bytes );
void  RasterFree( void* raster );

uint8* Raster8FromAny(
	const char*	name,
	uint32		&w,
	uint32		&h,
	FILE*		flog,
	bool		transpose = false );

/* --------------------------------------------------------------- */
/* Tif ----------------------------------------------------------- */
/* --------------------------------------------------------------- */

uint8* Raster8FromTif(
	const char*	name,
	uint32		&w,
	uint32		&h,
	FILE*		flog,
	bool		transpose = false );

uint16* Raster16FromTif16(
	const char*	name,
	uint32		&w,
	uint32		&h,
	FILE*		flog = stdout );

void Raster8ToTifFlt(
	const char*		name,
	const uint8*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

void Raster8ToTif8(
	const char*		name,
	const uint8*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

void Raster32ToTifRGBA(
	const char*		name,
	const uint32*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

void Raster16ToTif8(
	const char*		name,
	const uint16*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

void CorrThmToTif8(
	const char*				name,
	const vector<double>	&I,
	int						iw,
	int						tw,
	int						th,
	FILE*					flog = stdout );

void VectorDblToTif8(
	const char*				name,
	const vector<double>	&vals,
	int						w,
	int						h,
	FILE*					flog = stdout );

void VectorDblToTif8(
	const vector<Point>		&pts,
	const vector<double>	&vals,
	int						id,
	FILE*					flog = stdout );

/* --------------------------------------------------------------- */
/* Png ----------------------------------------------------------- */
/* --------------------------------------------------------------- */

uint16* Raster16FromPng(
	const char*	name,
	uint32		&w,
	uint32		&h,
	FILE*		flog = stdout );

uint8* Raster8FromPng(
	const char*	name,
	uint32		&w,
	uint32		&h,
	FILE*		flog = stdout );

void Raster8ToPng8(
	const char*		name,
	const uint8*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

void Raster16ToPng16(
	const char*		name,
	const uint16*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

void Raster32ToPngRGBA(
	const char*		name,
	const uint32*	raster,
	int				w,
	int				h,
	FILE*			flog = stdout );

