#include "oregon.h"
#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"
#include <lib/toolbox/manchester_decoder.h>

#define TAG "SubGhzProtocolOregon"


static const SubGhzBlockConst oregon_const = {
    .te_long = 1000,
    .te_short = 500,
    .te_delta = 200
};

#define OREGON_V2_PREAMBLE_BITS 19
#define OREGON_V2_PREAMBLE_MASK ((1 << (OREGON_V2_PREAMBLE_BITS+1)) - 1)

// 15 ones + 0101 (inverted A)
#define OREGON_V2_PREAMBLE 0b1111111111111110101



struct SubGhzProtocolDecoderOregon {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    bool prev_bit;
    bool have_bit;
};


typedef enum {
    OregonDecoderStepReset = 0,
    OregonDecoderStepFoundPreamble,
} OregonDecoderStep;


const SubGhzProtocolDecoder subghz_protocol_oregon_decoder = {
    .alloc = subghz_protocol_decoder_oregon_alloc,
    .free = subghz_protocol_decoder_oregon_free,

    .feed = subghz_protocol_decoder_oregon_feed,
    .reset = subghz_protocol_decoder_oregon_reset,

//    .get_hash_data = subghz_protocol_decoder_oregon_get_hash_data,
//    .serialize = subghz_protocol_decoder_oregon_serialize,
//    .deserialize = subghz_protocol_decoder_oregon_deserialize,
//    .get_string = subghz_protocol_decoder_oregon_get_string,
};


const SubGhzProtocol subghz_protocol_oregon = {
    .name = SUBGHZ_PROTOCOL_OREGON_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable |
            SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,

    .decoder = &subghz_protocol_oregon_decoder,
//    .encoder = &subghz_protocol_oregon_encoder,
};


void* subghz_protocol_decoder_oregon_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderOregon* instance = malloc(sizeof(SubGhzProtocolDecoderOregon));
    instance->base.protocol = &subghz_protocol_oregon;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}


void subghz_protocol_decoder_oregon_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    free(instance);
}


void subghz_protocol_decoder_oregon_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    instance->decoder.parser_step = OregonDecoderStepReset;
    instance->decoder.decode_data = 0UL;
    instance->decoder.decode_count_bit = 0;
    manchester_advance(
        instance->manchester_state,
        ManchesterEventReset,
        &instance->manchester_state,
        NULL);
    instance->prev_bit = false;
    instance->have_bit = false;
}


ManchesterEvent level_and_duration_to_event(bool level, uint32_t duration) {
    bool is_long = false;

    if (DURATION_DIFF(duration, oregon_const.te_long) < oregon_const.te_delta) {
        is_long = true;
    } else if (DURATION_DIFF(duration, oregon_const.te_short) < oregon_const.te_delta) {
        is_long = false;
    } else {
        return ManchesterEventReset;
    }

    if (level)
        if (is_long)
            return ManchesterEventLongHigh;
        else
            return ManchesterEventShortHigh;
    else
        if (is_long)
            return ManchesterEventLongLow;
        else
            return ManchesterEventShortLow;
}


void subghz_protocol_decoder_oregon_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    // oregon signal is inverted
    ManchesterEvent event = level_and_duration_to_event(!level, duration);
    bool data;

    if (!level) {
        if (duration > 9000) {
            FURI_LOG_I(TAG, "_____________ %lu", duration);
        }
    }

    // low-level bit sequence decoding
    if (event == ManchesterEventReset) {
        instance->prev_bit = false;
        instance->have_bit = false;
    }
    if (manchester_advance(
        instance->manchester_state,
        event,
        &instance->manchester_state,
        &data))
    {
        FURI_LOG_I(TAG, "Man: %d", data);
        if (instance->have_bit) {
            if(!instance->prev_bit && data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
            } else if(instance->prev_bit && !data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
            } else {
                subghz_protocol_decoder_oregon_reset(context);
                FURI_LOG_I(TAG, "Wrong bit combination, reset");
            }
            instance->have_bit = false;
        }
        else {
            instance->prev_bit = data;
            instance->have_bit = true;
        }
    }

    if (instance->decoder.parser_step == OregonDecoderStepReset) {
        // check for preamble + sync bits
        if (instance->decoder.decode_count_bit >= OREGON_V2_PREAMBLE_BITS &&
            ((instance->decoder.decode_data & OREGON_V2_PREAMBLE_MASK) == OREGON_V2_PREAMBLE))
        {
            FURI_LOG_I(TAG, "Oregon v2.1 preamble detected!");
            instance->decoder.parser_step = OregonDecoderStepFoundPreamble;
        }
    }
}


//uint8_t subghz_protocol_decoder_oregon_get_hash_data(void* context) {
//    furi_assert(context);
//    SubGhzProtocolDecoderOregon* instance = context;
//    return subghz_protocol_blocks_get_hash_data(
//        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
//}
//
//bool subghz_protocol_decoder_oregon_serialize(
//    void* context,
//    FlipperFormat* flipper_format,
//    SubGhzPresetDefinition* preset) {
//    furi_assert(context);
//    SubGhzProtocolDecoderOregon* instance = context;
//    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
//}
//
//bool subghz_protocol_decoder_oregon_deserialize(void* context, FlipperFormat* flipper_format) {
//    furi_assert(context);
//    SubGhzProtocolDecoderOregon* instance = context;
//    bool ret = false;
//    do {
//        if(!subghz_block_generic_deserialize(&instance->generic, flipper_format)) {
//            break;
//        }
////        if(instance->generic.data_count_bit !=
////           subghz_protocol_oregon_const.min_count_bit_for_found) {
////            FURI_LOG_E(TAG, "Wrong number of bits in key");
////            break;
////        }
//        ret = true;
//    } while(false);
//    return ret;
//}
//
//void subghz_protocol_decoder_oregon_get_string(void* context, string_t output) {
//    furi_assert(context);
//    SubGhzProtocolDecoderOregon* instance = context;
////    subghz_protocol_oregon_check_remote_controller(&instance->generic);
//    string_cat_printf(
//        output,
//        "%s %dbit\r\n",
////        "Key:%02lX%08lX\r\n"
////        "Btn:%lX\r\n"
////        "DIP:" DIP_PATTERN "\r\n",
//        instance->generic.protocol_name,
//        instance->generic.data_count_bit
////        (uint32_t)(instance->generic.data >> 32) & 0xFFFFFFFF,
////        (uint32_t)(instance->generic.data & 0xFFFFFFFF),
////        instance->generic.btn,
////        CNT_TO_DIP(instance->generic.cnt)
//    );
//}
