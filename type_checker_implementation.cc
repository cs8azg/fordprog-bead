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

bool assign_instruction::always_returns() {
    return false;
}

void read_instruction::type_check(routine_context* _context) {
    _context->get_variable_type(line, id);
}

bool read_instruction::always_returns() {
    return false;
}

void write_instruction::type_check(routine_context* _context) {
    exp_type = exp->get_type(_context);
}

bool write_instruction::always_returns() {
    return false;
}

void if_instruction::type_check(routine_context* _context) {
    if(condition->get_type(_context) != boolean) {
        error(line, "Condition of 'if' instruction is not boolean.");
    }
    type_check_commands(true_branch, _context);
    type_check_commands(false_branch, _context);
}

bool if_instruction::always_returns() {
    if (false_branch == nullptr) {
        return false;
    }
    return true_branch_always_returns() && false_branch_always_returns();
}

bool if_instruction::true_branch_always_returns() {
    for (std::list<instruction*>::iterator it = true_branch->begin(); it != true_branch->end(); ++it) {
        if ((*it)->always_returns()) {
            return true;
        }
    }
    return false;
}

bool if_instruction::false_branch_always_returns() {
    for (std::list<instruction*>::iterator it = false_branch->begin(); it != false_branch->end(); ++it) {
        if ((*it)->always_returns()) {
            return true;
        }
    }
    return false;
}

void while_instruction::type_check(routine_context* _context) {
    if(condition->get_type(_context) != boolean) {
        error(line, "Condition of 'while' instruction is not boolean.");
    }
    type_check_commands(body, _context);
}

bool while_instruction::always_returns() {
    // This returns true only if the loop body returns on the first iteration, 
    // it does not check changing conditions on branches that could return a value
    for (std::list<instruction*>::iterator it = body->begin(); it != body->end(); ++it) {
        if ((*it)->always_returns()) {
            return true;
        }
    }
    return false;
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

bool for_instruction::always_returns() {
    // This returns true only if the loop body returns on the first iteration, 
    // it does not check changing conditions on branches that could return a value
    for (std::list<instruction*>::iterator it = body->begin(); it != body->end(); ++it) {
        if ((*it)->always_returns()) {
            return true;
        }
    }
    return false;
}

bool switch_case::always_returns() {
    for (std::list<instruction*>::iterator it = body->begin(); it != body->end(); ++it) {
        if ((*it)->always_returns()) {
            return true;
        }
    }
    return false;
}

void switch_instruction::type_check(routine_context* _context) {
    type match_exp_type = exp->get_type(_context);
    if (cases == nullptr || cases->size() == 0) {
        error(line, "Switch-case must have at least one case");
    }
    for (std::list<switch_case*>::iterator it = cases->begin(); it != cases->end(); ++it) {
        if (!(*it)->is_default_case() && (*it)->exp->get_type(_context) != match_exp_type) {
            error(line, "Type of switch-case expression does not match the type of the expression to match.");
        }
        if ((*it)->is_default_case() && (*it) != cases->back()) {
            error(line, "A switch can only have one default case and it must be the last one.");
        }
    }
    for (std::list<switch_case*>::iterator it = cases->begin(); it != cases->end(); ++it) {
        type_check_commands((*it)->body, _context);
    }
}

bool switch_instruction::always_returns() {
    // Checks whether or not all branches return and if there is a default branch
    bool has_default_branch = false;
    for (std::list<switch_case*>::iterator it = cases->begin(); it != cases->end(); ++it) {
        if (!(*it)->always_returns()) {
            return false;
        }
        if ((*it)->exp == nullptr) {
            has_default_branch = true;
        }
    }
    return has_default_branch;
}

void break_instruction::type_check(routine_context* _context) {}

bool break_instruction::always_returns() {
    return false;
}

void return_instruction::type_check(routine_context* _context) {
    if (exp->get_type(_context) != _context->get_expected_return_type()) {
        error(line, std::string("Return value has unexpected type"));
    }
}

bool return_instruction::always_returns() {
    return true;
}

void function_call_instruction::type_check(routine_context* _context) {
    func_exp->get_type(_context);
}

bool function_call_instruction::always_returns() {
    return false;
}

routine_context::routine_context(int _line, std::list<instruction*>* _commands, std::list<symbol*>* _symbols)
    : line(_line), commands(_commands)
{
    symbol_table = new std::map<std::string, symbol*>();
    for (std::list<symbol*>::iterator it = _symbols->begin(); it != _symbols->end(); ++it) {
        declare_variable(*it);
    }
    should_return_value = false;
}

routine_context::routine_context(
    int _line, 
    std::list<instruction*>* _commands, 
    std::list<symbol*>* _symbols, 
    std::list<symbol*>* _parameters,
    type _expected_return_type
) : routine_context(_line, _commands, _symbols) {
    for (std::list<symbol*>::iterator it = _parameters->begin(); it != _parameters->end(); ++it) {
        declare_variable(*it);
    }
    expected_return_type = _expected_return_type;
    should_return_value = true;
}

routine_context::~routine_context() {
    delete symbol_table;
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

std::list<instruction*>* routine_context::get_commands() {
    return commands;
}

type routine_context::get_expected_return_type() {
    return expected_return_type;
}

void routine_context::return_check() {
    if (!should_return_value) {
        return;
    }
    bool always_returns = false;
    for (std::list<instruction*>::iterator it = commands->begin(); it != commands->end(); ++it) {
        if ((*it)->always_returns()) {
            return;
        }
    }
    error(line, std::string("Routine does not return value on all branches."));
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