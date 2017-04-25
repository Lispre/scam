#pragma once
#include <stdio.h>
#include "scamval.h"
#include "tokenize.h"

// Parse a string, returning either an AST or an error
scamval* parse_str(char* s);

// Parse a file given its path, returning either an AST or an error
scamval* parse_file(char* fp);

// Convert a token to a scamval
scamval* scamval_from_token(Token*);