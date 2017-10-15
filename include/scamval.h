#pragma once
#include <stdio.h>


/* The possible values for the type field of the scamval struct, populated with an X-macro.
 * Note that some of these types are never exposed to the user.
 */
enum {
#define EXPAND_TYPE(type_val, type_name) \
    type_val,
#include "../src/type.def"
};


/* Forward declaration of scamval */
struct scamval;
typedef struct scamval scamval;


typedef scamval* (*scambuiltin_fun)(scamval*);
typedef struct {
    scambuiltin_fun fun;
    int constant;
} scambuiltin_t;


typedef struct {
    scamval* env; /* A pointer to the environment the function was created in, for closures. */
    scamval* parameters;
    scamval* body;
} scamfun_t;


enum { SCAMPORT_OPEN, SCAMPORT_CLOSED };
typedef struct {
    int status;
    FILE* fp;
} scamport_t;


typedef struct {
    /* A pointer to the enclosing dictionary (if the dictionary is an environment). */
    scamval* enclosing;
    /* Symbols and values are stored as scamval lists. */
    scamval* syms;
    scamval* vals;
} scamdict_t;


struct scamval {
    int type;
    size_t count, mem_size; // Used by SCAM_LIST, SCAM_SEXPR and SCAM_STR.
    union {
        /* DO NOT ACCESS THESE VALUES DIRECTLY! Use the APIs defined below. */
        long long n; // Used by SCAM_INT and SCAM_BOOL
        double d; // SCAM_DEC
        char* s; // SCAM_STR, SCAM_SYM and SCAM_ERR
        scamval** arr; // SCAM_LIST, SCAM_SEXPR and SCAM_DOT_SYM
        scamfun_t* fun; // SCAM_LAMBDA
        scamport_t* port; // SCAM_PORT
        scambuiltin_t* bltin; // SCAM_BUILTIN
        scamdict_t* dct; // SCAM_DICT and SCAM_TYPE_OBJ
    } vals;
    /* Bookkeeping for the garbage collector. */
    int seen;
    int is_root;
};


/*** SCAMVAL CONSTRUCTORS ***/
scamval* scamsym(const char*);
scamval* scamsym_no_copy(char*);
scamval* scamnull(void);


/*** NUMERIC API ***/
scamval* scamint(long long);
scamval* scamdec(double);
scamval* scambool(int);
long long scam_as_int(const scamval*);
long long scam_as_bool(const scamval*);
double scam_as_dec(const scamval*);


/*** SEQUENCE API ***/
scamval* scamlist(void);
scamval* scamlist_from(size_t, ...);
scamval* scamsexpr(void);
scamval* scamsexpr_from(size_t, ...);

/* Return a reference to the i'th element of the sequence. */
scamval* scamseq_get(const scamval*, size_t i);

/* Remove the i'th element of the sequence. */
void scamseq_delete(scamval*, size_t i);

/* Remove and return the i'th element of the sequence. */
scamval* scamseq_pop(scamval*, size_t i);

/* Set the i'th element of the sequence. 
 *   - This obliterates the old element without freeing it. 
 *   - DO NOT USE unless you know the i'th element is already free.
 */
void scamseq_set(scamval* seq, size_t i, scamval* v);

/* Return the actual number of elements in the sequence or string. */
size_t scamseq_len(const scamval*);

/* Append/prepend/insert a value into a sequence.
 *   - The sequence takes responsibility for freeing the value, so it's best not to use a value once
 *     you've inserted it somewhere.
 */
void scamseq_insert(scamval* seq, size_t i, scamval* v);
void scamseq_append(scamval* seq, scamval* v);
void scamseq_prepend(scamval* seq, scamval* v);

/* Concatenate the second argument to the first.
 *   - The second argument is free'd.
 */
void scamseq_concat(scamval* seq1, scamval* seq2);

/* Return a newly allocated subsequence. */
scamval* scamseq_subseq(const scamval* seq, size_t start, size_t end);


/*** STRING API ***/
/* Initialize a string from a character array by copying it. */
scamval* scamstr(const char*);
/* Initialize a string from a character array without making a copy. */
scamval* scamstr_no_copy(char*);
scamval* scamstr_from_literal(char*);
scamval* scamstr_empty(void);

/* Read a line from a file and return it as a string. */
scamval* scamstr_read(FILE*);

scamval* scamstr_from_char(char);
const char* scam_as_str(const scamval*);
void scamstr_set(scamval*, size_t, char);
void scamstr_map(scamval*, int map_f(int));

/* Return the i'th character without removing it. */
char scamstr_get(const scamval*, size_t i);

/* Remove and return the i'th character. */
char scamstr_pop(scamval*, size_t i);
void scamstr_remove(scamval*, size_t beg, size_t end);
void scamstr_truncate(scamval*, size_t);

/* Return a newly-allocated substring. */
scamval* scamstr_substr(const scamval*, size_t, size_t);
void scamstr_concat(scamval* s1, scamval* s2);
size_t scamstr_len(const scamval*);


/*** FUNCTION API ***/
scamval* scamlambda(scamval* env, scamval* parameters, scamval* body);
scamval* scambuiltin(scambuiltin_fun);

/* Construct a constant scambuiltin (one that doesn't change its arguments). */
scamval* scambuiltin_const(scambuiltin_fun);
size_t scamlambda_nparams(const scamval*);
scamval* scamlambda_param(const scamval*, size_t);
scamval* scamlambda_body(const scamval*);

/* Initialize an environment enclosed by the function's environment. */
scamval* scamlambda_env(const scamval*);

/* Return a reference to the function's environment itself. */
const scamval* scamlambda_env_ref(const scamval*);
scambuiltin_fun scambuiltin_function(const scamval*);
int scambuiltin_is_const(const scamval*);


/*** ERROR API ***/
scamval* scamerr(const char*, ...);
scamval* scamerr_arity(const char* name, size_t got, size_t expected);
scamval* scamerr_min_arity(const char* name, size_t got, size_t expected);
scamval* scamerr_eof();


/*** PORT API ***/
scamval* scamport(FILE*);
FILE* scam_as_file(scamval*);
int scamport_status(const scamval*);
void scamport_set_status(scamval*, int);


/*** DICTIONARY API ***/
scamval* scamdict(scamval* enclosing);
scamval* scamdict_from(size_t, ...);
scamval* scamdict_builtins(void);

/* Create a new binding in the dictionary, or update an existing one. */
void scamdict_bind(scamval* dct, scamval* sym, scamval* val);

/* Lookup the symbol in the dictionary and return a copy of the value if it exists and an error if 
 * it doesn't.
 */
scamval* scamdict_lookup(const scamval* dct, const scamval* sym);
size_t scamdict_len(const scamval* dct);
scamval* scamdict_enclosing(const scamval*);

/* Get references to the dictionary keys and values. */
scamval* scamdict_keys(const scamval*);
scamval* scamdict_vals(const scamval*);

/* Get references to individual keys and values. */
scamval* scamdict_key(const scamval* dct, size_t);
scamval* scamdict_val(const scamval* dct, size_t);

/* Set dictionary keys and values. */
void scamdict_set_keys(scamval* dct, scamval* new_keys);
void scamdict_set_vals(scamval* dct, scamval* new_vals);


/*** SCAMVAL PRINTING ***/
char* scamval_to_repr(const scamval*);
char* scamval_to_str(const scamval*);
void scamval_print(const scamval*);
void scamval_println(const scamval*);
void scamval_print_debug(const scamval*);
void scamval_print_ast(const scamval*, int indent);


/*** SCAMVAL COMPARISONS ***/
int scamval_eq(const scamval*, const scamval*);
int scamval_gt(const scamval*, const scamval*);


/*** TYPECHECKING ***/
/* Return the names of types as strings. */
const char* scamtype_name(int type);
const char* scamtype_debug_name(int type);

/* Check if the value belongs to the given type. */
int scamval_typecheck(const scamval*, int type);

/* Return the narrowest type applicable to both types. */
int narrowest_type(int, int);

/* Return the narrowest type applicable to all elements of the sequence. */
int scamseq_narrowest_type(scamval*);

/* Construct a type error message. */
scamval* scamerr_type(const char* name, size_t pos, int got, int expected);
