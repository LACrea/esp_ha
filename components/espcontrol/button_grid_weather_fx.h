#pragma once

// Procedural animated weather backgrounds for hero weather cards.
//
// A background layer is created behind the card content with a
// condition-tinted vertical gradient, and a small pool of LVGL objects is
// animated over it by one timer per card: rain streaks, snow flakes,
// drifting clouds, fog bands, a pulsing sun glow, twinkling stars, and a
// lightning flash overlay for storms. Everything is drawn procedurally, so
// no image assets are required and the effect scales to any card size.

constexpr int WEATHER_FX_MAX_CARDS = 6;
constexpr int WEATHER_FX_MAX_PARTICLES = 18;
constexpr uint32_t WEATHER_FX_TICK_MS = 60;

enum class WeatherFxKind : uint8_t {
  NONE,
  RAIN,
  POUR,
  STORM,
  SNOW,
  SLEET,
  CLOUDS,
  SUN,
  STARS,
  FOG,
};

struct WeatherFxParticle {
  lv_obj_t *obj = nullptr;
  int16_t x = 0;
  int16_t y = 0;
  int16_t vx = 0;
  int16_t vy = 0;
  uint16_t phase = 0;
};

struct WeatherFxCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *layer = nullptr;
  lv_obj_t *flash = nullptr;
  lv_timer_t *timer = nullptr;
  WeatherFxKind kind = WeatherFxKind::NONE;
  WeatherFxParticle particles[WEATHER_FX_MAX_PARTICLES];
  int particle_count = 0;
  uint8_t flash_step = 0;
  uint32_t next_flash_ms = 0;
  uint32_t rng = 0;
  int16_t glow_level = 28;
  int8_t glow_dir = 1;
};

inline WeatherFxCtx *weather_fx_cards() {
  static WeatherFxCtx cards[WEATHER_FX_MAX_CARDS];
  return cards;
}

inline int &weather_fx_count() {
  static int count = 0;
  return count;
}

inline uint32_t weather_fx_rand(WeatherFxCtx *fx) {
  fx->rng = fx->rng * 1664525u + 1013904223u;
  return fx->rng >> 8;
}

inline int weather_fx_rand_range(WeatherFxCtx *fx, int lo, int hi) {
  if (hi <= lo) return lo;
  return lo + static_cast<int>(weather_fx_rand(fx) % static_cast<uint32_t>(hi - lo + 1));
}

inline WeatherFxKind weather_fx_kind_for(const std::string &condition) {
  if (condition == "rainy") return WeatherFxKind::RAIN;
  if (condition == "pouring") return WeatherFxKind::POUR;
  if (condition == "lightning" || condition == "lightning-rainy") return WeatherFxKind::STORM;
  if (condition == "snowy" || condition == "snowy-heavy") return WeatherFxKind::SNOW;
  if (condition == "snowy-rainy" || condition == "hail") return WeatherFxKind::SLEET;
  if (condition == "cloudy" || condition == "partlycloudy" ||
      condition == "windy" || condition == "windy-variant") return WeatherFxKind::CLOUDS;
  if (condition == "sunny") return WeatherFxKind::SUN;
  if (condition == "clear-night" || condition == "night-partly-cloudy") return WeatherFxKind::STARS;
  if (condition == "fog") return WeatherFxKind::FOG;
  return WeatherFxKind::NONE;
}

inline void weather_fx_gradient_for(WeatherFxKind kind, uint32_t &top, uint32_t &bottom) {
  switch (kind) {
    case WeatherFxKind::RAIN:
    case WeatherFxKind::POUR:   top = 0x1B2735; bottom = 0x2C3E50; return;
    case WeatherFxKind::STORM:  top = 0x141726; bottom = 0x2B3050; return;
    case WeatherFxKind::SNOW:
    case WeatherFxKind::SLEET:  top = 0x27313B; bottom = 0x3D4C5C; return;
    case WeatherFxKind::CLOUDS: top = 0x222933; bottom = 0x333E4A; return;
    case WeatherFxKind::SUN:    top = 0x5A4414; bottom = 0x33280E; return;
    case WeatherFxKind::STARS:  top = 0x0D1220; bottom = 0x1D2437; return;
    case WeatherFxKind::FOG:    top = 0x272C31; bottom = 0x3A424A; return;
    default:                    top = 0x202020; bottom = 0x202020; return;
  }
}

inline lv_coord_t weather_fx_width(WeatherFxCtx *fx) {
  lv_coord_t w = fx->layer ? lv_obj_get_width(fx->layer) : 0;
  if (w <= 0) w = fx->btn ? lv_obj_get_width(fx->btn) : 0;
  return w > 0 ? w : 240;
}

inline lv_coord_t weather_fx_height(WeatherFxCtx *fx) {
  lv_coord_t h = fx->layer ? lv_obj_get_height(fx->layer) : 0;
  if (h <= 0) h = fx->btn ? lv_obj_get_height(fx->btn) : 0;
  return h > 0 ? h : 240;
}

inline lv_obj_t *weather_fx_make_particle(WeatherFxCtx *fx, lv_coord_t w, lv_coord_t h,
                                          uint32_t color, lv_opa_t opa, lv_coord_t radius) {
  lv_obj_t *obj = lv_obj_create(fx->layer);
  if (!obj) return nullptr;
  lv_obj_set_size(obj, w, h);
  lv_obj_set_style_bg_color(obj, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(obj, opa, LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
  lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  return obj;
}

inline void weather_fx_clear_particles(WeatherFxCtx *fx) {
  for (int i = 0; i < fx->particle_count; i++) {
    if (fx->particles[i].obj) lv_obj_del(fx->particles[i].obj);
    fx->particles[i] = WeatherFxParticle();
  }
  fx->particle_count = 0;
}

inline void weather_fx_spawn_particles(WeatherFxCtx *fx) {
  weather_fx_clear_particles(fx);
  lv_coord_t w = weather_fx_width(fx);
  lv_coord_t h = weather_fx_height(fx);

  auto add = [&](lv_obj_t *obj, int x, int y, int vx, int vy) {
    if (!obj || fx->particle_count >= WEATHER_FX_MAX_PARTICLES) {
      if (obj) lv_obj_del(obj);
      return;
    }
    WeatherFxParticle &p = fx->particles[fx->particle_count++];
    p.obj = obj;
    p.x = x; p.y = y; p.vx = vx; p.vy = vy;
    p.phase = static_cast<uint16_t>(weather_fx_rand(fx));
    lv_obj_set_pos(obj, x, y);
  };

  switch (fx->kind) {
    case WeatherFxKind::RAIN:
    case WeatherFxKind::POUR:
    case WeatherFxKind::STORM: {
      int drops = fx->kind == WeatherFxKind::POUR ? 12 : fx->kind == WeatherFxKind::STORM ? 12 : 10;
      // Only the storm gets diagonal drops; vertical-only movement keeps the
      // invalidated redraw strips narrow, which matters on large hero cards.
      int drift = fx->kind == WeatherFxKind::STORM ? -1 : 0;
      for (int i = 0; i < drops; i++) {
        int len = weather_fx_rand_range(fx, fx->kind == WeatherFxKind::POUR ? 14 : 10,
                                        fx->kind == WeatherFxKind::POUR ? 26 : 16);
        add(weather_fx_make_particle(fx, 2, len, 0x7FA8D9, 130, 1),
            weather_fx_rand_range(fx, 0, w), weather_fx_rand_range(fx, -h, h),
            drift, weather_fx_rand_range(fx, 6, fx->kind == WeatherFxKind::POUR ? 14 : 10));
      }
      break;
    }
    case WeatherFxKind::SNOW:
    case WeatherFxKind::SLEET: {
      for (int i = 0; i < 12; i++) {
        bool drop = fx->kind == WeatherFxKind::SLEET && (i % 2 == 0);
        int size = drop ? 2 : weather_fx_rand_range(fx, 3, 5);
        add(weather_fx_make_particle(fx, size, drop ? 10 : size,
              drop ? 0x7FA8D9 : 0xE8EEF4, drop ? 130 : 165, drop ? 1 : LV_RADIUS_CIRCLE),
            weather_fx_rand_range(fx, 0, w), weather_fx_rand_range(fx, -h, h),
            0, drop ? weather_fx_rand_range(fx, 6, 10) : weather_fx_rand_range(fx, 1, 3));
      }
      break;
    }
    case WeatherFxKind::CLOUDS: {
      for (int i = 0; i < 3; i++) {
        lv_coord_t cw = w * weather_fx_rand_range(fx, 45, 65) / 100;
        lv_coord_t ch = h * weather_fx_rand_range(fx, 22, 30) / 100;
        add(weather_fx_make_particle(fx, cw, ch, 0xFFFFFF,
              static_cast<lv_opa_t>(weather_fx_rand_range(fx, 14, 22)), ch / 2),
            weather_fx_rand_range(fx, -cw, w), h * (10 + i * 26) / 100,
            i % 2 == 0 ? 1 : 2, 0);
      }
      break;
    }
    case WeatherFxKind::SUN: {
      lv_coord_t side = (w < h ? w : h) * 5 / 4;
      lv_obj_t *glow = weather_fx_make_particle(fx, side, side, 0xFFD24A, 44, LV_RADIUS_CIRCLE);
      add(glow, w - side * 2 / 3, -side / 3, 0, 0);
      fx->glow_level = 44;
      fx->glow_dir = 1;
      break;
    }
    case WeatherFxKind::STARS: {
      for (int i = 0; i < 9; i++) {
        int size = weather_fx_rand_range(fx, 2, 3);
        add(weather_fx_make_particle(fx, size, size, 0xE8EEF4, 120, LV_RADIUS_CIRCLE),
            weather_fx_rand_range(fx, 4, w - 8), weather_fx_rand_range(fx, 4, h * 3 / 5),
            0, 0);
      }
      break;
    }
    case WeatherFxKind::FOG: {
      for (int i = 0; i < 3; i++) {
        lv_coord_t bh = h * 16 / 100;
        add(weather_fx_make_particle(fx, w * 13 / 10, bh, 0xC9D3DC, 22, bh / 2),
            weather_fx_rand_range(fx, -w / 3, 0), h * (18 + i * 26) / 100,
            i % 2 == 0 ? 1 : -1, 0);
      }
      break;
    }
    default:
      break;
  }
}

inline void weather_fx_tick(lv_timer_t *timer) {
  WeatherFxCtx *fx = static_cast<WeatherFxCtx *>(lv_timer_get_user_data(timer));
  if (!fx || !fx->layer || fx->kind == WeatherFxKind::NONE) return;
  if (lv_obj_has_flag(fx->layer, LV_OBJ_FLAG_HIDDEN)) return;
  if (!lv_obj_is_visible(fx->layer)) return;
  lv_coord_t w = weather_fx_width(fx);
  lv_coord_t h = weather_fx_height(fx);

  for (int i = 0; i < fx->particle_count; i++) {
    WeatherFxParticle &p = fx->particles[i];
    if (!p.obj) continue;
    switch (fx->kind) {
      case WeatherFxKind::RAIN:
      case WeatherFxKind::POUR:
      case WeatherFxKind::STORM:
      case WeatherFxKind::SLEET:
      case WeatherFxKind::SNOW: {
        p.y = static_cast<int16_t>(p.y + p.vy);
        if (fx->kind == WeatherFxKind::SNOW || fx->kind == WeatherFxKind::SLEET) {
          p.phase = static_cast<uint16_t>(p.phase + 13);
          p.x = static_cast<int16_t>(p.x + ((p.phase >> 6) % 3) - 1);
        } else {
          p.x = static_cast<int16_t>(p.x + p.vx);
        }
        if (p.y > h) {
          p.y = static_cast<int16_t>(-lv_obj_get_height(p.obj));
          p.x = static_cast<int16_t>(weather_fx_rand_range(fx, 0, w));
        }
        if (p.x < -8) p.x = static_cast<int16_t>(w);
        lv_obj_set_pos(p.obj, p.x, p.y);
        break;
      }
      case WeatherFxKind::CLOUDS:
      case WeatherFxKind::FOG: {
        p.x = static_cast<int16_t>(p.x + p.vx);
        lv_coord_t pw = lv_obj_get_width(p.obj);
        if (p.vx > 0 && p.x > w) p.x = static_cast<int16_t>(-pw);
        if (p.vx < 0 && p.x < -pw) p.x = static_cast<int16_t>(w);
        lv_obj_set_pos(p.obj, p.x, p.y);
        break;
      }
      case WeatherFxKind::SUN: {
        fx->glow_level = static_cast<int16_t>(fx->glow_level + fx->glow_dir);
        if (fx->glow_level >= 72) fx->glow_dir = -1;
        if (fx->glow_level <= 34) fx->glow_dir = 1;
        lv_obj_set_style_bg_opa(p.obj, static_cast<lv_opa_t>(fx->glow_level), LV_PART_MAIN);
        break;
      }
      case WeatherFxKind::STARS: {
        if ((weather_fx_rand(fx) & 0x1F) == 0) {
          lv_obj_set_style_bg_opa(p.obj,
            static_cast<lv_opa_t>(weather_fx_rand_range(fx, 60, 210)), LV_PART_MAIN);
        }
        break;
      }
      default:
        break;
    }
  }

  if (fx->kind == WeatherFxKind::STORM && fx->flash) {
    uint32_t now = esphome::millis();
    if (fx->flash_step == 0) {
      if (fx->next_flash_ms == 0) {
        fx->next_flash_ms = now + weather_fx_rand_range(fx, 4000, 12000);
      } else if (now >= fx->next_flash_ms) {
        fx->flash_step = 1;
      }
    }
    if (fx->flash_step > 0) {
      static constexpr lv_opa_t FLASH_SEQ[] = {150, 40, 190, 90, 0};
      lv_obj_set_style_bg_opa(fx->flash, FLASH_SEQ[fx->flash_step - 1], LV_PART_MAIN);
      fx->flash_step++;
      if (fx->flash_step > static_cast<int>(sizeof(FLASH_SEQ) / sizeof(FLASH_SEQ[0]))) {
        fx->flash_step = 0;
        fx->next_flash_ms = esphome::millis() + weather_fx_rand_range(fx, 5000, 16000);
      }
    }
  }
}

inline void weather_fx_apply_condition(WeatherFxCtx *fx, const std::string &condition) {
  WeatherFxKind kind = weather_fx_kind_for(condition);
  if (kind == fx->kind && fx->particle_count > 0) return;
  fx->kind = kind;
  fx->flash_step = 0;
  fx->next_flash_ms = 0;
  if (!fx->layer) return;
  if (kind == WeatherFxKind::NONE) {
    weather_fx_clear_particles(fx);
    lv_obj_add_flag(fx->layer, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  uint32_t top = 0, bottom = 0;
  weather_fx_gradient_for(kind, top, bottom);
  lv_obj_set_style_bg_color(fx->layer, lv_color_hex(top), LV_PART_MAIN);
  lv_obj_set_style_bg_grad_color(fx->layer, lv_color_hex(bottom), LV_PART_MAIN);
  lv_obj_set_style_bg_grad_dir(fx->layer, LV_GRAD_DIR_VER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(fx->layer, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(fx->layer, LV_OBJ_FLAG_HIDDEN);
  weather_fx_spawn_particles(fx);
  if (fx->flash) {
    lv_obj_set_style_bg_opa(fx->flash, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_move_foreground(fx->flash);
  }
}

inline void weather_fx_attach(lv_obj_t *btn) {
  if (!btn) return;
  int &count = weather_fx_count();
  if (count >= WEATHER_FX_MAX_CARDS) {
    ESP_LOGW("weather_fx", "Too many hero weather backgrounds; skipping");
    return;
  }
  WeatherFxCtx *fx = &weather_fx_cards()[count];

  lv_obj_t *layer = lv_obj_create(btn);
  if (!layer) return;
  count++;
  fx->btn = btn;
  fx->layer = layer;
  fx->rng = esphome::millis() ^ reinterpret_cast<uintptr_t>(btn);
  lv_obj_set_size(layer, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(layer, 0, 0);
  lv_obj_set_style_border_width(layer, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(layer, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(layer, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_radius(layer, lv_obj_get_style_radius(btn, LV_PART_MAIN), LV_PART_MAIN);
  lv_obj_set_style_clip_corner(layer, true, LV_PART_MAIN);
  lv_obj_clear_flag(layer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(layer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(layer, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_background(layer);

  fx->flash = lv_obj_create(layer);
  if (fx->flash) {
    lv_obj_set_size(fx->flash, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(fx->flash, 0, 0);
    lv_obj_set_style_bg_color(fx->flash, lv_color_hex(0xF4F7FF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fx->flash, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(fx->flash, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(fx->flash, 0, LV_PART_MAIN);
    lv_obj_clear_flag(fx->flash, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(fx->flash, LV_OBJ_FLAG_SCROLLABLE);
  }

  fx->timer = lv_timer_create(weather_fx_tick, WEATHER_FX_TICK_MS, fx);
}

inline void weather_fx_set_condition_for_btn(lv_obj_t *btn, const std::string &condition) {
  if (!btn) return;
  WeatherFxCtx *cards = weather_fx_cards();
  int count = weather_fx_count();
  for (int i = 0; i < count; i++) {
    if (cards[i].btn == btn) {
      weather_fx_apply_condition(&cards[i], condition);
      return;
    }
  }
}

inline void reset_weather_fx_cards() {
  WeatherFxCtx *cards = weather_fx_cards();
  int count = weather_fx_count();
  for (int i = 0; i < count; i++) {
    if (cards[i].timer) lv_timer_del(cards[i].timer);
    // The layer and particles are children of the card button; the grid
    // rebuild deletes them with the other dynamic children.
    cards[i] = WeatherFxCtx();
  }
  weather_fx_count() = 0;
}
