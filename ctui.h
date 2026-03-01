#ifndef CTUI_H_SENTRY
#define CTUI_H_SENTRY

#include <ncurses.h>
#include "arena.h"
#include "hash.h"
#include "stdio.h"

#define CTUI__WRAPPER_TYPE(type) CTUI__##type##Wrapper
#define CTUI__WRAPPER_STRUCT(type) typedef struct { type wrapped; } CTUI__WRAPPER_TYPE(type)
#define CTUI__CONFIG_WRAPPER(type, ...) (CTUI__WRAPPER_TYPE(type)) { .wrapped = __VA_ARGS__ }.wrapped

typedef enum LOG_LEVEL {
    LOG_INFO,
    LOG_DEBUG,
    LOG_WARNING,
    LOG_ERROR
} LOG_LEVEL;

typedef struct CTUI_Component_Id {
    uint32_t value;
    uint32_t base_value;
    const char *string_value;
} CTUI_Component_Id;

typedef enum CTUI_SizingType {
    SIZE_FIXED,
    SIZE_PERCENT,
    SIZE_GROW,
} CTUI_SizingType;

typedef struct CTUI_Component_Sizing_Minmax {
    float min, max;
} CTUI_Component_Sizing_Minmax;

typedef struct CTUI_Component_Sizing {
    union {
        float fixvalue;
        float percent;
    };
    CTUI_Component_Sizing_Minmax minmax;
    CTUI_SizingType type; 
} CTUI_Component_Sizing;

typedef struct CTUI_Component_Sizing_Axis {
    CTUI_Component_Sizing width;
    CTUI_Component_Sizing height;
} CTUI_Component_Sizing_Axis;

typedef enum CTUI_ComponentType {
    COMPONENT_TYPE_CONTAINER,
    COMPONENT_TYPE_TEXT,
    COMPONENT_TYPE_LABEL,
    COMPONENT_TYPE_BUTTON
} CTUI_ComponentType;

typedef enum CTUI_Component_X_Alignment {
    X_LEFT,
    X_CENTER,
    X_RIGHT
} CTUI_Component_X_Alignment;

typedef enum CTUI_Component_Y_Alignment {
    Y_TOP,
    Y_CENTER,
    Y_BOTTOM 
} CTUI_Component_Y_Alignment;

typedef struct CTUI_Component_Padding {
    uint16_t top, bottom, left, right;
} CTUI_Component_Padding;

typedef struct CTUI_Component_BorderWidth {
    uint16_t top, bottom, left, right;
} CTUI_Component_BorderWidth;

typedef struct CTUI_Component_Border {
    CTUI_Component_BorderWidth width;
} CTUI_Component_Border;

typedef struct CTUI_Component CTUI_Component;

typedef struct CTUI_Component_Container_Data_Child CTUI_Component_Container_Data_Child;
struct CTUI_Component_Container_Data_Child {
    CTUI_Component_Container_Data_Child *next;
    CTUI_Component *comp;
};

typedef struct CTUI_Component_Container_Data_Children {
    CTUI_Component_Container_Data_Child *begin, *end;
} CTUI_Component_Container_Data_Children;

typedef enum CTUI_Component_Flow_Dir {
    FLOW_DIR_ROW,
    FLOW_DIR_COLUMN
} CTUI_Component_Flow_Dir;

typedef struct CTUI_Component_Container_Data {
    CTUI_Component_X_Alignment x_alignment;
    CTUI_Component_Y_Alignment y_alignment;
    CTUI_Component_Flow_Dir flow_dir;
    CTUI_Component_Container_Data_Children *children;
} CTUI_Component_Container_Data;

typedef struct CTUI_Component_Text_Data {
    char *value;
} CTUI_Component_Text_Data;

typedef struct CTUI_Component_Data {
    CTUI_Component_Sizing_Axis sizes;
    CTUI_Component_Padding padding;
    CTUI_Component_Border border;
    union {
        CTUI_Component_Container_Data container;
        CTUI_Component_Text_Data text;
    };
} CTUI_Component_Data;
CTUI__WRAPPER_STRUCT(CTUI_Component_Data);

typedef struct CTUI_BoundingBox {
    uint16_t x, y, width, height;
} CTUI_BoundingBox;

typedef struct CTUI_Component_Computed_Data {
    CTUI_BoundingBox bounding_box;
} CTUI_Component_Computed_Data;

struct CTUI_Component {
    CTUI_Component_Id id;
    CTUI_Component_Data *data;
    CTUI_Component_Computed_Data *computed;
    CTUI_ComponentType type;
    CTUI_Component *parent;
};

DECLARE_HASHMAP(CTUI_Components_HashMap, CTUI_Component *);

typedef struct CTUI_Open_Components_Stack_Node CTUI_Open_Components_Stack_Node;
struct CTUI_Open_Components_Stack_Node {
    CTUI_Open_Components_Stack_Node *next;
    CTUI_Component_Id comp_id;
};

typedef struct CTUI_Open_Components_Stack {
    CTUI_Open_Components_Stack_Node *top;
    unsigned int length;
} CTUI_Open_Components_Stack;

typedef struct CTUI_Children_Main_Dir_Size_Config {
    int containers;
    int percentages;
    int growing;
    int fixed;

    CTUI_Component_Container_Data_Children grow_children;
    int grow_children_len;
} CTUI_Children_Main_Dir_Size_Config;

typedef struct CTUI_Flow_Refs {
    uint16_t *bbox_value;
    CTUI_Component_Sizing *sizing; 
} CTUI_Flow_Refs;

typedef struct CTUI_Modifier_Flow_Refs {
    uint16_t *padding_a;
    uint16_t *padding_b;
    uint16_t *border_a;
    uint16_t *border_b;
} CTUI_Modifier_Flow_Refs;

typedef struct CTUI_LogConfig {
    const char *log_file_path;
    FILE *f_stream;
} CTUI_LogConfig;

typedef struct CTUI_Screen_Config {
    uint16_t width;
    uint16_t height;
    WINDOW *win; 
} CTUI_Screen_Config;

typedef struct CTUI_Context {
    Arena *arena;

    CTUI_LogConfig *log_config;
    CTUI_Screen_Config *screen_config;

    CTUI_Component *root;
    CTUI_Components_HashMap *components_hm;

    CTUI_Component *current_element;
    CTUI_Open_Components_Stack *open_components_stack;
} CTUI_Context;

#define CTUI_CONFIGURE_COMPONENT(type, ...)                                                                                                                                   \
    do {\
        configure_open_component(type, CTUI__CONFIG_WRAPPER(CTUI_Component_Data, __VA_ARGS__)); \
    } while (0)

#define CTUI(id, type, ...)                                                                                                                                               \
    for (                                                                                                                                                           \
        int i = (open_component_with_id(make_component_id(id)), configure_open_component(type, CTUI__CONFIG_WRAPPER(CTUI_Component_Data, __VA_ARGS__)), 0);  \
        i < 1;                                                                                                                         \
        i = 1, close_component()                                                                                                      \
    )

#endif
