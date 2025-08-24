#pragma once

#include <cstdio>
#include <ctype.h>
#include <functional>
#include <iterator>
#include <memory>
#include <fstream>
#include <stdint.h>
#include <cstdint>
#include <iostream>
#include <string.h>

using fec_number = float;
class FecObject;
using FecObjectPtr = FecObject*;
using fec_cfunc = std::function<FecObjectPtr(FecObjectPtr)>;
using fec_cfuncptr = fec_cfunc*;
using fec_readfn = std::function<char(std::ifstream&)>;

enum class FecTokenType {
    FEC_TPAIR,
    FEC_TFREE,
    FEC_TNIL,
    FEC_TNUMBER,
    FEC_TSYMBOL,
    FEC_TSTRING,
    FEC_TPRIM
};

class FecObject {
private:
    union {FecObjectPtr obj; fec_cfuncptr f; fec_number n; std::uint8_t c;} _car, _cdr;
    static FecObject _nil;
public:

    static constexpr int STRING_BUFFER_SIZE = sizeof(FecObjectPtr) - 1;

    bool isnil() {
        return this == &_nil;
    }
    FecObjectPtr get_car() {
        return _car.obj;
    }
    FecObjectPtr get_cdr() {
        return _cdr.obj;
    }
    FecObjectPtr& get_cdr_ptr() {
        return _cdr.obj;
    }
    void set_car(FecObjectPtr obj) {
        _car.obj = obj;
    }
    void set_cdr(FecObjectPtr obj) {
        _cdr.obj = obj;
    }
    void set_type(FecTokenType t) {
        _car.c = static_cast<uint8_t>(static_cast<unsigned>(t) << 2 | 1);
    }
    FecTokenType get_type() {
        return _car.c & 0x1 ? static_cast<FecTokenType>(_car.c >> 2) : FecTokenType::FEC_TPAIR;
    }
    fec_number get_number() {
        return _cdr.n;
    }
    void set_number(fec_number n) {
        _cdr.n = n;
    }
    void set_primitive(uint8_t p) {
        _cdr.c = p;
    }
    uint8_t get_primitive() {
        return _cdr.c;
    }
    static FecObjectPtr nil() {
        return &_nil;
    }
    char* string_buffer() {
        return reinterpret_cast<char*>(&_car.c + 1);
    }
};

class FecInterpreter {
private:
    struct FecContext {
        FecObjectPtr objects;
        std::size_t object_count;
        FecObjectPtr callist;
        FecObjectPtr freelist;
        FecObjectPtr symlist;
        FecObjectPtr t;
        char nextchar;
    };
    using FecContextPtr = FecContext*;
    FecContextPtr _ctx;

    enum Primitive {
        P_PRINT, P_ADD, P_SUB, P_MUL, P_DIV, P_MAX
    };

    static constexpr const char* primnames[] = {
        "print", "+", "-", "*", "/"
    };

    static FecObject rparen;

public:
    FecInterpreter() = delete;
    FecInterpreter(void* ptr, std::size_t size) {
        
        // set context struct into memory region
        void *tmp = ptr;
        _ctx = static_cast<FecContext*>(tmp);
        ptr = (char *) ptr + sizeof(FecContext);
        size -= sizeof(FecContext);

        // init objects in the memory region
        _ctx->objects = static_cast<FecObjectPtr>(ptr);
        _ctx->object_count = size / sizeof(FecObject);

        _ctx->callist = FecObject::nil();
        _ctx->freelist = FecObject::nil();
        _ctx->symlist = FecObject::nil();

        for (std::size_t i = 0; i < _ctx->object_count; i++)  {
            FecObjectPtr obj = &_ctx->objects[i];
            obj->set_type(FecTokenType::FEC_TFREE);
            obj->set_cdr(_ctx->freelist);
            _ctx->freelist = obj;
        }

        _ctx->t = fec_symbolc("t");
        fec_set( _ctx->t, _ctx->t);

        for (std::uint8_t i = 0; i < P_MAX; i++) {
            FecObjectPtr v = object();
            v->set_type( FecTokenType::FEC_TPRIM);
            v->set_primitive(i);
            fec_set( fec_symbolc(primnames[i]), v);
        }
    }

    int streq(FecObjectPtr obj, const char* str) {
        while (!obj->isnil()) {
            for (int i = 0; i < FecObject::STRING_BUFFER_SIZE; i++) {
                if (obj->string_buffer()[i] != *str) {
                    return 0;
                }
                if (*str) {
                    str++;
                }
            }
            obj = obj->get_cdr();
        }
        return *str == '\0';
    }

    FecObjectPtr object() {
        FecObjectPtr obj;
        obj = _ctx->freelist;
        _ctx->freelist = obj->get_cdr();
        return obj;
    }

    FecObjectPtr fec_cons(FecObjectPtr car, FecObjectPtr cdr) {
        FecObjectPtr obj = object();
        obj->set_car(car);
        obj->set_cdr(cdr);
        return obj;

    }

    FecObjectPtr buildstring(FecObjectPtr tail, char chr) {
        if (!tail || tail->string_buffer()[FecObject::STRING_BUFFER_SIZE - 1] != '\0') {
            FecObjectPtr obj = fec_cons(nullptr, FecObject::nil());
            obj->set_type(FecTokenType::FEC_TSTRING);
            if (tail) {
                tail->set_cdr(obj);
            }
            tail = obj;
        }
        tail->string_buffer()[strlen(reinterpret_cast<const char*>(tail->string_buffer()))] = chr;
        return tail;
    }

    FecObjectPtr fec_stringc(const char* str) {
        FecObjectPtr obj = buildstring(nullptr, '\0');
        FecObjectPtr tail = obj;
        while (*str) {
            tail = buildstring(tail, *str++);
        }
        return obj;
    }

    FecObjectPtr fec_numberc(fec_number n) {
        FecObjectPtr obj = object();
        obj->set_type(FecTokenType::FEC_TNUMBER);
        obj->set_number(n);
        return obj;
    }

    FecObjectPtr fec_symbolc(const char *name) {
        FecObjectPtr obj;
        for (obj = _ctx->symlist; !obj->isnil(); obj = obj->get_cdr()) {
            if (streq(obj->get_car()->get_cdr()->get_car(), name)) {
                return obj->get_car();
            }
        }
        obj = object();
        obj->set_type(FecTokenType::FEC_TSYMBOL);
        obj->set_cdr(fec_cons(fec_stringc(name), FecObject::nil()));
        _ctx->symlist = fec_cons(obj, _ctx->symlist);
        return obj;
    }

    void fec_set(FecObjectPtr sym, FecObjectPtr v) {
        getbound(sym, FecObject::nil())->set_cdr(v);
    }

    FecObjectPtr getbound(FecObjectPtr sym, FecObjectPtr env) {
        for (; !env->isnil(); env = env->get_cdr()) {
            FecObjectPtr x = env->get_car();
            if (x->get_car() == sym) {
                return x;
            }  
        }
        return sym->get_cdr();
    }

    FecObjectPtr eval(FecObjectPtr obj, FecObjectPtr env, FecObjectPtr) {
        FecObjectPtr fn, arg, res;
        FecObject cl;

        if (obj->get_type() == FecTokenType::FEC_TSYMBOL) {
            return getbound(obj, env)->get_cdr();
        }

        if (obj->get_type() != FecTokenType::FEC_TPAIR) {
            return obj;
        }

        cl.set_car(obj);
        cl.set_cdr(_ctx->callist);
        _ctx->callist = &cl;

        fn = eval(obj->get_car(), env, NULL);
        arg = obj->get_cdr();
        res = FecObject::nil();
        switch (fn->get_type()) {
            case FecTokenType::FEC_TPRIM:
                switch (fn->get_primitive()) {
                    case P_PRINT:
                        while (!arg->isnil()) {
                            res = evalarg(arg, env);
                            fec_writep(res);
                            if (!arg->isnil()) {
                                std::cout << " ";
                            }
                        }
                        break;
                    case P_ADD: res = arithop(arg, env, std::plus<fec_number>()); break;
                    case P_SUB: res = arithop(arg, env, std::minus<fec_number>()); break;
                    case P_MUL: res = arithop(arg, env, std::multiplies<fec_number>()); break;
                    case P_DIV: res = arithop(arg, env, std::divides<fec_number>()); break;
                    default: break;

                }
                break;
            default: break;
        }
        _ctx->callist = cl.get_cdr();
        return res;
    }

    FecObjectPtr fec_eval(FecObjectPtr obj) {
        return eval(obj, FecObject::nil(), nullptr);
    }

    FecObjectPtr evalarg(FecObjectPtr& arg, FecObjectPtr env) {
        return eval(fec_nextarg(&arg), env, nullptr);
    }

    FecObjectPtr fec_read_(fec_readfn fn, std::ifstream& source) {
        char chr;
        FecObjectPtr v, res, *tail;
        char *p, buf[64];
        fec_number n;


        chr = _ctx->nextchar ? _ctx->nextchar : fn(source);
        _ctx->nextchar = '\0';

        while (chr && isspace(chr)) {
            chr = fn(source);
        }
        switch (chr) {
            case '\0':
                return nullptr;
            case ')':
                return &rparen;
            case '(':
                res = FecObject::nil();
                tail = &res;
                while ( (v = fec_read_(fn, source)) != &rparen) {
                    if (v  == nullptr) {
                        return nullptr;
                    }
                    *tail = fec_cons(v, FecObject::nil());
                    tail = &((*tail)->get_cdr_ptr());
                }
                return res;
            default:
                p = buf;
                do {
                    if (p == buf + sizeof(buf) - 1) {
                        return nullptr;
                    }
                    *p++ = chr;
                    chr = fn(source);
                } while(chr && !is_delimiter(chr));
                *p = '\0';
                _ctx->nextchar = chr;
                n = strtof(buf, &p);
                if (p != buf && is_delimiter(chr)) {
                    return fec_numberc(n);
                }
                return fec_symbolc(buf);

        }
    }

    FecObjectPtr fec_read(fec_readfn fn, std::ifstream& source) {
        FecObjectPtr obj = fec_read_(fn, source);
        return obj;

    }

    static char readp(std::ifstream& source) {
        char chr;
        if (!source.get(chr)) {
            return '\0';
        }
        return chr;
    }

    FecObjectPtr fec_readf(std::ifstream& source) {
        return fec_read(readp, source);
    }

    void fec_writep(FecObjectPtr obj) {
        char buf[32];

        switch (obj->get_type()) {
            case FecTokenType::FEC_TNUMBER:
                std::snprintf(buf, sizeof(buf),"%.7g", obj->get_number());
                std::cout << buf;
                break;
            default:
                std::cout << static_cast<int>(obj->get_type()) << '\n';
                break;
        }
    }

    FecObjectPtr fec_nextarg(FecObjectPtr *arg) {
        FecObjectPtr a = *arg;

        *arg = a->get_cdr();
        return a->get_car();
    }

    template <typename OP>
    FecObjectPtr arithop(FecObjectPtr& arg, FecObjectPtr env, OP op) {
        fec_number x = evalarg(arg, env)->get_number(); 
        while (!arg->isnil()) { 
            x = op(x, evalarg(arg, env)->get_number()); 
        } 
        return fec_numberc(x); 
    }

    static bool is_delimiter(char c) {
        return c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '(' || c == ')';
    }

};