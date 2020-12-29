%language "c++"
%locations
%define api.value.type variant

%code top {
#include "implementation.hh"
#include <list>
}

%code provides {
int yylex(yy::parser::semantic_type* yylval, yy::parser::location_type* yylloc);
}

%token PRG
%token BEG
%token END
%token FUN
%token BOO
%token NAT
%token REA
%token WRI
%token IF
%token THE
%token ELS
%token EIF
%token WHI
%token FOR
%token TO
%token DO
%token DON
%token TRU
%token FAL
%token ASN
%token COM
%token QM
%token COL
%token OP
%token CL
%token <std::string> ID
%token <std::string> NUM

%left OR
%left AND
%left EQ
%left LS GR LSE GRE
%left ADD SUB
%left MUL DIV MOD
%precedence NOT

%type <expression*> expression
%type <instruction*> command
%type <std::list<instruction*>* > commands
%type <function*> function
%type <std::list<function*>* > functions

%%

start:
    function_declarations program
;

function_declarations:
    // empty
|    
    function_declarations function_declaration
;

function_declaration:
    // declarations: tudni kell, hogy milyen környezethez deklaráljuk a változókat
    FUN type ID OP parameter_declarations CL variable_declarations BEG commands END
    {
        // Merge declarations
        std::list<symbol*>* symbols = new std::list<symbol*>();
        for (std::list<declaration*>::iterator it = $5->begin(); it != $5->end(); ++it) {
            symbols->push_back(*it);
        }
        for (it = $7->begin(); it != $7->end(); ++it) {
            symbols->push_back(*it);
        }

        // Create function context
        routine_context context($9, symbols);

        // Type check function
        type_check_commands($9, &context);

        function_declaration(@1.begin.line, $3, $2, $5, $7, $9).declare();
    }
;

parameter_declarations:
    // empty
|
    // declaration: paraméter deklaráció külön módon kéne történjen, hogy ne keveredjenek a változókkal
    parameter_declarations COM parameter_declaration
;

parameter_declaration:
    type ID { $$ = symbol(@1.begin.line, $2, $3); }
;

program:
    PRG ID declarations BEG commands END
    {
        // Create main context
        routine_context context($5, $3);

	    // Type check
        context.type_check_commands($5);

        // Handle commands
        if (current_mode == compiler) {
            generate_code($5);
        } else {
            execute_commands($5);
        }
        delete_commands($5);
    }
;

type:
    BOO { $$ = boolean; }
|
    NAT { $$ = natural; }
;

variable_declarations:
    // empty
    {
	    $$ = new std::list<symbol*>();
    }
|
    variable_declarations variable_declaration
    {
        $1->push_back($2);
        $$ = $1;
    }
;

variable_declaration:
    type ID
    { 
        $$ = symbol(@1.begin.line, $2, $1);
    }
;

commands:
    // empty
    {
        $$ = new std::list<instruction*>();
    }
|
    commands command
    {
        $1->push_back($2);
        $$ = $1;
    }
;

command:
    REA OP ID CL
    {
        $$ = new read_instruction(@1.begin.line, $3);
    }
|
    WRI OP expression CL
    {
        $$ = new write_instruction(@1.begin.line, $3);
    }
|
    ID ASN expression
    {
        $$ = new assign_instruction(@2.begin.line, $1, $3);
    }
|
    IF expression THE commands EIF
    {
        $$ = new if_instruction(@1.begin.line, $2, $4, 0);
    }
|
    IF expression THE commands ELS commands EIF
    {
        $$ = new if_instruction(@1.begin.line, $2, $4, $6);
    }
|
    WHI expression DO commands DON
    {
        $$ = new while_instruction(@1.begin.line, $2, $4);
    }
|
    FOR ID ASN expression TO expression DO commands DON
    {
	    $$ = new for_instruction(@1.begin.line, $2, $4, $6, $8);
    }
|
    function_expression
    {
        $$ = new function_call_instruction(@1.begin.line, $1);
    }
;

parameters:
    // empty
    {
        $$ = new std::list<unsigned*>();
    }
|
    parameters COM expression
    {
        $1->push_back($2);
        $$ = $1;
    }
;

assignment:
    ID ASN expression
    {

    }
|
    ID COL assignment COL expression
    {

    }
;

expression:
    NUM
    {
        $$ = new number_expression($1);
    }
|
    TRU
    {
        $$ = new boolean_expression(true);
    }
|
    FAL
    {
        $$ = new boolean_expression(false);
    }
|
    ID
    {
        $$ = new id_expression(@1.begin.line, $1);
    }
|
    expression ADD expression
    {
        $$ = new binop_expression(@2.begin.line, "+", $1, $3);
    }
|
    expression SUB expression
    {
        $$ = new binop_expression(@2.begin.line, "-", $1, $3);
    }
|
    expression MUL expression
    {
        $$ = new binop_expression(@2.begin.line, "*", $1, $3);
    }
|
    expression DIV expression
    {
        $$ = new binop_expression(@2.begin.line, "/", $1, $3);
    }
|
    expression MOD expression
    {
        $$ = new binop_expression(@2.begin.line, "%", $1, $3);
    }
|
    expression LS expression
    {
        $$ = new binop_expression(@2.begin.line, "<", $1, $3);
    }
|
    expression GR expression
    {
        $$ = new binop_expression(@2.begin.line, ">", $1, $3);
    }
|
    expression LSE expression
    {
        $$ = new binop_expression(@2.begin.line, "<=", $1, $3);
    }
|
    expression GRE expression
    {
        $$ = new binop_expression(@2.begin.line, ">=", $1, $3);
    }
|
    expression AND expression
    {
        $$ = new binop_expression(@2.begin.line, "and", $1, $3);
    }
|
    expression OR expression
    {
        $$ = new binop_expression(@2.begin.line, "or", $1, $3);
    }
|
    expression EQ expression
    {
        $$ = new binop_expression(@2.begin.line, "=", $1, $3);
    }
|
    NOT expression
    {
        $$ = new not_expression(@1.begin.line, "not", $2);
    }
|
    OP expression CL
    {
        $$ = $2;
    }
|
    OP expression QM expression COL expression CL
    {
        $$ = new ternary_expression(@3.begin.line, $2, $4, $6);
    }
|
    function_expression
    {
        $$ = $1;
    }
;

function_expression:
    ID OP parameters CL
    {
        $$ = new function_call_expression(@1.begin.line, $1, $3);
    }
;

%%
