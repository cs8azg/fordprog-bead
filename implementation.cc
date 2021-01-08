#include "implementation.hh"
#include <iostream>
#include <sstream>

mode current_mode;

void error(int line, std::string text) {
    std::cerr << "Line " << line << ": Error: " << text << std::endl;
    exit(1);
}

expression::~expression() {
}

number_expression::number_expression(std::string text) {
    std::stringstream ss(text);
    ss >> value;
}

boolean_expression::boolean_expression(bool _value) {
    value = _value;
}

long id = 0;

symbol::symbol(int _line, std::string _name, type _type) : line(_line), name(_name), symbol_type(_type) {
    label = next_label();
}

function_declaration::function_declaration(int _line, std::string _name, type _return_type, std::list<symbol*>* _parameter_symbols, std::list<symbol*>* _symbols, std::list<instruction*>* _commands) 
    : line(_line), name(_name), return_type(_return_type), parameter_symbols(_parameter_symbols), symbols(_symbols), commands(_commands) {
    std::map<std::string, bool> parameter_symbol_names;
    for (std::list<symbol*>::iterator it = parameter_symbols->begin(); it != parameter_symbols->end(); ++it) {
        if (parameter_symbol_names.count((*it)->name) > 0) {
            error(line, std::string("Duplicate function parameter: ") + name);
        }
    }
    label = next_label();
    r_context = new routine_context(line, commands, symbols, parameter_symbols, return_type);
}

id_expression::id_expression(int _line, std::string _name)
    : line(_line), name(_name)
{}

int id_expression::get_line() const {
    return line;
}

std::string id_expression::get_name() const {
    return name;
}

binop_expression::~binop_expression() {
    delete left;
    delete right;
}

binop_expression::binop_expression(int _line, std::string _op, expression* _left, expression* _right)
    : line(_line), op(_op), left(_left), right(_right)
{}

not_expression::~not_expression() {
    delete operand;
}

not_expression::not_expression(int _line, std::string _op, expression* _operand)
    : line(_line), op(_op), operand(_operand)
{}

ternary_expression::~ternary_expression() {
    delete cond;
    delete exp_then;
    delete exp_else;
}

ternary_expression::ternary_expression(int _line, expression* _cond, expression* _exp_then, expression* _exp_else)
    : line(_line), cond(_cond), exp_then(_exp_then), exp_else(_exp_else)
{}

function_call_expression::function_call_expression(
    int _line, 
    std::string _id, 
    std::list<expression*>* _parameters
) : line(_line), id(_id), parameters(_parameters) {
    // Checking if the function has been declared
    if (function_table.count(id) == 0) {
        error(line, std::string("Undefined function: ") + id);
    }
    // Getting the function declaration
    function_declaration* func = function_table[id];
    // Checking parameter count
    if (func->parameter_symbols->size() != parameters->size()) {
        std::string message = std::string("Incorrect number of arguments when calling function \"") 
                            + func->name 
                            + std::string("\": expected ") 
                            + std::to_string(func->parameter_symbols->size()) 
                            + std::string(", got ") 
                            + std::to_string(parameters->size());
        error(line, message);
    }
}

function_call_expression::~function_call_expression() {
    delete parameters;
}

instruction::instruction(int _line)
    : line(_line)
{}

instruction::~instruction() {
}

int instruction::get_line() {
    return line;
}

assign_instruction::assign_instruction(int _line, std::string _left, expression* _right)
    : instruction(_line), left(_left), right(_right)
{}

assign_instruction::~assign_instruction() {
    delete right;
}

read_instruction::read_instruction(int _line, std::string _id)
    : instruction(_line), id(_id)
{}

write_instruction::write_instruction(int _line, expression* _exp)
    : instruction(_line), exp(_exp)
{}

write_instruction::~write_instruction() {
    delete exp;
}

if_instruction::if_instruction(int _line, expression* _condition, std::list<instruction*>* _true_branch, std::list<instruction*>* _false_branch)
    : instruction(_line), condition(_condition), true_branch(_true_branch), false_branch(_false_branch)
{}

if_instruction::~if_instruction() {
    delete condition;
    delete_commands(true_branch);
    delete_commands(false_branch);
}
    
while_instruction::while_instruction(int _line, expression* _condition, std::list<instruction*>* _body)
    : instruction(_line), condition(_condition), body(_body)
{}

while_instruction::~while_instruction() {
    delete condition;
    delete_commands(body);
}

for_instruction::for_instruction(int _line, std::string _id, expression* _from, expression* _to, std::list<instruction*>* _body)
    : instruction(_line), id(_id), from(_from), to(_to), body(_body)
{}

for_instruction::~for_instruction() {
    delete from;
    delete to;
    delete_commands(body);
}

return_instruction::return_instruction(int _line, expression* _exp) : instruction(_line), exp(_exp) {}

return_instruction::~return_instruction() {
    delete exp;
}

function_call_instruction::function_call_instruction(int _line, function_call_expression* _func_exp)
    : instruction(_line), func_exp(_func_exp)
{}

function_call_instruction::~function_call_instruction() {
    delete func_exp;
}

void delete_commands(std::list<instruction*>* commands) {
    if(!commands) {
        return;
    }
    
    std::list<instruction*>::iterator it;
    for(it = commands->begin(); it != commands->end(); ++it) {
        delete (*it);
    }
    delete commands;
}
