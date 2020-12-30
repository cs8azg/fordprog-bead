#include "implementation.hh"
#include <sstream>
#include <stack>
#include <iostream>

// execution_context
execution_context::execution_context(routine_context* _routine_context, std::list<instruction*>* _commands) {
    symbol_table = _routine_context->get_symbol_table();
    commands = _commands;
    value_table = new std::map<std::string, unsigned>();
}

execution_context::~execution_context() {
    delete value_table;
}

unsigned execution_context::execute() {
    context_stack.push(this);
    execute_commands(commands);
    context_stack.pop();
}

unsigned execution_context::get_variable_value(const id_expression* _id_exp) const {
    // if (symbol_table->count(_id_exp->get_name()) == 0) {
    //     error(_id_exp->get_line(), std::string("Variable has not been initialized: ") + _id_exp->get_name());
    // }
    return value_table->at(_id_exp->get_name());
}

unsigned execution_context::get_variable_value(int _line, std::string _id) const {
    // if (symbol_table->count(_id) == 0) {
    //     error(_line, std::string("Variable has not been initialized: ") + _id);
    // }
    return value_table->at(_id);
}

type execution_context::get_variable_type(std::string _id) const {
    return symbol_table->at(_id)->symbol_type;
}

void execution_context::set_variable_value(assign_instruction* _as_inst) {
    set_variable_value(_as_inst->get_id(), _as_inst->get_value());
}

void execution_context::set_variable_value(std::string _id, unsigned _value) {
    value_table->insert_or_assign(_id, _value);
}

// function_execution_context
function_execution_context::function_execution_context(routine_context* _routine_context, std::list<instruction*>* _commands, std::map<std::string, unsigned>* _argument_value_table)
    : execution_context(_routine_context, _commands), argument_value_table(_argument_value_table)
{
    initialize_from_arguments();
}

function_execution_context::~function_execution_context() {}

void function_execution_context::initialize_from_arguments() {
    for (std::map<std::string, unsigned>::iterator it = argument_value_table->begin(); it != argument_value_table->end(); ++it) {
        value_table->insert(*it);
    }
}

std::stack<execution_context*> context_stack;

unsigned number_expression::get_value() const {
    return value;
}

unsigned boolean_expression::get_value() const {
    return (unsigned)value;
}

unsigned id_expression::get_value() const {
    return current_context()->get_variable_value(this);
}

unsigned binop_expression::get_value() const {
    int left_value = left->get_value();
    int right_value = right->get_value();
    if(op == "+") {
        return left_value + right_value;
    } else if(op == "-") {
        return left_value - right_value;
    } else if(op == "*") {
        return left_value * right_value;
    } else if(op == "/") {
        return left_value / right_value;
    } else if(op == "%") {
        return left_value % right_value;
    } else if(op == "<") {
        return left_value < right_value;
    } else if(op == ">") {
        return left_value > right_value;
    } else if(op == "<=") {
        return left_value <= right_value;
    } else if(op == ">=") {
        return left_value >= right_value;
    } else if(op == "and") {
        return left_value && right_value;
    } else if(op == "or") {
        return left_value || right_value;
    } else if(op == "=") {
        return left_value == right_value;
    } else {
        error(line, std::string("Unknown operator: ") + op);
    }
}

unsigned not_expression::get_value() const {
    return !(bool)(operand->get_value());
}

unsigned ternary_expression::get_value() const {
    if (cond->get_value()) {
        return exp_then->get_value();
    } else {
        return exp_else->get_value();
    }
}

unsigned function_call_expression::get_value() const {
    function_declaration* func_decl = function_table[id];
    std::map<std::string, unsigned> argument_value_table;
    std::list<expression*>::iterator arg_it;
    std::list<symbol*>::iterator par_it;
    for (
        arg_it = parameters->begin(), par_it = func_decl->parameter_symbols->begin(); 
        arg_it != parameters->end() && par_it != func_decl->parameter_symbols->end(); 
        ++arg_it, ++par_it
    ) {
        argument_value_table[(*par_it)->name] = (*arg_it)->get_value();
    }
    function_execution_context exec_context(func_decl->r_context, func_decl->commands, &argument_value_table);
    exec_context.execute();
}

bool instruction::had_return_instruction() {
    return returned;
}

unsigned instruction::get_return_value() {
    return return_value;
}

execution_results assign_instruction::execute() {
    current_context()->set_variable_value(this);
    return { false, 0 };
}

std::string assign_instruction::get_id() {
    return left;
}

unsigned assign_instruction::get_value() {
    return right->get_value();
}

execution_results read_instruction::execute() {
    std::string input_line;
    getline(std::cin, input_line);
    if (current_context()->get_variable_type(id) == natural) {
        std::stringstream ss(input_line);
        unsigned input;
        ss >> input;
        current_context()->set_variable_value(id, input);
    } else if (current_context()->get_variable_type(id) == boolean) {
        if (input_line == "true") {
            current_context()->set_variable_value(id, 1);
        } else {
            current_context()->set_variable_value(id, 0);
        }
    }
    return { false, 0 };
}

execution_results write_instruction::execute() {
    if(exp_type == natural) {
        std::cout << exp->get_value() << std::endl;
    } else if(exp_type == boolean) {
        if(exp->get_value()) {
            std::cout << "true" << std::endl;
        } else {
            std::cout << "false" << std::endl;
        }
    }
    return { false, 0 };
}

execution_results if_instruction::execute() {
    if (condition->get_value()) {
        return execute_commands(true_branch);
    } else {
        return execute_commands(false_branch);
    }
}

execution_results while_instruction::execute() {
    execution_results results;
    while(condition->get_value()) {
        results = execute_commands(body);
        if (results.had_return_instruction) {
            return results;
        }
    }
    return { false, 0 };
}

execution_results for_instruction::execute() {
    execution_results results;
    for(
        current_context()->set_variable_value(id, from->get_value()); 
        current_context()->get_variable_value(line, id) < to->get_value(); 
        current_context()->set_variable_value(id, current_context()->get_variable_value(line, id) + 1)
    ) {
    	results = execute_commands(body);
        if (results.had_return_instruction) {
            return results;
        }
    }
    return { false, 0 };
}

execution_results return_instruction::execute() {
    if (exp != nullptr) {
        return { true, exp->get_value() };
    }
    return { true, 0 };
}

execution_results function_call_instruction::execute() {
    expression->get_value();
    return { false, 0 };
}

execution_results execute_commands(std::list<instruction*>* commands) {
    if(!commands) {
        return;
    }

    execution_results results;
    std::list<instruction*>::iterator it;
    for(it = commands->begin(); it != commands->end(); ++it) {
        results = (*it)->execute();
        if (results.had_return_instruction) {
            return results;
        }
    }
    return { false, 0 };
}

execution_context* current_context() {
    return context_stack.top();
}