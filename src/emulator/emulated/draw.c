static void emulated_system_emulate_draw(struct EmulatedSystem *emulated_system) {
    // 0xDXYN: Draw N-height sprite at coords X,Y; Read from memory location I;
    //   Screen pixels are XOR'd with sprite bits, 
    //   VF (Carry flag) is set if any screen pixels are set off; This is useful
    //   for collision detection or other reasons.
    uint32_t desired_window_width = 64; // TODO: fix this
    uint32_t desired_window_height = 32;
    uint8_t X_coord = emulated_system->V[emulated_system->decoded_instruction.register_indexes[0]] % desired_window_width;
    uint8_t Y_coord = emulated_system->V[emulated_system->decoded_instruction.register_indexes[1]] % desired_window_height;
    const uint8_t orig_X = X_coord; // Original X value

    emulated_system->V[0xF] = 0;  // Initialize carry flag to 0

    // Loop over all N rows of the sprite
    for (uint8_t i = 0; i < emulated_system->decoded_instruction.half_value; i++) {
        // Get next byte/row of sprite data
        const uint8_t sprite_data = emulated_system->ram[emulated_system->I + i];
        X_coord = orig_X;   // Reset X for next row to draw

        for (int8_t j = 7; j >= 0; j--) {
            // set carry flag
            bool *pixel = &emulated_system->display[Y_coord * desired_window_width + X_coord]; 
            const bool sprite_bit = (sprite_data & (1 << j));

            if (sprite_bit && *pixel) {
                emulated_system->V[0xF] = 1;  
            }

            // XOR display pixel
            *pixel ^= sprite_bit;

            // Para de desenhar se bater no cantos para esquerda ou direita
            if (++X_coord >= desired_window_width) break;
        }

        if (++Y_coord >= desired_window_height) break;
    }
}