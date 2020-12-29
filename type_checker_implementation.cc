#include "implementation.hh"
#include <iostream>
#include <sstream>

std::map<std::string, function_declaration> function_table;

type number_expression::get_type(routine_context* _context) const {
    return natural;
}

type boolean_expression::get_type(routine_context* _context) const {
    return boolean;
}

void function_declaration::declare() {
    if(function_table.count(name) > 0) {
        error(line, std::string("Re-declared function: ") + name);
    }
    function_table[name] = *this;
}

type id_expression::get_type(routine_context* _context) const {
    return _context->get_variable_type(line, name);
}

type operand_type(std::string op) {
    if(op == "+" || op == "-" || op == "*" || op == "/" || op == "%" ||
       op == "<" || op == ">" || op == "<=" || op == ">=") {
           return natural;
    } else {
        return boolean;
    }
}

type return_type(std::string op) {
    if(op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
           return natural;
    } else {
        return boolean;
    }
}

type binop_expression::get_type(routine_context* _context) const {
    if(op == "=") {
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
    if(cond->get_type(_context) != boolean) {
        error(line, std::string("Condition expression of ternary is not boolean."));
    }
    if (exp_then->get_type(_context) != exp_else->get_type()) {
        error(line, std::string("Then and else expressions of ternary are of different types."));
    }
    return exp_then->get_type(_context); 
}

type not_expression::get_type(routine_context* _context) const {
    if(operand->get_type(_context) != boolean) {
        error(line, "Operand of 'not' is not boolean.");
    }
    return boolean;
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

void type_check_commands(std::list<instruction*>* commands, routine_context* context) {
    if(!commands) {
        return;
    }
    for(std::list<instruction*>::iterator it = commands->begin(); it != commands->end(); ++it) {
        (*it)->type_check(context);
    }
}

routine_context::routine_context(std::list<instruction*>* _commands, std::list<symbol*>* _symbols)
    : commands(_commands)
{
    for(std::list<symbol*>::iterator it = _symbols->begin(); it != _symbols->end(); ++it) {
        declare_variable(*it);
    }
}

void routine_context::declare_variable(symbol* _symbol) {
    if(symbol_table.count(_symbol->name) > 0) {
        error(_symbol->line, std::string("Re-declared variable: ") + _symbol->name);
    }
    symbol_table[_symbol->name] = _symbol;
}

type routine_context::get_variable_type(int _line, std::string _name) {
    if (symbol_table.count(_name) == 0) {
        error(_line, std::string("Undefined variable: ") + _name);
    }
    return symbol_table[_name]->symbol_type;
}