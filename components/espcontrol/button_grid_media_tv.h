#pragma once

// TV Now Playing media mode: responsive grid tile + tap-to-open control modal.

struct MediaTvNowPlayingCtx {
  std::string entity_id;
  std::string room_label;
  std::string title_text;
  std::string subtitle_text;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *subtitle_lbl = nullptr;
  int width_compensation_percent = 100;
  int row_span = 1;
  int col_span = 1;
  const lv_font_t *title_font = nullptr;
  const lv_font_t *subtitle_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  const lv_font_t *modal_title_font = nullptr;
  const lv_font_t *modal_subtitle_font = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
  bool idle = true;
  bool playing = false;
  bool available = true;
  std::string player_state;
  // Card-lifetime seek/position state. The modal attaches its transient
  // widgets to this context on open and detaches them on close, so the
  // HA subscriptions registered at grid build never touch dead widgets.
  SliderCtx *progress_ctx = nullptr;
  // Album art: one downloader instance shared by all TV cards is enough
  // because art is only fetched while a single modal is open.
  esphome::artwork_image::ArtworkImage *art_image = nullptr;
  std::function<std::string()> art_base_url;
  std::string art_picture;
  std::function<void()> suspend_display_takeover;
  std::function<void()> resume_display_takeover;
};

struct MediaTvModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *art_widget = nullptr;
  lv_coord_t art_side = 0;         // desired art size from the panel dimensions
  lv_coord_t art_render_side = 0;  // actual size after fitting around the controls
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *subtitle_lbl = nullptr;
  lv_obj_t *progress_row = nullptr;
  lv_obj_t *elapsed_lbl = nullptr;
  lv_obj_t *progress_slider = nullptr;
  lv_obj_t *duration_lbl = nullptr;
  lv_obj_t *transport_row = nullptr;
  lv_obj_t *prev_btn = nullptr;
  lv_obj_t *play_btn = nullptr;
  lv_obj_t *next_btn = nullptr;
  lv_obj_t *play_lbl = nullptr;
  lv_obj_t *volume_row = nullptr;
  lv_obj_t *vol_down_btn = nullptr;
  lv_obj_t *vol_up_btn = nullptr;
  SliderCtx *progress_ctx = nullptr;
  MediaTvNowPlayingCtx *active = nullptr;
};

inline MediaTvModalUi &media_tv_modal_ui() {
  static MediaTvModalUi ui;
  return ui;
}

inline bool media_tv_player_idle(const std::string &state) {
  return state == "idle" || state == "off" || state == "standby";
}

inline bool media_tv_tile_wide(const MediaTvNowPlayingCtx *ctx) {
  return ctx && (ctx->col_span >= CARD_SIZE_WIDE_COL_SPAN ||
                 card_span_is_wide(ctx->row_span, ctx->col_span));
}

inline bool media_tv_tile_large(const MediaTvNowPlayingCtx *ctx) {
  return ctx && card_span_is_large(ctx->row_span, ctx->col_span);
}

inline void media_tv_set_label_text(lv_obj_t *label, const std::string &text) {
  if (!label) return;
  lv_label_set_text(label, text.c_str());
}

inline void media_tv_refresh_play_button(MediaTvNowPlayingCtx *ctx) {
  MediaTvModalUi &ui = media_tv_modal_ui();
  if (!ctx || ui.active != ctx || !ui.play_lbl) return;
  lv_label_set_text(ui.play_lbl, ctx->playing ? find_icon("Pause") : find_icon("Play"));
}

inline void media_tv_apply_tile_content(MediaTvNowPlayingCtx *ctx) {
  if (!ctx) return;
  if (ctx->idle) {
    media_tv_set_label_text(ctx->title_lbl, ctx->room_label.empty()
      ? espcontrol_i18n(std::string("Media")) : ctx->room_label);
    media_tv_set_label_text(ctx->subtitle_lbl, espcontrol_i18n(std::string("Idle")));
  } else {
    media_tv_set_label_text(ctx->title_lbl,
      ctx->title_text.empty() ? std::string("--") : ctx->title_text);
    media_tv_set_label_text(ctx->subtitle_lbl, ctx->subtitle_text);
  }
}

inline void media_tv_layout_modal(MediaTvNowPlayingCtx *ctx);

inline void media_tv_apply_modal_content(MediaTvNowPlayingCtx *ctx) {
  MediaTvModalUi &ui = media_tv_modal_ui();
  if (!ctx || ui.active != ctx) return;
  if (ctx->idle) {
    media_tv_set_label_text(ui.title_lbl, ctx->room_label.empty()
      ? espcontrol_i18n(std::string("Media")) : ctx->room_label);
    media_tv_set_label_text(ui.subtitle_lbl, espcontrol_i18n(std::string("Idle")));
  } else {
    media_tv_set_label_text(ui.title_lbl,
      ctx->title_text.empty() ? std::string("--") : ctx->title_text);
    media_tv_set_label_text(ui.subtitle_lbl, ctx->subtitle_text);
  }
  media_tv_refresh_play_button(ctx);
  if (ui.art_widget && ctx->idle) {
    lv_obj_add_flag(ui.art_widget, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.progress_row && ui.progress_ctx) {
    bool show = ui.progress_ctx->media_duration > 0.0f;
    if (show) lv_obj_clear_flag(ui.progress_row, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.progress_row, LV_OBJ_FLAG_HIDDEN);
    if (show) media_apply_position(ui.progress_ctx);
  }
  media_tv_layout_modal(ctx);
}

inline void media_tv_refresh_tile_layout(MediaTvNowPlayingCtx *ctx, lv_coord_t pad) {
  if (!ctx || !ctx->btn) return;
  lv_obj_update_layout(ctx->btn);
  const bool wide = media_tv_tile_wide(ctx);
  const bool large = media_tv_tile_large(ctx);
  const bool show_icon = ctx->idle || wide;
  lv_coord_t text_inset = pad;
  lv_coord_t title_y = pad;
  lv_coord_t icon_w = 0;

  if (ctx->icon_lbl) {
    if (show_icon) {
      lv_obj_clear_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
      if (wide) {
        lv_obj_align(ctx->icon_lbl, LV_ALIGN_LEFT_MID, pad, 0);
        icon_w = lv_obj_get_width(ctx->icon_lbl);
        if (icon_w <= 0) icon_w = pad * 4;
        text_inset = pad + icon_w + pad;
      } else {
        lv_obj_align(ctx->icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
        lv_coord_t icon_h = lv_obj_get_height(ctx->icon_lbl);
        if (icon_h <= 0 && ctx->icon_font) {
          icon_h = lv_font_get_line_height(ctx->icon_font);
        }
        title_y = pad + icon_h + 4;
      }
    } else {
      lv_obj_add_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }

  lv_coord_t text_width = lv_obj_get_width(ctx->btn) - text_inset - pad;
  if (text_width < 1) text_width = lv_pct(100);

  if (ctx->title_lbl) {
    // Large tiles keep the big media title font; everything else drops to the
    // standard text font so the title cannot crowd the subtitle row.
    const lv_font_t *tile_title_font = large
      ? ctx->title_font
      : (ctx->subtitle_font ? ctx->subtitle_font : ctx->title_font);
    if (tile_title_font) {
      lv_obj_set_style_text_font(ctx->title_lbl, tile_title_font, LV_PART_MAIN);
    }
    const lv_font_t *resolved_font = tile_title_font
      ? tile_title_font : lv_obj_get_style_text_font(ctx->title_lbl, LV_PART_MAIN);
    lv_coord_t title_lh = resolved_font ? lv_font_get_line_height(resolved_font) : 16;
    const lv_font_t *sub_font = ctx->subtitle_font
      ? ctx->subtitle_font
      : (ctx->subtitle_lbl
           ? lv_obj_get_style_text_font(ctx->subtitle_lbl, LV_PART_MAIN) : nullptr);
    lv_coord_t sub_lh = sub_font ? lv_font_get_line_height(sub_font) : 16;

    // Clamp the title line count to what physically fits above the subtitle.
    lv_coord_t avail = lv_obj_get_height(ctx->btn) - title_y - pad - sub_lh - 4;
    int lines = large ? 3
      : (wide || ctx->row_span >= CARD_SIZE_TALL_ROW_SPAN) ? 2 : 1;
    if (title_lh > 0) {
      int fit = avail > 0 ? (int)(avail / title_lh) : 1;
      if (fit < 1) fit = 1;
      if (lines > fit) lines = fit;
    }
    lv_label_set_long_mode(ctx->title_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_size(ctx->title_lbl, text_width,
                    title_lh > 0 ? title_lh * lines - 1 : LV_SIZE_CONTENT);
    lv_obj_align(ctx->title_lbl, LV_ALIGN_TOP_LEFT, text_inset, title_y);
    lv_obj_move_foreground(ctx->title_lbl);
  }

  if (ctx->subtitle_lbl) {
    if (ctx->subtitle_font) {
      lv_obj_set_style_text_font(ctx->subtitle_lbl, ctx->subtitle_font, LV_PART_MAIN);
    }
    lv_label_set_long_mode(ctx->subtitle_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(ctx->subtitle_lbl, text_width);
    lv_obj_align(ctx->subtitle_lbl, LV_ALIGN_BOTTOM_LEFT, text_inset, -pad);
    lv_obj_move_foreground(ctx->subtitle_lbl);
  }
}

inline void media_tv_update_idle_state(MediaTvNowPlayingCtx *ctx, const std::string &state) {
  if (!ctx) return;
  ctx->player_state = state;
  ctx->playing = state == "playing";
  ctx->idle = media_tv_player_idle(state) ||
              (ctx->title_text.empty() && !ctx->playing);
  media_tv_apply_tile_content(ctx);
  media_tv_apply_modal_content(ctx);
  media_tv_refresh_tile_layout(ctx, lv_obj_get_style_radius(ctx->btn, LV_PART_MAIN) + 4);
}

inline std::string media_tv_clean_text(esphome::StringRef value) {
  std::string text = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
  if (text == "unknown" || text == "unavailable" || text == "None" || text == "none") {
    text.clear();
  }
  return text;
}

inline void media_tv_update_metadata(MediaTvNowPlayingCtx *ctx) {
  if (!ctx) return;
  ctx->idle = media_tv_player_idle(ctx->player_state) ||
              (ctx->title_text.empty() && !ctx->playing);
  media_tv_apply_tile_content(ctx);
  media_tv_apply_modal_content(ctx);
  media_tv_refresh_tile_layout(ctx, lv_obj_get_style_radius(ctx->btn, LV_PART_MAIN) + 4);
}

inline std::string media_tv_join_art_url(const std::string &base, const std::string &path) {
  if (path.empty()) return "";
  if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) return path;
  if (base.empty() || path[0] != '/') return "";
  return base + path;
}

inline void media_tv_set_art_source(lv_obj_t *widget,
                                    esphome::artwork_image::ArtworkImage *image) {
  if (!widget || !image) return;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_image_set_src(widget, image->get_lv_image_dsc());
#else
  lv_img_set_src(widget, image->get_lv_img_dsc());
#endif
  lv_obj_clear_flag(widget, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(widget);
}

// Fetch (or hide) the modal art for the active card's current entity_picture.
inline void media_tv_request_art(MediaTvNowPlayingCtx *ctx) {
  MediaTvModalUi &ui = media_tv_modal_ui();
  if (!ctx || ui.active != ctx || !ui.art_widget) return;
  std::string base = ctx->art_base_url ? ctx->art_base_url() : std::string();
  std::string url = media_tv_join_art_url(base, ctx->art_picture);
  if (!ctx->art_image || url.empty() || ctx->idle) {
    lv_obj_add_flag(ui.art_widget, LV_OBJ_FLAG_HIDDEN);
    media_tv_layout_modal(ctx);
    return;
  }
  int side = ui.art_render_side > 0 ? (int)ui.art_render_side
    : (ui.art_side > 0 ? (int)ui.art_side : 200);
  ctx->art_image->set_resize_mode(esphome::artwork_image::ImageResizeMode::COVER);
  ctx->art_image->set_target_size(side, side);
  ctx->art_image->request_update_url(url, side * 2);
}

inline void media_tv_register_art_callback(MediaTvNowPlayingCtx *ctx) {
  static bool registered = false;
  if (registered || !ctx || !ctx->art_image) return;
  registered = true;
  // The bool means "served from cache" (HTTP 304), not success; failures
  // arrive on the separate error callback below.
  ctx->art_image->add_on_finished_callback([](bool) {
    MediaTvModalUi &ui = media_tv_modal_ui();
    if (!ui.active || !ui.art_widget || !ui.active->art_image) return;
    media_tv_set_art_source(ui.art_widget, ui.active->art_image);
    media_tv_layout_modal(ui.active);
  });
  ctx->art_image->add_on_error_callback([]() {
    MediaTvModalUi &ui = media_tv_modal_ui();
    if (!ui.active || !ui.art_widget) return;
    lv_obj_add_flag(ui.art_widget, LV_OBJ_FLAG_HIDDEN);
    media_tv_layout_modal(ui.active);
  });
}

// Re-fetch the artist (falling back to the app name) from scratch. Needed on
// title changes because HA never fires the media_artist subscription when the
// new item simply has no artist attribute (e.g. YouTube -> live TV).
inline void media_tv_refresh_subtitle(MediaTvNowPlayingCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_get_attribute(
    ctx->entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef artist) {
        ctx->subtitle_text = media_tv_clean_text(artist);
        if (ctx->subtitle_text.empty()) {
          ha_get_attribute(
            ctx->entity_id, std::string("app_name"),
            std::function<void(esphome::StringRef)>(
              [ctx](esphome::StringRef app) {
                ctx->subtitle_text = media_tv_clean_text(app);
                media_tv_update_metadata(ctx);
              })
          );
        } else {
          media_tv_update_metadata(ctx);
        }
      })
  );
}

inline void media_tv_hide_modal() {
  MediaTvModalUi &ui = media_tv_modal_ui();
  if (ui.progress_ctx) {
    if (ui.progress_ctx->media_timer) lv_timer_pause(ui.progress_ctx->media_timer);
    ui.progress_ctx->media_slider = nullptr;
    ui.progress_ctx->fill = nullptr;
    ui.progress_ctx->media_value_lbl = nullptr;
  }
  esphome::artwork_image::ArtworkImage *art =
    ui.active ? ui.active->art_image : nullptr;
  control_modal_delete_overlay(ControlModalKind::MEDIA_TV, ui.overlay);
  ui = MediaTvModalUi();
  if (art) {
    art->cancel_update();
    art->release();
  }
}

inline lv_obj_t *media_tv_create_modal_slider_row(lv_obj_t *panel, MediaTvNowPlayingCtx *ctx,
                                                  lv_coord_t content_w) {
  MediaTvModalUi &ui = media_tv_modal_ui();
  ui.progress_row = lv_obj_create(panel);
  lv_obj_set_size(ui.progress_row, content_w, LV_SIZE_CONTENT);
  lv_obj_clear_flag(ui.progress_row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.progress_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(ui.progress_row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.progress_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.progress_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.progress_row, 10, LV_PART_MAIN);
  lv_obj_set_layout(ui.progress_row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.progress_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.progress_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  ui.elapsed_lbl = lv_label_create(ui.progress_row);
  lv_label_set_text(ui.elapsed_lbl, "0:00");
  lv_obj_set_style_text_color(ui.elapsed_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  if (ctx->modal_subtitle_font) {
    lv_obj_set_style_text_font(ui.elapsed_lbl, ctx->modal_subtitle_font, LV_PART_MAIN);
  }

  lv_obj_t *track = lv_obj_create(ui.progress_row);
  lv_obj_set_height(track, 14);
  lv_obj_set_flex_grow(track, 1);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
  // The modal panel itself is DARK_BACKGROUND_TERTIARY, so the rail needs the
  // lighter control grey to be visible.
  lv_obj_set_style_bg_color(track, lv_color_hex(DARK_CONTROL_NEUTRAL), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(track, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(track, 7, LV_PART_MAIN);
  // Let the knob extend beyond the 14px rail instead of being clipped.
  lv_obj_add_flag(track, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_flag(ui.progress_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_set_style_border_width(track, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN);

  ui.duration_lbl = lv_label_create(ui.progress_row);
  lv_label_set_text(ui.duration_lbl, "0:00");
  lv_obj_set_style_text_color(ui.duration_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  if (ctx->modal_subtitle_font) {
    lv_obj_set_style_text_font(ui.duration_lbl, ctx->modal_subtitle_font, LV_PART_MAIN);
  }

  ui.progress_slider = setup_slider_widget(track, ctx->accent_color, true);
  if (ui.progress_slider) {
    lv_obj_set_style_bg_opa(ui.progress_slider, LV_OPA_COVER,
      static_cast<lv_style_selector_t>(LV_PART_KNOB));
    lv_obj_set_style_bg_color(ui.progress_slider, lv_color_hex(ctx->secondary_color),
      static_cast<lv_style_selector_t>(LV_PART_KNOB));
    lv_obj_set_style_radius(ui.progress_slider, LV_RADIUS_CIRCLE,
      static_cast<lv_style_selector_t>(LV_PART_KNOB));
    lv_obj_set_style_width(ui.progress_slider, 22,
      static_cast<lv_style_selector_t>(LV_PART_KNOB));
    lv_obj_set_style_height(ui.progress_slider, 22,
      static_cast<lv_style_selector_t>(LV_PART_KNOB));
    lv_obj_add_flag(ui.progress_slider, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_ext_click_area(ui.progress_slider, 16);
  }
  ui.progress_ctx = ctx->progress_ctx;
  ui.progress_ctx->fill = lv_obj_get_child(track, 0);
  ui.progress_ctx->media_slider = ui.progress_slider;
  ui.progress_ctx->media_value_lbl = ui.elapsed_lbl;
  lv_obj_set_user_data(ui.progress_slider, (void *)ui.progress_ctx);
  slider_bind_geometry_refresh(track, ui.progress_slider);

  lv_obj_add_event_cb(ui.progress_slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *slider_ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!slider_ctx) return;
    int val = lv_slider_get_value(sl);
    slider_update_ctx_fill(slider_ctx, lv_obj_get_parent(sl), val);
    if (slider_ctx->media_duration > 0.0f && slider_ctx->media_value_lbl) {
      char time_buf[16];
      media_format_time(slider_ctx->media_duration * val / 100.0f, time_buf, sizeof(time_buf));
      lv_label_set_text(slider_ctx->media_value_lbl, time_buf);
    }
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.duration_lbl && slider_ctx->media_duration > 0.0f) {
      char dur_buf[16];
      media_format_time(slider_ctx->media_duration, dur_buf, sizeof(dur_buf));
      lv_label_set_text(modal_ui.duration_lbl, dur_buf);
    }
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(ui.progress_slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *slider_ctx = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!slider_ctx || slider_ctx->entity_id.empty() || !slider_ctx->available) return;
    int val = lv_slider_get_value(sl);
    media_set_pending_seek_position(slider_ctx, val);
    send_media_seek_action(slider_ctx->entity_id, val, slider_ctx->media_duration);
  }, LV_EVENT_RELEASED, nullptr);

  if (ui.progress_ctx->media_timer && ui.progress_ctx->media_playing) {
    lv_timer_resume(ui.progress_ctx->media_timer);
  }

  return ui.progress_row;
}

inline void media_tv_layout_modal(MediaTvNowPlayingCtx *ctx) {
  MediaTvModalUi &ui = media_tv_modal_ui();
  if (!ctx || !ui.overlay || !ui.panel) return;
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, control_modal_card_radius(ctx->btn));
  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  if (content_w < 120) content_w = layout.panel_w;
  lv_coord_t y = layout.inset + layout.back_size + 8;

  if (ui.back_btn) {
    control_modal_apply_back_button_layout(ui.back_btn, layout);
    lv_obj_move_foreground(ui.back_btn);
  }

  if (ui.title_lbl) {
    lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_size(ui.title_lbl, content_w, LV_SIZE_CONTENT);
  }
  if (ui.subtitle_lbl) lv_obj_set_width(ui.subtitle_lbl, content_w);
  if (ui.progress_row) lv_obj_set_width(ui.progress_row, content_w);
  if (ui.transport_row) lv_obj_set_width(ui.transport_row, content_w);
  if (ui.volume_row) lv_obj_set_width(ui.volume_row, content_w);
  lv_obj_update_layout(ui.panel);

  if (ui.title_lbl) {
    const lv_font_t *title_font = lv_obj_get_style_text_font(ui.title_lbl, LV_PART_MAIN);
    lv_coord_t line_h = title_font ? lv_font_get_line_height(title_font) : 0;
    if (line_h > 0 && lv_obj_get_height(ui.title_lbl) > line_h * 2 + 4) {
      lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_DOT);
      lv_obj_set_size(ui.title_lbl, content_w, line_h * 2);
      lv_obj_update_layout(ui.panel);
    }
  }

  bool progress_visible =
    ui.progress_row && !lv_obj_has_flag(ui.progress_row, LV_OBJ_FLAG_HIDDEN);
  lv_coord_t content_h = 0;
  if (ui.title_lbl) content_h += lv_obj_get_height(ui.title_lbl) + 6;
  if (ui.subtitle_lbl) content_h += lv_obj_get_height(ui.subtitle_lbl) + layout.title_gap;
  if (progress_visible) content_h += lv_obj_get_height(ui.progress_row) + layout.title_gap;
  if (ui.transport_row) content_h += lv_obj_get_height(ui.transport_row) + layout.controls_gap;
  if (ui.volume_row) content_h += lv_obj_get_height(ui.volume_row);

  // The art gets whatever vertical space the controls leave over; it shrinks
  // (or disappears) rather than pushing the volume row out of the panel.
  lv_coord_t max_total = layout.panel_h - y - layout.inset;
  lv_coord_t art_room = max_total - content_h - layout.title_gap;
  lv_coord_t art_side = ui.art_side < art_room ? ui.art_side : art_room;
  bool art_fits = art_side >= 96;
  ui.art_render_side = art_fits ? art_side : 0;
  if (ui.art_widget && !art_fits) {
    lv_obj_add_flag(ui.art_widget, LV_OBJ_FLAG_HIDDEN);
  }
  bool art_visible = ui.art_widget && art_fits &&
                     !lv_obj_has_flag(ui.art_widget, LV_OBJ_FLAG_HIDDEN);
  if (art_visible) lv_obj_set_size(ui.art_widget, art_side, art_side);

  lv_coord_t total_h = content_h + (art_visible ? art_side + layout.title_gap : 0);
  lv_coord_t centered_y = (layout.panel_h - total_h) / 2;
  if (centered_y > y) y = centered_y;

  if (art_visible) {
    lv_obj_align(ui.art_widget, LV_ALIGN_TOP_MID, 0, y);
    y += art_side + layout.title_gap;
  }
  if (ui.title_lbl) {
    lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, y);
    y += lv_obj_get_height(ui.title_lbl) + 6;
  }
  if (ui.subtitle_lbl) {
    lv_obj_align(ui.subtitle_lbl, LV_ALIGN_TOP_MID, 0, y);
    y += lv_obj_get_height(ui.subtitle_lbl) + layout.title_gap;
  }
  if (progress_visible) {
    lv_obj_align(ui.progress_row, LV_ALIGN_TOP_MID, 0, y);
    y += lv_obj_get_height(ui.progress_row) + layout.title_gap;
  }
  if (ui.transport_row) {
    lv_obj_align(ui.transport_row, LV_ALIGN_TOP_MID, 0, y);
    y += lv_obj_get_height(ui.transport_row) + layout.controls_gap;
  }
  if (ui.volume_row) {
    lv_obj_align(ui.volume_row, LV_ALIGN_TOP_MID, 0, y);
  }
}

inline void media_tv_now_playing_open_modal(MediaTvNowPlayingCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::MEDIA_TV, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, media_tv_hide_modal);
  MediaTvModalUi &ui = media_tv_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;

  ui.art_side = shell.layout.panel_h / 3;
  if (ui.art_side < 96) ui.art_side = 96;
  if (ui.art_side > 280) ui.art_side = 280;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  ui.art_widget = lv_image_create(ui.panel);
#else
  ui.art_widget = lv_img_create(ui.panel);
#endif
  if (ui.art_widget) {
    lv_obj_set_size(ui.art_widget, ui.art_side, ui.art_side);
    lv_obj_set_style_radius(ui.art_widget, 12, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(ui.art_widget, true, LV_PART_MAIN);
    lv_obj_clear_flag(ui.art_widget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(ui.art_widget, LV_OBJ_FLAG_HIDDEN);
  }

  ui.title_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->modal_title_font) {
    lv_obj_set_style_text_font(ui.title_lbl, ctx->modal_title_font, LV_PART_MAIN);
  }
  apply_width_compensation(ui.title_lbl, ctx->width_compensation_percent);
  lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_WRAP);

  ui.subtitle_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.subtitle_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.subtitle_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->modal_subtitle_font) {
    lv_obj_set_style_text_font(ui.subtitle_lbl, ctx->modal_subtitle_font, LV_PART_MAIN);
  }
  apply_width_compensation(ui.subtitle_lbl, ctx->width_compensation_percent);
  lv_label_set_long_mode(ui.subtitle_lbl, LV_LABEL_LONG_WRAP);

  media_tv_create_modal_slider_row(ui.panel, ctx, shell.content_w);

  ui.transport_row = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.transport_row, shell.content_w, LV_SIZE_CONTENT);
  lv_obj_clear_flag(ui.transport_row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.transport_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(ui.transport_row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.transport_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.transport_row, 0, LV_PART_MAIN);
  lv_obj_set_layout(ui.transport_row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.transport_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.transport_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.transport_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.transport_row, shell.layout.controls_gap, LV_PART_MAIN);

  lv_coord_t side_btn = shell.layout.btn_size;
  lv_coord_t center_btn = shell.layout.btn_size + 8;
  ui.prev_btn = control_modal_create_round_button(
    ui.transport_row, side_btn, find_icon("Skip Previous"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.play_btn = control_modal_create_round_button(
    ui.transport_row, center_btn, find_icon("Play"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.next_btn = control_modal_create_round_button(
    ui.transport_row, side_btn, find_icon("Skip Next"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.play_lbl = ui.play_btn ? lv_obj_get_child(ui.play_btn, 0) : nullptr;

  lv_obj_add_event_cb(ui.prev_btn, [](lv_event_t *) {
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.active && !modal_ui.active->entity_id.empty()) {
      send_media_playback_action(modal_ui.active->entity_id, "previous");
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.play_btn, [](lv_event_t *) {
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.active && !modal_ui.active->entity_id.empty()) {
      send_media_playback_action(modal_ui.active->entity_id, "play_pause");
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.next_btn, [](lv_event_t *) {
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.active && !modal_ui.active->entity_id.empty()) {
      send_media_playback_action(modal_ui.active->entity_id, "next");
    }
  }, LV_EVENT_CLICKED, nullptr);

  ui.volume_row = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.volume_row, shell.content_w, LV_SIZE_CONTENT);
  lv_obj_clear_flag(ui.volume_row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.volume_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(ui.volume_row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.volume_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.volume_row, 0, LV_PART_MAIN);
  lv_obj_set_layout(ui.volume_row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.volume_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.volume_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.volume_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.volume_row, shell.layout.controls_gap, LV_PART_MAIN);

  ui.vol_down_btn = control_modal_create_round_button(
    ui.volume_row, side_btn, find_icon("Minus"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  lv_obj_t *volume_glyph = lv_label_create(ui.volume_row);
  lv_label_set_text(volume_glyph, find_icon("Volume High"));
  lv_obj_set_style_text_color(volume_glyph, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  if (ctx->icon_font) {
    lv_obj_set_style_text_font(volume_glyph, ctx->icon_font, LV_PART_MAIN);
  }
  apply_width_compensation(volume_glyph, ctx->width_compensation_percent);
  ui.vol_up_btn = control_modal_create_round_button(
    ui.volume_row, side_btn, find_icon("Plus"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);

  lv_obj_add_event_cb(ui.vol_down_btn, [](lv_event_t *) {
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.active && !modal_ui.active->entity_id.empty()) {
      send_media_player_action(modal_ui.active->entity_id, "media_player.volume_down");
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.vol_up_btn, [](lv_event_t *) {
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.active && !modal_ui.active->entity_id.empty()) {
      send_media_player_action(modal_ui.active->entity_id, "media_player.volume_up");
    }
  }, LV_EVENT_CLICKED, nullptr);

  if (ui.progress_ctx && ui.duration_lbl && ui.progress_ctx->media_duration > 0.0f) {
    char dur_buf[16];
    media_format_time(ui.progress_ctx->media_duration, dur_buf, sizeof(dur_buf));
    lv_label_set_text(ui.duration_lbl, dur_buf);
  }

  media_tv_register_art_callback(ctx);
  media_tv_apply_modal_content(ctx);
  // Defer the art fetch one tick so the modal paints before the HTTP
  // connection and JPEG decode start competing for the main loop.
  lv_timer_create([](lv_timer_t *timer) {
    MediaTvModalUi &modal_ui = media_tv_modal_ui();
    if (modal_ui.active) media_tv_request_art(modal_ui.active);
    lv_timer_del(timer);
  }, 60, nullptr);
}

inline MediaTvNowPlayingCtx *create_media_tv_now_playing_context(
  BtnSlot &s, const ParsedCfg &p, uint32_t accent_color, uint32_t secondary_color,
  uint32_t tertiary_color, const lv_font_t *title_font, const lv_font_t *subtitle_font,
  const lv_font_t *icon_font, const lv_font_t *modal_title_font,
  const lv_font_t *modal_subtitle_font, int width_compensation_percent,
  int row_span, int col_span,
  std::function<void()> suspend_display_takeover = nullptr,
  std::function<void()> resume_display_takeover = nullptr,
  esphome::artwork_image::ArtworkImage *art_image = nullptr,
  std::function<std::string()> art_base_url = nullptr) {
  MediaTvNowPlayingCtx *ctx = new MediaTvNowPlayingCtx();
  ctx->entity_id = p.entity;
  ctx->room_label = p.label;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  lv_color_t text_color = lv_obj_get_style_text_color(s.sensor_lbl, LV_PART_MAIN);
  lv_obj_t *title_lbl = lv_label_create(s.btn);
  lv_obj_set_style_text_color(title_lbl, text_color, LV_PART_MAIN);
  apply_width_compensation(title_lbl, width_compensation_percent);
  s.sensor_lbl = title_lbl;
  ctx->title_lbl = title_lbl;
  ctx->subtitle_lbl = s.text_lbl;
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->tertiary_color = tertiary_color;
  ctx->title_font = title_font;
  ctx->subtitle_font = subtitle_font;
  ctx->icon_font = icon_font;
  ctx->modal_title_font = modal_title_font ? modal_title_font : title_font;
  ctx->modal_subtitle_font = modal_subtitle_font ? modal_subtitle_font : subtitle_font;
  ctx->width_compensation_percent = normalize_width_compensation_percent(width_compensation_percent);
  ctx->row_span = row_span;
  ctx->col_span = col_span;
  ctx->suspend_display_takeover = suspend_display_takeover;
  ctx->resume_display_takeover = resume_display_takeover;
  ctx->art_image = art_image;
  ctx->art_base_url = art_base_url;

  ctx->progress_ctx = new SliderCtx();
  ctx->progress_ctx->entity_id = p.entity;
  ctx->progress_ctx->horizontal = true;
  ctx->progress_ctx->cover_tilt = false;
  ctx->progress_ctx->inverted = false;
  ctx->progress_ctx->radius = 6;
  ctx->progress_ctx->media_position = true;
  ctx->progress_ctx->interactive = true;
  ctx->progress_ctx->media_timer =
    lv_timer_create(media_position_timer_cb, 1000, ctx->progress_ctx);
  if (ctx->progress_ctx->media_timer) lv_timer_pause(ctx->progress_ctx->media_timer);

  if (s.icon_lbl) {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(s.icon_lbl, media_default_icon("tv_now_playing", p.icon));
  }
  if (s.sensor_container) lv_obj_set_user_data(s.sensor_container, (void *)ctx);
  if (s.btn) lv_obj_set_user_data(s.btn, (void *)ctx);

  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  apply_push_button_transition(s.btn);
  lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  media_tv_apply_tile_content(ctx);
  media_tv_refresh_tile_layout(ctx, pad);
  return ctx;
}

inline void subscribe_media_tv_now_playing_state(MediaTvNowPlayingCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !ha_state_unavailable_ref(state);
        apply_control_availability(ctx->btn, ctx->btn, ctx->available);
        if (!ctx->available && media_tv_modal_ui().active == ctx) media_tv_hide_modal();
        ctx->playing = state_text == "playing";
        media_tv_update_idle_state(ctx, state_text);
      })
  );
  subscribe_media_slider_ctx(ctx->btn, ctx->progress_ctx, ctx->entity_id);
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_duration"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef) {
        // subscribe_media_slider_ctx already stored the parsed duration on
        // ctx->progress_ctx; here we only refresh the open modal's duration
        // label and seek-row visibility.
        MediaTvModalUi &ui = media_tv_modal_ui();
        if (ui.active != ctx) return;
        if (ui.duration_lbl && ctx->progress_ctx &&
            ctx->progress_ctx->media_duration > 0.0f) {
          char dur_buf[16];
          media_format_time(ctx->progress_ctx->media_duration, dur_buf, sizeof(dur_buf));
          lv_label_set_text(ui.duration_lbl, dur_buf);
        }
        media_tv_apply_modal_content(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_title"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef title) {
        std::string new_title = media_tv_clean_text(title);
        bool changed = new_title != ctx->title_text;
        ctx->title_text = new_title;
        if (changed) {
          // Drop the previous item's artist right away so it can never
          // linger, then re-resolve it for the new item.
          ctx->subtitle_text.clear();
          media_tv_refresh_subtitle(ctx);
        }
        media_tv_update_metadata(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef artist) {
        ctx->subtitle_text = media_tv_clean_text(artist);
        if (ctx->subtitle_text.empty()) {
          ha_get_attribute(
            ctx->entity_id, std::string("app_name"),
            std::function<void(esphome::StringRef)>(
              [ctx](esphome::StringRef app) {
                ctx->subtitle_text = media_tv_clean_text(app);
                media_tv_update_metadata(ctx);
              })
          );
        } else {
          media_tv_update_metadata(ctx);
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("entity_picture"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef picture) {
        std::string pic = string_ref_limited(picture, 4096);
        if (pic == "unknown" || pic == "unavailable" || pic == "None" || pic == "none") {
          pic.clear();
        }
        if (pic == ctx->art_picture) return;
        ctx->art_picture = pic;
        media_tv_request_art(ctx);  // no-op unless this card's modal is open
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef name) {
        if (!ctx->room_label.empty()) return;
        ctx->room_label = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
        if (ctx->idle) {
          media_tv_apply_tile_content(ctx);
          media_tv_apply_modal_content(ctx);
          media_tv_refresh_tile_layout(ctx, lv_obj_get_style_radius(ctx->btn, LV_PART_MAIN) + 4);
        }
      })
  );
}

inline void setup_media_tv_now_playing_card(BtnSlot &s, const ParsedCfg &p,
                                            uint32_t on_color, uint32_t secondary_color,
                                            uint32_t tertiary_color,
                                            const lv_font_t *title_font,
                                            const lv_font_t *subtitle_font,
                                            const lv_font_t *icon_font,
                                            const lv_font_t *modal_title_font,
                                            int width_compensation_percent,
                                            int row_span, int col_span,
                                            std::function<void()> suspend_display_takeover,
                                            std::function<void()> resume_display_takeover,
                                            esphome::artwork_image::ArtworkImage *art_image = nullptr,
                                            std::function<std::string()> art_base_url = nullptr) {
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  create_media_tv_now_playing_context(
    s, p, on_color, secondary_color, tertiary_color, title_font, subtitle_font,
    icon_font, modal_title_font, subtitle_font, width_compensation_percent,
    row_span, col_span, suspend_display_takeover, resume_display_takeover,
    art_image, art_base_url);
}

inline void refresh_media_tv_now_playing_layout(BtnSlot &s, const ParsedCfg &p,
                                              const DisplayProfile &display,
                                              int row_span, int col_span) {
  MediaTvNowPlayingCtx *ctx = (MediaTvNowPlayingCtx *)lv_obj_get_user_data(s.sensor_container);
  if (!ctx) return;
  ctx->row_span = row_span;
  ctx->col_span = col_span;
  if (ctx->title_lbl) display_apply_main_width(ctx->title_lbl, display);
  if (ctx->subtitle_lbl) display_apply_main_width(ctx->subtitle_lbl, display);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  media_tv_refresh_tile_layout(ctx, pad);
}
