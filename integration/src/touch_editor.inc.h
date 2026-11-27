/*
 * touch_editor.inc.h -- the Controls Pack's layout editor on the 1.0.3 touch controls (src/touch_pad.c includes
 * this; one translation unit, so it shares that file's statics).
 *
 * The gear at the top middle opens the editor (or two fingers held in the top middle for a second, should the gear
 * be hidden). In it: drag a control to move it, drag its corner handle or pinch to resize it, the panel at the top
 * steps its size and opacity, hides or shows it, and DONE keeps what was set while CANCEL drops it. GAME and MENU are
 * two sets of places (the menu one is shown by the game's own menu / card / movie signals: ngb_touch_menu_auto),
 * and each is kept for both layouts the switch at the left edge changes between (the default and Classic). The
 * settings drawer (hold the panel's UP/DOWN tab) has the grid, snap, grid size, the skins and the layout.
 * Everything is kept in <config folder>/touch-layout.ini.
 *
 * All of it runs under s_lock unless it says otherwise; the picture is drawn from the copies in touch_pad.c's d_*.
 */

static uint64_t touch_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

enum {
    EDIT_UI_NONE = -1,
    EDIT_UI_PROFILE, EDIT_UI_SIZE_DEC, EDIT_UI_SIZE_INC, EDIT_UI_HIDE,
    EDIT_UI_OPACITY_DEC, EDIT_UI_OPACITY_INC,
    EDIT_UI_RESET, EDIT_UI_CANCEL, EDIT_UI_SAVE, EDIT_UI_COLLAPSE,
    EDIT_UI_GRID, EDIT_UI_SNAP, EDIT_UI_GRID_SIZE, EDIT_UI_SKIN, EDIT_UI_LAYOUT,
    EDIT_UI_ALL, EDIT_UI_SIZE_VALUE, EDIT_UI_OPACITY_VALUE
};

static bool s_edit_mode, s_edit_gesture_fired;
static int s_edit_profile = -1, s_edit_layout, s_edit_selected = -1;
static bool s_edit_settings_open, s_editor_collapsed, s_grid_enabled = true, s_snap_enabled = true, s_reset_confirm;
static int s_grid_size = 1;                      /* 0 small, 1 medium, 2 large */
static unsigned s_edit_rev = 1;                  /* +1 each change to what the editor's picture shows */
static Ov s_bk_ov[PROFILES][LAYOUTS][NCTRL];     /* what CANCEL puts back */
static bool s_bk_collapsed, s_bk_grid, s_bk_snap;
static int s_bk_grid_size, s_bk_skin;
static float s_anchor_x[NCTRL], s_anchor_y[NCTRL], s_anchor_r[NCTRL];   /* the places when a gesture began */
static float s_edit_start_x, s_edit_start_y, s_edit_start_distance;
static int s_pinch_finger[2] = { -1, -1 };
static bool s_pinch_active;
static float s_pinch_distance;
static int s_edit_value_kind;                    /* 1 size, 2 opacity: the figure shown by the control for a moment */
static uint64_t s_edit_value_until;
static int s_repeat_action = -1;                 /* a held - or +: repeating */
static uint64_t s_repeat_next_ms;
static unsigned s_repeat_count;
static uint64_t s_gesture_since;
static struct { SDL_FingerID id; float x, y; bool used; } s_gesture[2];

static void ov_default(Ov *o, int id)
{
    o->custom = false;
    o->visible = true;
    o->x = o->y = o->r = 0;
    o->opacity = id == GEAR ? 0.35f : 1.0f;
}

static float def_r(int layout, int i)
{
    return layout == 1 ? s_classic[i].r : s_ctrl[i].r;
}

/* Where a control is in the editor's set and layout, on this screen */
static Place edit_place(int i)
{
    return place_at(&s_ctrl[i], s_aspect, s_standin_places, s_edit_layout, s_ov[s_edit_profile][s_edit_layout]);
}

/* A control kept at (ax, ay) with radius r, on the screen, as the player's own (stored from its edge) */
static void ov_put(int profile, int layout, int id, float ax, float ay, float r)
{
    Ov *o = &s_ov[profile][layout][id];
    float dr = def_r(layout, id);
    r = fminf(fmaxf(r, dr * 0.50f), dr * 2.00f);
    float half = art_half(id, r), max = s_aspect - half;
    ax = max < half ? s_aspect * 0.5f : fminf(fmaxf(ax, half), max);
    ay = fminf(fmaxf(ay, half), 1.0f - half);
    int edge = ctl_edge(id, layout);
    o->x = edge == 2 ? ax - s_aspect / 2 : edge ? s_aspect - ax : ax;
    o->y = ay;
    o->r = r;
    o->custom = true;
}

/* The editor's box round a control at place p: the part that takes a finger (also the picture's: draw side) */
static void bounds_of(int id, Place p, float *cx, float *cy, float *hw, float *hh, bool *circle)
{
    *cx = p.x;
    *cy = p.y;
    *circle = true;
    if (s_ctrl[id].kind == STICK) {
        *hw = *hh = p.r * 1.70f;
    } else if (s_ctrl[id].kind == DPAD) {
        *hw = *hh = p.r * 1.10f;
        *circle = false;
    } else if (id == LB || id == RB || id == LT || id == RT) {
        *hw = p.r * 1.55f;
        *hh = p.r * 1.15f;
        *circle = false;
    } else {
        *hw = *hh = p.r * (id == GEAR ? 1.25f : 1.35f);
    }
}

static void editor_bounds(int id, float *cx, float *cy, float *hw, float *hh, bool *circle)
{
    bounds_of(id, edit_place(id), cx, cy, hw, hh, circle);
}

static int editor_hit(float x, float y)
{
    int best = -1;
    float best_d = 1e9f;
    for (int i = 0; i < NCTRL; i++) {
        float cx, cy, hw, hh, d;
        bool circle;
        editor_bounds(i, &cx, &cy, &hw, &hh, &circle);
        float dx = x - cx, dy = y - cy;
        if (circle) {
            d = sqrtf(dx * dx + dy * dy);
            if (d > hw)
                continue;
            d /= hw;
        } else {
            if (fabsf(dx) > hw || fabsf(dy) > hh)
                continue;
            d = fmaxf(fabsf(dx) / hw, fabsf(dy) / hh);
        }
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

/* The selected control's resize handle (a small circle at its lower outer corner) */
static int editor_handle_hit(float x, float y)
{
    if (s_edit_selected < 0)
        return -1;
    float cx, cy, hw, hh;
    bool circle;
    editor_bounds(s_edit_selected, &cx, &cy, &hw, &hh, &circle);
    float hx = cx + hw, hy = cy + hh;
    if (hx + 0.055f > s_aspect)
        hx = cx - hw;
    float dx = x - hx, dy = y - hy;
    return dx * dx + dy * dy < 0.060f * 0.060f ? s_edit_selected : -1;
}

static void clear_touch_locked(void)
{
    memset(s_finger, 0, sizeof s_finger);
    for (int i = 0; i < MAX_FINGERS; i++)
        s_finger[i].ui_action = EDIT_UI_NONE;
    s_pinch_active = false;
    s_pinch_finger[0] = s_pinch_finger[1] = -1;
    memset(s_down, 0, sizeof s_down);
    memset(s_stick, 0, sizeof s_stick);
    s_dpad = 0;
}

static void editor_begin_locked(int profile)
{
    memcpy(s_bk_ov, s_ov, sizeof s_bk_ov);
    s_bk_collapsed = s_editor_collapsed;
    s_bk_grid = s_grid_enabled;
    s_bk_snap = s_snap_enabled;
    s_bk_grid_size = s_grid_size;
    s_bk_skin = s_skin;
    s_edit_profile = profile != 0;
    s_edit_layout = s_layout == 1;
    s_edit_selected = -1;
    s_edit_settings_open = false;
    s_edit_value_kind = 0;
    s_edit_value_until = 0;
    s_repeat_action = -1;
    s_repeat_count = 0;
    s_reset_confirm = false;
    s_edit_mode = true;
    s_edit_rev++;
    clear_touch_locked();
    s_serial++;
}

static void editor_end_locked(bool save)
{
    if (!save && s_edit_profile >= 0) {
        memcpy(s_ov, s_bk_ov, sizeof s_ov);
        s_editor_collapsed = s_bk_collapsed;
        s_grid_enabled = s_bk_grid;
        s_snap_enabled = s_bk_snap;
        s_grid_size = s_bk_grid_size;
        s_skin = s_bk_skin;
    }
    if (save) {
        if (s_layout != s_edit_layout)
            s_layout_save = 1;
        s_layout = s_edit_layout;
        s_ini_save = true;
    }
    s_edit_mode = false;
    s_edit_profile = -1;
    s_edit_selected = -1;
    s_edit_settings_open = false;
    s_edit_value_kind = 0;
    s_repeat_action = -1;
    s_reset_confirm = false;
    s_ov_rev++;
    s_edit_rev++;
    clear_touch_locked();
    s_serial++;
}

static void editor_reset_locked(void)
{
    if (s_edit_profile < 0)
        return;
    if (s_edit_selected < 0 && !s_reset_confirm) {   /* all of them: asked twice */
        s_reset_confirm = true;
        s_serial++;
        return;
    }
    for (int i = 0; i < NCTRL; i++)
        if (s_edit_selected < 0 || i == s_edit_selected)
            ov_default(&s_ov[s_edit_profile][s_edit_layout][i], i);
    s_reset_confirm = false;
    s_ov_rev++;
    s_edit_rev++;
    clear_touch_locked();
    s_serial++;
}

static void editor_anchor_locked(float x, float y)
{
    for (int i = 0; i < NCTRL; i++) {
        Place q = edit_place(i);
        s_anchor_x[i] = q.x;
        s_anchor_y[i] = q.y;
        s_anchor_r[i] = q.r;
    }
    s_edit_start_x = x;
    s_edit_start_y = y;
    s_edit_start_distance = 0;
    if (s_edit_selected >= 0) {
        float dx = x - s_anchor_x[s_edit_selected], dy = y - s_anchor_y[s_edit_selected];
        s_edit_start_distance = sqrtf(dx * dx + dy * dy);
    }
}

static void editor_move_locked(float x, float y)
{
    int i = s_edit_selected;
    if (i < 0)
        return;
    float cx = s_anchor_x[i] + (x - s_edit_start_x), cy = s_anchor_y[i] + (y - s_edit_start_y);
    if (s_snap_enabled) {   /* to the grid's lines and the middle, when within a hair of them */
        static const float grid[3] = { 0.045f, 0.067f, 0.089f };
        float step = grid[s_grid_size], tol = 0.012f;
        float nx = s_aspect * 0.5f + roundf((cx - s_aspect * 0.5f) / step) * step;
        float ny = 0.5f + roundf((cy - 0.5f) / step) * step;
        if (fabsf(nx - cx) <= tol)
            cx = nx;
        if (fabsf(ny - cy) <= tol)
            cy = ny;
        if (fabsf(cx - s_aspect * 0.5f) <= tol)
            cx = s_aspect * 0.5f;
    }
    ov_put(s_edit_profile, s_edit_layout, i, cx, cy, s_anchor_r[i]);
    s_ov_rev++;
    s_edit_rev++;
}

/* The size as a factor of the default radius (0.5 to 2), from the places the gesture began at */
static void editor_size_locked(float factor)
{
    const int p = s_edit_profile, L = s_edit_layout;
    factor = fminf(fmaxf(factor, 0.50f), 2.00f);
    factor = roundf(factor * 100.0f) * 0.01f;
    if (s_edit_selected < 0) {
        for (int i = 0; i < NCTRL; i++) {
            Place q = edit_place(i);
            ov_put(p, L, i, q.x, q.y, def_r(L, i) * factor);
        }
    } else {
        int i = s_edit_selected;
        ov_put(p, L, i, s_anchor_x[i], s_anchor_y[i], def_r(L, i) * factor);
    }
    s_ov_rev++;
    s_edit_rev++;
}

static void editor_mark_value_locked(int kind, uint64_t now)
{
    s_edit_value_kind = kind;
    s_edit_value_until = now + 1350;
}

static void editor_step_size_locked(float delta)
{
    const int p = s_edit_profile, L = s_edit_layout;
    for (int i = 0; i < NCTRL; i++) {
        if (s_edit_selected >= 0 && i != s_edit_selected)
            continue;
        Place q = edit_place(i);
        float dr = def_r(L, i), factor = fminf(fmaxf(q.r / dr + delta, 0.50f), 2.00f);
        ov_put(p, L, i, q.x, q.y, dr * factor);
    }
    s_ov_rev++;
    s_edit_rev++;
    editor_mark_value_locked(1, touch_now_ms());
}

static void editor_step_opacity_locked(float delta)
{
    for (int i = 0; i < NCTRL; i++)
        if (s_edit_selected < 0 || i == s_edit_selected) {
            Ov *o = &s_ov[s_edit_profile][s_edit_layout][i];
            o->opacity = fminf(fmaxf(o->opacity + delta, 0.10f), 1.0f);
        }
    s_ov_rev++;
    s_edit_rev++;
    editor_mark_value_locked(2, touch_now_ms());
}

static void editor_toggle_visible_locked(void)
{
    Ov *list = s_ov[s_edit_profile][s_edit_layout];
    if (s_edit_selected < 0) {   /* all: shown if any is hidden, else all hidden */
        bool show_all = false;
        for (int i = 0; i < NCTRL; i++)
            if (!list[i].visible)
                show_all = true;
        for (int i = 0; i < NCTRL; i++)
            list[i].visible = show_all;
    } else {
        list[s_edit_selected].visible = !list[s_edit_selected].visible;
    }
    s_ov_rev++;
    s_edit_rev++;
}

/* ---- the editor's panel ---- */

static void editor_ui_geometry(float aspect, float *x0, float *panel_w)
{
    *panel_w = fmaxf(0.30f, aspect - 0.05f);
    *x0 = 0.025f;
}

#define EDIT_UI_SLOTS 13
static const float s_ui_width[EDIT_UI_SLOTS] = { 0.18f, 0.06f, 0.04f, 0.10f, 0.04f, 0.07f,
                                                 0.04f, 0.16f, 0.04f, 0.07f, 0.07f, 0.07f, 0.06f };

static int editor_ui_hit(float x, float y, float aspect)
{
    float x0, panel_w;
    editor_ui_geometry(aspect, &x0, &panel_w);
    if (s_edit_settings_open && y >= 0.13f && y <= 0.23f && x >= x0 && x <= x0 + panel_w) {
        static const int act[5] = { EDIT_UI_GRID, EDIT_UI_SNAP, EDIT_UI_GRID_SIZE, EDIT_UI_SKIN, EDIT_UI_LAYOUT };
        int k = (int)(((x - x0) / panel_w) * 5);
        return act[k < 0 ? 0 : k > 4 ? 4 : k];
    }
    const float top = 0.018f, h = s_editor_collapsed ? 0.060f : 0.105f;
    if (y < top - 0.015f || y > top + h + 0.015f || x < x0 || x > x0 + panel_w)
        return -1;
    if (s_editor_collapsed)
        return fabsf(x - aspect * 0.5f) < 0.08f ? EDIT_UI_COLLAPSE : -1;
    static const int action[EDIT_UI_SLOTS] = {
        EDIT_UI_ALL, EDIT_UI_PROFILE, EDIT_UI_SIZE_DEC, EDIT_UI_SIZE_VALUE, EDIT_UI_SIZE_INC, EDIT_UI_HIDE,
        EDIT_UI_OPACITY_DEC, EDIT_UI_OPACITY_VALUE, EDIT_UI_OPACITY_INC, EDIT_UI_RESET, EDIT_UI_CANCEL,
        EDIT_UI_SAVE, EDIT_UI_COLLAPSE };
    float t = (x - x0) / panel_w, edge = 0;
    for (int i = 0; i < EDIT_UI_SLOTS; i++) {
        edge += s_ui_width[i];
        if (t <= edge)
            return action[i];
    }
    return -1;
}

static void editor_action_locked(int action, bool down, uint64_t held_ms)
{
    if (down) {   /* - and + act at once and repeat while held */
        if (action == EDIT_UI_SIZE_DEC || action == EDIT_UI_SIZE_INC) {
            editor_step_size_locked(action == EDIT_UI_SIZE_INC ? 0.05f : -0.05f);
            s_repeat_action = action;
            s_repeat_count = 0;
            s_repeat_next_ms = touch_now_ms() + 420;
        } else if (action == EDIT_UI_OPACITY_DEC || action == EDIT_UI_OPACITY_INC) {
            editor_step_opacity_locked(action == EDIT_UI_OPACITY_INC ? 0.05f : -0.05f);
            s_repeat_action = action;
            s_repeat_count = 0;
            s_repeat_next_ms = touch_now_ms() + 420;
        }
        s_serial++;
        return;
    }
    if (s_repeat_action == action)
        s_repeat_action = -1;
    switch (action) {
    case EDIT_UI_ALL:
        s_edit_selected = -1;
        s_edit_rev++;
        break;
    case EDIT_UI_PROFILE:
        s_edit_profile = !s_edit_profile;
        s_edit_selected = -1;
        s_reset_confirm = false;
        s_edit_rev++;
        break;
    case EDIT_UI_HIDE:
        editor_toggle_visible_locked();
        break;
    case EDIT_UI_RESET:
        editor_reset_locked();
        break;
    case EDIT_UI_CANCEL:
        editor_end_locked(false);
        break;
    case EDIT_UI_SAVE:
        editor_end_locked(true);
        break;
    case EDIT_UI_COLLAPSE:   /* held half a second: the settings drawer; a tap: the panel up or down */
        if (held_ms >= 500) {
            s_edit_settings_open = !s_edit_settings_open;
        } else {
            s_editor_collapsed = !s_editor_collapsed;
            s_edit_settings_open = false;
        }
        break;
    case EDIT_UI_GRID:
        s_grid_enabled = !s_grid_enabled;
        s_edit_rev++;
        break;
    case EDIT_UI_SNAP:
        s_snap_enabled = !s_snap_enabled;
        break;
    case EDIT_UI_GRID_SIZE:
        s_grid_size = (s_grid_size + 1) % 3;
        s_edit_rev++;
        break;
    case EDIT_UI_SKIN:
        s_skin = (s_skin + 1) % 3;
        s_ov_rev++;
        s_edit_rev++;
        break;
    case EDIT_UI_LAYOUT:
        s_edit_layout = !s_edit_layout;
        s_edit_selected = -1;
        s_reset_confirm = false;
        s_edit_rev++;
        break;
    default:
        break;
    }
    s_serial++;
}

/* ---- the touches while editing (none of them the game's) ---- */

static void editor_touch_locked(const SDL_Event *ev, float x, float y, uint64_t tick)
{
    int f;
    for (f = 0; f < MAX_FINGERS && !(s_finger[f].used && s_finger[f].id == ev->tfinger.fingerId); f++)
        ;
    if (ev->type == SDL_FINGERDOWN && f == MAX_FINGERS) {
        int ui = editor_ui_hit(x, y, s_aspect);
        for (f = 0; f < MAX_FINGERS && s_finger[f].used; f++)
            ;
        if (f >= MAX_FINGERS)
            return;
        if (ui >= 0) {
            memset(&s_finger[f], 0, sizeof s_finger[f]);
            s_finger[f].used = s_finger[f].editing = true;
            s_finger[f].id = ev->tfinger.fingerId;
            s_finger[f].ui_action = ui;
            s_finger[f].x = x;
            s_finger[f].y = y;
            s_finger[f].pressed_ms = tick;
            editor_action_locked(ui, true, 0);
            return;
        }
        int active = -1;   /* a finger already dragging a control: a second one pinches */
        for (int k = 0; k < MAX_FINGERS; k++)
            if (s_finger[k].used && s_finger[k].editing && s_finger[k].ui_action < 0) {
                active = k;
                break;
            }
        if (active >= 0 && s_edit_selected >= 0 && !s_pinch_active) {
            memset(&s_finger[f], 0, sizeof s_finger[f]);
            s_finger[f].used = s_finger[f].editing = true;
            s_finger[f].id = ev->tfinger.fingerId;
            s_finger[f].ctrl = s_edit_selected;
            s_finger[f].ui_action = -1;
            s_finger[f].x = x;
            s_finger[f].y = y;
            s_finger[active].resizing = false;
            editor_anchor_locked((s_finger[active].x + x) * 0.5f, (s_finger[active].y + y) * 0.5f);
            float dx = s_finger[active].x - x, dy = s_finger[active].y - y;
            s_pinch_distance = sqrtf(dx * dx + dy * dy);
            s_pinch_finger[0] = active;
            s_pinch_finger[1] = f;
            s_pinch_active = s_pinch_distance > 0.001f;
            s_serial++;
            return;
        }
        int c = editor_handle_hit(x, y);
        bool resize = c >= 0;
        if (c < 0)
            c = editor_hit(x, y);
        if (c >= 0) {
            if (s_edit_selected != c)
                s_edit_rev++;
            s_edit_selected = c;
            editor_anchor_locked(x, y);
            memset(&s_finger[f], 0, sizeof s_finger[f]);
            s_finger[f].used = s_finger[f].editing = true;
            s_finger[f].resizing = resize;
            s_finger[f].id = ev->tfinger.fingerId;
            s_finger[f].ctrl = c;
            s_finger[f].ui_action = -1;
            s_finger[f].x = x;
            s_finger[f].y = y;
        } else if (s_edit_selected >= 0) {   /* a tap on nothing: back to all of them */
            s_edit_selected = -1;
            s_edit_rev++;
        }
        s_serial++;
    } else if (ev->type == SDL_FINGERMOTION && f < MAX_FINGERS && s_finger[f].editing) {
        if (s_finger[f].ui_action >= 0)
            return;
        s_finger[f].x = x;
        s_finger[f].y = y;
        int sel = s_edit_selected;
        if (s_pinch_active && (f == s_pinch_finger[0] || f == s_pinch_finger[1]) && sel >= 0) {
            float dx = s_finger[s_pinch_finger[0]].x - s_finger[s_pinch_finger[1]].x;
            float dy = s_finger[s_pinch_finger[0]].y - s_finger[s_pinch_finger[1]].y;
            float factor = s_anchor_r[sel] / def_r(s_edit_layout, sel);
            editor_size_locked(factor * sqrtf(dx * dx + dy * dy) / s_pinch_distance);
            editor_mark_value_locked(1, tick);
        } else if (s_finger[f].resizing && sel >= 0 && s_edit_start_distance > 0.001f) {
            float dx = x - s_anchor_x[sel], dy = y - s_anchor_y[sel];
            float factor = (s_anchor_r[sel] / def_r(s_edit_layout, sel)) *
                           (sqrtf(dx * dx + dy * dy) / s_edit_start_distance);
            editor_size_locked(factor);
            editor_mark_value_locked(1, tick);
        } else {
            editor_move_locked(x, y);
        }
        s_serial++;
    } else if (ev->type == SDL_FINGERUP && f < MAX_FINGERS && s_finger[f].editing) {
        int action = s_finger[f].ui_action;
        int released = editor_ui_hit(x, y, s_aspect);
        uint64_t held_ms = tick - s_finger[f].pressed_ms;
        if (s_pinch_active && (f == s_pinch_finger[0] || f == s_pinch_finger[1])) {
            int other = s_pinch_finger[f == s_pinch_finger[0] ? 1 : 0];
            s_finger[f].used = s_finger[f].editing = false;
            s_pinch_active = false;
            s_pinch_finger[0] = s_pinch_finger[1] = -1;
            if (other >= 0 && other < MAX_FINGERS && s_finger[other].used) {   /* the other goes on moving it */
                s_finger[other].resizing = false;
                editor_anchor_locked(s_finger[other].x, s_finger[other].y);
            }
        } else {
            s_finger[f].used = false;
            s_finger[f].editing = false;
            if (action >= 0 && action == released)
                editor_action_locked(action, false, held_ms);
            else if (s_repeat_action == action)
                s_repeat_action = -1;
        }
        s_serial++;
    }
}

/* ---- the two-finger recovery gesture: held in the top middle for a second opens the editor ---- */

static int gesture_count(void)
{
    return (int)s_gesture[0].used + (int)s_gesture[1].used;
}

static int gesture_slot(SDL_FingerID id)
{
    for (int i = 0; i < 2; i++)
        if (s_gesture[i].used && s_gesture[i].id == id)
            return i;
    return -1;
}

static bool gesture_zone(float x, float y, float aspect)
{
    return y < 0.20f && x > aspect * 0.38f && x < aspect * 0.62f;
}

/* A finger event for the gesture: true if it is one of its fingers (the controls do not see it) */
static bool gesture_touch_locked(const SDL_Event *ev, float x, float y, uint64_t tick)
{
    int gs = gesture_slot(ev->tfinger.fingerId);
    if (gs >= 0) {
        if (ev->type == SDL_FINGERUP) {
            s_gesture[gs].used = false;
            if (gesture_count() < 2)
                s_gesture_since = 0;
            if (!gesture_count())
                s_edit_gesture_fired = false;
        } else {
            s_gesture[gs].x = x;
            s_gesture[gs].y = y;
            if (gesture_count() == 2 && gesture_zone(s_gesture[0].x, s_gesture[0].y, s_aspect) &&
                gesture_zone(s_gesture[1].x, s_gesture[1].y, s_aspect)) {
                if (!s_gesture_since)
                    s_gesture_since = tick;
            } else {
                s_gesture_since = 0;
            }
        }
        return true;
    }
    if (ev->type == SDL_FINGERDOWN && gesture_zone(x, y, s_aspect)) {
        int slot = !s_gesture[0].used ? 0 : !s_gesture[1].used ? 1 : -1;
        if (slot >= 0) {
            s_gesture[slot].used = true;
            s_gesture[slot].id = ev->tfinger.fingerId;
            s_gesture[slot].x = x;
            s_gesture[slot].y = y;
            if (gesture_count() == 2)
                s_gesture_since = tick;
            return false;   /* (a lone finger there is also the game's: the menus' taps; only the pair is ours) */
        }
    }
    return false;
}

/* The presenter's call each frame: the held gesture, the repeating - and +, the figure's fade */
static void editor_tick_locked(uint64_t tick)
{
    if (!s_edit_mode && !s_edit_gesture_fired && gesture_count() == 2 && s_gesture_since &&
        tick - s_gesture_since >= 1000 && gesture_zone(s_gesture[0].x, s_gesture[0].y, s_aspect) &&
        gesture_zone(s_gesture[1].x, s_gesture[1].y, s_aspect)) {
        s_edit_gesture_fired = true;
        editor_begin_locked(s_profile);
    }
    if (s_edit_mode && s_repeat_action >= 0 && tick >= s_repeat_next_ms) {
        bool held = false;
        for (int i = 0; i < MAX_FINGERS; i++)
            if (s_finger[i].used && s_finger[i].editing && s_finger[i].ui_action == s_repeat_action) {
                held = true;
                break;
            }
        if (held) {
            if (s_repeat_action == EDIT_UI_SIZE_DEC || s_repeat_action == EDIT_UI_SIZE_INC)
                editor_step_size_locked(s_repeat_action == EDIT_UI_SIZE_INC ? 0.05f : -0.05f);
            else
                editor_step_opacity_locked(s_repeat_action == EDIT_UI_OPACITY_INC ? 0.05f : -0.05f);
            s_repeat_count++;
            s_repeat_next_ms = tick + (s_repeat_count < 5 ? 130 : 80);
            s_serial++;
        } else {
            s_repeat_action = -1;
        }
    }
    if (s_edit_mode && s_edit_value_kind && tick >= s_edit_value_until) {
        if (tick >= s_edit_value_until + 150)
            s_edit_value_kind = 0;
        s_serial++;
    }
}

/* ---- touch-layout.ini ---- */

static void ini_path(char *out, size_t n, int make)
{
    char dir[1024];
    ngb_config_dir(dir, sizeof dir, make);
    snprintf(out, n, "%s/touch-layout.ini", dir);
}

/* At start (under s_lock): the places, sizes, opacities and visibility the player kept, and the editor's settings.
 * Lines of version 3 (0.9.66: eight numbers, before the layouts) are skipped: that file's controls were not these. */
static void ini_load(void)
{
    for (int p = 0; p < PROFILES; p++)
        for (int l = 0; l < LAYOUTS; l++)
            for (int i = 0; i < NCTRL; i++)
                ov_default(&s_ov[p][l][i], i);
    char path[1200], line[256];
    ini_path(path, sizeof path, 0);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    while (fgets(line, sizeof line, f)) {
        if (line[0] == '#')
            continue;
        if (!strncmp(line, "pref ", 5)) {
            int collapsed, grid, snap, grid_size, skin = 0;
            if (sscanf(line + 5, "%d %d %d %d %d", &collapsed, &grid, &snap, &grid_size, &skin) >= 4) {
                s_editor_collapsed = collapsed != 0;
                s_grid_enabled = grid != 0;
                s_snap_enabled = snap != 0;
                s_grid_size = grid_size < 0 ? 0 : grid_size > 2 ? 2 : grid_size;
                s_skin = skin < 0 ? 0 : skin > 2 ? 2 : skin;
            }
            continue;
        }
        int profile, layout, id, custom, visible;
        float x, y, r, opacity;
        if (sscanf(line, "%d %d %d %f %f %f %d %f %d", &profile, &layout, &id, &x, &y, &r, &custom, &opacity,
                   &visible) != 9)
            continue;
        if (profile < 0 || profile >= PROFILES || layout < 0 || layout >= LAYOUTS || id < 0 || id >= NCTRL ||
            !isfinite(x) || !isfinite(y) || !isfinite(r) || !isfinite(opacity) || x < -3.0f || x > 3.0f ||
            y < 0.02f || y > 0.98f || r < 0.025f || r > 0.40f || (custom != 0 && custom != 1) ||
            opacity < 0.10f || opacity > 1.0f || (visible != 0 && visible != 1))
            continue;
        Ov *o = &s_ov[profile][layout][id];
        o->x = x;
        o->y = y;
        o->r = r;
        o->custom = custom != 0;
        o->opacity = opacity;
        o->visible = visible != 0;
    }
    fclose(f);
}

/* Written on the game's side (ngb_touch_map), never on Android's UI thread; not under s_lock */
static void ini_save(void)
{
    Ov ov[PROFILES][LAYOUTS][NCTRL];
    bool collapsed, grid, snap;
    int grid_size, skin;
    pthread_mutex_lock(&s_lock);
    memcpy(ov, s_ov, sizeof ov);
    collapsed = s_editor_collapsed;
    grid = s_grid_enabled;
    snap = s_snap_enabled;
    grid_size = s_grid_size;
    skin = s_skin;
    pthread_mutex_unlock(&s_lock);
    char path[1200], tmp[1280];
    ini_path(path, sizeof path, 1);
    snprintf(tmp, sizeof tmp, "%s.tmp.%ld", path, (long)getpid());
    FILE *f = fopen(tmp, "w");
    if (!f)
        return;
    fprintf(f, "# NGB virtual controls layout, version 4; profile 0=gameplay 1=menu; layout 0=default 1=classic\n");
    fprintf(f, "pref %d %d %d %d %d\n", collapsed ? 1 : 0, grid ? 1 : 0, snap ? 1 : 0, grid_size, skin);
    for (int p = 0; p < PROFILES; p++)
        for (int l = 0; l < LAYOUTS; l++)
            for (int i = 0; i < NCTRL; i++)
                fprintf(f, "%d %d %d %.6f %.6f %.6f %d %.4f %d\n", p, l, i, ov[p][l][i].x, ov[p][l][i].y,
                        ov[p][l][i].r, ov[p][l][i].custom ? 1 : 0, ov[p][l][i].opacity, ov[p][l][i].visible ? 1 : 0);
    int ok = fflush(f) == 0 && fsync(fileno(f)) == 0;
    if (fclose(f) != 0)
        ok = 0;
    if (!ok || rename(tmp, path) != 0)
        unlink(tmp);
}


