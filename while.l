%option c++ noyywrap yylineno

%{
#include <iostream>
#include <cstdlib>
#include <list>
#include "implementation.hh"
#include "while.tab.hh"

using namespace yy;
%}

%%

program                     return parser::token::PRG;
begin                       return parser::token::BEG;
end                         return parser::token::END;
function		                return parser::token::FUN;
return                      return parser::token::RET;
boolean                     return parser::token::BOO;
natural                     return parser::token::NAT;
read                        return parser::token::REA;
write                       return parser::token::WRI;
if                          return parser::token::IF;
then                        return parser::token::THE;
else                        return parser::token::ELS;
endif                       return parser::token::EIF;
while                       return parser::token::WHI;
for			                    return parser::token::FOR;
to			                    return parser::token::TO;
do                          return parser::token::DO;
done                        return parser::token::DON;
switch                      return parser::token::SWI;
endswitch                   return parser::token::ESW;
case                        return parser::token::CAS;
default                     return parser::token::DEF;
break                       return parser::token::BRE;
true                        return parser::token::TRU;
false                       return parser::token::FAL;
":="                        return parser::token::ASN;
","			                    return parser::token::COM;
"+"                         return parser::token::ADD;
"-"                         return parser::token::SUB;
"*"                         return parser::token::MUL;
"/"                         return parser::token::DIV;
"%"                         return parser::token::MOD;
"<"                         return parser::token::LS;
">"                         return parser::token::GR;
"<="                        return parser::token::LSE;
">="                        return parser::token::GRE;
"="                         return parser::token::EQ;
"?"                         return parser::token::QM;
":"                         return parser::token::COL;
and                         return parser::token::AND;
or                          return parser::token::OR;
not                         return parser::token::NOT;
"("                         return parser::token::OP;
")"                         return parser::token::CL;
[a-zA-Z_][a-zA-Z0-9_]*      return parser::token::ID;
[0-9]+                      return parser::token::NUM;
[ \t\n]+                    // Whitespaces skipped.
#.*                         // Comments skipped.
.                           { std::cerr << "Line " << lineno() << ": Unexpected character: '" << YYText() << "'." << std::endl; exit(1); }
