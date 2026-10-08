/*
 *  file = UIB.H
 *  project = RQDX3
 *  author = Stephen F. Shirron
 *
 *  definition of $UIB structure
 *
 *  filler_1		-- header bytes (for data integrity)
 *  xbnsize		-- number of XBNs
 *  dbnsize		-- number of DBNs
 *  lbnsize		-- number of LBNs
 *  rbnsize		-- number of RBNs
 *  sec			-- actual number of sectors (per track)
 *  sur			-- actual number of surfaces
 *  cyl			-- actual number of cylinders
 *  pccyl		-- first cylinder for (write) precompensation
 *  rccyl		-- first cylinder for reduced (write) current
 *  type		-- unit class and model value
 *  crc_or_ecc		-- 0 for CRC, 1 for ECC
 *  rctsize		-- size of unit's replacement control table
 *  rctcopies		-- number of RCT copies
 *  media		-- media identifier (creation of Ed Gardner)
 *  sec_interleave	-- sector-to-sector interleave factor
 *  sur_skew		-- surface-to-surface skew amount
 *  cyl_skew		-- cylinder-to-cylinder skew amount
 *  gap0		-- gap0 value for reformatted tracks
 *  gap1		-- gap1 value for reformatted tracks
 *  gap2		-- gap2 value for reformatted tracks
 *  gap3		-- gap3 value for reformatted tracks
 *  sync		-- sync value for reformatted tracks
 *  filler_2		-- stuff specific to the DUP formatter
 *  volume		-- serial number of unit
 */

struct $uib
    {
    byte		filler_1[10];
    word		xbnsize[2];
    word		dbnsize[2];
    word		lbnsize[2];
    word		rbnsize[2];
    word		sec;
    word		sur;
    word		cyl;
    word		pccyl;
    word		rccyl;
    word		type;
    word		crc_or_ecc;
    word		rctsize;
    word		rctcopies;
    word		media[2];
    word		sec_interleave;
    word		sur_skew;
    word		cyl_skew;
    word		gap0;
    word		gap1;
    word		gap2;
    word		gap3;
    word		sync;
    byte		filler_2[32];
    word		volume[2];
    };
