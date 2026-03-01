#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <ncurses.h>

#include "arena.h"
#include "hash.h"
#include "ctui.h"

DEFINE_HASHMAP(CTUI_Components_HashMap, CTUI_Component *);

enum {
    key_escape = 27
};

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

void log_component(CTUI_Component *comp){
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


void init_ctx(Arena *a) {
    CTUI_Components_HashMap *comp_hm;
    CTUI_Open_Components_Stack *o_comp_stack;
    CTUI_LogConfig *log_conf; 
    CTUI_Screen_Config *screen_conf; 
    FILE *log_f;

    arena_init_hm(a, comp_hm, 100);

    o_comp_stack = arena_alloc(a, sizeof(*o_comp_stack));

    log_conf = arena_alloc(a, sizeof(*log_conf));
    log_conf->log_file_path = "ctui_log.txt";
    log_f = fopen(log_conf->log_file_path, "a");
    if (!log_f) {
        perror(log_conf->log_file_path);
        exit(2);
    }
    log_conf->f_stream = log_f;

    screen_conf = arena_alloc(a, sizeof(*screen_conf));
    screen_conf->height = LINES;
    screen_conf->width  = COLS;
    screen_conf->win = stdscr;

    CTUI_Context *ctx = get_current_ctx();
    ctx->arena                  = a;
    ctx->screen_config          = screen_conf;
    ctx->components_hm          = comp_hm;
    ctx->current_element        = NULL;
    ctx->open_components_stack  = o_comp_stack;
    ctx->log_config             = log_conf;
};

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

CTUI_Component_Container_Data_Children *init_children()
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Component_Container_Data_Children *children;
    children = arena_alloc(ctx->arena, sizeof(*children));
    children->begin = children->end = NULL;
    return children;
}

CTUI_Component_Container_Data_Child *init_child(CTUI_Component *comp)
{
    CTUI_Context *ctx = get_current_ctx();
    CTUI_Component_Container_Data_Child *child;
    child = arena_alloc(ctx->arena, sizeof(*child));
    child->comp = comp;
    child->next = NULL;
    return child;
}

void append_child(
    CTUI_Component_Container_Data_Children *children,
    CTUI_Component_Container_Data_Child *new_child
) {
    if (!children->begin) {
        children->begin = children->end = new_child;
    } else {
        children->end->next = new_child;
        children->end = children->end->next;
    }
}

void pin_component_to_parent(CTUI_Component *parent, CTUI_Component *comp)
{
    if (parent->type == COMPONENT_TYPE_CONTAINER) {
        comp->parent = parent; 
        CTUI_Component_Container_Data_Child *new_child = init_child(comp);
        append_child(parent->data->container.children, new_child);
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
    CTUI_Open_Components_Stack *o_stack = ctx->open_components_stack; 
    ctui_log(LOG_DEBUG, "stack length = %d", o_stack->length);
    ctui_log(LOG_DEBUG, "in configure input type = %d", type);
    if (ctx->current_element) {
        ctui_log(
            LOG_DEBUG, "in configure component name = %s", 
            ctx->current_element->id.string_value
        );
        ctx->current_element->type = type; 
        *ctx->current_element->data = data;
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
            .container = {
                .x_alignment = X_LEFT,
                .y_alignment = Y_TOP,
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
        .border = {.width = {.left = 1, .right = 1, .top = 1, .bottom = 1}},
        .padding = {.top = 1, .bottom = 1, .left = 2, .right = 2},
        .sizes = {
            .width = {.type = SIZE_PERCENT, .percent = 40},
            .height= {.type = SIZE_FIXED, .percent = 10}
        },
        .container = {
            .x_alignment = X_LEFT,
            .y_alignment = Y_TOP,
            .flow_dir = FLOW_DIR_ROW,
            .children = init_children() 
        }
    }) {
        CTUI("title", COMPONENT_TYPE_LABEL, {
            .text = { .value = arena_strdup(ctx->arena, "h title") },
            .padding = {0},
            .border = {{0}},
            .sizes = {
                .width = {.type = SIZE_GROW},
                .height = {.type = SIZE_FIXED, .fixvalue = 2}
            }
        });
        CTUI("title2", COMPONENT_TYPE_LABEL, {
            .text = { .value = arena_strdup(ctx->arena, "h title 2") },
            .padding = {0},
            .border = {{0}},
            .sizes = {
                .width = {.type = SIZE_GROW, .minmax = {.min = 20, .max = 40}},
                .height = {.type = SIZE_FIXED, .fixvalue = 4}
            }
        });
        CTUI("subtitle", COMPONENT_TYPE_LABEL, {
            .text = { .value = arena_strdup(ctx->arena, "h subtitle") },
            .padding = {0},
            .border = {{0}},
            .sizes = {
                .width = {.type = SIZE_FIXED, .fixvalue = 20},
                .height = {.type = SIZE_FIXED, .fixvalue = 4}
            }
        });
    }
}

int calc_component_x_spacing(CTUI_Component *comp)
{
    int w = 0;
    w += comp->data->padding.left; 
    w += comp->data->padding.right; 
    w += comp->data->border.width.left; 
    w += comp->data->border.width.right;  
    return w;
}

int calc_component_y_spacing(CTUI_Component *comp)
{
    int h = 0;
    h += comp->data->padding.top; 
    h += comp->data->padding.bottom; 
    h += comp->data->border.width.top; 
    h += comp->data->border.width.bottom; 
    return h;
}

int calc_children_len(CTUI_Component_Container_Data_Children *children) {
    CTUI_Component_Container_Data_Child *tmp = children->begin;
    int n = 0;
    while (tmp) {
        ++n;
        tmp = tmp->next;
    } 
    return n;
} 

int calc_percent_component(CTUI_Component *comp) 
{
    CTUI_Component *parent = comp->parent;
    int w;
    w = calc_component_x_spacing(comp);
    w += comp->data->sizes.width.percent * parent->data->sizes.width.minmax.max;
    return w;
}

void init_w_flow_refs(CTUI_Flow_Refs *refs, CTUI_Component *comp)
{
    refs->bbox_value = &comp->computed->bounding_box.width; 
    refs->sizing = &comp->data->sizes.width;
}

void init_h_flow_refs(CTUI_Flow_Refs *refs, CTUI_Component *comp)
{
    refs->bbox_value = &comp->computed->bounding_box.height; 
    refs->sizing = &comp->data->sizes.height;
}

void init_flow_refs(
    CTUI_Component_Flow_Dir dir,
    CTUI_Component *comp, 
    CTUI_Flow_Refs *refs, 
    CTUI_Flow_Refs *opt_refs
) {
    if (dir == FLOW_DIR_ROW) {
        init_w_flow_refs(refs, comp);
        if (opt_refs) 
            init_h_flow_refs(opt_refs, comp);
    } else {
        init_h_flow_refs(refs, comp);
        if (opt_refs) 
            init_w_flow_refs(opt_refs, comp);
    } 
}

void init_x_mod_flow_refs(CTUI_Modifier_Flow_Refs *refs, CTUI_Component *comp)
{
    refs->border_a  = &comp->data->border.width.left;
    refs->border_b  = &comp->data->border.width.right;
    refs->padding_a = &comp->data->padding.left;
    refs->padding_b = &comp->data->padding.right;
}

void init_y_mod_flow_refs(CTUI_Modifier_Flow_Refs *refs, CTUI_Component *comp)
{
    refs->border_a  = &comp->data->border.width.top;
    refs->border_b  = &comp->data->border.width.bottom;
    refs->padding_a = &comp->data->padding.top;
    refs->padding_b = &comp->data->padding.bottom;
}

void init_mod_flow_refs(
    CTUI_Component_Flow_Dir dir,
    CTUI_Component *comp, 
    CTUI_Modifier_Flow_Refs *refs, 
    CTUI_Modifier_Flow_Refs *opt_refs
) {
    if (dir == FLOW_DIR_ROW) {
        init_x_mod_flow_refs(refs, comp);
        if (opt_refs) 
            init_y_mod_flow_refs(opt_refs, comp);
    } else {
        init_y_mod_flow_refs(refs, comp);
        if (opt_refs) 
            init_x_mod_flow_refs(opt_refs, comp);
    } 
}

void resize_growing_children(
    CTUI_Component_Flow_Dir dir,
    CTUI_Component *ch_comp,
    CTUI_Children_Main_Dir_Size_Config *children_sizes
) {
    CTUI_Component_Sizing *p_sizing_ref;
    CTUI_Modifier_Flow_Refs p_modifier_refs = {0};
    CTUI_Flow_Refs grow_child_flow_refs = {0};
    CTUI_Component_Container_Data_Child *tmp;

    init_mod_flow_refs(dir, ch_comp->parent, &p_modifier_refs, NULL);

    if (dir == FLOW_DIR_ROW)
        p_sizing_ref = &ch_comp->parent->data->sizes.width;
    else 
        p_sizing_ref = &ch_comp->parent->data->sizes.height;

    children_sizes->growing = (
        p_sizing_ref->minmax.max
        - *p_modifier_refs.padding_a 
        - *p_modifier_refs.padding_b 
        - *p_modifier_refs.border_a 
        - *p_modifier_refs.border_b
    ) - children_sizes->fixed - children_sizes->percentages;  

    tmp = children_sizes->grow_children.begin;
    while (tmp) {
        if (dir == FLOW_DIR_ROW) 
            init_w_flow_refs(&grow_child_flow_refs, tmp->comp);
        else
            init_h_flow_refs(&grow_child_flow_refs, tmp->comp);
        *grow_child_flow_refs.bbox_value = children_sizes->growing / children_sizes->grow_children_len;
        tmp = tmp->next;
    } 
}

void calc_children_main_dir_sizes(
    CTUI_Component_Flow_Dir flow_dir,
    CTUI_Component *ch_comp,
    CTUI_Flow_Refs *ch_refs,
    CTUI_Children_Main_Dir_Size_Config *children_sizes
) {
    ch_comp->computed->bounding_box.width = calc_component_x_spacing(ch_comp); 
    ch_comp->computed->bounding_box.height = calc_component_y_spacing(ch_comp); 
    switch (ch_refs->sizing->type) {
    case SIZE_FIXED: 
        *ch_refs->bbox_value = ch_refs->sizing->fixvalue;
        children_sizes->fixed += *ch_refs->bbox_value;
        resize_growing_children(flow_dir, ch_comp, children_sizes);
        break;
    case SIZE_GROW: 
        append_child(&children_sizes->grow_children, init_child(ch_comp));
        ++children_sizes->grow_children_len;
        resize_growing_children(flow_dir, ch_comp, children_sizes);
        break;
    case SIZE_PERCENT: 
        *ch_refs->bbox_value += calc_percent_component(ch_comp);
        children_sizes->percentages += *ch_refs->bbox_value;
        break;
    }
}

void calc_children_secondary_dir_sizes(
    CTUI_Component_Flow_Dir dir,
    CTUI_Component *ch_comp, 
    CTUI_Flow_Refs *ch_refs, 
    int child_grow_qt
) {
    int h, available_height;
    CTUI_Component_Sizing *parent_sizing;

    if (dir == FLOW_DIR_ROW) 
        parent_sizing = &ch_comp->parent->data->sizes.height;
    else
        parent_sizing = &ch_comp->parent->data->sizes.width;

    switch (ch_refs->sizing->type) {
        case SIZE_FIXED:
            *ch_refs->bbox_value += ch_refs->sizing->fixvalue;
            break;
        case SIZE_PERCENT:
            *ch_refs->bbox_value += ch_refs->sizing->fixvalue;
            h = parent_sizing->minmax.max / ch_refs->sizing->percent; 
            ch_comp->computed->bounding_box.height += h;
            break;
        case SIZE_GROW:
            available_height = parent_sizing->minmax.max;  
            *ch_refs->bbox_value += available_height / child_grow_qt;
            break;
    }
}

void measure_component(CTUI_Component *comp) 
{
    if (!comp) 
        return;
    int x_spacing, y_spacing; 
    CTUI_Component_Container_Data_Child *child;
    CTUI_Children_Main_Dir_Size_Config children_sizes = {0}; 
    CTUI_Component_Flow_Dir flow_dir;
    CTUI_Flow_Refs 
        main_refs       = {0}, 
        sec_refs        = {0}, 
        child_main_refs = {0}, 
        child_sec_refs  = {0};
    CTUI_Modifier_Flow_Refs mod_refs = {0};
    bool need_to_calc_sec_size = false;

    x_spacing = calc_component_x_spacing(comp);
    y_spacing = calc_component_y_spacing(comp);

    flow_dir = comp->data->container.flow_dir;
    init_flow_refs(flow_dir, comp, &main_refs, &sec_refs);
    init_mod_flow_refs(flow_dir, comp, &mod_refs, NULL);

    if (sec_refs.sizing->type == SIZE_FIXED){
        *sec_refs.bbox_value += sec_refs.sizing->fixvalue;
    } else {
        need_to_calc_sec_size = true;
        *sec_refs.bbox_value += sec_refs.sizing->minmax.max;
    } 

    if (comp->parent) {
        CTUI_Modifier_Flow_Refs p_main_mod_refs = {0};
        CTUI_Component_Sizing *parent_sizing;

        init_mod_flow_refs(flow_dir, comp->parent, &p_main_mod_refs, NULL);

        if (flow_dir == FLOW_DIR_ROW)
            parent_sizing = &comp->parent->data->sizes.width;
        else 
            parent_sizing = &comp->parent->data->sizes.height;

        main_refs.sizing->minmax.min = parent_sizing->minmax.min; 
        main_refs.sizing->minmax.max = 
            parent_sizing->minmax.max
            - *p_main_mod_refs.border_a
            - *p_main_mod_refs.border_b
            - *p_main_mod_refs.padding_a
            - *p_main_mod_refs.padding_b;
    } 

    child = comp->data->container.children->begin;
    while (child) { 
        init_flow_refs(flow_dir, child->comp, &child_main_refs, &child_sec_refs);
        if (child->comp->type == COMPONENT_TYPE_CONTAINER) {
            measure_component(child->comp);
            children_sizes.containers += *child_main_refs.bbox_value;
        } else {
            calc_children_main_dir_sizes(
                flow_dir,
                child->comp, 
                &child_main_refs, 
                &children_sizes
            );
        }
        calc_children_secondary_dir_sizes(
            flow_dir, 
            child->comp, 
            &child_sec_refs, 
            children_sizes.grow_children_len
        );
        if (
            need_to_calc_sec_size 
            && *child_sec_refs.bbox_value > *sec_refs.bbox_value 
        ) {
            *sec_refs.bbox_value = *child_sec_refs.bbox_value;
        } 
        child = child->next;
    }

    if (comp->parent) {
        ctui_log(LOG_WARNING, "comp parent name %s", comp->parent->id.string_value);
        comp->computed->bounding_box.width = x_spacing;
        comp->computed->bounding_box.height = y_spacing;
    } else {
        *main_refs.bbox_value +=
             *mod_refs.border_a
            + *mod_refs.border_b
            + *mod_refs.padding_a
            + *mod_refs.padding_b;
    }

    *main_refs.bbox_value += 
        children_sizes.fixed
        + children_sizes.containers
        + children_sizes.percentages
        + children_sizes.growing; 
    
}


void calculate_comp_layout(CTUI_Component *comp, int x_offset, int y_offset)
{
    if (!comp) {
        return;
    }
    CTUI_Component_Container_Data_Child *child;

    child = comp->data->container.children->begin;

    if (comp->parent) {
        x_offset += 
            + comp->parent->computed->bounding_box.x
            + comp->parent->data->border.width.left
            + comp->parent->data->padding.left;
        y_offset += 
            + comp->parent->computed->bounding_box.y
            + comp->parent->data->border.width.top
            + comp->parent->data->padding.top;
        comp->computed->bounding_box.x = x_offset;
        comp->computed->bounding_box.y = y_offset;
        x_offset += 
            + comp->data->border.width.left
            + comp->data->padding.left;
        y_offset += 
            + comp->data->border.width.top
            + comp->data->padding.top;
    } else {
        comp->computed->bounding_box.x = 0;
        comp->computed->bounding_box.y = 0;
    }


    while (child) { 
        if (child->comp->type == COMPONENT_TYPE_CONTAINER) {
            calculate_comp_layout(child->comp, x_offset, y_offset);
        } else { 
            child->comp->computed->bounding_box.x = x_offset;
            child->comp->computed->bounding_box.y = y_offset;
            x_offset += child->comp->computed->bounding_box.width;
        }
        child = child->next;
    }
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

    for (int i = 0; i < ctx->components_hm->capacity; i++) {
        if (ctx->components_hm->items[i]) {
            log_component(ctx->components_hm->items[i]->value);
        }
    }
}

void draw_comp(CTUI_Component *comp) {
    CTUI_Context *ctx = get_current_ctx();
    CTUI_BoundingBox *comp_bbox = &comp->computed->bounding_box;
    mvwaddch(
        ctx->screen_config->win, 
        comp_bbox->y, comp_bbox->x, 
        ACS_ULCORNER
    );
    mvwaddch(
        ctx->screen_config->win, 
        comp_bbox->y + comp_bbox->height - 1, 
        comp_bbox->x, ACS_LLCORNER
    );
    mvwaddch(
        ctx->screen_config->win, 
        comp_bbox->y, 
        comp_bbox->x + comp_bbox->width - 1, 
        ACS_URCORNER
    );
    mvwaddch(
        ctx->screen_config->win, 
        comp_bbox->y + comp_bbox->height - 1, 
        comp_bbox->x + comp_bbox->width - 1, 
        ACS_LRCORNER
    );
    mvwhline(ctx->screen_config->win, comp_bbox->y, comp_bbox->x + 1,  0, comp_bbox->width - 2);
    mvwhline(ctx->screen_config->win, comp_bbox->y + comp_bbox->height - 1, comp_bbox->x + 1, 0, comp_bbox->width - 2);

    mvwvline(ctx->screen_config->win, comp_bbox->y + 1, comp_bbox->x,  0, comp_bbox->height - 2);
    mvwvline(ctx->screen_config->win, comp_bbox->y + 1, comp_bbox->x + comp_bbox->width - 1, 0, comp_bbox->height - 2);

}

void draw_layout(CTUI_Component *comp)
{
    if (!comp) {
        return;
    }
    ctui_log(LOG_INFO, "render comp %s", comp->id.string_value);
    CTUI_Component_Container_Data_Child *child;

    child = comp->data->container.children->begin;

    draw_comp(comp);

    while (child) {
        ctui_log(LOG_INFO, "render child comp %s", child->comp->id.string_value);
        if (child->comp->type == COMPONENT_TYPE_CONTAINER) {
            draw_layout(child->comp);
        } else {
            draw_comp(child->comp);
        }
        child = child->next;
    }
}

void render_layout() 
{
    CTUI_Context *ctx = get_current_ctx(); 
    if (!ctx->root) {
        ctui_log(LOG_ERROR, "Root component is not defined!");
    }
    measure_component(ctx->root);
    calculate_comp_layout(ctx->root, 0, 0);
    draw_layout(ctx->root);
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
    print_log_ctui_ctx();

    while ((key = getch()) != key_escape) {
        switch (key) {
        case KEY_ENTER:
            break;
        }
    }

    endwin();

    return 0;
} 
