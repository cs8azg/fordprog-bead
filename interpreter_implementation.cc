#include "implementation.hh"
#include <sstream>
#include <stack>

class execution_context {
    public:
        execution_context(std::list<std::string>* _symbols, std::list<instruction*>* _commands) {
            value_table = new std::map<std::string, unsigned>();
            commands = _commands;
        }
        ~execution_context() {
            delete value_table;
            delete commands;
        }
        unsigned execute();
    protected:
        std::map<std::string, symbol> symbol_table;
        std::map<std::string, unsigned>* value_table;
    private:
        std::list<instruction*>* commands;
};

class function_context : public execution_context {
    public:
        function_context(std::list<std::string>* _symbols, std::list<instruction*>* _commands, std::map<std::string, unsigned>* _argument_value_table)
            : execution_context(_symbols, _commands), argument_value_table(_argument_value_table)
        {
            initialize_from_arguments();
        }
        ~function_context() {
            delete argument_value_table;
        }
    private:
        void initialize_from_arguments() {
            for (std::map<std::string, unsigned>::iterator it = argument_value_table->begin(); it != argument_value_table->end(); ++it) {
                value_table->insert(*it);
            }
        }
        std::map<std::string, unsigned>* argument_value_table;
};

std::stack<execution_context*> context_stack;

// std::map<std::string, unsigned> value_table;

unsigned number_expression::get_value() const {
    return value;
}

unsigned boolean_expression::get_value() const {
    return (unsigned)value;
}

unsigned id_expression::get_value() const {
    if(context_stack.top()->count(name) == 0) {
        error(line, std::string("Variable has not been initialized: ") + name);
    }
    return value_table[name];
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

unsigned ternary_expression::get_value() const {
    if (cond->get_value()) {
        return exp_then->get_value();
    } else {
        return exp_else->get_value();
    }
}

unsigned not_expression::get_value() const {
    return !(bool)(operand->get_value());
}

void assign_instruction::execute() {
    value_table[left] = right->get_value();
}

void read_instruction::execute() {
    std::string input_line;
    getline(std::cin, input_line);
    if(symbol_table[id].symbol_type == natural) {
        std::stringstream ss(input_line);
        unsigned input;
        ss >> input;
        value_table[id] = input;
    } else if(symbol_table[id].symbol_type == boolean) {
        if(input_line == "true") {
            value_table[id] = 1;
        } else {
            value_table[id] = 0;
        }
    }
}

void write_instruction::execute() {
    if(exp_type == natural) {
        std::cout << exp->get_value() << std::endl;
    } else if(exp_type == boolean) {
        if(exp->get_value()) {
            std::cout << "true" << std::endl;
        } else {
            std::cout << "false" << std::endl;
        }
    }
}

void if_instruction::execute() {
    if(condition->get_value()) {
        execute_commands(true_branch);
    } else {
        execute_commands(false_branch);
    }
}

void while_instruction::execute() {
    while(condition->get_value()) {
        execute_commands(body);
    }
}

void for_instruction::execute() {
    for(value_table[id] = from->get_value(); value_table[id] < to->get_value(); value_table[id]++) {
    	execute_commands(body);
    }
}

void execute_commands(std::list<instruction*>* commands) {
    if(!commands) {
        return;
    }

    context_stack.push(new execution_context())

    std::list<instruction*>::iterator it;
    for(it = commands->begin(); it != commands->end(); ++it) {
        (*it)->execute();
    }
}