#define HTML_PARSER_IMPL
#include "html_parser.h"

int main() {
    HTMLNode* node = html_parse("./test.html");
    assert(node != NULL && "Node was null");
    printf("children: %d\n", node->children.count);
    return 0;
}