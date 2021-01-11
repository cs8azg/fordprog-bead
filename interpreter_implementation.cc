#include "implementation.hh"
#include <sstream>
#include <stack>
#include <iostream>

std::stack<execution_context*> context_stack;

// execution_context
execution_context::execution_context(routine_context* _routine_context) : r_context(_routine_context) {
    value_table = new std::map<std::string, unsigned>();
}

execution_context::~execution_context() {
    delete value_table;
}

unsigned execution_context::execute() {
    context_stack.push(this);
    execution_results results = execute_commands(r_context->get_commands());
    context_stack.pop();
    return results.return_value;
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
    return r_context->get_symbol_table()->at(_id)->symbol_type;
}

void execution_context::set_variable_value(assign_instruction* _as_inst) {
    set_variable_value(_as_inst->get_id(), _as_inst->get_value());
}

void execution_context::set_variable_value(std::string _id, unsigned _value) {
    (*value_table)[_id] = _value;
}

// function_execution_context
function_execution_context::function_execution_context(routine_context* _routine_context, std::map<std::string, unsigned>* _argument_value_table)
    : execution_context(_routine_context), argument_value_table(_argument_value_table)
{
    initialize_from_arguments();
}

function_execution_context::~function_execution_context() {}

void function_execution_context::initialize_from_arguments() {
    for (std::map<std::string, unsigned>::iterator it = argument_value_table->begin(); it != argument_value_table->end(); ++it) {
        value_table->insert(*it);
    }
}

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
    function_execution_context exec_context(func_decl->r_context, &argument_value_table);
    return exec_context.execute();
}

execution_results assign_instruction::execute() {
    current_context()->set_variable_value(this);
    return { false, 0, false };
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
    return { false, 0, false };
}

execution_results write_instruction::execute() {
    if (exp_type == natural) {
        std::cout << exp->get_value() << std::endl;
    } else if (exp_type == boolean) {
        if(exp->get_value()) {
            std::cout << "true" << std::endl;
        } else {
            std::cout << "false" << std::endl;
        }
    }
    return { false, 0, false };
}

execution_results if_instruction::execute() {
    if (condition->get_value()) {
        return execute_commands(true_branch);
    } else {
        return execute_commands(false_branch);
    }
}

execution_results while_instruction::execute() {
    while(condition->get_value()) {
        execution_results results = execute_commands(body);
        if (results.had_return_instruction) {
            return results;
        }
        if (results.had_break_instruction) {
            break;
        }
    }
    return { false, 0, false };
}

execution_results for_instruction::execute() {
    for(
        current_context()->set_variable_value(id, from->get_value()); 
        current_context()->get_variable_value(line, id) < to->get_value(); 
        current_context()->set_variable_value(id, current_context()->get_variable_value(line, id) + 1)
    ) {
    	execution_results results = execute_commands(body);
        if (results.had_return_instruction) {
            return results;
        }
        if (results.had_break_instruction) {
            break;
        }
    }
    return { false, 0, false };
}

bool switch_case::matches(unsigned _value) {
    return is_default_case() || exp->get_value() == _value;
}

execution_results switch_instruction::execute() {
    // Enters the first case with matching value and executes all following cases until a break instruction is executed, or until the default case is found.
    // If there is a default case and no other cases were entered prior to it, then it is entered.
    unsigned match_exp_value = exp->get_value();
    bool entered = false;
    for (
        std::list<switch_case*>::iterator it = cases->begin(); 
        it != cases->end();
        ++it
    ) {
        if (entered && (*it)->is_default_case()) {
            // A case has already been entered and we found the default
            break;
        }
        if (!entered && (*it)->matches(match_exp_value)) {
            // We found a case with matching value (only evaluated if we haven't entered yet)
            entered = true;
        }
        if (entered) {
            // A case with matching value has been found before
            execution_results results = execute_commands((*it)->body);
            if (results.had_return_instruction) {
                return results;
            }
            if (results.had_break_instruction) {
                break;
            }
        }
    }
    return { false, 0, false };
}

execution_results break_instruction::execute() {
    return { false, 0, true };
}

execution_results return_instruction::execute() {
    return { true, exp->get_value(), false };
}

execution_results function_call_instruction::execute() {
    func_exp->get_value();
    return { false, 0, false };
}

execution_results execute_commands(std::list<instruction*>* commands) {
    if(!commands) {
        return { false, 0, false };
    }

    execution_results results;
    std::list<instruction*>::iterator it;
    for(it = commands->begin(); it != commands->end(); ++it) {
        results = (*it)->execute();
        if (results.had_return_instruction || results.had_break_instruction) {
            return results;
        }
    }
    return { false, 0, false };
}

execution_context* current_context() {
    return context_stack.top();
}