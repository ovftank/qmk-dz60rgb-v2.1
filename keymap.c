// Copyright 2026 ovftank (@ovftank)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#ifndef uint_farptr_t
typedef uint32_t uint_farptr_t;
#endif
#include QMK_KEYBOARD_H
#include "deferred_exec.h"
#include "process_key_override.h"

enum {
  MOV_A = 1 << 0,
  MOV_D = 1 << 1,
  MOV_W = 1 << 2,
  MOV_S = 1 << 3,
};
static uint8_t movement_state = 0;
static uint16_t last_auto_pair = KC_NO;
static uint16_t last_keycode_pressed = KC_NO;

static const key_override_t tilde_esc_override =
    ko_make_basic(MOD_MASK_SHIFT, KC_ESC, KC_TILDE);
const key_override_t *key_overrides[] = {&tilde_esc_override, NULL};

enum custom_keycodes {
  EML_MAC = SAFE_RANGE,
  PWD_MAC,
};

static deferred_token tap_tokens[4] = {
    INVALID_DEFERRED_TOKEN, INVALID_DEFERRED_TOKEN, INVALID_DEFERRED_TOKEN,
    INVALID_DEFERRED_TOKEN};

static uint8_t get_move_idx(uint16_t keycode) {
  switch (keycode) {
  case KC_W:
    return 0;
  case KC_A:
    return 1;
  case KC_S:
    return 2;
  case KC_D:
    return 3;
  default:
    return 255;
  }
}

static uint32_t unregister_tap_callback(uint32_t trigger_time, void *cb_arg) {
  (void)trigger_time;
  uint16_t keycode = (uintptr_t)cb_arg;
  unregister_code16(keycode);
  uint8_t idx = get_move_idx(keycode);
  if (idx != 255)
    tap_tokens[idx] = INVALID_DEFERRED_TOKEN;
  return 0;
}

static void cancel_tap(uint16_t keycode) {
  uint8_t idx = get_move_idx(keycode);
  if (idx != 255 && tap_tokens[idx] != INVALID_DEFERRED_TOKEN) {
    cancel_deferred_exec(tap_tokens[idx]);
    tap_tokens[idx] = INVALID_DEFERRED_TOKEN;
  }
}

static void apply_counter_strafe(uint8_t key_bit, uint8_t opp_bit, uint16_t key,
                                 uint16_t opp, const keyrecord_t *record) {
  if (record->event.pressed) {
    movement_state |= key_bit;
    cancel_tap(key);
    unregister_code16(opp);
    register_code16(key);
  } else {
    movement_state &= ~key_bit;
    unregister_code16(key);
    if (movement_state & opp_bit) {
      register_code16(opp);
    } else {
      cancel_tap(opp);
      register_code16(opp);
      uint8_t opp_idx = get_move_idx(opp);
      if (opp_idx != 255) {
        tap_tokens[opp_idx] =
            defer_exec(35, unregister_tap_callback, (void *)opp);
      }
    }
  }
}

static void handle_bracket_pair(const char *pair_str, uint16_t auto_pair_val) {
  uint8_t mods = get_mods();
  unregister_mods(MOD_MASK_SHIFT);
  send_string(pair_str);
  tap_code(KC_LEFT);
  register_mods(mods & MOD_MASK_SHIFT);
  last_auto_pair = last_keycode_pressed = auto_pair_val;
}

static void handle_jump_over(uint16_t keycode) {
  uint8_t mods = get_mods();
  unregister_mods(MOD_MASK_SHIFT);
  tap_code(KC_RGHT);
  register_mods(mods & MOD_MASK_SHIFT);
  last_auto_pair = KC_NO;
  last_keycode_pressed = keycode;
}

static bool handle_asterisk_pairing(uint16_t keycode, uint8_t mods) {
  if (keycode == KC_8 && (mods & MOD_MASK_SHIFT)) {
    if (last_keycode_pressed == KC_8) {
      send_string("***");
      uint8_t phys_mods = get_mods();
      unregister_mods(MOD_MASK_SHIFT);
      tap_code(KC_LEFT);
      tap_code(KC_LEFT);
      register_mods(phys_mods & MOD_MASK_SHIFT);
      last_auto_pair = KC_8;
      last_keycode_pressed = KC_NO;
      return false;
    }
    last_keycode_pressed = KC_8;
    last_auto_pair = KC_NO;
    return true;
  }
  return true;
}

static bool handle_standard_pairs(uint16_t keycode, uint8_t mods) {
  switch (keycode) {
  case KC_9:
    if (mods & MOD_MASK_SHIFT) {
      handle_bracket_pair("()", KC_9);
      return false;
    }
    break;
  case KC_0:
    if ((mods & MOD_MASK_SHIFT) && last_auto_pair == KC_9) {
      handle_jump_over(KC_0);
      return false;
    }
    break;
  case KC_LBRC:
    handle_bracket_pair((mods & MOD_MASK_SHIFT) ? "{}" : "[]", KC_LBRC);
    return false;
  case KC_RBRC:
    if (last_auto_pair == KC_LBRC) {
      handle_jump_over(KC_RBRC);
      return false;
    }
    break;
  default:
    break;
  }
  return true;
}

static bool handle_quotes_backticks(uint16_t keycode, uint8_t mods) {
  switch (keycode) {
  case KC_QUOT:
    if (mods & MOD_MASK_SHIFT) {
      handle_bracket_pair("\"\"", KC_QUOT);
      return false;
    }
    break;
  case KC_GRAVE:
    if (!(mods & MOD_MASK_SHIFT)) {
      handle_bracket_pair("``", KC_GRAVE);
      return false;
    }
    break;
  default:
    break;
  }
  return true;
}

static bool process_bracket_auto_complete(uint16_t keycode,
                                          keyrecord_t *record) {
  if (!record->event.pressed)
    return true;

  uint8_t mods = get_mods() | get_weak_mods();
  if (!handle_asterisk_pairing(keycode, mods))
    return false;
  if (!handle_standard_pairs(keycode, mods))
    return false;
  if (!handle_quotes_backticks(keycode, mods))
    return false;

  if (keycode == KC_BSPC) {
    if (last_auto_pair != KC_NO) {
      tap_code(KC_DEL);
      last_auto_pair = KC_NO;
    }
    last_keycode_pressed = KC_BSPC;
  } else {
    last_auto_pair = KC_NO;
    last_keycode_pressed = keycode;
  }
  return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  if (layer_state_is(2)) {
    switch (keycode) {
    case KC_A:
      apply_counter_strafe(MOV_A, MOV_D, KC_A, KC_D, record);
      return false;
    case KC_D:
      apply_counter_strafe(MOV_D, MOV_A, KC_D, KC_A, record);
      return false;
    case KC_W:
      apply_counter_strafe(MOV_W, MOV_S, KC_W, KC_S, record);
      return false;
    case KC_S:
      apply_counter_strafe(MOV_S, MOV_W, KC_S, KC_W, record);
      return false;
    default:
      break;
    }
  }

  if (!process_bracket_auto_complete(keycode, record))
    return false;

  if (record->event.pressed) {
    switch (keycode) {
    case EML_MAC:
      SEND_STRING("");
      return false;
    case PWD_MAC:
      SEND_STRING("");
      return false;
    default:
      break;
    }
  }
  return true;
}

void keyboard_post_init_user(void) {
  rgb_matrix_enable_noeeprom();
  rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
  rgb_matrix_sethsv_noeeprom(HSV_OFF);
}

static void set_leds_by_coords(const uint8_t coords[][2], uint8_t count,
                               uint8_t led_min, uint8_t led_max, uint8_t r,
                               uint8_t g, uint8_t b) {
  for (uint8_t i = 0; i < count; i++) {
    uint8_t led = g_led_config.matrix_co[coords[i][0]][coords[i][1]];
    if (led >= led_min && led < led_max)
      rgb_matrix_set_color(led, r, g, b);
  }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
  if (layer_state_is(2)) {
    static const uint8_t gaming_keys[4][2] = {{1, 2}, {2, 1}, {2, 2}, {2, 3}};
    set_leds_by_coords(gaming_keys, 4, led_min, led_max, 0xFF, 0xFF, 0xFF);
  } else if (layer_state_is(1)) {
    static const uint8_t fn_keys[4][2] = {{2, 6}, {2, 7}, {2, 8}, {2, 9}};
    set_leds_by_coords(fn_keys, 4, led_min, led_max, 0xFF, 0xFF, 0xFF);
  }
  return true;
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9,
                 KC_0, KC_MINS, KC_EQL, KC_BSPC, KC_TAB, KC_Q, KC_W, KC_E, KC_R,
                 KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_BSLS,
                 KC_CAPS, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L,
                 KC_SCLN, KC_QUOT, KC_ENT, KC_LSFT, KC_Z, KC_X, KC_C, KC_V,
                 KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_VOLU, KC_INS,
                 KC_LCTL, MO(1), KC_LALT, KC_SPC, _______, TG(2), _______,
                 KC_VOLD, KC_DEL),
    [1] = LAYOUT(KC_GRAVE, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7,
                 KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, _______, LGUI(KC_TAB),
                 KC_HOME, _______, KC_END, LGUI(KC_R), _______, _______,
                 _______, LGUI(KC_I), _______, _______, _______, _______,
                 _______, _______, _______, LGUI(KC_S), LGUI(KC_D), _______,
                 _______, KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, _______, _______,
                 QK_BOOT, _______, _______, _______, _______, LGUI(KC_V),
                 _______, _______, _______, LGUI(KC_COMM), LGUI(KC_DOT),
                 _______, _______, _______, _______, _______, _______, KC_LGUI,
                 EML_MAC, _______, PWD_MAC, _______, _______),
    [2] =
        LAYOUT(_______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______,
               _______, _______, _______, _______, _______, _______, _______)};
