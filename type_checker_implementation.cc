#include "implementation.hh"
#include <iostream>
#include <sstream>

std::map<std::string, function_declaration*> function_table;

type number_expression::get_type(routine_context* _context) const {
    return natural;
}

type boolean_expression::get_type(routine_context* _context) const {
    return boolean;
}

type id_expression::get_type(routine_context* _context) const {
    return _context->get_variable_type(line, name);
}

type operand_type(std::string op) {
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
       op == "<" || op == ">" || op == "<=" || op == ">=") {
           return natural;
    } else {
        return boolean;
    }
}

type return_type(std::string op) {
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
           return natural;
    } else {
        return boolean;
    }
}

type binop_expression::get_type(routine_context* _context) const {
    if (op == "=") {
        if(left->get_type(_context) != right->get_type(_context)) {
            error(line, "Left and right operands of '=' have different types.");
        }
    } else {
        if(left->get_type(_context) != operand_type(op)) {
            error(line, std::string("Left operand of '") + op + "' has unexpected type.");
        }
        if(right->get_type(_context) != operand_type(op)) {
            error(line, std::string("Right operand of '") + op + "' has unexpected type.");
        }
    }
    return return_type(op);
}

type ternary_expression::get_type(routine_context* _context) const {
    if (cond->get_type(_context) != boolean) {
        error(line, std::string("Condition expression of ternary is not boolean."));
    }
    if (exp_then->get_type(_context) != exp_else->get_type(_context)) {
        error(line, std::string("Then and else expressions of ternary are of different types."));
    }
    return exp_then->get_type(_context); 
}

type not_expression::get_type(routine_context* _context) const {
    if (operand->get_type(_context) != boolean) {
        error(line, "Operand of 'not' is not boolean.");
    }
    return boolean;
}

type function_call_expression::get_type(routine_context* _context) const {
    // Checking if the function has been declared
    if (function_table.count(id) == 0) {
        error(line, std::string("Undefined function: ") + id);
    }
    // Getting the function declaration
    function_declaration* func = function_table[id];
    // Type checking arguments
    std::list<symbol*>::iterator par_it;
    std::list<expression*>::iterator arg_it;
    for (
        par_it = func->parameter_symbols->begin(), arg_it = parameters->begin(); 
        par_it != func->parameter_symbols->end() && arg_it != parameters->end(); 
        ++par_it, ++arg_it
    ) {
        type argument_type = (*arg_it)->get_type(_context);
        if ((*par_it)->symbol_type != argument_type) {
            error(line, std::string("Argument has unexpected type when calling function \"") + func->name + std::string("\" at parameter \"") + (*par_it)->name + "\"");
        }
    }
    return function_table[id]->return_type;
}

void assign_instruction::type_check(routine_context* _context) {
    if(_context->get_variable_type(line, left) != right->get_type(_context)) {
        error(line, "Left and right hand sides of assignment are of different types.");
    }
}

void read_instruction::type_check(routine_context* _context) {
    _context->get_variable_type(line, id);
}

void write_instruction::type_check(routine_context* _context) {
    exp_type = exp->get_type(_context);
}

void if_instruction::type_check(routine_context* _context) {
    if(condition->get_type(_context) != boolean) {
        error(line, "Condition of 'if' instruction is not boolean.");
    }
    type_check_commands(true_branch, _context);
    type_check_commands(false_branch, _context);
}

void while_instruction::type_check(routine_context* _context) {
    if(condition->get_type(_context) != boolean) {
        error(line, "Condition of 'while' instruction is not boolean.");
    }
    type_check_commands(body, _context);
}

void for_instruction::type_check(routine_context* _context) {
    if(_context->get_variable_type(line, id) != natural) {
    	error(line, std::string("Type of for loop iterator is not natural."));
    }
    if(from->get_type(_context) != natural) {
    	error(line, "Type of for loop lower bound expression is not natural.");
    }
    if(to->get_type(_context) != natural) {
    	error(line, "Type of for loop higher bound expression is not natural.");
    }
    type_check_commands(body, _context);
}

void return_instruction::type_check(routine_context* _context) {
    if (exp->get_type(_context) != _context->get_expected_return_type()) {
        error(line, std::string("Return value has unexpected type"));
    }
}

void function_call_instruction::type_check(routine_context* _context) {
    expression->get_type(_context);
}

routine_context::routine_context(std::list<instruction*>* _commands, std::list<symbol*>* _symbols)
    : commands(_commands)
{
    for(std::list<symbol*>::iterator it = _symbols->begin(); it != _symbols->end(); ++it) {
        declare_variable(*it);
    }
}

routine_context::routine_context(
    std::list<instruction*>* _commands, 
    std::list<symbol*>* _symbols, 
    type _expected_return_type
) : routine_context(_commands, _symbols) {
    expected_return_type = _expected_return_type;
}

void routine_context::declare_variable(symbol* _symbol) {
    if (symbol_table->count(_symbol->name) > 0) {
        error(_symbol->line, std::string("Re-declared variable: ") + _symbol->name);
    }
    (*symbol_table)[_symbol->name] = _symbol;
}

type routine_context::get_variable_type(int _line, std::string _name) {
    if (symbol_table->count(_name) == 0) {
        error(_line, std::string("Undefined variable: ") + _name);
    }
    return (*symbol_table)[_name]->symbol_type;
}

std::map<std::string, symbol*>* routine_context::get_symbol_table() {
    return symbol_table;
}

type routine_context::get_expected_return_type() {
    return expected_return_type;
}

void declare_function(function_declaration* function) {
    if (function_table.count(function->name) > 0) {
        error(function->line, std::string("Re-declared function: ") + function->name);
    }
    function_table[function->name] = function;
}

void type_check_commands(std::list<instruction*>* commands, routine_context* context) {
    if(!commands) {
        return;
    }
    for(std::list<instruction*>::iterator it = commands->begin(); it != commands->end(); ++it) {
        (*it)->type_check(context);
    }
}