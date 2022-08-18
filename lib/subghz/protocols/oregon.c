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


struct SubGhzProtocolDecoderOregon {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
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
    manchester_advance(
        instance->manchester_state,
        ManchesterEventReset,
        &instance->manchester_state,
        NULL);
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
//    SubGhzProtocolDecoderOregon* instance = context;
    ManchesterEvent event = level_and_duration_to_event(level, duration);
//    bool data;
//    ManchesterState old_state = instance->manchester_state;

    if (level) {
        if(duration > 9000)
            FURI_LOG_I(TAG, "^^^^^^^^^^^^^^ %lu", duration);
        else
            FURI_LOG_I(TAG, "^^^^ %lu", duration);
    }
    else {
        if (duration > 9000)
            FURI_LOG_I(TAG, "_____________ %lu", duration);
        else
            FURI_LOG_I(TAG, "____ %lu", duration);
    }

    if (event == ManchesterEventLongHigh)
        FURI_LOG_I(TAG, "long high");
    else if (event == ManchesterEventLongLow)
        FURI_LOG_I(TAG, "long low");
    else if (event == ManchesterEventShortHigh)
        FURI_LOG_I(TAG, "short high");
    else if (event == ManchesterEventShortLow)
        FURI_LOG_I(TAG, "short low");
    else
        FURI_LOG_I(TAG, "reset");


//    if (manchester_advance(
//        instance->manchester_state,
//        event,
//        &instance->manchester_state,
//        &data))
//    {
//        FURI_LOG_I(TAG, "New bit: %d", data);
//    }
//    if (old_state != instance->manchester_state) {
//        FURI_LOG_I(TAG, "New state %d -> %d", old_state, instance->manchester_state);
//    }
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
