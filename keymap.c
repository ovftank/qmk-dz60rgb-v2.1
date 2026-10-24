// Copyright 2026 ovftank (@ovftank)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#ifndef uint_farptr_t
typedef uint32_t uint_farptr_t;
#endif
#include QMK_KEYBOARD_H

enum layers {
  _BASE,
  _FN,
};

static const uint8_t socd_keys[] = {KC_A, KC_D};
static bool socd_held[2] = {false, false};
static void socd_update(uint8_t index, bool pressed) {
  const uint8_t other = index ^ 1;

  socd_held[index] = pressed;
  if (pressed) {
    if (socd_held[other])
      del_key(socd_keys[other]);
    add_key(socd_keys[index]);
  } else {
    del_key(socd_keys[index]);
    if (socd_held[other])
      add_key(socd_keys[other]);
  }
  send_keyboard_report();
}

static const key_override_t tilde_esc_override =
    ko_make_basic(MOD_MASK_SHIFT, KC_ESC, KC_TILDE);
const key_override_t *key_overrides[] = {&tilde_esc_override, NULL};

enum custom_keycodes {
  EML_MAC = QK_USER,
  PWD_MAC,
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  for (uint8_t i = 0; i < ARRAY_SIZE(socd_keys); i++) {
    if (keycode == socd_keys[i]) {
      socd_update(i, record->event.pressed);
      return false;
    }
  }

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

void keyboard_post_init_user(void) { rgb_matrix_disable_noeeprom(); }

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9,
                 KC_0, KC_MINS, KC_EQL, KC_BSPC, KC_TAB, KC_Q, KC_W, KC_E, KC_R,
                 KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, KC_RBRC, KC_BSLS,
                 KC_CAPS, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L,
                 KC_SCLN, KC_QUOT, KC_ENT, KC_LSFT, KC_Z, KC_X, KC_C, KC_V,
                 KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_VOLU, KC_INS,
                 KC_LCTL, MO(_FN), KC_LALT, KC_SPC, _______, _______, _______,
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
                 EML_MAC, _______, PWD_MAC, _______, _______)};
