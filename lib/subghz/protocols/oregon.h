#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_OREGON_NAME "Oregon"

typedef struct SubGhzProtocolDecoderOregon SubGhzProtocolDecoderOregon;

extern const SubGhzProtocolDecoder subghz_protocol_oregon_decoder;
extern const SubGhzProtocol subghz_protocol_oregon;

/**
 * Allocate SubGhzProtocolDecoderOregon.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return SubGhzProtocolDecoderOregon* pointer to a SubGhzProtocolDecoderOregon instance
 */
void* subghz_protocol_decoder_oregon_alloc(SubGhzEnvironment* environment);

/**
 * Free SubGhzProtocolDecoderOregon.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 */
void subghz_protocol_decoder_oregon_free(void* context);

/**
 * Reset decoder SubGhzProtocolDecoderOregon.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 */
void subghz_protocol_decoder_oregon_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void subghz_protocol_decoder_oregon_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 * @return hash Hash sum
 */
uint8_t subghz_protocol_decoder_oregon_get_hash_data(void* context);

/**
 * Serialize data SubGhzProtocolDecoderOregon.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzPresetDefinition
 * @return true On success
 */
bool subghz_protocol_decoder_oregon_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzPresetDefinition* preset);

/**
 * Deserialize data SubGhzProtocolDecoderOregon.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return true On success
 */
bool subghz_protocol_decoder_oregon_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a SubGhzProtocolDecoderOregon instance
 * @param output Resulting text
 */
void subghz_protocol_decoder_oregon_get_string(void* context, string_t output);
