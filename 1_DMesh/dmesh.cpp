

#include	"CGBL_dmesh.h"
#include	"dmesh.h"
#include	"ApproximateMatch.h"
#include	"RegionToRegionMap.h"

#include	"Disk.h"
#include	"ImageIO.h"


/* --------------------------------------------------------------- */
/* Macros -------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Types --------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Globals ------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Statics ------------------------------------------------------- */
/* --------------------------------------------------------------- */






/* --------------------------------------------------------------- */
/* RoughMatch ---------------------------------------------------- */
/* --------------------------------------------------------------- */

static bool RoughMatch(
	vector<TForm>		&guesses,
	const PixPair		&px,
	const ConnRegion	&acr,
	const ConnRegion	&bcr,
	FILE*				flog )
{
	if( guesses.size() > 0 )
		return true;

	if( GBL.A.layer == GBL.B.layer ||
		GBL.arg.NoFolds || !GBL.thm.FLD ) {

		return ApproximateMatch_NoCR( guesses, px, flog );
	}
	else
		return ApproximateMatch( guesses, px, acr, bcr, flog );
}

/* --------------------------------------------------------------- */
/* CheckTransforms ----------------------------------------------- */
/* --------------------------------------------------------------- */

// All checking done at scaled (downsampled) sizes.
//
static int CheckTransforms( const vector<TForm> &vT, FILE* flog )
{
	int	nXY, nT = vT.size();

	if( nT > 1 ) {

		// compute value spread for each transform element

		double			ang_min = PI;
		double			ang_max = -PI;
		vector<double>	pmax( 6, -1e30 );
		vector<double>	pmin( 6,  1e30 );
		vector<double>	pspan( 6 );
		bool			extreme = false;

		// collect maxima and minima

		for( int i = 0; i < nT; ++i ) {

			double	ang = atan2( vT[i].t[3], vT[i].t[0] );

			// handle atan() branch cuts

			if( i > 0 ) {

				if( ang > ang_max + PI )
					ang = ang - 2.0*PI;
				else if( ang < ang_min - PI )
					ang = ang + 2.0*PI;
			}

			ang_min = fmin( ang_min, ang );
			ang_max = fmax( ang_max, ang );

			for( int j = 0; j < 6; ++j ) {
				pmax[j] = max( pmax[j], vT[i].t[j] );
				pmin[j] = min( pmin[j], vT[i].t[j] );
			}
		}

		fprintf( flog,
		"Pipe: Angle span: min, max, delta = %f %f %f.\n",
		ang_min, ang_max, ang_max - ang_min );

		// now calculate spans

		for( int j = 0; j < 6; ++j ) {

			pspan[j] = pmax[j] - pmin[j];

			if( j == 2 || j == 5 )	// translation
				extreme |= pspan[j] > GBL.msh.LDC;
			else					// rotation
				extreme |= pspan[j] > GBL.msh.LDR;
		}

		// any extreme spans?

		if( ang_max - ang_min > GBL.msh.LDA || extreme ) {

			fprintf( flog,
			"STAT: Pipe: Triangles too different:"
			" angles %f %f %f; limit %f.\n",
			ang_min, ang_max, ang_max-ang_min, GBL.msh.LDA );

			fprintf( flog,
			"STAT: Pipe: Triangles too different:"
			" elements %f %f %f %f %f %f; limits %f %f.\n",
			pspan[0], pspan[1], pspan[2],
			pspan[3], pspan[4], pspan[5],
			GBL.msh.LDR, GBL.msh.LDC );

			nT = 0;
		}
	}

// Translations in bounds?

	if( nT && (nXY = GBL.XYusr.size()) ) {

		bool	allok = true;

		for( int i = 0; i < nT; ++i ) {

			bool	iok = false;

			// iok true if ANY bound satisfied

			for( int j = 0; j < nXY; ++j ) {

				Point	p( vT[i].t[2], vT[i].t[5] );

				iok |= p.Dist( GBL.XYusr[j] ) <= GBL.msh.DXY;
			}

			if( !iok ) {

				fprintf( flog,
				"Pipe: Transform translation (%f %f)"
				" not on allowed list, tolerance %f.\n",
				vT[i].t[2], vT[i].t[5], GBL.msh.DXY );
			}

			allok &= iok;
		}

		if( !allok ) {

			fprintf( flog,
			"STAT: Pipe: Transform not on allowed list.\n" );

			nT = 0;
		}
	}

	return nT;
}

/* --------------------------------------------------------------- */
/* UpscaleCoords ------------------------------------------------- */
/* --------------------------------------------------------------- */

static void UpscaleCoords( ffmap &maps, int scale )
{
	int	nT;

	if( scale > 1 && (nT = maps.transforms.size()) ) {

		for( int i = 0; i < nT; ++i ) {

			maps.transforms[i].MulXY( scale );

			maps.centers[i].x *= scale;
			maps.centers[i].y *= scale;
		}
	}
}

/* --------------------------------------------------------------- */
/* WritePOINTEntries --------------------------------------------- */
/* --------------------------------------------------------------- */

// CPOINT entries are reported at full size.
//
static void WritePOINTEntries(
	const ffmap		&maps,
	const uint8*	fold_mask_a,
	const uint8*	fold_mask_b,
	int				wf,
	int				hf,
	FILE*			flog )
{
	CMutex	M;
	char	name[256];
	char	*sud;

// set pts file type and layer

	if( GBL.A.layer < GBL.B.layer )
		sud = "up";
	else if( GBL.A.layer == GBL.B.layer )
		sud = "same";
	else
		sud = "down";

	sprintf( name, "%s_%d", sud, GBL.A.layer );

	if( M.Get( name ) ) {

		sprintf( name, "pts.%s", sud );
		FILE *f = fopen( name, "a" );

		if( f ) {

			int	nT = maps.transforms.size();

			for( int i = 0; i < nT; ++i ) {

				Point	pa = maps.centers[i],
						pb = pa;
				int		ma, mb, ix, iy;

				// Lookup for A-point

				ix = int(pa.x);
				iy = int(pa.y);

				if( ix >= 0 && ix < wf && iy >= 0 && iy < hf ) {

					ma = fold_mask_a[ix + wf*iy];

					if( ma <= 0 ) {

						fprintf( flog,
						"CPOINT: A-centroid has bad mask value:"
						" mask=%d @ (%d, %d).\n", ma, ix, iy );

						continue;
					}
				}
				else {

					fprintf( flog,
					"CPOINT: A-centroid out of A-image bounds"
					" (%d, %d).\n", ix, iy );

					continue;
				}

				// Lookup for B-point

				maps.transforms[i].Transform( pb );

				ix = int(pb.x);
				iy = int(pb.y);

				if( ix >= 0 && ix < wf && iy >= 0 && iy < hf ) {

					mb = fold_mask_b[ix + wf*iy];

					if( mb <= 0 ) {

						fprintf( flog,
						"CPOINT: B-centroid has bad mask value:"
						" mask=%d @ (%d, %d).\n", mb, ix, iy );

						continue;
					}
				}
				else {

					fprintf( flog,
					"CPOINT: B-centroid out of B-image bounds"
					" (%d, %d).\n", ix, iy );

					continue;
				}

				// Write entry

				fprintf( f,
				"CPOINT"
				" '%s' %d.%d:%d %f %f"
				" '%s' %d.%d:%d %f %f\n",
				GBL.A.t2i.path,
				GBL.A.layer, GBL.A.tile, ma, pa.x, pa.y,
				GBL.B.t2i.path,
				GBL.B.layer, GBL.B.tile, mb, pb.x, pb.y );
			}

			fflush( f );
			fclose( f );
		}
	}

	M.Release();
}

/* --------------------------------------------------------------- */
/* PipelineDeformableMap ----------------------------------------- */
/* --------------------------------------------------------------- */

// The main routine the pipeline should call.
//
// Ntrans		- returned tform count
// tr_array		- array of these values
// map_mask		- <10=no map, else tform[pix-10]
// px			- a and b image pixels
// fold_mask_a	- 0=fold, 1,2,3...=region #
// fold_mask_b	- 0=fold, 1,2,3...=region #
// flog			- detailed output
//
void PipelineDeformableMap(
	int				&Ntrans,
	double*			&tr_array,
	uint16*			map_mask,
	const PixPair	&px,
	const uint8*	fold_mask_a,
	const uint8*	fold_mask_b,
	FILE*			flog )
{
	int	wf = px.wf, hf = px.hf;

/* --------------- */
/* Default results */
/* --------------- */

	Ntrans		= 0;
	tr_array	= NULL;

	memset( map_mask, 0, wf * hf * sizeof(uint16) );

/* --------------------------------- */
/* Create the connected region lists */
/* --------------------------------- */

// Note that the connected region lists are always
// at the reduced resolution, if this is used.

	fprintf( flog, "\n---- Connected regions ----\n" );

	vector<ConnRegion>	Acr, Bcr;

	if( GBL.A.layer == GBL.B.layer ||
		GBL.arg.NoFolds || !GBL.thm.FLD ) {

		fprintf( flog, "Forcing single connected region.\n" );

		ConnRgnForce1( Acr, px.ws, px.hs );
		Bcr = Acr;
	}
	else {

		ConnRgnsFromFoldMask( Acr, fold_mask_a,
			wf, hf, px.scl, uint32(0.9 * GBL.msh.MMA), flog );

		ConnRgnsFromFoldMask( Bcr, fold_mask_b,
			wf, hf, px.scl, uint32(0.9 * GBL.msh.MMA), flog );
	}

/* ----------------------------------------- */
/* Find mappings for each region-region pair */
/* ----------------------------------------- */

	ffmap	maps;  // transforms and centers
	FILE	*ftri	= NULL;

	//ftri = fopen( "Triangles.txt", "w" );

	for( int i = 0; i < Acr.size(); ++i ) {

		for( int j = 0; j < Bcr.size(); ++j ) {

			fprintf( flog, "\n---- Begin A-%d to B-%d ----\n",
			Acr[i].id, Bcr[j].id );

			// start with transform arguments

			vector<TForm>	guesses = GBL.Tusr;

			if( RoughMatch( guesses, px, Acr[i], Bcr[j], flog ) ) {

				// num = how many approximate transforms
				// result in good matches

				int		num = 0;

				for( int k = 0; k < guesses.size(); ++k ) {

					fprintf( flog, "\nSTAT: Pipe: try guess %d.\n", k );

					int		before = maps.transforms.size();

					// Downscale coordinates
					guesses[k].MulXY( 1.0 / px.scl );

					RegionToRegionMap( maps, map_mask,
						px, Acr[i], Bcr[j],
						guesses[k], flog, ftri );

					num += (maps.transforms.size() != before);

					// BLOOT - this makes next warning useless
					if( maps.transforms.size() != before )
						break;
				}

				if( num > 1 ) {
					fprintf( flog,
					"Pipe: Warning! %d approximate"
					" transforms matched!\n", num );
				}
			}
		}
	}

	//if( ftri )
	//	fclose( ftri );

/* -------------------- */
/* Sanity check results */
/* -------------------- */

	if( maps.transforms.size() )
		Ntrans = CheckTransforms( maps.transforms, flog );

	fprintf( flog,
	"Pipe: Returning %d transforms.\n", Ntrans );

/* ------ */
/* Report */
/* ------ */

	if( Ntrans ) {

		UpscaleCoords( maps, px.scl );

		WritePOINTEntries( maps, fold_mask_a, fold_mask_b,
			wf, hf, flog );

		tr_array = (double*)malloc( Ntrans * 6 * sizeof(double) );

		for( int i = 0; i < Ntrans; ++i ) {

			maps.transforms[i].ToMatlab();
			maps.transforms[i].CopyOut( tr_array + i*6 );
		}
	}
	else
		memset( map_mask, 0, wf * hf * sizeof(uint16) );
}

