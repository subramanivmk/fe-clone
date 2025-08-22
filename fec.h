#pragma once

#include <functional>
#include <memory>
#include <fstream>
using fec_number = float;
struct fec_object;
struct fec_context;
using fec_objectptr = std::unique_ptr<fec_object>;
using fec_contextptr = std::unique_ptr<fec_context>;

using fec_cfunc = std::function<fec_objectptr(fec_objectptr)>;

enum class fec_token {
    FEC_TPAIR,
    FEC_TFREE,
    FEC_TNIL,
    FEC_TNUMBER,
    FEC_TSYMBOL,
    FEC_TSTRING,
    FEC_TFUNC,
    FEC_TMACRO,
    FEC_TPRIM,
    FEC_TCFUNC,
    FEC_TPTR
};


class fec_interpreter {
public:
    fec_contextptr fec_open(void *ptr, int size);
    fec_objectptr fec_readf(fec_contextptr ctx, std::ifstream &source);
    fec_objectptr fec_eval(fec_contextptr ctx, fec_objectptr obj);
};