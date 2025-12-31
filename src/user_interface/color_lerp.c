#include "user_interface/color_lerp.h"

uint32_t user_interface_color_lerp(const uint32_t start_color, const uint32_t end_color, const float t) {
    const uint8_t s_r = (start_color >> 24) & 0xFF;
    const uint8_t s_g = (start_color >> 16) & 0xFF;
    const uint8_t s_b = (start_color >> 8)  & 0xFF;
    const uint8_t s_a = (start_color >> 0)  & 0xFF;

    const uint8_t e_r = (end_color >> 24)   & 0xFF;
    const uint8_t e_g = (end_color >> 16)   & 0xFF;
    const uint8_t e_b = (end_color >> 8)    & 0xFF;
    const uint8_t e_a = (end_color >> 0)    & 0xFF;

    const uint8_t ret_r = ((1-t)*(s_r)) + (t*e_r);
    const uint8_t ret_g = ((1-t)*(s_g)) + (t*e_g);
    const uint8_t ret_b = ((1-t)*(s_b)) + (t*e_b);
    const uint8_t ret_a = ((1-t)*(s_a)) + (t*e_a);

    return (ret_r << 24) | (ret_g << 16) | (ret_b << 8) | (ret_a);
}