#include "fec.h"
#include <iostream>
#include <stdlib.h>
#include <variant>

using fec_value = std::variant<fec_objectptr, fec_cfunc, fec_number, char>;

struct fec_object {
    fec_value car, cdr;
};

struct fec_context {

};


fec_contextptr fec_interpreter::fec_open(void *, int) { return nullptr; }
fec_objectptr fec_interpreter::fec_readf(fec_contextptr , std::ifstream& ) {return nullptr;}
fec_objectptr fec_interpreter::fec_eval(fec_contextptr , fec_objectptr) { return nullptr;}


static char buf[64000];

int main(int argc, char **argv) {
    fec_interpreter feci;
    fec_objectptr obj;
    fec_contextptr ctx = feci.fec_open(buf, sizeof(buf)); 
    std::ifstream source;

    if (argc > 1) {
        source.open(argv[1]);
        if (!source) {
            std::cerr << "cant open error\n";
            return EXIT_FAILURE;
        }
    }
    while (true) {
        if (!(obj = feci.fec_readf(std::move(ctx), source))) {
            break;
        }
        obj = feci.fec_eval(std::move(ctx), std::move(obj));
    }
    return EXIT_SUCCESS;
}