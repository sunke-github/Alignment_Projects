

#include	"CGBL_dmesh.h"

#include	"Cmdline.h"


/* --------------------------------------------------------------- */
/* Constants ----------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Macros -------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Types --------------------------------------------------------- */
/* --------------------------------------------------------------- */

/* --------------------------------------------------------------- */
/* Globals ------------------------------------------------------- */
/* --------------------------------------------------------------- */

CGBL_dmesh	GBL;

/* --------------------------------------------------------------- */
/* Statics ------------------------------------------------------- */
/* --------------------------------------------------------------- */






/* --------------------------------------------------------------- */
/* Object management --------------------------------------------- */
/* --------------------------------------------------------------- */

CGBL_dmesh::CGBL_dmesh()
{
	arg.SCALE			= 999.0;
	arg.XSCALE			= 999.0;
	arg.YSCALE			= 999.0;
	arg.SKEW			= 999.0;
	arg.CTR				= 999.0;
	arg.ima				= NULL;
	arg.imb				= NULL;
	arg.fma				= NULL;
	arg.fmb				= NULL;
	arg.Transpose		= false;
	arg.WithinSection	= false;
	arg.Verbose			= false;
	arg.NoFolds			= false;
	arg.SingleFold		= false;
	arg.Heatmap			= false;

	A.layer	= 0;
	A.tile	= 0;

	B.layer	= 0;
	B.tile	= 0;
}

/* --------------------------------------------------------------- */
/* SetCmdLine ---------------------------------------------------- */
/* --------------------------------------------------------------- */

bool CGBL_dmesh::SetCmdLine( int argc, char* argv[] )
{
// Parse args

	char			*key;
	vector<double>	vdbl;

	for( int i = 1; i < argc; ++i ) {

		if( argv[i][0] != '-' )
			key = argv[i];
		else if( GetArg( &arg.SCALE, "-SCALE=%lf", argv[i] ) )
			;
		else if( GetArg( &arg.XSCALE, "-XSCALE=%lf", argv[i] ) )
			;
		else if( GetArg( &arg.YSCALE, "-YSCALE=%lf", argv[i] ) )
			;
		else if( GetArg( &arg.SKEW, "-SKEW=%lf", argv[i] ) )
			;
		else if( GetArg( &arg.CTR, "-CTR=%lf", argv[i] ) )
			;
		else if( GetArgStr( arg.ima, "-ima=", argv[i] ) )
			;
		else if( GetArgStr( arg.imb, "-imb=", argv[i] ) )
			;
		else if( GetArgStr( arg.fma, "-fma=", argv[i] ) )
			;
		else if( GetArgStr( arg.fmb, "-fmb=", argv[i] ) )
			;
		else if( IsArg( "-tr", argv[i] ) )
			arg.Transpose = true;
		else if( IsArg( "-ws", argv[i] ) )
			arg.WithinSection = true;
		else if( IsArg( "-nf", argv[i] ) )
			arg.NoFolds = true;
		else if( IsArg( "-sf", argv[i] ) )
			arg.SingleFold = true;
		else if( IsArg( "-v", argv[i] ) )
			arg.Verbose = true;
		else if( IsArg( "-heatmap", argv[i] ) )
			arg.Heatmap = true;
		else if( GetArgList( vdbl, "-TRA=", argv[i] ) ) {

			if( 6 == vdbl.size() )
				Tusr.push_back( TForm( &vdbl[0] ) );
			else {
				printf(
				"main: WARNING: Bad format in -TRA [%s].\n",
				argv[i] );
			}
		}
		else if( GetArgList( vdbl, "-EXY=", argv[i] ) ) {

			if( 2 == vdbl.size() )
				XYusr.push_back( Point( vdbl[0], vdbl[1] ) );
			else {
				printf(
				"main: WARNING: Bad format in -EXY [%s].\n",
				argv[i] );
			}
		}
		else {
			printf( "Did not understand option '%s'.\n", argv[i] );
			return false;
		}
	}

// Decode labels in key

	if( !key ||
		(4 != sscanf( key, "%d/%d@%d/%d",
				&A.layer, &A.tile,
				&B.layer, &B.tile )) ) {

		printf( "main: Usage: ptest <za/ia@zb/ib>.\n" );
		return false;
	}

// Rename stdout using image labels

	OpenPairLog( A.layer, A.tile, B.layer, B.tile );

	printf( "\n---- dmesh start ----\n" );

// Record start time

	time_t	t0 = time( NULL );
	printf( "main: Start: %s\n", ctime(&t0) );

// Get default parameters

	if( !ReadMatchParams( mch, A.layer ) )
		return false;

// Context dependent choices: (same,cross) layer

	if( A.layer == B.layer ) {

		ctx.SCALE	= 1.0;
		ctx.XSCALE	= 1.0;
		ctx.YSCALE	= 1.0;
		ctx.SKEW	= 0.0;
		ctx.NBMXHT	= mch.NBMXHT_SL;
		ctx.HFANGDN	= mch.HFANGDN_SL;
		ctx.HFANGPR	= mch.HFANGPR_SL;
		ctx.RTRSH	= mch.RTRSH_SL;
		ctx.DIT		= mch.DIT_SL;
		ctx.DFA		= mch.DFA_SL;
		ctx.DFT		= mch.DFT_SL;
		ctx.OLAP2D	= mch.OLAP2D_SL;
		ctx.PKWID	= mch.PKWID_SL;
		ctx.PKGRD	= mch.PKGRD_SL;
	}
	else {

		ctx.SCALE	= mch.SCALE;
		ctx.XSCALE	= mch.XSCALE;
		ctx.YSCALE	= mch.YSCALE;
		ctx.SKEW	= mch.SKEW;
		ctx.NBMXHT	= mch.NBMXHT_XL;
		ctx.HFANGDN	= mch.HFANGDN_XL;
		ctx.HFANGPR	= mch.HFANGPR_XL;
		ctx.RTRSH	= mch.RTRSH_XL;
		ctx.DIT		= mch.DIT_XL;
		ctx.DFA		= mch.DFA_XL;
		ctx.DFT		= mch.DFT_XL;
		ctx.OLAP2D	= mch.OLAP2D_XL;
		ctx.PKWID	= mch.PKWID_XL;
		ctx.PKGRD	= mch.PKGRD_XL;
	}

// Commandline overrides

	printf( "\n---- Command-line overrides ----\n" );

	if( A.layer != B.layer ) {

		if( arg.SCALE != 999.0 && arg.SCALE != ctx.SCALE ) {
			ctx.SCALE = arg.SCALE;
			printf( "SCALE=%g\n", arg.SCALE );
		}

		if( arg.XSCALE != 999.0 && arg.XSCALE != ctx.XSCALE ) {
			ctx.XSCALE = arg.XSCALE;
			printf( "XSCALE=%g\n", arg.XSCALE );
		}

		if( arg.YSCALE != 999.0 && arg.YSCALE != ctx.YSCALE ) {
			ctx.YSCALE = arg.YSCALE;
			printf( "YSCALE=%g\n", arg.YSCALE );
		}

		if( arg.SKEW != 999.0 && arg.SKEW != ctx.SKEW ) {
			ctx.SKEW = arg.SKEW;
			printf( "SKEW=%g\n", arg.SKEW );
		}
	}

	if( arg.CTR != 999.0 )
		printf( "CTR=%g\n", arg.CTR );

	printf( "\n" );

// Fetch Til2Img entries

	printf( "\n---- Input images ----\n" );

	IDBReadImgParams( idb );

	if( !IDBTil2Img( A.t2i, idb, A.layer, A.tile, arg.ima ) ||
		!IDBTil2Img( B.t2i, idb, B.layer, B.tile, arg.imb ) ) {

		return false;
	}

	PrintTil2Img( stdout, 'A', A.t2i );
	PrintTil2Img( stdout, 'B', B.t2i );

	printf( "\n" );

// Extract file name as useful label

	A.file = ExtractFilename( A.t2i.path.c_str() );
	B.file = ExtractFilename( B.t2i.path.c_str() );

	return true;
}


