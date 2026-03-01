
    /* if (argc < 2) {
        fprintf(stderr, "File is not provided");
        return 1;
    }

    char *fpath = argv[1]; 
    Arena arena = {0};

    CTUI_String_Builder buf_sb = (CTUI_String_Builder) {
        .length = 0,
        .capacity = 0,
        .items = NULL
    }; 

    if (!ctui_read_file_to_buf(&arena, fpath, &buf_sb)) return 1;

    CTUI_String_View buf_sv = (CTUI_String_View) {
        .data = buf_sb.items,
        .length = buf_sb.length
    }; 

    CTUI_Int_HashMap *hm;
    arena_init_hm(&arena, hm, 100000);

    while (buf_sv.length > 0) {
        buf_sv = ctui_sv_trim_left(buf_sv);
        CTUI_String_View token = ctui_sv_chop_by_space(&buf_sv);
        char *token_value = arena_strdup_n(&arena, token.data, token.length);
        CTUI_Int_HashMapItem *cur = CTUI_Int_HashMap_get(hm, token_value);
        if (!cur)
            CTUI_Int_HashMap_set(&arena, hm, token_value, 1);
        else
            cur->value++;
    }

    int total = 0;
    CTUI_Int_HashMapItem *tmp;
    for (size_t i = 0; i < hm->capacity; i++) {
        tmp =hm->items[i]; 
        if (tmp) {
            printf("[HM RECORD START] ----- %d -----\n", (int)i);
            while (tmp) {
                printf("[INFO]: hm item %s -> %d\n", tmp->key, tmp->value);
                total += tmp->value; 
                tmp = tmp->next;
            }
            printf("[HM RECORD end] ----- %d -----\n", (int)i);
        }
    }

    printf("[INFO]: hm table length %d\n", (int)hm->length);
    printf("[INFO]: total %d\n", total);
 */
