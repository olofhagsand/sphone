/*
 * iLBCInterface.h
 *
 * This header file contains all of the API's between GIPS and iLBC.
 *
 * Created by: Henrik Åström
 * Date: 020802
 *
 * Copyright (c) 2001
 * Global IP Sound AB, Organization number: 5565739017
 * Rosenlundsgatan 54, SE-118 63 Stockholm, Sweden
 * All rights reserved.
 */

#ifndef _iLBCinterface_H
#define _iLBCinterface_H

/*
 * Define the fixpoint numeric formats
 */

#include "gipsfixdefines.h"

/* 
 * Solution to support multiple instances 
 * Customer has to cast instance to proper type
 */

typedef struct iLBC_encinst_t_ iLBC_encinst_t;

typedef struct iLBC_decinst_t_ iLBC_decinst_t;

/*
 * Comfort noise constants
 */

#define ILBC_GIPS_SPEECH	1
#define ILBC_GIPS_CNG		2

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * iLBCFIXXXX_GIPS_create(...)
 *
 * These functions create a instance to the specified structure
 *
 * Input:
 *      - XXX_inst      : Pointer to created instance that should be created
 *
 * Return value         :  0 - Ok
 *                        -1 - Error
 */

int iLBCFIXENC_GIPS_create(iLBC_encinst_t **iLBC_encinst);
int iLBCFIXDEC_GIPS_create(iLBC_decinst_t **iLBC_decinst);

/****************************************************************************
 * iLBCFIXXXX_GIPS_free(...)
 *
 * These functions frees the dynamic memory of a specified instance
 *
 * Input:
 *      - XXX_inst      : Pointer to created instance that should be freed
 *
 * Return value         :  0 - Ok
 *                        -1 - Error
 */

int iLBCFIXENC_GIPS_free(iLBC_encinst_t *iLBC_encinst);
int iLBCFIXDEC_GIPS_free(iLBC_decinst_t *iLBC_decinst);


/****************************************************************************
 * iLBCFIX_GIPS_encoderinit(...)
 *
 * This function initializes a iLBC instance
 *
 * Input:
 *		- iLBCenc_inst		: iLBC instance, i.e. the user that should receive 
 *							  be initialized
 *
 * Return value				:  0 - Ok
 *							  -1 - Error
 */

Word16 iLBCFIX_GIPS_encoderinit(iLBC_encinst_t *iLBCenc_inst);

/****************************************************************************
 * iLBCFIX_GIPS_encode(...)
 *
 * This function encodes one iLBC frame. Input speech length has be 240 
 * samples.
 *
 * Input:
 *		- iLBCenc_inst		: iLBC instance, i.e. the user that should encode
 *							  a package
 *      - speechIn			: Input speech vector
 *      - len				: Samples in speechIn (240 or 480)
 *
 * Output:
 *		- encoded			: The encoded data vector
 *
 * Return value				: >0 - Length (in bytes) of coded data
 *							  -1 - Error
 */

Word16 iLBCFIX_GIPS_encode(iLBC_encinst_t *iLBCenc_inst, 
						   Word16 *speechIn, 
						   Word16 len, 
						   Word16 *encoded);

/****************************************************************************
 * iLBCFIX_GIPS_decoderinit(...)
 *
 * This function initializes a iLBC instance
 *
 * Input:
 *		- iLBC_decinst_t	: iLBC instance, i.e. the user that should receive 
 *						      be initialized
 *
 * Return value				:  0 - Ok
 *							  -1 - Error
 */

Word16 iLBCFIX_GIPS_decoderinit(iLBC_decinst_t *iLBCdec_inst);

/****************************************************************************
 * iLBCFIX_GIPS_decode(...)
 *
 * This function decodes a packet with iLBC frame(s). Output speech length 
 * will be a multiple of 240 samples (240*frames/packet).
 *
 * Input:
 *		- iLBCdec_inst		: iLBC instance, i.e. the user that should decode
 *							  a packet
 *      - encoded			: Encoded iLBC frame(s)
 *      - len				: Bytes in encoded vector
 *
 * Output:
 *		- decoded			: The decoded vector
 *      - speechType		: 1 normal, 2 CNG
 *
 * Return value				: >0 - Samples in decoded vector
 *							  -1 - Error
 */

Word16 iLBCFIX_GIPS_decode(iLBC_decinst_t *iLBCdec_inst, 
						   Word16 *encoded, 
						   Word16 len, 
						   Word16 *decoded,
						   Word16 *speechType);

/****************************************************************************
 * iLBCFIX_GIPS_decodePLC(...)
 *
 * This function conducts PLC for iLBC frame(s). Output speech length 
 * will be a multiple of 240 samples.
 *
 * Input:
 *		- iLBCdec_inst		: iLBC instance, i.e. the user that should perform
 *							  a PLC
 *      - noOfLostFrames	: Number of PLC frames to produce
 *
 * Output:
 *		- decoded			: The "decoded" vector
 *
 * Return value				: >0 - Samples in decoded PLC vector
 *							  -1 - Error
 */

Word16 iLBCFIX_GIPS_decodePLC(iLBC_decinst_t *iLBCdec_inst, 
							  Word16 *decoded, 
							  Word16 noOfLostFrames);

#ifdef __cplusplus
}
#endif

#endif

