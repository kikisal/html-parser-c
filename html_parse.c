#define HTML_PARSER_IMPL
#include "html_parser.h"

int main() {
    HTMLDocument doc = html_parse("./test.html");
    assert(doc.root != NULL && "Node was null");

    printf("DOCTYPE => ");
    str_log(doc.doctype);
    printf("Children => %d\n", doc.root->children.count);
    
    return 0;
}