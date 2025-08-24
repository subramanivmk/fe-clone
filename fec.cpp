#include "fec.h"
#include <algorithm>
#include <iostream>
#include <stdlib.h>

FecObject FecInterpreter::rparen;

FecObject FecObject::_nil = []{
    FecObject obj{};
    obj.set_type(FecTokenType::FEC_TNIL);
    obj.set_cdr(nullptr);
    return obj;
}();

static char buf[64000];

int main(int argc, char **argv) {
    FecInterpreter feci(buf, sizeof(buf));
    FecObjectPtr obj;
    std::ifstream source;

    if (argc > 1) {
        source.open(argv[1]);
        if (!source) {
            std::cerr << "cant open error\n";
            return EXIT_FAILURE;
        }
    }
    while (true) {
        if (!(obj = feci.fec_readf(source))) {
            break;
        }
        obj = feci.fec_eval(obj);
    }
    return EXIT_SUCCESS;
}