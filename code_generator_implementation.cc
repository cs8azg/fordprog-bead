#include "implementation.hh"
#include <iostream>
#include <sstream>

std::string number_expression::get_code(routine_context* _context) const {
    std::stringstream ss;
    ss << "mov eax," << value << std::endl;
    return ss.str();
}

std::string boolean_expression::get_code(routine_context* _context) const {
    std::stringstream ss;
    ss << "mov al," << (value ? 1 : 0) << std::endl;
    return ss.str();
}

std::string next_label() {
    std::stringstream ss;
    ss << "label_" << id++;
    return ss.str();
}

int symbol::get_size() {
    return symbol_type == boolean ? 1 : 4;
}

std::string get_register(type t) {
    return t == boolean ? "al" : "eax";
}

std::string id_expression::get_code(routine_context* _context) const {
    unsigned offset_from_ebp = _context->get_variable_offset_from_ebp(name);
    return "mov eax, [ebp-" + std::to_string(offset_from_ebp) + "]\n";
}

std::string operator_code(std::string op) {
    std::stringstream ss;
    if(op == "+") {
        ss << "add eax,ecx" << std::endl;
    } else if(op == "-") {
        ss << "sub eax,ecx" << std::endl;
    } else if(op == "*") {
        ss << "xor edx,edx" << std::endl;
        ss << "mul ecx" << std::endl;
    } else if(op == "/") {
        ss << "xor edx,edx" << std::endl;
        ss << "div ecx" << std::endl;   
    } else if(op == "%") {
        ss << "xor edx,edx" << std::endl;
        ss << "div ecx" << std::endl;
        ss << "mov eax,edx" << std::endl;
    } else if(op == "<") {
        ss << "cmp eax,ecx" << std::endl;
        ss << "mov al,0" << std::endl;
        ss << "mov cx,1" << std::endl;
        ss << "cmovb ax,cx" << std::endl;
    } else if(op == "<=") {
        ss << "cmp eax,ecx" << std::endl;
        ss << "mov al,0" << std::endl;
        ss << "mov cx,1" << std::endl;
        ss << "cmovbe ax,cx" << std::endl;
    } else if(op == ">") {
        ss << "cmp eax,ecx" << std::endl;
        ss << "mov al,0" << std::endl;
        ss << "mov cx,1" << std::endl;
        ss << "cmova ax,cx" << std::endl;
    } else if(op == ">=") {
        ss << "cmp eax,ecx" << std::endl;
        ss << "mov al,0" << std::endl;
        ss << "mov cx,1" << std::endl;
        ss << "cmovae ax,cx" << std::endl;
    } else if(op == "and") {
        ss << "cmp al,1" << std::endl;
        ss << "cmove ax,cx" << std::endl;
    } else if(op == "or") {
        ss << "cmp al,0" << std::endl;
        ss << "cmove ax,cx" << std::endl;
    } else {
        error(-1, "Bug: Unsupported binary operator: " + op);
    }
    return ss.str();
}

std::string eq_code(type t) {
    std::stringstream ss;
    if(t == natural) {
        ss << "cmp eax,ecx" << std::endl;
    } else {
        ss << "cmp al,cl" << std::endl;
    }
    ss << "mov al,0" << std::endl;
    ss << "mov cx,1" << std::endl;
    ss << "cmove ax,cx" << std::endl;
    return ss.str();
}

std::string binop_expression::get_code(routine_context* _context) const {
    std::stringstream ss;
    ss << left->get_code(_context);
    ss << "push eax" << std::endl;
    ss << right->get_code(_context);
    ss << "mov ecx,eax" << std::endl;
    ss << "pop eax" << std::endl;
    ss << (op == "=" ? eq_code(left->get_type(_context)) : operator_code(op));
    return ss.str();
}

std::string not_expression::get_code(routine_context* _context) const {
    std::stringstream ss;
    ss << operand->get_code(_context);
    ss << "xor al,1" << std::endl;
    return ss.str();
}

std::string ternary_expression::get_code(routine_context* _context) const {
    std::string else_label = next_label();
    std::string end_label = next_label();
    std::stringstream ss;
    ss << cond->get_code(_context);
    ss << "cmp al,1" << std::endl;
    ss << "jne near " << else_label << std::endl;
    ss << exp_then->get_code(_context);
    ss << "jmp " << end_label << std::endl;
    ss << else_label << ":" << std::endl;
    ss << exp_else->get_code(_context);
    ss << end_label << ":" << std::endl;
    return ss.str();
}

std::string function_call_expression::get_code(routine_context* _context) const {
    std::stringstream ss;
    for (std::list<expression*>::reverse_iterator it = parameters->rbegin(); it != parameters->rend(); ++it) {
        ss << (*it)->get_code(_context);
        ss << "push " << get_register((*it)->get_type(_context)) << std::endl;
    }
    ss << "call function_" << id << std::endl;
    return ss.str();
}

std::string assign_instruction::get_code(routine_context* _context) {
    unsigned offset_from_ebp = _context->get_variable_offset_from_ebp(left);

    std::stringstream ss;
    ss << right->get_code(_context);
    ss << "mov [ebp-" << std::to_string(offset_from_ebp) << "]," << get_register(_context->get_symbol_table()->at(left)->symbol_type) << std::endl;
    return ss.str();
}

std::string get_type_name(type t) {
    return t == boolean ? "boolean" : "natural";
}

std::string read_instruction::get_code(routine_context* _context) {
    symbol* destination = _context->get_symbol_table()->at(id);
    unsigned offset_from_ebp = _context->get_variable_offset_from_ebp(id);
    std::stringstream ss;
    ss << "call read_" << get_type_name(destination->symbol_type) << std::endl;
    ss << "mov [ebp-" << std::to_string(offset_from_ebp) << "]," << get_register(destination->symbol_type) << std::endl;
    return ss.str();
}

std::string write_instruction::get_code(routine_context* _context) {
    std::stringstream ss;
    ss << exp->get_code(_context);
    if(exp_type == boolean) {
        ss << "and eax,1" << std::endl;
    }
    ss << "push eax" << std::endl;
    ss << "call write_" << get_type_name(exp_type) << std::endl;
    ss << "add esp,4" << std::endl;
    return ss.str();
}

std::string if_instruction::get_code(routine_context* _context) {
    std::string else_label = next_label();
    std::string end_label = next_label();
    std::stringstream ss;
    ss << condition->get_code(_context);
    ss << "cmp al,1" << std::endl;
    ss << "jne near " << else_label << std::endl;
    generate_code_of_commands(ss, _context, true_branch);
    ss << "jmp " << end_label << std::endl;
    ss << else_label << ":" << std::endl;
    generate_code_of_commands(ss, _context, false_branch);
    ss << end_label << ":" << std::endl;
    return ss.str();
}

std::string while_instruction::get_code(routine_context* _context) {
    std::string begin_label = next_label();
    std::string end_label = next_label();
    std::stringstream ss;
    ss << begin_label << ":" << std::endl;
    ss << condition->get_code(_context);
    ss << "cmp al,1" << std::endl;
    ss << "jne near " << end_label << std::endl;
    generate_code_of_commands(ss, _context, body);
    ss << "jmp " << begin_label << std::endl;
    ss << end_label << ":" << std::endl;
    return ss.str();
}

std::string for_instruction::get_code(routine_context* _context) {
    std::string begin_label = next_label();
    std::string end_label = next_label();
    std::stringstream ss;

    unsigned offset_from_ebp = _context->get_variable_offset_from_ebp(id);

    // Initializing iterator
    ss << from->get_code(_context);
    ss << "mov [ebp-" + std::to_string(offset_from_ebp) + "], eax" << std::endl;

    // Loop condition evaluation
    ss << begin_label << ":" << std::endl;
    // - Moving iterator and upper bound values to eax ecx registers
    ss << "mov eax,[ebp-" << _context->get_variable_offset_from_ebp(id) << "]" << std::endl;
    ss << "push eax" << std::endl;
    ss << to->get_code(_context);
    ss << "mov ecx,eax" << std::endl;
    ss << "pop eax" << std::endl;
    // - Comparing iterator to upper bound
    ss << operator_code("<");
    // - Based on results continuing loop or jumping to end
    ss << "cmp al,1" << std::endl;
    ss << "jne near " << end_label << std::endl;

    // Body execution
    generate_code_of_commands(ss, _context, body);
    // - Stepping iterator
    ss << "mov eax,[ebp-" + std::to_string(offset_from_ebp) + "]" << std::endl;
    ss << "inc eax" << std::endl;
    ss << "mov [ebp-" + std::to_string(offset_from_ebp) + "]," << get_register(_context->get_symbol_table()->at(id)->symbol_type) << std::endl;
    // - Jumping to condition evaluation
    ss << "jmp " << begin_label << std::endl;

    // End of loop
    ss << end_label << ":" << std::endl;
    return ss.str();
}

std::string function_call_instruction::get_code(routine_context* _context) {
    std::stringstream ss;
    ss << "push eax";
    ss << func_exp->get_code(_context);
    ss << "pop eax";
    return ss.str();
}

std::string return_instruction::get_code(routine_context* _context) {
    std::stringstream ss;
    // Placing return value on eax
    ss << exp->get_code(_context);
    // Jumping to the end of the routine
    ss << "jmp " << _context->end_label << std::endl;
    return ss.str();
}

std::string function_declaration::get_code() {
    std::stringstream ss;
    std::string function_label = "function_" + name;
    std::string end_label = next_label();
    r_context->end_label = end_label;

    // Function label
    ss << function_label << ":" << std::endl;

    // Creating current stack
    unsigned stack_size = r_context->get_total_offset_in_bytes();
    ss << "enter " << std::to_string(stack_size) << ", 0" << std::endl;

    unsigned parameter_offset_from_ebp = 0;
    if (parameter_symbols->size() > 0) {
        // Moving arguments from outside of stack to corresponding offset of local variable
        for (std::list<symbol*>::iterator it = parameter_symbols->begin(); it != parameter_symbols->end(); ++it) {
            // Increasing offset by the size of the current parameter
            parameter_offset_from_ebp += (*it)->get_size();
            // Getting parameter register by its type
            std::string regist = get_register((*it)->symbol_type);
            // Moving argument to current stack as local variable
            ss << "mov " << regist << ", [ebp+" << std::to_string(parameter_offset_from_ebp + 4) << "]" << std::endl;
            ss << "mov [ebp-" << std::to_string(r_context->get_variable_offset_from_ebp((*it)->name)) << "], " << regist << std::endl;
        }
    }

    // Function body
    generate_code_of_commands(ss, r_context, r_context->get_commands());

    ss << end_label << ":" << std::endl;
    // Restoring previous stack
    ss << "leave" << std::endl;
    ss << "ret " << std::to_string(parameter_offset_from_ebp) << std::endl;

    return ss.str();
}

unsigned routine_context::get_variable_offset_from_ebp(std::string _name) const {
    if (symbol_table->count(_name) == 0) {
        error(line, "Undefined variable: " + _name);
    }

    unsigned offset_from_ebp = 0;
    bool found = false;
    for (
        std::map<std::string, symbol*>::iterator it = symbol_table->begin(); 
        it != symbol_table->end() && !found;
        ++it
    ) {
        offset_from_ebp += it->second->get_size();
        if (it->first.compare(_name) == 0) {
            found = true;
        }
    }
    return offset_from_ebp;
}

unsigned routine_context::get_total_offset_in_bytes() const {
    unsigned total_offset = 0;
    for (
        std::map<std::string, symbol*>::iterator it = symbol_table->begin(); 
        it != symbol_table->end();
        ++it
    ) {
        total_offset += it->second->get_size();
    }
    return total_offset;
}

std::string allocate_stack(unsigned total_bytes) {
    std::stringstream ss;
    ss << "sub esp, " << std::to_string(total_bytes) << std::endl;
    return ss.str();
}

void generate_code_of_commands(std::ostream& out, routine_context* context, std::list<instruction*>* commands) {
    if (!commands || commands->size() == 0) {
        return;
    }

    std::list<instruction*>::iterator it;
    for(it = commands->begin(); it != commands->end(); ++it) {
        out << (*it)->get_code(context);
    }
}

void generate_code(routine_context* context) {
    std::string exit_label = next_label();

    std::cout << "global main" << std::endl;
    std::cout << "extern write_natural" << std::endl;
    std::cout << "extern read_natural" << std::endl;
    std::cout << "extern write_boolean" << std::endl;
    std::cout << "extern read_boolean" << std::endl;
    std::cout << std::endl;

    std::cout << "section .text" << std::endl << std::endl;

    for (std::map<std::string, function_declaration*>::iterator it = function_table.begin(); it != function_table.end(); ++it) {
        std::cout << it->second->get_code();
        std::cout << std::endl;
    }

    std::cout << "main:" << std::endl;
    std::cout << "enter " << std::to_string(context->get_total_offset_in_bytes()) << ", 0" << std::endl;
    generate_code_of_commands(std::cout, context, context->get_commands());
    std::cout << exit_label << ":" << std::endl;
    std::cout << "xor eax,eax" << std::endl;
    std::cout << "leave" << std::endl;
    std::cout << "ret" << std::endl;
}

void debug_comment(std::string comment) {
    std::cout << "; " << comment << std::endl;
}
