#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#define TEMP_BUFF_SIZE 1024
char* temp_sprintf(const char* format, ...);

const char* read_file(const char* filename);

// string module
typedef struct str {
    char* data;
    size_t count;
    size_t capacity;
} str;


#define STR_GROWTH_FACTOR 2
#define QUEUE_INITIAL_CAPACITY 1

str  str_init();
bool str_realloc(str* s, size_t len);
bool str_append(str* s, const char* str);
void str_log(str s);

typedef struct str_view {
    const char* data;
    size_t count;
} str_view;

typedef struct str_stream {
    str_view str;
    int current;
    int line;
    int col;
} str_stream;

static str_stream _strm_state;

bool stream_end(str_stream* stream);
char stream_next(str_stream* stream);
char stream_ahead(str_stream* stream);
char stream_current(str_stream* stream);
void stream_put_back(str_stream* stream);
void stream_skip_current(str_stream* stream);
void stream_skip_spaces(str_stream* stream);
void stream_save(str_stream* stream);
void stream_restore(str_stream* stream);
bool stream_expect(str_stream* stream, const char* str, bool restore);

str_stream str_to_stream(str_view str);

typedef struct NodeAttribute NodeAttribute;
typedef struct HTMLNode HTMLNode;

struct NodeAttribute {
    size_t         count;
    bool           is_root;
    NodeAttribute* root;
    const char*    key;
    const char*    value;
    NodeAttribute* next;
    NodeAttribute* last;
};

NodeAttribute* attrib_create(const char* key, const char* value);
NodeAttribute* attrib_add_existing(NodeAttribute* root, NodeAttribute* existing);
NodeAttribute* attrib_add(NodeAttribute* root, const char* key, const char* value);
void           attrib_log(NodeAttribute* attrib);

// node array module

#define QUEUE_GROWTH_FACTOR    2
#define QUEUE_INITIAL_CAPACITY 1

// dynamic array of nodes
typedef struct node_array {
    HTMLNode** data;
    size_t     count;
    size_t     capacity;
} node_array;

node_array node_array_init();
void       node_array_remove_last(node_array* q);
bool       node_array_add(node_array* q, HTMLNode* node);
bool       node_array_realloc(node_array* q);
HTMLNode*  node_array_last(node_array q);

// maps child index to inner text.
typedef struct InnerTextIndex { 
    size_t index;
    str    innerText;
} InnerTextIndex;

typedef struct HTMLNode {
    const char*    name;
    str_view       text;
    str_view       innerHTML;
    str            innerText;
    NodeAttribute* attrib;
    node_array     children;
    bool           is_text;
} HTMLNode;

typedef struct HTMLDocument {
    HTMLNode* root;
    str       doctype;
} HTMLDocument;

HTMLNode* node_create(const char* name);

// parser module

#define STR_BUFF_SIZE    1024
#define STR_BUFF_MAXLEN  STR_BUFF_LEN - 1

typedef enum parser_state_enum {
    PARSER_START,
    PARSER_FINISH,
    PARSER_READ_STRING,
    PARSER_TAG,
    PARSER_COMMENT,
    PARSER_CLOSING_TAG,
    PARSER_DOCTYPE
} parser_state_enum;

// TODO: Parse innerHTML substrings for each node. 
typedef struct parser_state {
    const char*       filename;
    HTMLNode*         root;
    str_stream        stream;
    char              c;
    char              current;
    int               str_pos;
    char              str_buffer[STR_BUFF_SIZE];
    parser_state_enum state;
    node_array        node_queue;
    HTMLNode*         curr_node;
    bool              parse_attribs;
    str               doctype;
} parser_state;

void parser_get_string(parser_state* parser, char* last_ch, char quote);
void parser_attrib(parser_state* parser);
void parser_log_line(parser_state* p);

// customizable
#define ERROR_BUFF_SIZE            512
#define ERROR_WINDOW_SIZE          16

// private
#define _ERROR_DEFAULT_WINDOW_SIZE 8

// Node Handling Interface
HTMLDocument html_parse(const char* filename);
void      inner_text_log(HTMLNode* node);
// TODO: Add the rest of the functions to handle node search, creation, and so forth.


#define HTML_PARSER_IMPL
#ifdef HTML_PARSER_IMPL

void attrib_log(NodeAttribute* attrib) {
    for (NodeAttribute* attr = attrib; attr != NULL; attr = attr->next) {
        printf("%s=\"%s\"\n", attr->key, attr->value);
    }
}

NodeAttribute* attrib_add_existing(NodeAttribute* root, NodeAttribute* existing) {
    if (!root->is_root) {
        root->is_root   = true;
        root->root      = NULL;
        root->count     = 1;
        root->next      = existing;
    } else {
        root->last->next = existing;
    }

    existing->root = root;
    root->last   = existing;
    root->count++;
    return existing;
}

NodeAttribute* attrib_add(NodeAttribute* root, const char* key, const char* value) {
    return attrib_add_existing(root, attrib_create(key, value));
}

NodeAttribute* attrib_create(const char* key, const char* value) {
    NodeAttribute* attrib = malloc(sizeof(NodeAttribute));
    *attrib = (NodeAttribute) {
        .count      = 0,
        .is_root    = false,
        .key        = key,
        .value      = value,
        .root       = NULL,
        .last       = NULL
    };

    return attrib;
}

HTMLNode* node_create(const char* name) {
    HTMLNode* node = malloc(sizeof(HTMLNode));
    *node = (HTMLNode) {
        .attrib  = NULL,
        .name    = name,
        .is_text = false,
        .innerHTML = (str_view) {
            .data   = NULL,
            .count  = 0
        },
        .text    = (str_view) {
            .data   = NULL,
            .count  = 0
        },
        .children = node_array_init()
    };    
    return node;
}

str_view to_str_view(const char* src) {
    return (str_view) {
        .count = strlen(src),
        .data = src
    };
}

const char* read_file(const char* filename) {
    FILE* fh = fopen(filename, "r");
    if (!fh) {
        printf("Couldn't open file %s\n", filename);
        return NULL;
    }

    size_t len = 0;
    fseek(fh, 0, SEEK_END);
    len = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    char* data = calloc(len + 1, sizeof(char));
    fread(data, sizeof(char), len, fh);
    fclose(fh);

    data[len] = 0;
    return data;
}

void html_parser_error(parser_state* ps, const char* err)
{
    static char parser_err_buff[ERROR_BUFF_SIZE];

    int n            = 0;
    int error_offset = 0;
    int ch_count     = 0;
    int err_size     = ERROR_WINDOW_SIZE - (ERROR_WINDOW_SIZE % 2);
    if (err_size < 0) {
        err_size = _ERROR_DEFAULT_WINDOW_SIZE;
    } else if(err_size >= ERROR_BUFF_SIZE - 1) {
        err_size = ERROR_BUFF_SIZE - 2;
    }

    str_stream* sp = &ps->stream;

    printf("%s, near: %s:%d:%d\n", err, ps->filename, sp->line + 1, sp->col + 1);

    for (int i = 0; i < err_size; ++i)
    {
        int cursor = (sp->current - (err_size / 2)) + i;
        if (cursor >= sp->str.count)
        {
            parser_err_buff[n] = ' ';
            n++;
            ch_count = 1;
        }
        else
        {
            if (sp->str.data[cursor] == '\n')
            {
                parser_err_buff[n] = '\\';
                parser_err_buff[n + 1] = 'n';
                ch_count = 2;
            }
            else
            {
                parser_err_buff[n] = sp->str.data[cursor];
                ch_count = 1;
            }
        }

        n += ch_count;

        if (cursor == sp->current)
        {
            error_offset = n - ch_count;
        }
    }
    parser_err_buff[n] = '\0';
    printf("%s\n", parser_err_buff);

    for (int i = 0; i < n; ++i)
    {
        if (i != error_offset) 
            printf(" ");
        else 
            printf("^");
    }

    printf("\n");
}

void parser_log_line(parser_state* p) {
    printf("%s:%d:%d\n", p->filename, p->stream.line + 1, p->stream.col + 1);
}

void parser_get_string(parser_state* parser, char* last_ch, char quote) {
    str_stream* s   = &parser->stream;
    char ch         = stream_next(s);
    parser->str_pos = 0;

    while (isalnum(ch) || (!isalnum(ch) && quote != -1) || (isspace(ch) && (quote != -1))) {
        if (ch == quote) break;

        parser->str_buffer[parser->str_pos++] = ch;
        ch = stream_next(s);
    }

    if (last_ch) {
        *last_ch = ch;
    }
 
    parser->str_buffer[parser->str_pos] = '\0';
}

void parser_attrib(parser_state* parser)
{
    str_stream* s = &parser->stream;

    while (true)
    {
        // skip spaces
        if (stream_end(s)) 
            break;

        stream_skip_spaces(s);

        if (!isalpha(stream_current(s)) && stream_current(s) != '>') {
            html_parser_error(parser, temp_sprintf("syntax error, bad char: %c", stream_current(s)));
            stream_skip_current(s);
            continue;
        }

        char last_ch;
        parser_get_string(parser, &last_ch, -1);

        if (strlen(parser->str_buffer) <= 0)
            break;

        bool equal_found = false;

        if (isspace(last_ch)) {
            char _ch = stream_next(s); 
            while(!isalpha(_ch) && _ch != '=' && _ch != '>' && _ch != -1) {
                if (_ch != '=' && _ch != -1 && _ch != '>' && !isspace(_ch)) {
                    stream_put_back(s);
                    html_parser_error(parser, temp_sprintf("syntax error, bad char: %c", _ch));
                    stream_skip_current(s);
                }

                _ch = stream_next(s);
            }

            if (_ch == '=')
                equal_found = true;
            else {
                if (_ch != -1)
                    stream_put_back(s);
            }
        } else if(last_ch == '=') {
            equal_found = true;
        } else {
            if (last_ch != '>') {
                stream_put_back(s);
                html_parser_error(parser, temp_sprintf("syntax error, bad char: %c", last_ch));
                stream_skip_current(s);
            }
        }

        if (last_ch == '>' || !equal_found || stream_end(s)) {
            NodeAttribute* attr = attrib_create(strdup(parser->str_buffer), "");
            if (!parser->curr_node->attrib) {
                parser->curr_node->attrib = attr;
            } else {
                attrib_add_existing(parser->curr_node->attrib, attr);
            }

            if (last_ch == '>') break;

        } else if (equal_found) {
            stream_skip_spaces(s);

            NodeAttribute* attr = attrib_create(strdup(parser->str_buffer), "");

            last_ch           = stream_next(s);
            char quote        = -1;
            if (last_ch == '"' || last_ch == '\'') {
                quote         = last_ch;
            } else {
                stream_put_back(s);
            }

            parser_get_string(parser, &last_ch, quote);

            if (strlen(parser->str_buffer) > 0)
                attr->value = strdup(parser->str_buffer);

            if (!parser->curr_node->attrib) {
                parser->curr_node->attrib = attr;
            } else {
                attrib_add_existing(parser->curr_node->attrib, attr);
            }

            if (last_ch == '>') 
                break;
        } else {
            stream_put_back(s);
            html_parser_error(parser, "Bad char in parsing of the attributes.");
            stream_skip_current(s);
            break;
        }
    }
}

// TODOs:
// Handle self closing tags

HTMLDocument html_parse(const char* filename) {
    const char* src   = read_file(filename);
    
    if (!src)
        return (HTMLDocument) {.doctype = {0}, .root = NULL};

    parser_state parser;
    parser.filename   = filename;

    str_view src_view = to_str_view(src);

    HTMLNode* root       = node_create(NULL);
    root->innerHTML      = src_view;
    
    parser.root          = root;
    parser.stream        = str_to_stream(src_view);    
    parser.node_queue    = node_array_init();
    parser.curr_node     = root;
    parser.parse_attribs = false;
    node_array_add(&parser.node_queue, root);

    parser.doctype       = str_init();

    str_stream* sp       = &parser.stream;
    parser.state         = PARSER_START;
    char* str_buff       = parser.str_buffer;


    while (parser.state != PARSER_FINISH) {
        switch (parser.state) {
            case PARSER_TAG: {
                // printf("Parse tag!");
                // parser_log_line(&parser);

                char c = stream_next(sp);
                if (!parser.parse_attribs) {

                    if (isalpha(c) || isdigit(c)) {
                        str_buff[parser.str_pos++] = c;
                    } else {

                        // if (c == '<') {
                        //     html_parser_error(&parser, "Bad syntax, invalid tag name start char '<'");
                        //     parser.state = PARSER_START;
                        //     break;
                        // }

                        if (c != ' ' && c != '>' && c != -1) {
                            // stream_put_back(sp);
                            html_parser_error(&parser, "Bad syntax, expected space, '>' char, or end of file");
                        }

                        str_buff[parser.str_pos] = '\0';
                        // create a new html node here.
                        HTMLNode* n = node_create(strdup(str_buff));

                        node_array_add(&parser.curr_node->children, n);

                        parser.curr_node = n;
                        node_array_add(&parser.node_queue, n);

                        if (c == -1 || c == '>') {
                            // node has no attributes.
                            parser.state = PARSER_START;
                        } else {
                            // parse attributes.
                            stream_put_back(sp);
                            parser.parse_attribs = true;
                        }
                    }
                } else {
                    stream_put_back(sp);
                    parser_attrib(&parser);
                    parser.state         = PARSER_START;
                    parser.parse_attribs = false;
                }

                break;
            }
            case PARSER_COMMENT: {
                int curr = sp->current;
                if (stream_expect(sp, "-->", false)) {
                    parser.state = PARSER_START;
                } else {
                    if (stream_end(sp)) {
                        parser.state = PARSER_FINISH;
                        break;
                    }
                    
                    int end = sp->current;
                    int len = end - curr + 1;
                    for (int i = 0; i < len; ++i) {
                        stream_skip_current(sp);
                    }
                }
                break;
            }
            case PARSER_START: {
                parser.c = stream_next(sp);
                if (parser.c == -1) {
                    parser.state = PARSER_FINISH;
                    break;
                }

                if (parser.c == ' ' || parser.c == '\n')
                    break;
                    
                if (parser.c == '<') {
                    parser.current = stream_current(sp);
                    if (isalpha(parser.current)) {
                        parser.state   = PARSER_TAG;
                        parser.str_pos = 0;
                    } else if (parser.current == '!') {
                        stream_skip_current(sp);
                        if (isalpha(stream_current(sp))) {
                            stream_save(sp);
                            char lc;
                            parser_get_string(&parser, &lc, -1);
                            if (strcmp(parser.str_buffer, "DOCTYPE") == 0 && lc == ' ') {
                                parser.state = PARSER_DOCTYPE;
                                stream_put_back(sp);
                            } else {
                                stream_restore(sp);
                                parser.state = PARSER_START;
                            }
                        } else {
                            if (!stream_expect(sp, "--", true)) {
                                parser.state = PARSER_START;
                            } else {
                                parser.state = PARSER_COMMENT;
                            }
                        }
                    } else if (parser.current == '/') {
                        if (!isalpha(stream_ahead(sp))) {
                            stream_skip_current(sp);
                            stream_skip_current(sp);
                            html_parser_error(&parser, "Bad syntax. Expected tag name after '/'");
                        } else {
                            parser.state = PARSER_CLOSING_TAG;
                            stream_skip_current(sp);
                        }
                    }
                    break;
                } else /* (isalnum(parser.c)) */ {
                    parser.state   = PARSER_READ_STRING;
                    parser.str_pos = 0;
                    stream_put_back(sp);
                }
                break;
            }
            case PARSER_CLOSING_TAG: {
                char last_ch;
                int current = sp->current;
                parser_get_string(&parser, &last_ch, -1);
                
                if (parser.curr_node && strcmp(parser.str_buffer, parser.curr_node->name) == 0) {
                    if (parser.node_queue.count > 1) {
                        node_array_remove_last(&parser.node_queue);
                        parser.curr_node = parser.node_queue.data[parser.node_queue.count - 1];
                    }
                } else {
                    if (parser.node_queue.count > 1) {
                        int curr = sp->current;
                        sp->current = current;
                        html_parser_error(&parser, "invalid closing tag");
                        sp->current = curr;
                    }
                }

                if (last_ch != '>') {
                    while(stream_next(sp) != '>' && !stream_end(sp));
                }
                
                parser.state   = PARSER_START;
                break;
            }
            case PARSER_READ_STRING: {
                parser.c = stream_next(sp);
                if (parser.c == ' ' || parser.c == '\n' || parser.c == '<' /* || (!isalpha(parser.c) && !isdigit(parser.c)) */) {
                    stream_put_back(sp);
                    parser.state = PARSER_START;
                    parser.str_buffer[parser.str_pos] = '\0';

                    HTMLNode* text  = node_create(NULL);
                    text->is_text   = true;
                    text->innerHTML = to_str_view(strdup(parser.str_buffer));

                    node_array_add(&(parser.curr_node->children), text);

                    // TODO: Store inner text positions withing current tag children
                    // For better rendering.
                    // A inner text will always come before a child node:
                    // INNER TEXT HERE
                    // <TAG>...</TAG>
                    // ANOTHER INNER TEXXT HERE
                    // <TAG2>...</TAG>
                    // store index and associated inner text.
                    // struct InnerTextIndex { size_t index, char* innerText; };

                    // propagate this text to all nodes in the queue
                    for (int i = 0; i < parser.node_queue.count; ++i) {
                        str_append(&parser.node_queue.data[i]->innerText, parser.str_buffer);
                        str_append(&parser.node_queue.data[i]->innerText, " ");
                    }

                    break;
                }

                parser.str_buffer[parser.str_pos++] = parser.c;

                break;
            };
            case PARSER_DOCTYPE: {

                // make sure you only read the first doctype found in entire file
                bool was_null = parser.doctype.data == NULL;

                str* doctype = &parser.doctype;
                char lc;
                char _tmp[2] = {'\0', '\0'};

                do {
                    stream_skip_spaces(sp);
                    parser_get_string(&parser, &lc, -1);

                    _tmp[0] = lc;
                    
                    if (stream_end(sp)) 
                        break;

                    if (was_null) {
                        str_append(doctype, parser.str_buffer);
                    }

                    if (lc != '>') {
                        if (was_null) {
                            str_append(doctype, _tmp);
                        }
                        stream_put_back(sp);
                    }
                } while(lc != '>');

                parser.state = PARSER_START;
                
                break;
            }
        }
    }

    return (HTMLDocument) {
        .root    = root,
        .doctype = parser.doctype
    };
}

bool stream_end(str_stream* stream) {
    return stream->current >= stream->str.count;
}

char stream_next(str_stream* stream) {
    if (stream_end(stream)) 
        return -1;

    
    char c = stream->str.data[stream->current++];

    if (c == '\n') {
        stream->line++;
        stream->col = 0;
    } else {
        stream->col++;
    }

    return c;
}

char stream_ahead(str_stream* stream) {
    if (stream_end(stream) || stream->current + 1 >= stream->str.count) 
        return -1;
    
    return stream->str.data[stream->current + 1];
}

char stream_current(str_stream* stream) {
    if (stream_end(stream))
        return -1;
    
    return stream->str.data[stream->current];
}

void stream_skip_current(str_stream* stream) {
    if (stream_end(stream))
        return;
        
    char c = stream->str.data[stream->current++];

    if (c == '\n') {
        stream->line++;
        stream->col = 0;
    } else {
        stream->col++;
    }
}

void stream_skip_spaces(str_stream* stream) {
    if (!stream) 
        return;

    while (isspace(stream_next(stream)));
    stream_put_back(stream);
}

void stream_put_back(str_stream* stream) {
    stream->current--;
    if (stream->current < 0)
        stream->current = 0;

    if (stream->col > 0) {
        stream->col--;
    } else {
        if (stream->line > 0)
            stream->line--;
    }
}

str_stream str_to_stream(str_view str) {
    return (str_stream) {
        .current = 0,
        .str = str
    };
}

void stream_save(str_stream* stream) {
    _strm_state = *stream;
}

void stream_restore(str_stream* stream) {
    *stream = _strm_state;
}

bool stream_expect(str_stream* stream, const char* str, bool restore) {
    int len = strlen(str);

    stream_save(stream);

    for (int i = 0; i < len; ++i) {
        if (stream_current(stream) != str[i]) {
            if (restore)
                stream_restore(stream);
            return false;
        }

        stream_skip_current(stream);
    }

    return true;
}

// node queue
node_array node_array_init() {
    return (node_array) {
        .capacity   = 0,
        .count      = 0,
        .data       = NULL
    };
}

bool node_array_realloc(node_array* q) {
    if (!q->data) {
        q->data     = malloc(sizeof(HTMLNode*) * QUEUE_GROWTH_FACTOR);
        q->capacity = QUEUE_INITIAL_CAPACITY;
    }

    if (q->count >= q->capacity) {
        HTMLNode** tmp = q->data;
        size_t cap = q->capacity * QUEUE_GROWTH_FACTOR;
        q->data = realloc(q->data, cap * sizeof(q->data[0]));

        if (!q->data) {
            q->data = tmp;
            return false;
        }

        q->capacity = cap;
    }

    return true;
}

void node_array_remove_last(node_array* q) {
    if (q->count > 0)
        q->count--;
}

bool node_array_add(node_array* q, HTMLNode* node) {
    if (!node_array_realloc(q))
        return false;

    q->data[q->count++] = node;
    return true;
}

HTMLNode* node_array_last(node_array q) {
    return q.data[q.count - 1];
}

str str_init() {
    return (str) {
        .capacity = 0,
        .count    = 0,
        .data     = NULL
    };
}

// string module
bool str_realloc(str* s, size_t len) {
    if (!s->data) {
        s->data     = malloc(sizeof(s->data[0]) * STR_GROWTH_FACTOR);
        s->capacity = QUEUE_INITIAL_CAPACITY;
    }

    if (s->count + len >= s->capacity) {
        char* tmp = s->data;
        size_t cap = s->capacity * STR_GROWTH_FACTOR;
        while (s->count + len >= cap) {
            cap = cap * STR_GROWTH_FACTOR;
        }

        s->data = realloc(s->data, cap);

        if (!s->data) {
            s->data = tmp;
            return false;
        }

        s->capacity = cap;
    }

    return true;
}

bool str_append(str* s, const char* str) {
    size_t src_len = strlen(str);

    if (!str_realloc(s, src_len))
        return false;

    memcpy(s->data + s->count, str, src_len);
    s->count += src_len;
    
    return true;
}

void str_log(str s) {
    printf("%.*s\n", s.count, s.data);
}

char* temp_sprintf(const char* format, ...) {
    static char tmp_buff[TEMP_BUFF_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(tmp_buff, sizeof(tmp_buff), format, args);
    va_end(args);

    return tmp_buff;
}

void inner_text_log(HTMLNode* node) {
    if (node->is_text) return;
    
    printf("%s => ", node->name);
    str_log(node->innerText);
    printf("\n");

    for (int i = 0; i < node->children.count; ++i) {
        inner_text_log(node->children.data[i]);
    }
}


#endif // HTML_PARSER_IMPL