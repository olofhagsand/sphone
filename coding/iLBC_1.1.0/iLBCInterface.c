/*
 * G729Interface.cpp
 *
 * Copyright Global IP Sound AB 2000
 *
 * This source file contains all of the API's between GIPS and iLBC.
 *
 * Created by: Henrik Åström
 * Date: 020802
 */

#include "iLBCInterface.h"
#include "iLBC_define.h"
#include "iLBC_encode.h"
#include "iLBC_decode.h"

#define ILBCFLOAT_GIPS_BLOCKL		BLOCKL
#define ILBCFLOAT_GIPS_NOOFBYTES	NO_OF_BYTES


int iLBCFIXENC_GIPS_create(iLBC_encinst_t **iLBC_encinst) {
	*iLBC_encinst=(iLBC_encinst_t*)malloc(sizeof(iLBC_Enc_Inst_t));
	if (iLBC_encinst!=NULL) {
		return(0);
	} else {
		return(-1);
	}
}

int iLBCFIXDEC_GIPS_create(iLBC_decinst_t **iLBC_decinst) {
	*iLBC_decinst=(iLBC_decinst_t*)malloc(sizeof(iLBC_Dec_Inst_t));
	if (iLBC_decinst!=NULL) {
		return(0);
	} else {
		return(-1);
	}
}

int iLBCFIXENC_GIPS_free(iLBC_encinst_t *iLBC_encinst) {
	free(iLBC_encinst);
	return(0);
}

int iLBCFIXDEC_GIPS_free(iLBC_decinst_t *iLBC_decinst) {
	free(iLBC_decinst);
	return(0);
}


Word16 iLBCFIX_GIPS_encoderinit(iLBC_encinst_t *iLBCenc_inst) 
{
	initEncode((iLBC_Enc_Inst_t*) iLBCenc_inst);
	return(0);
}

Word16 iLBCFIX_GIPS_encode(iLBC_encinst_t *iLBCenc_inst, Word16 *speechIn, Word16 len, Word16 *encoded) {

	if (len!=ILBCFLOAT_GIPS_BLOCKL) {
		return(-1);
	} else {
		float block[ILBCFLOAT_GIPS_BLOCKL];
		int i;
		for (i=0;i<ILBCFLOAT_GIPS_BLOCKL;i++) {
			block[i]=(float)speechIn[i];
		}
		/* call encoder */

		iLBC_encode((unsigned char*) encoded, block, (iLBC_Enc_Inst_t*) iLBCenc_inst);
		return (ILBCFLOAT_GIPS_NOOFBYTES);
	}
}

Word16 iLBCFIX_GIPS_decoderinit(iLBC_decinst_t *iLBCdec_inst) {
	initDecode((iLBC_Dec_Inst_t*) iLBCdec_inst, 1);
	return(0);
}

Word16 iLBCFIX_GIPS_decode(iLBC_decinst_t *iLBCdec_inst, 
						   Word16 *encoded, 
						   Word16 len, 
						   Word16 *decoded,
						   Word16 *speechType)
{
	if (len%ILBCFLOAT_GIPS_NOOFBYTES!=0) {
		return(-1);
	} else {
		int i, k;
		int frames=0;
		if (len==ILBCFLOAT_GIPS_NOOFBYTES) {
			frames=1;
		} else if (len==2*ILBCFLOAT_GIPS_NOOFBYTES) {
			frames=2;
		} else {
			/* Max 2 frames per packet supported */
			return(-1);
		}
		for (i=0;i<frames;i++) {
			float decblock[ILBCFLOAT_GIPS_BLOCKL];

			/* call decoder */
			iLBC_decode(decblock, (unsigned char*) encoded, (iLBC_Dec_Inst_t*) iLBCdec_inst, 1);

			for (k=0;k<ILBCFLOAT_GIPS_BLOCKL;k++) {
				decoded[i*ILBCFLOAT_GIPS_BLOCKL+k]=(Word16)decblock[k];
			}
		}
		/* iLBC does not support VAD/CNG yet */
		*speechType=1;
		return(frames*ILBCFLOAT_GIPS_BLOCKL);
	}
}

Word16 iLBCFIX_GIPS_decodePLC(iLBC_decinst_t *iLBCdec_inst, Word16 *decoded, Word16 noOfLostFrames) {
	int i,k;
	float decblock[ILBCFLOAT_GIPS_BLOCKL];
	unsigned char dummy;

	for (i=0;i<noOfLostFrames;i++) {

		/* call decoder */
		iLBC_decode(decblock, &dummy, (iLBC_Dec_Inst_t*) iLBCdec_inst, 0);

		for (k=0;k<ILBCFLOAT_GIPS_BLOCKL;k++) {
			decoded[i*ILBCFLOAT_GIPS_BLOCKL+k]=(Word16)decblock[k];
		}
	}
	return (noOfLostFrames*ILBCFLOAT_GIPS_BLOCKL);
}

