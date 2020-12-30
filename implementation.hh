#ifndef IMPLEMENTATION_HH
#define IMPLEMENTATION_HH

#include <string>
#include <list>
#include <map>
#include <stack>

enum mode {compiler, interpreter};

extern mode current_mode;

enum type {boolean, natural};

void error(int line, std::string text);

class expression {
  public:
    virtual type get_type(routine_context* _context) const = 0;
    virtual ~expression();
    virtual std::string get_code() const = 0;
    virtual unsigned get_value() const = 0;
};

class number_expression : public expression {
  public:
    number_expression(std::string text);  
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;    
  private:
    unsigned value;
};

class boolean_expression : public expression {
  public:
    boolean_expression(bool _value);  
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;    
  private:
    bool value;
};

extern long id;

extern std::string next_label();

struct symbol {
    symbol() {}
    symbol(int _line, std::string _name, type _type);
    std::string get_code();
    int get_size();
    int line;
    std::string name;
    type symbol_type;
    std::string label;
};

struct function_declaration {
    function_declaration() {}
    function_declaration(int _line, std::string _name, type _return_type, std::list<symbol*>* _parameter_symbols, std::list<symbol*>* _symbols, std::list<instruction*>* _commands);
    std::string get_code();
    int line;
    std::string name;
    type return_type;
    std::string label;
    std::list<symbol*>* parameter_symbols;
    std::list<symbol*>* symbols;
    std::list<instruction*>* commands;
    routine_context* r_context;
};

extern std::map<std::string, function_declaration*> function_table;

class id_expression : public expression {
  public:
    id_expression(int line, std::string _name);  
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;  
    int get_line() const;  
    std::string get_name() const;
  private:
    int line;
    std::string name;
};

class binop_expression : public expression {
  public:
    binop_expression(int _line, std::string _op, expression* _left, expression* _right);
    ~binop_expression();
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;    
  private:
    int line;
    std::string op;
    expression* left;
    expression* right;
};

class not_expression : public expression {
  public:
    not_expression(int _line, std::string _op, expression* _operand);
    ~not_expression();
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;    
  private:
    int line;
    std::string op;
    expression* operand;
};

class ternary_expression : public expression {
  public:
    ternary_expression(int _line, expression* _cond, expression* _exp_then, expression* _exp_else);
    ~ternary_expression();
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;
  private:
    int line;
    expression* cond;
    expression* exp_then;
    expression* exp_else;
};

class function_call_expression : public expression {
  public:
    function_call_expression(int _line, std::string _id, std::list<expression*>* _parameters);
    ~function_call_expression();
    type get_type(routine_context* _context) const;
    std::string get_code() const;
    unsigned get_value() const;
  private:
    int line;
    std::string id;
    std::list<expression*>* parameters;
};

class instruction {
  public:
    instruction(int _line);
    virtual ~instruction();
    virtual void type_check(routine_context* _context) = 0;
    virtual std::string get_code() = 0;
    virtual execution_results execute() = 0;
    int get_line();
  protected:
    int line;
};

class assign_instruction : public instruction {
  public:
    assign_instruction(int _line, std::string _left, expression* right);
    ~assign_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
    std::string get_id();
    unsigned get_value();
  private:
    std::string left;
    expression* right;
};

class read_instruction : public instruction {
  public:
    read_instruction(int _line, std::string _id);
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    std::string id;
};

class write_instruction : public instruction {
  public:
    write_instruction(int _line, expression* _exp);
    ~write_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    expression* exp;
    type exp_type;
};

class if_instruction : public instruction {
  public:
    if_instruction(int _line, expression* _condition, std::list<instruction*>* _true_branch, std::list<instruction*>* _false_branch);
    ~if_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    expression* condition;
    std::list<instruction*>* true_branch;
    std::list<instruction*>* false_branch;
};

class while_instruction : public instruction {
  public:
    while_instruction(int _line, expression* _condition, std::list<instruction*>* _body);
    ~while_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    expression* condition;
    std::list<instruction*>* body;
};

class for_instruction : public instruction {
  public:
    for_instruction(int _line, std::string _id, expression* _from, expression* _to, std::list<instruction*>* _body);
    ~for_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    std::string id;
    expression* from;
    expression* to;
    std::list<instruction*>* body;
};

class return_instruction : public instruction {
  public:
    return_instruction(int _line, expression* _exp);
    ~return_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    expression* exp;
};

class function_call_instruction : public instruction {
  public:
    function_call_instruction(int _line, function_call_expression* _expression);
    ~function_call_instruction();
    void type_check(routine_context* _context);
    std::string get_code();
    execution_results execute();
  private:
    function_call_expression* func_exp;
};

class routine_context {
  public:
    routine_context(std::list<instruction*>* _commands, std::list<symbol*>* _symbols);
    routine_context(std::list<instruction*>* _commands, std::list<symbol*>* _symbols, type _expected_return_type);
    type get_variable_type(int _line, std::string _name);
    std::map<std::string, symbol*>* get_symbol_table();
    type get_expected_return_type();
  private:
    void declare_variable(symbol* _symbol);
    std::map<std::string, symbol*>* symbol_table;
    std::list<instruction*>* commands;
    type expected_return_type;
};

class execution_context {
    public:
        execution_context(routine_context* _routine_context, std::list<instruction*>* _commands);
        ~execution_context();
        unsigned execute();
        unsigned get_variable_value(const id_expression* _id_exp) const;
        unsigned get_variable_value(int _line, std::string _id) const;
        type get_variable_type(std::string _id) const;
        void set_variable_value(assign_instruction* _as_inst);
        void set_variable_value(std::string _id, unsigned _value);
    protected:
        std::map<std::string, symbol*>* symbol_table;
        std::map<std::string, unsigned>* value_table;
    private:
        std::list<instruction*>* commands;
};

class function_execution_context : public execution_context {
  public:
    function_execution_context(routine_context* _routine_context, std::list<instruction*>* _commands, std::map<std::string, unsigned>* _argument_value_table);
    ~function_execution_context();
  private:
    void initialize_from_arguments();
    std::map<std::string, unsigned>* argument_value_table;
};

struct execution_results {
  bool had_return_instruction;
  unsigned return_value;
};

void declare_function(function_declaration* function);

void type_check_commands(std::list<instruction*>* commands, routine_context* context);

void generate_code_of_commands(std::ostream& out, std::list<instruction*>* commands);

void execute_commands_in_new_context(routine_context context, std::list<instruction*>* commands);

execution_results execute_commands(std::list<instruction*>* commands);

execution_context* current_context();

void delete_commands(std::list<instruction*>* commands);

void generate_code(std::list<instruction*>* commands);

#endif // IMPLEMENTATION_HH
