#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

// mpc_parser_t * Adjective = mpc_or(4, 
//     mpc_sym("wow"), mpc_sym("many"),
//     mpc_sym("so"), mpc_sym("such")
// );

// mpc_parser_t * Noun = mpc_or(5,
//     mpc_sym("lisp"), mpc_sym("language"),
//     mpc_sym("book"), mpc_sym("build"),
//     mpc_sym("c")
// );

// mpc_parser_t * Phrase = mpc_and(2, 
//     mpcf_strfold, Adjective, Noun, free
// );

// mpc_parser_t * Doge = mpc_many(
//     mpcf_strfold, Phrase
// );

mpc_parser_t * Adjective = mpc_new("adjective");
mpc_parser_t * Noun = mpc_new("noun");
mpc_parser_t * Phrase = mpc_new("phrase");
mpc_parser_t * Doge = mpc_new("doge");


mpca_lang(MPCA_LANG_DEFAULT,
    " \
        adjective : \"wow\" | \"many\"     \
                  | \"so\"  | \"such\" | <noun> \"cool\";    \
        noun      : \"lisp\" | \"language\"    \
                  | \"book\" | \"build\" | \"c\";    \
        phrase    : <adjective> | <noun>;    \
        doge      : <phrase>*;    \
    ",
    Adjective Noun, Phrase, Doge);

mpc_cleanup(4, Adjective, Noun, Phrase, Doge);

// ch5 parsing
// https://www.buildyourownlisp.com -> <scheme>, <domain name>
// scheme -> <protocol> "://"
// domain name -> (<subdomain> <dot>)* <sld> <dot> <tld>
// 
// dot -> "."
// protocol -> "http" | "https" | "ftp" | "file"
// subdomain -> "www" | "" 
// sld -> "buildyourownlisp"
// tld -> "com" | "net" | "org" | "edu"
// 

// 0.01 and 52.221 -> <num>+ <period> <num>+
// num -> [0-9]+
// period -> "."

// polish notation -> exp
// operator -> "/" | "+" | "-" | "*" | "%"
// operand -> <int> | <float> | <big> | exp
// exp -> "(" <operator> <operand> <operand>+ ")"
// int -> [0-9]
// big -> 
// float -> 