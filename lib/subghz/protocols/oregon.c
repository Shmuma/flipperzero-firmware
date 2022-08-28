#include "oregon.h"
#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"
#include <lib/toolbox/manchester_decoder.h>
#include <m-string.h>

#define TAG "SubGhzProtocolOregon"


static const SubGhzBlockConst oregon_const = {
    .te_long = 1000,
    .te_short = 500,
    .te_delta = 200,
    .min_count_bit_for_found = 32,
};

#define OREGON_V2_PREAMBLE_BITS 19
#define OREGON_V2_PREAMBLE_MASK ((1 << (OREGON_V2_PREAMBLE_BITS+1)) - 1)
#define OREGON_V2_CHECKSUM_BITS 8

// 15 ones + 0101 (inverted A)
#define OREGON_V2_PREAMBLE 0b1111111111111110101

// bit indicating the low battery
#define OREGON_V2_FLAG_BAT_LOW 0x4


struct SubGhzProtocolDecoderOregon {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;
    ManchesterState manchester_state;
    bool prev_bit;
    bool have_bit;

    uint16_t sensor_id;
    uint8_t var_bits;
    uint32_t var_data;
};


typedef enum {
    OregonDecoderStepReset = 0,
    OregonDecoderStepFoundPreamble,
    OregonDecoderStepVarData,
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


const SubGhzProtocol subghz_protocol_oregon = {
    .name = SUBGHZ_PROTOCOL_OREGON_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable |
            SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save, //| SubGhzProtocolFlag_Send,

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


// From sensor id code return amount of bits in variable section
static uint8_t oregon_sensor_id_var_bits(uint16_t sensor_id) {
    if (sensor_id == 0xEC40)
        return 16;
    return 0;
}


void subghz_protocol_decoder_oregon_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    // oregon signal is inverted
    ManchesterEvent event = level_and_duration_to_event(!level, duration);
    bool data;

    // low-level bit sequence decoding
    if (event == ManchesterEventReset) {
        instance->decoder.parser_step = OregonDecoderStepReset;
        instance->have_bit = false;
        instance->decoder.decode_data = 0UL;
        instance->decoder.decode_count_bit = 0;
    }
    if (manchester_advance(
        instance->manchester_state,
        event,
        &instance->manchester_state,
        &data))
    {
        if (instance->have_bit) {
            if(!instance->prev_bit && data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 1);
            } else if(instance->prev_bit && !data) {
                subghz_protocol_blocks_add_bit(&instance->decoder, 0);
            } else {
                subghz_protocol_decoder_oregon_reset(context);
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
            instance->decoder.parser_step = OregonDecoderStepFoundPreamble;
            instance->decoder.decode_count_bit = 0;
            instance->decoder.decode_data = 0UL;
        }
    } else if (instance->decoder.parser_step == OregonDecoderStepFoundPreamble) {
        if (instance->decoder.decode_count_bit == 32) {
            instance->generic.data = instance->decoder.decode_data;
            instance->generic.data_count_bit = instance->decoder.decode_count_bit;
            instance->decoder.decode_data = 0UL;
            instance->decoder.decode_count_bit = 0;

            // reverse nibbles in decoded data
            instance->generic.data = (instance->generic.data & 0x55555555) << 1 |
                                     (instance->generic.data & 0xAAAAAAAA) >> 1;
            instance->generic.data = (instance->generic.data & 0x33333333) << 2 |
                                     (instance->generic.data & 0xCCCCCCCC) >> 2;

            instance->sensor_id = (instance->generic.data >> 16) & 0xFFFF;
            instance->var_bits = oregon_sensor_id_var_bits(instance->sensor_id);

            if (!instance->var_bits) {
                // sensor is not supported, stop decoding
                instance->decoder.parser_step = OregonDecoderStepReset;
                if(instance->base.callback)
                    instance->base.callback(&instance->base, instance->base.context);
            } else {
                instance->decoder.parser_step = OregonDecoderStepVarData;
            }
        }
    }
    else if (instance->decoder.parser_step == OregonDecoderStepVarData) {
        if (instance->decoder.decode_count_bit == instance->var_bits + OREGON_V2_CHECKSUM_BITS) {
            instance->var_data = instance->decoder.decode_data & 0xFFFFFFFF;

            // reverse nibbles in var data
            instance->var_data = (instance->var_data & 0x55555555) << 1 |
                                 (instance->var_data & 0xAAAAAAAA) >> 1;
            instance->var_data = (instance->var_data & 0x33333333) << 2 |
                                 (instance->var_data & 0xCCCCCCCC) >> 2;

            FURI_LOG_I(TAG, "Var data: %lX", instance->var_data);

            instance->decoder.parser_step = OregonDecoderStepReset;
            if(instance->base.callback)
                instance->base.callback(&instance->base, instance->base.context);
        }
    }
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
        if(instance->generic.data_count_bit != oregon_const.min_count_bit_for_found) {
            FURI_LOG_E(TAG, "Wrong number of bits in key: %d", instance->generic.data_count_bit);
            break;
        }
        ret = true;
    } while(false);
    return ret;
}


// append string of the variable data
static void oregon_var_data_append_string(uint16_t sensor_id, uint32_t var_data, string_t output) {
    uint32_t val;
    uint8_t checksum = var_data & 0xFF;
    var_data >>= 8;

    if (sensor_id == 0xEC40) {
        val = ((var_data >> 4) & 0xF) * 10 + ((var_data >> 8) & 0xF);
        string_cat_printf(
            output,
            "Temp: %s%d.%d C\r\n",
            (var_data & 0xF) ? "-" : "+",
            val,
            (uint32_t)(var_data >> 12) & 0xF
        );
    }

    string_cat_printf(
        output,
        "CRC: %X",
        checksum
    );
}


void subghz_protocol_decoder_oregon_get_string(void* context, string_t output) {
    furi_assert(context);
    SubGhzProtocolDecoderOregon* instance = context;
    string_cat_printf(
        output,
        "%s v2.1\r\n"
        "ID: 0x%04lX, ch: %d%s\r\n"
        "Rolling: 0x%02lX\r\n",
        instance->generic.protocol_name,
        instance->sensor_id,
        (uint32_t)(instance->generic.data >> 12) & 0xF,
        ((instance->generic.data & OREGON_V2_FLAG_BAT_LOW) ? ", low bat" : ""),
        (uint32_t)(instance->generic.data >> 4) & 0xFF
    );

    if (instance->var_bits)
        oregon_var_data_append_string(instance->sensor_id, instance->var_data, output);
}
