#include "oregon.h"
#include "../blocks/decoder.h"
#include "../blocks/generic.h"

#define TAG "SubGhzProtocolOregon"


typedef enum {
    OregonDecoderStepReset = 0,
    OregonDecoderStepFoundStartBit,
    OregonDecoderStepSaveDuration,
    OregonDecoderStepCheckDuration,
} OregonDecoderStep;


const SubGhzProtocolDecoder subghz_protocol_oregon_decoder = {
    .alloc = subghz_protocol_decoder_oregon_alloc,
    .free = subghz_protocol_decoder_oregon_free,

    .feed = subghz_protocol_decoder_oregon_feed,
    .reset = subghz_protocol_decoder_oregon_reset,

    .get_hash_data = subghz_protocol_decoder_oregon_get_hash_data,
    .serialize = subghz_protocol_decoder_oregon_serialize,
    .deserialize = subghz_protocol_decoder_oregon_deserialize,
    .get_string = subghz_protocol_decoder_oregon_get_string,
};


struct SubGhzProtocolDecoderOregon {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
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
    FURI_LOG_I(TAG, "Alloc done");
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
    FURI_LOG_I(TAG, "Reset");
}

void subghz_protocol_decoder_oregon_feed(void* context, bool level, uint32_t duration) {
    UNUSED(context);
    UNUSED(level);
    UNUSED(duration);
}


uint8_t subghz_protocol_decoder_oregon_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

bool subghz_protocol_decoder_oregon_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzPresetDefinition* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

bool subghz_protocol_decoder_oregon_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    bool ret = false;
    do {
        if(!subghz_block_generic_deserialize(&instance->generic, flipper_format)) {
            break;
        }
//        if(instance->generic.data_count_bit !=
//           subghz_protocol_oregon_const.min_count_bit_for_found) {
//            FURI_LOG_E(TAG, "Wrong number of bits in key");
//            break;
//        }
        ret = true;
    } while(false);
    return ret;
}

void subghz_protocol_decoder_oregon_get_string(void* context, string_t output) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
//    subghz_protocol_oregon_check_remote_controller(&instance->generic);
    string_cat_printf(
        output,
        "%s %dbit\r\n",
//        "Key:%02lX%08lX\r\n"
//        "Btn:%lX\r\n"
//        "DIP:" DIP_PATTERN "\r\n",
        instance->generic.protocol_name,
        instance->generic.data_count_bit
//        (uint32_t)(instance->generic.data >> 32) & 0xFFFFFFFF,
//        (uint32_t)(instance->generic.data & 0xFFFFFFFF),
//        instance->generic.btn,
//        CNT_TO_DIP(instance->generic.cnt)
    );
}
