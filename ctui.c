#define _POSIX_C_SOURCE 199309L

#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <ncurses.h>

#include "arena.h"
#include "hash.h"
#include "ctui.h"

DEFINE_HASHMAP(CTUI_Components_HashMap, CTUI_Component *);

enum {key_escape = 27};

CTUI_Context ctui_ctx;

CTUI_Context *get_current_ctx() {
    return &ctui_ctx;
}

static const char *log_level_values[] = {
    "INFO", "DEBUG", "WARNING", "ERROR"
};

void ctui_log(LOG_LEVEL level, const char *fmt, ...) {
    CTUI_Context *ctx = get_current_ctx();
    va_list args;
    va_start(args, fmt);
    fprintf(ctx->log_config->f_stream, "[%s]: ", log_level_values[level]);
    vfprintf(ctx->log_config->f_stream, fmt, args);
    fprintf(ctx->log_config->f_stream, "\n");
}

void print_component(CTUI_Component *comp){
    ctui_log(LOG_INFO, "----------------------- CTUI COMPONENT START -----------------------");
    ctui_log(LOG_INFO, "------------- component ID config -------------");
    ctui_log(LOG_INFO, "id = %d", comp->id.value);
    ctui_log(LOG_INFO, "string id = %s", comp->id.string_value);
    ctui_log(LOG_INFO, "------------- COMPONENT ID config -------------");

    ctui_log(LOG_INFO, "------------- component PADDING config -------------");
    ctui_log(LOG_INFO, "padding top = %d", comp->data->padding.top);
    ctui_log(LOG_INFO, "padding bottom = %d", comp->data->padding.bottom);
    ctui_log(LOG_INFO, "padding left = %d", comp->data->padding.left);
    ctui_log(LOG_INFO, "padding right = %d", comp->data->padding.right);
    ctui_log(LOG_INFO, "------------- component PADDING config -------------");

    ctui_log(LOG_INFO, "------------- component BORDER config -------------");
    ctui_log(LOG_INFO, "border top = %d", comp->data->border.width.top);
    ctui_log(LOG_INFO, "border bottom = %d", comp->data->border.width.bottom);
    ctui_log(LOG_INFO, "border left = %d", comp->data->border.width.left);
    ctui_log(LOG_INFO, "border right = %d", comp->data->border.width.right);
    ctui_log(LOG_INFO, "------------- component BORDER config -------------");

    ctui_log(LOG_INFO, "max width = %.10f", comp->data->sizes.width.minmax.max);
    ctui_log(LOG_INFO, "max height = %.5f", comp->data->sizes.height.minmax.max);

    switch (comp->type) {
        case COMPONENT_TYPE_CONTAINER: {
            break;
        }
        case COMPONENT_TYPE_LABEL:
        case COMPONENT_TYPE_TEXT:
        case COMPONENT_TYPE_BUTTON: {
            ctui_log(LOG_INFO, "------------- component TEXT config -------------");
            ctui_log(LOG_INFO, "label text = %s", comp->data->text.value);
            ctui_log(LOG_INFO, "------------- component TEXT config -------------");
            break;
        }
    }
    if (comp->computed) {
        ctui_log(LOG_INFO, "------------- component COMPUTED config -------------");
        ctui_log(LOG_INFO, "width = %d", comp->computed->bounding_box.width);
        ctui_log(LOG_INFO, "height = %d", comp->computed->bounding_box.height);
        ctui_log(LOG_INFO, "x = %d", comp->computed->bounding_box.x);
        ctui_log(LOG_INFO, "y = %d", comp->computed->bounding_box.y);
        ctui_log(LOG_INFO, "------------- component COMPUTED config -------------");
    }
    ctui_log(LOG_INFO, "----------------------- CTUI COMPONENT END -----------------------\n\n");
}

void print_log_ctui_ctx()
{
    CTUI_Context *ctx = get_current_ctx();
    ctui_log(LOG_INFO, "------------- CTUI CONTEXT INFO START -------------");
    ctui_log(LOG_INFO, "ctx screen width = %d", ctx->screen_config->width);
    ctui_log(LOG_INFO, "ctx screen height = %d", ctx->screen_config->height);
    ctui_log(LOG_INFO, "ctx arena end capacity = %d",  (int)ctx->arena->end->capacity);
    ctui_log(LOG_INFO, "ctx components hash map length (total components count) = %d", (int)ctx->components_hm->length);
    ctui_log(LOG_INFO, "ctx components hash map capacity = %d", (int)ctx->components_hm->capacity);
    ctui_log(LOG_INFO, "ctx open components stack length = %d", (int)ctx->open_components_stack->length);
    ctui_log(LOG_INFO, "------------- CTUI CONTEXT INFO END -------------\n\n");
}

void init_ctx(Arena *a) {
    CTUI_Components_HashMap *comp_hm;
    CTUI_Open_Components_Stack *o_comp_stack;
    CTUI_LogConfig *log_conf; 
    CTUI_Screen_Config *screen_conf; 
    FILE *log_f;

    arena_init_hm(a, comp_hm, 100);

    o_comp_stack = arena_alloc(a, sizeof(*o_comp_stack));

    log_conf = arena_alloc(a, sizeof(*log_conf));
    log_conf->log_file_path = "./logs/ctui_log.txt";
    log_f = fopen(log_conf->log_file_path, "a");
    if (!log_f) {
        perror(log_conf->log_file_path);
        exit(2);
    }
    log_conf->f_stream = log_f;

    screen_conf = arena_alloc(a, sizeof(*screen_conf));
    screen_conf->win = stdscr;
    getmaxyx(stdscr, screen_conf->height, screen_conf->width);

    CTUI_Context *ctx = get_current_ctx();
    ctx->arena                  = a;
    ctx->screen_config          = screen_conf;
    ctx->components_hm          = comp_hm;
    ctx->current_element        = NULL;
    ctx->open_components_stack  = o_comp_stack;
    ctx->log_config             = log_conf;
};

void close_ctx()
{
    CTUI_Context *ctx = get_current_ctx();
    fclose(ctx->log_config->f_stream);
}

void CTUI_Open_Components_Stack_Push(CTUI_Component_Id id) 
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Open_Components_Stack_Node *tmp;
    tmp = arena_alloc(ctx->arena, sizeof(*tmp));
    tmp->comp_id = id;
    tmp->next = ctx->open_components_stack->top; 
    ctx->open_components_stack->top = tmp;
    ctx->open_components_stack->length++;
}
CTUI_Open_Components_Stack_Node *CTUI_Open_Components_Stack_Pop()
{
    CTUI_Context *ctx = get_current_ctx();
    if (!ctx->open_components_stack->top)
        return NULL;
    CTUI_Open_Components_Stack_Node *tmp = ctx->open_components_stack->top; 
    ctx->open_components_stack->top = ctx->open_components_stack->top->next;
    ctx->open_components_stack->length--;
    return tmp;
}

CTUI_Component *get_parent_component()
{
    CTUI_Context *ctx = get_current_ctx();
    if (ctx->open_components_stack->length <= 1) 
        return NULL;
    CTUI_Open_Components_Stack_Node *next_node;
    next_node = ctx->open_components_stack->top->next;
    return CTUI_Components_HashMap_get(
        ctx->components_hm, 
        next_node->comp_id.value, 
        next_node->comp_id.string_value
    )->value;
}

CTUI_Component_Id make_component_id(const char *key)
{
    CTUI_Context *ctx = get_current_ctx();

    uint32_t hash = fnv1a(key);
    CTUI_Component *parent = get_parent_component();
    uint32_t p_id = parent ? parent->id.value : 0;
    uint64_t composite_key = make_key(p_id, hash);

    CTUI_Component_Id component_id;
    component_id.value = composite_key;
    component_id.base_value = p_id;
    component_id.string_value = arena_strdup(ctx->arena, key);
    return component_id;
}

CTUI_Component_Children *init_children()
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Component_Children *children;
    children = arena_alloc(ctx->arena, sizeof(*children));
    children->head = children->tail = NULL;
    children->length = 0;
    return children;
}

void append_child(
    CTUI_Component_Children *children,
    CTUI_Component *comp
) {
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Component_Child *child;
    child = arena_alloc(ctx->arena, sizeof(*child));
    child->comp = comp;
    child->next = NULL;
    if (!children->head) {
        child->prev = NULL;
        children->head = children->tail = child;
    } else {
        child->prev = children->tail;
        children->tail->next = child;
        children->tail = children->tail->next;
    }
    ++children->length;
}

void pin_component_to_parent(CTUI_Component *parent, CTUI_Component *comp)
{
    if (parent->type == COMPONENT_TYPE_CONTAINER) {
        comp->parent = parent; 
        append_child(parent->data->container.children, comp);
    }
}

void open_component_with_id(CTUI_Component_Id id)
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Open_Components_Stack *o_stack = ctx->open_components_stack;
    CTUI_Component *open_comp = arena_alloc(ctx->arena, sizeof(*open_comp));
    open_comp->id = id;
    open_comp->data = arena_alloc(ctx->arena, sizeof(*open_comp->data));
    open_comp->computed = arena_alloc(ctx->arena, sizeof(*open_comp->computed));
    open_comp->computed->bounding_box = (CTUI_BoundingBox) {0};
    if (o_stack->length > 0) {
        CTUI_Component *parent = CTUI_Components_HashMap_get(
            ctx->components_hm,
            o_stack->top->comp_id.value, 
            o_stack->top->comp_id.string_value
        )->value; 
        pin_component_to_parent(parent, open_comp);
    }
    CTUI_Components_HashMap_set(
        ctx->arena, 
        ctx->components_hm, 
        id.value, 
        id.string_value, 
        open_comp
    ); 
    CTUI_Open_Components_Stack_Push(id);
    ctx->current_element = open_comp;
}

void configure_open_component(CTUI_ComponentType type, CTUI_Component_Data data)
{
    CTUI_Context *ctx = get_current_ctx(); 
    if (ctx->current_element) {
        ctx->current_element->type = type; 
        *ctx->current_element->data = data;
        ctui_log(LOG_DEBUG, "cur component -> %s", ctx->current_element->id.string_value);
        if (ctx->current_element->parent)
            ctui_log(LOG_DEBUG, "parent component -> %s", ctx->current_element->parent->id.string_value);
    }
}

void close_component()
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Open_Components_Stack_Pop();
    CTUI_Open_Components_Stack *o_stack = ctx->open_components_stack; 
    if (o_stack->top) {
        ctx->current_element = CTUI_Components_HashMap_get(
            ctx->components_hm, 
            o_stack->top->comp_id.value, 
            o_stack->top->comp_id.string_value
        )->value;
    } else {
        ctx->current_element = NULL;
    } 
}

void begin_layout()
{
    CTUI_Context *ctx = get_current_ctx();   
    CTUI_Component_Id root_id = make_component_id("root");
    open_component_with_id(root_id);
    configure_open_component(
        COMPONENT_TYPE_CONTAINER, 
        CTUI__CONFIG_WRAPPER(
            CTUI_Component_Data, {
            .x_alignment = X_LEFT,
            .y_alignment = Y_TOP,
            .container = {
                .flow_dir = FLOW_DIR_ROW,
                .children = init_children() 
            },
            .sizes = {
                .width  = {
                    .type = SIZE_GROW, 
                    .minmax = {.min = 0, .max = ctx->screen_config->width}
                },
                .height = {
                    .type = SIZE_GROW, 
                    .minmax = {.min = 0, .max = ctx->screen_config->height}
                },
            },
            .border={.width = {.top = 1, .bottom = 1, .left = 1, .right = 1}},
            .padding = {.top = 1, .left = 2, .right = 2, .bottom = 1},
        })
    );
    ctx->root = CTUI_Components_HashMap_get(
        ctx->components_hm, 
        root_id.value, 
        root_id.string_value
    )->value;
}

void end_layout()
{
    close_component();
}

void header()
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI("header", COMPONENT_TYPE_CONTAINER, {
        .sizes = {
            .width = {.type = SIZE_GROW},
            .height= {.type = SIZE_GROW}
        },
        .border = {.width = {.left = 1, .right = 1, .top = 1, .bottom = 1}},
        .padding = {.top = 1, .bottom = 1, .left = 2, .right = 2},
        .x_alignment = X_CENTER,
        .y_alignment = Y_CENTER,
        .container = {
            .flow_dir = FLOW_DIR_COLUMN,
            .children = init_children() 
        }
    }) {
        CTUI("content", COMPONENT_TYPE_CONTAINER, {
            .sizes = {
                .width = {.type = SIZE_GROW},
                .height= {.type = SIZE_GROW}
            },
            .border = {.width = {.left = 1, .right = 1, .top = 1, .bottom = 1}},
            .padding = {.top = 1, .bottom = 1, .left = 2, .right = 2},
            .x_alignment = X_LEFT,
            .y_alignment = Y_TOP,
            .container = {
                .flow_dir = FLOW_DIR_ROW,
                .children = init_children() 
            }
        }) {
            CTUI("title", COMPONENT_TYPE_LABEL, {
                .text = { .value = arena_strdup(ctx->arena, "h title") },
                .sizes = {
                    .width = {.type = SIZE_GROW},
                    .height = {.type = SIZE_FIXED, .fixvalue = 2}
                },
                .padding = {0},
                .border = {{0}}
            });
            CTUI("title2", COMPONENT_TYPE_LABEL, {
                .sizes = {
                    .width = {.type = SIZE_GROW, .minmax = {.min = 20, .max = 40}},
                    .height = {.type = SIZE_FIXED, .fixvalue = 5}
                },
                .text = { .value = arena_strdup(ctx->arena, "h title 2") },
                .x_alignment = X_CENTER,
                .y_alignment = Y_CENTER,
                .padding = {0},
                .border = {
                    .width = {.top = 1, .bottom = 1, .left = 1, .right = 1}
                }
            });
            CTUI("subtitle", COMPONENT_TYPE_LABEL, {
                .sizes = {
                    .width = {.type = SIZE_FIXED, .fixvalue = 20},
                    .height = {.type = SIZE_FIXED, .fixvalue = 4}
                },
                .text = { .value = arena_strdup(ctx->arena, "h subtitle") },
                .x_alignment = X_RIGHT,
                .y_alignment = Y_BOTTOM,
                .padding = {0},
                .border = {
                    .width = {.top = 1, .bottom = 1, .left = 1, .right = 1}
                }
            });
        }
    }
    CTUI("content-bottom", COMPONENT_TYPE_CONTAINER, {
        .sizes = {
            .width = {.type = SIZE_GROW},
            .height= {.type = SIZE_GROW}
        },
        .border = {.width = {.left = 1, .right = 1, .top = 1, .bottom = 1}},
        .padding = {.top = 1, .bottom = 1, .left = 2, .right = 2},
        .x_alignment = X_LEFT,
        .y_alignment = Y_TOP,
        .container = {
            .flow_dir = FLOW_DIR_ROW,
            .children = init_children() 
        }
    });
}

void init_x_flow_refs(CTUI_Flow_Ref *ref, CTUI_Component *comp)
{
    ref->bbox_value = &comp->computed->bounding_box.width; 
    ref->sizing = &comp->data->sizes.width;
}

void init_y_flow_refs(CTUI_Flow_Ref *ref, CTUI_Component *comp)
{
    ref->bbox_value = &comp->computed->bounding_box.height; 
    ref->sizing = &comp->data->sizes.height;
}

void init_x_mod_flow_refs(CTUI_Modifier_Flow_Ref *ref, CTUI_Component *comp)
{
    ref->border_a  = comp->data->border.width.left;
    ref->border_b  = comp->data->border.width.right;
    ref->padding_a = comp->data->padding.left;
    ref->padding_b = comp->data->padding.right;
}

void init_y_mod_flow_refs(CTUI_Modifier_Flow_Ref *ref, CTUI_Component *comp)
{
    ref->border_a  = comp->data->border.width.top;
    ref->border_b  = comp->data->border.width.bottom;
    ref->padding_a = comp->data->padding.top;
    ref->padding_b = comp->data->padding.bottom;
}

void init_flow_refs(
    CTUI_Component_Flow_Dir dir,
    CTUI_Component *comp,
    CTUI_Flow_Refs *refs
) {
    if (dir == FLOW_DIR_ROW) {
        init_x_flow_refs(&refs->main, comp);
        init_x_mod_flow_refs(&refs->main.mod_ref, comp);
        init_y_flow_refs(&refs->sec, comp);
        init_y_mod_flow_refs(&refs->sec.mod_ref, comp);
    } else {
        init_x_flow_refs(&refs->sec, comp);
        init_x_mod_flow_refs(&refs->sec.mod_ref, comp);
        init_y_flow_refs(&refs->main, comp);
        init_y_mod_flow_refs(&refs->main.mod_ref, comp);
    } 
}


int calc_comp_spacing(CTUI_Modifier_Flow_Ref ref)
{
    int res;
    res = ref.padding_a; 
    res += ref.padding_b; 
    res += ref.border_a; 
    res += ref.border_b;  
    return res;
}

void sl_append_child(CTUI_Component_SL_Child **head, CTUI_Component *comp)
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Component_SL_Child *new_child;
    new_child = arena_alloc(ctx->arena, sizeof(*new_child)); 
    new_child->comp = comp;
    new_child->next = *head;
    *head = new_child;
}

void measure_comp_maxsize(CTUI_Component *comp)
{
    if (!comp) {
        return; 
    }
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Flow_Refs f_refs = {0}, ch_f_refs = {0}, p_f_refs = {0};
    CTUI_Component_Child *tmp;
    int grow_children_len = 0;

    init_flow_refs(comp->data->container.flow_dir, comp, &f_refs);

    if (comp->parent) {
        init_flow_refs(comp->data->container.flow_dir, comp->parent, &p_f_refs);
        if (f_refs.main.sizing->minmax.max == 0)
            f_refs.main.sizing->minmax.max =
                p_f_refs.main.sizing->minmax.max;
        if (f_refs.sec.sizing->minmax.max == 0)
            f_refs.sec.sizing->minmax.max = 
                p_f_refs.sec.sizing->minmax.max;
    } else {
        if (comp->data->container.flow_dir == FLOW_DIR_ROW) {
            f_refs.main.sizing->minmax.max = ctx->screen_config->width;
            f_refs.sec.sizing->minmax.max = ctx->screen_config->height;
        } else {
            f_refs.main.sizing->minmax.max = ctx->screen_config->height;
            f_refs.sec.sizing->minmax.max = ctx->screen_config->width;
        }
    } 

    tmp = comp->data->container.children->head;
    int occupied_space = 0; 
    while(tmp) {
        init_flow_refs(comp->data->container.flow_dir, tmp->comp, &ch_f_refs);
        switch(ch_f_refs.main.sizing->type){
        case SIZE_FIXED:
            ch_f_refs.main.sizing->minmax.max = ch_f_refs.main.sizing->fixvalue; 
            occupied_space += ch_f_refs.main.sizing->minmax.max;
            break;
        case SIZE_PERCENT:
            ch_f_refs.main.sizing->minmax.max = ceilf(
                (
                 f_refs.main.sizing->minmax.max
                 - calc_comp_spacing(f_refs.main.mod_ref)
                ) * ch_f_refs.main.sizing->percent
            ); 
            occupied_space += ch_f_refs.main.sizing->minmax.max;
            break;
        case SIZE_GROW:
            ++grow_children_len;
            break;
        }
        switch(ch_f_refs.sec.sizing->type){
        case SIZE_FIXED:
            ch_f_refs.sec.sizing->minmax.max = ch_f_refs.sec.sizing->fixvalue; 
            break;
        case SIZE_PERCENT:
            ch_f_refs.sec.sizing->minmax.max = ceilf(
                (
                 f_refs.sec.sizing->minmax.max
                 - calc_comp_spacing(f_refs.sec.mod_ref)
                ) * ch_f_refs.sec.sizing->percent
            ); 
            break;
        case SIZE_GROW:
            ch_f_refs.sec.sizing->minmax.max = 
                f_refs.sec.sizing->minmax.max
                - calc_comp_spacing(f_refs.sec.mod_ref); 
            break;
        }
        tmp = tmp->next;
    }

    comp->data->container.children->grow_children_length = grow_children_len;

    tmp = comp->data->container.children->head;
    while(tmp) {
        init_flow_refs(comp->data->container.flow_dir, tmp->comp, &ch_f_refs);
        if (ch_f_refs.main.sizing->type == SIZE_GROW) {
            if (tmp->comp->type == COMPONENT_TYPE_CONTAINER) {
                ch_f_refs.main.sizing->minmax.max = ceilf(
                    (
                        f_refs.main.sizing->minmax.max 
                        - calc_comp_spacing(f_refs.main.mod_ref) 
                        - occupied_space
                    ) / grow_children_len
                );
            } else {
                ch_f_refs.main.sizing->minmax.max = ceilf(
                    (
                        f_refs.main.sizing->minmax.max
                        - calc_comp_spacing(f_refs.main.mod_ref) 
                        - occupied_space
                    ) / grow_children_len
                );
                ctui_log(LOG_DEBUG, "ch_f_refs.main.sizing->minmax.max -> %.2f", ch_f_refs.main.sizing->minmax.max);
            }
        }
        if (tmp->comp->type == COMPONENT_TYPE_CONTAINER)
            measure_comp_maxsize(tmp->comp);
        tmp = tmp->next;
    }
}

void calc_children_main_sizes(
    CTUI_Component_Flow_Dir flow_dir,
    CTUI_Component *comp,
    CTUI_Flow_Ref f_ref,
    CTUI_Flow_Ref p_f_ref
) {
    switch (f_ref.sizing->type) {
    case SIZE_FIXED: 
        *f_ref.bbox_value = f_ref.sizing->fixvalue;
        break;
    case SIZE_PERCENT: 
        *f_ref.bbox_value = f_ref.sizing->minmax.max;
        break;
    case SIZE_GROW: 
        *f_ref.bbox_value = f_ref.sizing->minmax.max;
        break;
    }
}

void calc_children_sec_sizes(
    CTUI_Component_Flow_Dir dir,
    CTUI_Flow_Ref flow_ref,
    CTUI_Flow_Ref p_flow_ref
) {
    switch (flow_ref.sizing->type) {
    case SIZE_FIXED:
        *flow_ref.bbox_value = flow_ref.sizing->fixvalue;
        break;
    case SIZE_PERCENT:
        *flow_ref.bbox_value = flow_ref.sizing->minmax.max; 
        break;
    case SIZE_GROW:
        *flow_ref.bbox_value = flow_ref.sizing->minmax.max;
        break;
    }
}

void measure_comp(CTUI_Component *comp) 
{
    if (!comp) 
        return;
    CTUI_Component_Child *child;
    CTUI_Component_Flow_Dir flow_dir = comp->data->container.flow_dir;
    CTUI_Flow_Refs f_refs = {0}, ch_f_refs = {0};

    init_flow_refs(flow_dir, comp, &f_refs);

    if (!comp->parent) {
        *f_refs.main.bbox_value = f_refs.main.sizing->minmax.max;
        *f_refs.sec.bbox_value = f_refs.sec.sizing->minmax.max;
    } 

    child = comp->data->container.children->head;
    while (child) { 
        init_flow_refs(flow_dir, child->comp, &ch_f_refs);
        calc_children_main_sizes(flow_dir, child->comp, ch_f_refs.main, f_refs.main);
        calc_children_sec_sizes(flow_dir, ch_f_refs.sec, f_refs.sec);
        if (child->comp->type == COMPONENT_TYPE_CONTAINER)
            measure_comp(child->comp);
        child = child->next;
    }
}

void calculate_comp_layout(CTUI_Component *comp)
{
    if (!comp) {
        return;
    }
    CTUI_Component_Child *child;

    if (!comp->parent) {
        comp->computed->bounding_box.x = 0;
        comp->computed->bounding_box.y = 0;
    } 

    child = comp->data->container.children->head;
    int x_offset = 0, y_offset = 0;
    while (child) { 
        child->comp->computed->bounding_box.x = 
            comp->computed->bounding_box.x
            + comp->data->border.width.left
            + comp->data->padding.left
            + x_offset;
        child->comp->computed->bounding_box.y = 
            + comp->computed->bounding_box.y
            + comp->data->border.width.top
            + comp->data->padding.top
            + y_offset;
        if (comp->data->container.flow_dir == FLOW_DIR_ROW) 
            x_offset += child->comp->computed->bounding_box.width;
        else 
            y_offset += child->comp->computed->bounding_box.height;
        if (child->comp->type == COMPONENT_TYPE_CONTAINER) 
            calculate_comp_layout(child->comp);
        child = child->next;
    }
}

void draw_border_corner(
    WINDOW *win, uint16_t a, uint16_t b, uint16_t y, uint16_t x, int c
) {
    if (a > 0 && b > 0)
        mvwaddch(win, y, x, c);
    else if (a > 0)
        mvwaddch(win, y, x, c);
    else if (b > 0)
        mvwaddch(win, y, x, c);
}

void draw_border(WINDOW *win, CTUI_Component *comp)
{
    CTUI_BoundingBox *box = &comp->computed->bounding_box;
    CTUI_Component_Border *br = &comp->data->border; 

    draw_border_corner(
        win, 
        br->width.left, 
        br->width.top, 
        box->y, 
        box->x, 
        ACS_ULCORNER
    );
    draw_border_corner(
        win, 
        br->width.left, 
        br->width.bottom, 
        box->y + box->height - 1, 
        box->x,
        ACS_LLCORNER
    );
    draw_border_corner(
        win, 
        br->width.right, 
        br->width.top, 
        box->y, 
        box->x + box->width - 1, 
        ACS_URCORNER
    );
    draw_border_corner(
        win, 
        br->width.right, 
        br->width.bottom, 
        box->y + box->height - 1, 
        box->x + box->width - 1,
        ACS_LRCORNER
    );

    if (br->width.top > 0) 
        mvwhline(win, box->y, box->x + 1,  0, box->width - 2);
    if (br->width.bottom > 0) 
        mvwhline(win, box->y + box->height - 1, box->x + 1, 0, box->width - 2);
    if (br->width.left > 0) 
        mvwvline(win, box->y + 1, box->x,  0, box->height - 2);
    if (br->width.right > 0) 
        mvwvline(win, box->y + 1, box->x + box->width - 1, 0, box->height - 2);
}

void draw_comp(CTUI_Component *comp) {
    CTUI_Context *ctx = get_current_ctx();
    WINDOW *win = ctx->screen_config->win;
    CTUI_BoundingBox *box = &comp->computed->bounding_box;

    draw_border(win, comp);

    if (comp->type != COMPONENT_TYPE_CONTAINER) {

        float x, y;
        size_t text_len = strlen(comp->data->text.value);
        switch(comp->data->x_alignment){
        case X_LEFT:
            x = box->x + comp->data->padding.left + comp->data->border.width.left;
            break;
        case X_CENTER:
            x = ceilf(box->x + (float)box->width / 2) - ((float)text_len / 2);
            break;
        case X_RIGHT:
            x = box->x + (box->width - comp->data->border.width.right - comp->data->padding.right) - (float)text_len;
            break;
        }
        switch (comp->data->y_alignment) {
        case Y_TOP:
            y = box->y + comp->data->padding.top + comp->data->border.width.top;
            break;
        case Y_CENTER:
            y = ceilf(box->y + (float)box->height / 2) - 1;
            break;
        case Y_BOTTOM:
            y = box->y + (box->height - comp->data->border.width.bottom - comp->data->padding.bottom) - 1;
            break;
        }
        mvwaddstr(win, y, x, comp->data->text.value);
    }
    print_component(comp);
}

void draw_layout(CTUI_Component *comp)
{
    if (!comp) {
        return;
    }
    CTUI_Component_Child *tmp;

    tmp = comp->data->container.children->head;

    draw_comp(comp);

    while (tmp) {
        if (tmp->comp->type == COMPONENT_TYPE_CONTAINER) {
            draw_layout(tmp->comp);
        } else {
            draw_comp(tmp->comp);
        }
        tmp = tmp->next;
    }
}

void render_layout() 
{
    CTUI_Context *ctx = get_current_ctx(); 
    if (!ctx->root) {
        ctui_log(LOG_ERROR, "Root component is not defined!");
    }
    measure_comp_maxsize(ctx->root);
    measure_comp(ctx->root);
    calculate_comp_layout(ctx->root);
    clear();
    draw_layout(ctx->root);
    refresh();
    
}

void handle_resize()
{
    CTUI_Context *ctx = get_current_ctx();
    getmaxyx(stdscr, ctx->screen_config->height, ctx->screen_config->width);
    resizeterm(ctx->screen_config->height, ctx->screen_config->width);
    render_layout();
}

int main(int argc, char **argv) {
    int key;
    Arena arena = {0};
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, 1);

    init_ctx(&arena);

    begin_layout();
    header();
    end_layout();

    render_layout();

    struct timespec now, last_resize = {0, 0};
    long diff_ms;
    while ((key = getch()) != key_escape) {
        switch (key) {
        case 'q':
            goto shutdown;
            break;
        case 'r': 
            clear();
            handle_resize();
            break;
        case KEY_RESIZE:
            clock_gettime(CLOCK_MONOTONIC, &now);
            diff_ms = (now.tv_sec - last_resize.tv_sec) * 1000
                         + (now.tv_nsec - last_resize.tv_nsec) / 1000000;
            if (diff_ms > 16) { 
                clear();
                handle_resize();
                last_resize = now;
            }
            break;
        }
    }

shutdown:
    endwin();
    close_ctx();
    return 0;
} 
