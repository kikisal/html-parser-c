# HTML Parser
Simple lightweight single header HTML Subset Parser written in C.

## Example:
### print children of parsed root node:

```C
#define HTML_PARSER_IMPL
#include "html_parser.h"

int main() {
    HTMLNode* node = html_parse("./test.html");
    assert(node != NULL && "Node was null");
    printf("Children count: %d\n", node->children.count);

    return 0;
}
```

## Current Flaws

As of now it only supports ASCII encoded files. Do not feed UTF-8 Encoded or any other file with different encodings. Unexpected behaviour may occur.
