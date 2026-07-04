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
  std::function<void()> suspend_display_takeover;
  std::function<void()> resume_display_takeover;
};

struct MediaTvModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
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
      }
    } else {
      lv_obj_add_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }

  lv_coord_t text_width = lv_obj_get_width(ctx->btn) - text_inset - pad;
  if (text_width < 1) text_width = lv_pct(100);

  if (ctx->title_lbl) {
    if (ctx->title_font) lv_obj_set_style_text_font(ctx->title_lbl, ctx->title_font, LV_PART_MAIN);
    if (wide || large) {
      lv_label_set_long_mode(ctx->title_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(ctx->title_lbl, text_width);
      lv_coord_t title_h = large ? LV_SIZE_CONTENT : (ctx->title_font
        ? ctx->title_font->line_height * 2 : LV_SIZE_CONTENT);
      if (title_h != LV_SIZE_CONTENT) lv_obj_set_height(ctx->title_lbl, title_h);
      lv_obj_align(ctx->title_lbl, LV_ALIGN_TOP_LEFT, text_inset, pad);
    } else {
      lv_label_set_long_mode(ctx->title_lbl, LV_LABEL_LONG_DOT);
      const lv_font_t *font = ctx->title_font
        ? ctx->title_font : lv_obj_get_style_text_font(ctx->title_lbl, LV_PART_MAIN);
      int lines = ctx->row_span >= CARD_SIZE_TALL_ROW_SPAN ? 2 : 1;
      if (font && font->line_height > 0) {
        lv_obj_set_size(ctx->title_lbl, text_width, font->line_height * lines - 1);
      } else {
        lv_obj_set_width(ctx->title_lbl, text_width);
      }
      lv_obj_align(ctx->title_lbl, LV_ALIGN_TOP_LEFT, text_inset, pad);
    }
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

inline void media_tv_update_metadata(MediaTvNowPlayingCtx *ctx) {
  if (!ctx) return;
  ctx->idle = media_tv_player_idle(ctx->player_state) ||
              (ctx->title_text.empty() && !ctx->playing);
  media_tv_apply_tile_content(ctx);
  media_tv_apply_modal_content(ctx);
  media_tv_refresh_tile_layout(ctx, lv_obj_get_style_radius(ctx->btn, LV_PART_MAIN) + 4);
}

inline void media_tv_hide_modal() {
  MediaTvModalUi &ui = media_tv_modal_ui();
  if (ui.progress_ctx && ui.progress_ctx->media_timer) {
    lv_timer_pause(ui.progress_ctx->media_timer);
  }
  control_modal_delete_overlay(ControlModalKind::MEDIA_TV, ui.overlay);
  ui = MediaTvModalUi();
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
  lv_obj_set_style_bg_opa(track, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(track, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN);

  ui.duration_lbl = lv_label_create(ui.progress_row);
  lv_label_set_text(ui.duration_lbl, "0:00");
  lv_obj_set_style_text_color(ui.duration_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  if (ctx->modal_subtitle_font) {
    lv_obj_set_style_text_font(ui.duration_lbl, ctx->modal_subtitle_font, LV_PART_MAIN);
  }

  ui.progress_slider = setup_slider_widget(track, ctx->accent_color, true);
  ui.progress_ctx = new SliderCtx();
  ui.progress_ctx->entity_id = ctx->entity_id;
  ui.progress_ctx->fill = lv_obj_get_child(track, 0);
  ui.progress_ctx->horizontal = true;
  ui.progress_ctx->cover_tilt = false;
  ui.progress_ctx->inverted = false;
  ui.progress_ctx->radius = 6;
  ui.progress_ctx->media_position = true;
  ui.progress_ctx->media_slider = ui.progress_slider;
  ui.progress_ctx->media_value_lbl = ui.elapsed_lbl;
  ui.progress_ctx->interactive = true;
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

  ui.progress_ctx->media_timer = lv_timer_create(media_position_timer_cb, 1000, ui.progress_ctx);
  if (ui.progress_ctx->media_timer) lv_timer_pause(ui.progress_ctx->media_timer);

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
  lv_coord_t total_h = 0;
  if (ui.title_lbl) total_h += lv_obj_get_height(ui.title_lbl) + 6;
  if (ui.subtitle_lbl) total_h += lv_obj_get_height(ui.subtitle_lbl) + layout.title_gap;
  if (progress_visible) total_h += lv_obj_get_height(ui.progress_row) + layout.title_gap;
  if (ui.transport_row) total_h += lv_obj_get_height(ui.transport_row) + layout.controls_gap;
  if (ui.volume_row) total_h += lv_obj_get_height(ui.volume_row);
  lv_coord_t centered_y = (layout.panel_h - total_h) / 2;
  if (centered_y > y) y = centered_y;

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

  if (ui.progress_slider && ui.progress_ctx) {
    subscribe_media_slider_state(ui.panel, ui.progress_slider, ctx->entity_id);
    ha_subscribe_attribute(
      ctx->entity_id, std::string("media_duration"),
      std::function<void(esphome::StringRef)>(
        [ctx](esphome::StringRef val) {
          float duration = 0.0f;
          if (!parse_float_ref(val, duration) || duration < 0.0f) duration = 0.0f;
          MediaTvModalUi &modal_ui = media_tv_modal_ui();
          if (modal_ui.progress_ctx) {
            modal_ui.progress_ctx->media_duration = duration;
            if (modal_ui.duration_lbl) {
              char dur_buf[16];
              media_format_time(duration, dur_buf, sizeof(dur_buf));
              lv_label_set_text(modal_ui.duration_lbl, dur_buf);
            }
            media_tv_apply_modal_content(ctx);
          }
        })
    );
  }

  media_tv_apply_modal_content(ctx);
}

inline MediaTvNowPlayingCtx *create_media_tv_now_playing_context(
  BtnSlot &s, const ParsedCfg &p, uint32_t accent_color, uint32_t secondary_color,
  uint32_t tertiary_color, const lv_font_t *title_font, const lv_font_t *subtitle_font,
  const lv_font_t *icon_font, const lv_font_t *modal_title_font,
  const lv_font_t *modal_subtitle_font, int width_compensation_percent,
  int row_span, int col_span,
  std::function<void()> suspend_display_takeover = nullptr,
  std::function<void()> resume_display_takeover = nullptr) {
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
        if (!ctx->available) media_tv_hide_modal();
        ctx->playing = state_text == "playing";
        media_tv_update_idle_state(ctx, state_text);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_title"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef title) {
        ctx->title_text = string_ref_limited(title, HA_STATE_TEXT_MAX_LEN);
        if (ctx->title_text == "unknown" || ctx->title_text == "unavailable") {
          ctx->title_text.clear();
        }
        media_tv_update_metadata(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("media_artist"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef artist) {
        ctx->subtitle_text = string_ref_limited(artist, HA_STATE_TEXT_MAX_LEN);
        if (ctx->subtitle_text == "unknown" || ctx->subtitle_text == "unavailable") {
          ctx->subtitle_text.clear();
        }
        if (ctx->subtitle_text.empty()) {
          ha_get_attribute(
            ctx->entity_id, std::string("app_name"),
            std::function<void(esphome::StringRef)>(
              [ctx](esphome::StringRef app) {
                ctx->subtitle_text = string_ref_limited(app, HA_STATE_TEXT_MAX_LEN);
                if (ctx->subtitle_text == "unknown" || ctx->subtitle_text == "unavailable") {
                  ctx->subtitle_text.clear();
                }
                media_tv_update_metadata(ctx);
              })
          );
        } else {
          media_tv_update_metadata(ctx);
        }
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
                                            std::function<void()> resume_display_takeover) {
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  create_media_tv_now_playing_context(
    s, p, on_color, secondary_color, tertiary_color, title_font, subtitle_font,
    icon_font, modal_title_font, subtitle_font, width_compensation_percent,
    row_span, col_span, suspend_display_takeover, resume_display_takeover);
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
