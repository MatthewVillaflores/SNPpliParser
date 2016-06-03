#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <stack>

using namespace std;

const char *HEADER = "@model<spiking_psystems>";
const string RESERVE_KEYWORDS[] = {"@mu", "@marcs", "@ms", "@masynch", "@mseq", "@min", "@mout", "main"};
const int INDEX_MU = 0, INDEX_ARCS = 1, INDEX_MS = 2, 
          INDEX_MASYNCH = 3, INDEX_SEQ = 4, INDEX_IN = 5, 
          INDEX_OUT = 6, INDEX_MAIN = 7;
const int RESERVE_KEYWORD_COUNT = 8;
const string SPECIAL_KEYWORDS[] = {"def", "call"};
const short SPECIAL_DEF_INDEX = 0;
const short SPECIAL_CALL_INDEX = 1;
const int SPECIAL_KEYWORD_COUNT = 2;

class MethodHolder;
class SNP;
class Parameter;
class Neuron;
class Rule;
class Synapse;
class Range;
class TreeNode;

class MethodHolder{
  public:
    string label;
    vector<Parameter> parameters;
    vector<string> contents;
};

class Parameter{
  public:
    string label;
    int value;
};

class SNP{
  public:
    vector<Neuron> neurons;
    vector<Rule> rules;
    vector<Synapse> synapses;
};

class Neuron{
  public:
    string label;
    int spikes;
    Parameter param;
};

class Rule{
  public:
    string regex;
    int c;
    int p;
    int d;
};

class Synapse{
  public:
    string from;
    string to;
};

class Range{
  public:
    string label;
    string x1;
    bool inclusive_x1;
    string x2;
    bool inclusive_x2;
};

class TreeNode{
  public:
    string label;
    int value;
    vector<TreeNode> children;
    TreeNode *parent;
};

void parseFile(char *filename);
void runMethod(MethodHolder method, vector<Parameter> params);
void parseRule(string line, MethodHolder method, vector<Parameter> params);
void eval_mu(string line, MethodHolder method, vector<Parameter> params);
void recursiveCreateNeurons(TreeNode& root, vector<Neuron>& neurons, vector<Neuron>& temp);
void eval_ms(string line, MethodHolder method, vector<Parameter> params);
void recursiveCreateSpikes(TreeNode node, string entry);
void setSpike(string neuron_label, int spikes);
void addSpike(string neuron_label, int spikes);
void eval_arcs(string line, MethodHolder method, vector<Parameter> params);
void recursiveCreateSynapses(TreeNode node, string entry);
void addSynapse(string entry);
int findMethod(string query);
int evalMathExp(string mathexp);
string subsMathExp(string line, vector<Parameter> params);
string matchParameters(string line, vector<Parameter> params);
string matchParameters(string line, vector<Parameter> param_needed, vector<Parameter> param_provided);
void identifyRanges(string entry, TreeNode& root);
void recursiveBranching(TreeNode& node, Range range, vector<Range> exceptions);
vector<string> whitespace_split(string tosplit);
vector<string> split(string tosplit, string delimiter);
string trim(string entry);
string getMethodName(string buffer);
vector<Parameter> getDefParameters(string buffer);
vector<Parameter> getCallParameters(string buffer);
int checkLineSpecialKeyword(string query);
int checkLineReserveKeyword(string query);
int checkReserveKeyword(string query);
int checkSpecialKeyword(string query);
bool is_number(string s);
int findFromIndex(string source, string tofind, int index);
void printMethodHolder(MethodHolder method);
void printParameter(Parameter param);
void printSNP();
void printRange(Range r);
void printTraverseTree(TreeNode root);
void check(string r);
void check();

int main(int argc, char *argv[]){

  //Check command line arguments
  if (argc < 2){
    cout << "No input file\n";
    return 0;
  }

  char *filename = argv[1];

  cout << "Parsing " << filename << "\n";
  
  //Call main method for parsing
  parseFile(filename);
}

vector<MethodHolder> methods;
SNP snpsystem;
int linecount;

void parseFile(char *filename){
  
  //Open and verify file integrity
  ifstream file (filename);
  if(!file.is_open()){
    cout << "File \"" << filename << "\" does not exist\n";
    return;
  }

  string buffer;
  getline(file, buffer);
  linecount = 1;
  //Check header
  if(buffer != HEADER){
    cout << "SNP pli file must have \"" << HEADER << "\" as file header\n";
  }

  vector<MethodHolder> new_methods;

  while(getline(file, buffer)){
    linecount++;
    if(buffer.find("def")!=string::npos){
      MethodHolder method;
      
      method.label = getMethodName(buffer);
      method.parameters = getDefParameters(buffer);
      method.contents.push_back(buffer);
      
      int curlybraces = buffer.find("{");
      stack<string> stacky;
      stacky.push("{");
      while(!stacky.empty() && getline(file, buffer)){
        method.contents.push_back(buffer);
        int openbraces = count(buffer.begin(), buffer.end(), '{');
        int closebraces = count(buffer.begin(), buffer.end(), '}');
        for(int i=0;i<openbraces;i++){
          stacky.push("{");
        }
        for(int i=0;i<closebraces;i++){
          stacky.pop();
        }
      }

      new_methods.push_back(method);

    }
  }

  methods = new_methods;

  MethodHolder *main;

  /**
  for(int i=0;i<new_methods.size();i++){
    if(new_methods[i].label == "main"){
      main = &new_methods[i];
    }
  }
  **/

  main = &methods[findMethod("main")];
  vector<Parameter> params;

  runMethod(*main, params);
  printSNP();
}

void runMethod(MethodHolder method, vector<Parameter> params){

  vector<string> lines;
  for(int i=0;i<method.contents.size();i++){
    vector<string> split_by_semicolon = split(method.contents[i], ";");   //CONSIDER:: no semicolon in a line
    for(int j=0;j<split_by_semicolon.size();j++){
      lines.push_back(split_by_semicolon[j]);
    }
  }
  for(int i=0;i<lines.size();i++){
    cout << method.label << "::" << lines[i] << endl;

    //SPECIAL KEYWORDS DEF AND CALL
    int special_index = checkLineSpecialKeyword(lines[i]);
    if(special_index > 0){
      //cout << "Theres a keyword"<<endl;
      switch(special_index){
        case SPECIAL_DEF_INDEX:
          //cout << "DEF METHOD" << endl;
          break;
        case SPECIAL_CALL_INDEX:
          vector<Parameter> new_parameters = getCallParameters(lines[i]);
          string method_to_call = getMethodName(lines[i]);
          cout << "CALL:: " << method_to_call << endl;
          runMethod(methods[findMethod(method_to_call)], new_parameters);
          break;
        
      }
    }
    int reserve_index = checkLineReserveKeyword(lines[i]);
    if(reserve_index >= 0){
      switch(reserve_index){
        case INDEX_MU:
          eval_mu(lines[i], method, params);
          break;
        case INDEX_MS:
          eval_ms(lines[i], method, params);
          break;
        case INDEX_ARCS:
          eval_arcs(lines[i], method, params);
          break;
      }
    }
    int open_square = lines[i].find("[");
    int close_square = lines[i].find("]");
    int alpha_a = lines[i].find("a");

    if(open_square!=string::npos && close_square!=string::npos && alpha_a!=string::npos 
      && open_square<alpha_a && alpha_a<close_square){
      parseRule(lines[i], method, params);
    }
  }
}

void parseRule(string line, MethodHolder method, vector<Parameter> params){
  check("rule line:" +line);
  line = matchParameters(line, method.parameters, params);
  check("rule line:" +line);

  Rule rule;

  stack<char> parse_stack;
  for(int i=0;i<line.length();i++){
    char currchar = line.at(i);
    if(currchar == '['){
      parse_stack.push('[');
    } else if(currchar == ']'){
      parse_stack.pop();
    } else if(currchar == 'a'){

    } else if(currchar == '-'){

    } else if(currchar == ':'){
      
    } else if(currchar == '\"'){
      i++;currchar = line.at(i);
      stringstream regex;
      while(currchar!='\"'){
        regex << currchar;
        i++;currchar = line.at(i);
      }
      rule.regex = regex.str();
    }
  }
  vector<string> colon_split = split(line, ":");

  if(colon_split.size() > 1){
    TreeNode root;
    root.label = "root";
    root.value = -1;
    root.parent = NULL;

    identifyRanges(colon_split[1], root);
    cout << colon_split[0];
  

  } else {

  }
}

void eval_mu(string line, MethodHolder method, vector<Parameter> params){
  string keyword = RESERVE_KEYWORDS[checkReserveKeyword("@mu")];
  line = matchParameters(line, method.parameters, params);

  int mu_index = line.find(keyword);
  string substr = line.substr(mu_index+keyword.length(), line.length());

  vector<string> colon_split = split(substr, ":");
  //Case: Range is specified
  if(colon_split.size() > 1){
    TreeNode root;
    root.label = "root";
    root.value = -1;
    root.parent = NULL;
    
    identifyRanges(colon_split[1], root);

    //printTraverseTree(root);

    //Check Expression for = | +=
    bool is_newrons = false;
    string buffer = trim(colon_split[0]);
    int delim = buffer.find("+=");
    if(delim==string::npos){
      is_newrons = true;
      buffer = trim(buffer.substr(1, buffer.length()));
    } else {
      buffer = trim(buffer.substr(2, buffer.length()));
    }

    //Identify neurons to be created
    vector<string> comma_split = split(buffer, ",");
    vector<Neuron> temp_neurons;
    for(int i=0;i<comma_split.size();i++){
      string label;
      int openbraces = comma_split[i].find("{");
      int closebraces;
      if(openbraces!=string::npos){                           //EVALUATE MATH EXPRESSION
        label = comma_split[i].substr(0, openbraces);
        closebraces = comma_split[i].find("}");
        Neuron temp;
        temp.label = label;
        temp.param.label = comma_split[i].substr(openbraces+1, closebraces-openbraces-1);
        temp_neurons.push_back(temp);
      }                                                       //LIMITATION: Neuron without {}
    }

    vector<Neuron> new_rons;
    recursiveCreateNeurons(root, new_rons, temp_neurons);

    //Evaluation for expression: += | =
    if(is_newrons){
      snpsystem.neurons = new_rons;
    } else {
      for(int i=0;i<new_rons.size();i++){
        snpsystem.neurons.push_back(new_rons[i]);
      }
    }

  }
  //Case: No Range Specified
  else{
    if(colon_split[0].find("{")!=string::npos){
      cout << "Error: No Range given. Line:" << line << endl;
    }
    int delim =  colon_split[0].find("+=");
    if(delim != string::npos){
      string label = trim(colon_split[0].substr(delim+3, colon_split[0].length()));
      vector<string> comma_split = split(label, ",");
      for(int i=0;i<comma_split.size();i++){
        Neuron new_ron;
        new_ron.label = trim(comma_split[i]);
        new_ron.spikes = 0;
        snpsystem.neurons.push_back(new_ron);
      }
    } else {
      delim = colon_split[0].find("=");
      string label = trim(colon_split[0].substr(delim+2, colon_split[0].length()));
      vector<Neuron> new_rons;
      vector<string> comma_split = split(label, ",");
      for(int i=0;i<comma_split.size();i++){
        Neuron new_ron;
        new_ron.label = trim(comma_split[i]);
        new_ron.spikes = 0;
        new_rons.push_back(new_ron);
      }
      snpsystem.neurons = new_rons;
    }
  }
}

//Create Tree to Represent Range values
void recursiveCreateNeurons(TreeNode& node, vector<Neuron>& neurons, vector<Neuron>& temp){
  if(node.children.empty()){
    TreeNode *curr = &node;
    while(curr->parent!=NULL){
      for(int i=0;i<temp.size();i++){
        if(temp[i].param.label==curr->label){
          Neuron new_ron;
          stringstream ss;
          ss << temp[i].label << "{" << curr->value << "}";
          new_ron.label = ss.str();
          new_ron.spikes = 0;
          neurons.push_back(new_ron);
        }
      }
      curr = curr->parent;
    }
  }else{
    for(int i=0;i<node.children.size();i++){
      recursiveCreateNeurons(node.children[i], neurons, temp);
    }
  }
}

void eval_ms(string line, MethodHolder method, vector<Parameter> params){
  string keyword = RESERVE_KEYWORDS[checkReserveKeyword("@ms")];
  line = trim(line);
  if(!params.empty()){
    line = matchParameters(line, method.parameters, params);
  }
  vector<string> colon_split = split(line, ":");
  if(colon_split.size()>1){
    TreeNode root;
    root.value = -1;
    root.label = "root";

    identifyRanges(colon_split[1], root);

    recursiveCreateSpikes(root, colon_split[0]);
  } else {
    string neuron_label = line.substr(4, line.find(")")-4);
    bool set_spike = (line.find("+=") == string::npos);
    string mathexp = line.substr(line.find("a*"), line.length());
    mathexp = mathexp.substr(2, mathexp.length()-2);
    int mathexpresult = evalMathExp(mathexp);
    if(set_spike){
      setSpike(neuron_label, mathexpresult);
    } else {
      addSpike(neuron_label, mathexpresult);
    }
  }

}

void recursiveCreateSpikes(TreeNode node, string entry){

  for(int i=0;i<node.children.size();i++){
    recursiveCreateSpikes(node.children[i], entry);
  }

  TreeNode *curr = &node;

  if(node.children.empty()){
    vector<Parameter> params;
    while(curr->label != "root"){
      Parameter new_param;
      new_param.label = curr->label;
      new_param.value = curr->value;
      params.push_back(new_param);
      curr = curr->parent;
    }
    string to_eval = matchParameters(entry, params);
    string neuron_label = to_eval.substr(4, to_eval.find(")")-4);
    bool set_spike = (to_eval.find("+=") == string::npos);
    string mathexp = to_eval.substr(to_eval.find("a*("), to_eval.length());
    mathexp = mathexp.substr(2, mathexp.length()-2);
    int mathexpresult = evalMathExp(mathexp);
    if(set_spike){
      setSpike(neuron_label, mathexpresult);
    } else {
      addSpike(neuron_label, mathexpresult);
    }
  }
}

void setSpike(string neuron_label, int spikes){
  for(int i=0;i<snpsystem.neurons.size();i++){
    if(snpsystem.neurons[i].label == neuron_label){
      snpsystem.neurons[i].spikes = spikes;
    }
  }
}

void addSpike(string neuron_label, int spikes){
  for(int i=0;i<snpsystem.neurons.size();i++){
    if(snpsystem.neurons[i].label == neuron_label){
      snpsystem.neurons[i].spikes += spikes;
    }
  }
}

void eval_arcs(string line, MethodHolder method, vector<Parameter> params){
  line = matchParameters(line, method.parameters, params);
  vector<string> colon_split = split(line, ":");

  if(colon_split.size()>1){
    TreeNode root;
    root.label = "root";
    root.value = -1;
    identifyRanges(colon_split[1], root);

    bool set_synapses = (colon_split[0].find("+=")==string::npos);
    recursiveCreateSynapses(root, colon_split[0]);
  } else {
    addSynapse(colon_split[0]);
  }
}

void recursiveCreateSynapses(TreeNode node, string entry){
  for(int i=0;i<node.children.size();i++){
    recursiveCreateSynapses(node.children[i], entry);
  }

  TreeNode *curr = &node;

  if(node.children.empty()){
    vector<Parameter> params;
    while(curr->label != "root"){
      Parameter new_param;
      new_param.label = curr->label;
      new_param.value = curr->value;
      params.push_back(new_param);
      curr = curr->parent;
    }
    string to_eval = matchParameters(entry, params);
    addSynapse(to_eval);
  }
}

void addSynapse(string entry){
  string buffer = entry;
  int open_index = buffer.find("(");
  while(open_index != string::npos){
    buffer = buffer.substr(open_index+1, buffer.length());
    int comma_index = buffer.find(",");
    int close_index = buffer.find(")");
    string neuron_1 = trim(buffer.substr(0, comma_index));
    string neuron_2 = trim(buffer.substr(comma_index+1, close_index - comma_index - 1));
    open_index = buffer.find("(");
    Synapse syn;
    syn.from = neuron_1;
    syn.to = neuron_2;
    snpsystem.synapses.push_back(syn);
  }
}


//Finds the index of the method in MethodHolder list given a string query
//returns -1 if method does not exist
int findMethod(string query){
  for(int i=0;i<methods.size();i++){
    if(methods[i].label == query){
      return i;
    }
  }
  return -1;
}

int evalMathExp(string mathexp){
  vector<string> postfix_notation;
  stack<char> opstack;

  for(int i=0;i<mathexp.length();i++){
    char currchar = mathexp.at(i);
    if(currchar=='+'){
      if(!opstack.empty()){
        char stack_top = opstack.top();
        while(stack_top == '+' || stack_top == '-' || stack_top=='/' ||
             stack_top =='*' || stack_top=='^'){
          string opp(1, stack_top);
          postfix_notation.push_back(opp);
          opstack.pop();
          stack_top = opstack.top();
        }
      }
      opstack.push('+');
    } else if(currchar=='-'){
      if(!opstack.empty()){
        char stack_top = opstack.top();
        while(stack_top == '-' || stack_top=='/' ||
             stack_top =='*' || stack_top=='^'){
          string opp(1, stack_top);
          postfix_notation.push_back(opp);
          opstack.pop();
          stack_top = opstack.top();
        }
      }
      opstack.push('-');
    } else if(currchar=='/'){
      if(!opstack.empty()){
        char stack_top = opstack.top();
        while(stack_top=='/' || stack_top =='*' || stack_top=='^'){
          string opp(1, stack_top);
          postfix_notation.push_back(opp);
          opstack.pop();
          stack_top = opstack.top();
        }
      }
      opstack.push('/');
    } else if(currchar=='*'){
      if(!opstack.empty()){
        char stack_top = opstack.top();
        while(stack_top =='*' || stack_top=='^'){
          string opp(1, stack_top);
          postfix_notation.push_back(opp);
          opstack.pop();
          stack_top = opstack.top();
        }
      }
      opstack.push('*');
    } else if(currchar=='^'){
      if(!opstack.empty()){
        char stack_top = opstack.top();
        while(stack_top=='^'){
          string opp(1, stack_top);
          postfix_notation.push_back(opp);
          opstack.pop();
          stack_top = opstack.top();
        }
      }
      opstack.push('^');
    } else if(currchar=='('){
      opstack.push('(');
    } else if(currchar==')'){
      while(opstack.top()!='('){
        string opp(1, opstack.top());
        postfix_notation.push_back(opp);
        opstack.pop();
      }
      opstack.pop();
    } else if(currchar==' '){
      //donothing
    } else {
      stringstream operand_buffer;
      operand_buffer << currchar;
      while( (i+1) < mathexp.length() && mathexp.at(i+1) != '+' && mathexp.at(i+1) != '-' &&
            mathexp.at(i+1) != '*' && mathexp.at(i+1) != '/' &&
            mathexp.at(i+1) != '^' && mathexp.at(i+1) != '(' &&
            mathexp.at(i+1) != ')' && mathexp.at(i+1) != ' '){
        i++;
        currchar = mathexp.at(i+1);
        operand_buffer << currchar;
      }
      postfix_notation.push_back(operand_buffer.str());
    }
  }
  
  while(!opstack.empty()){
    string opp(1, opstack.top());
    postfix_notation.push_back(opp);
    opstack.pop();
  }
  
  stack<int> evalstack;
  for(int i=0;i<postfix_notation.size();i++){
    if(postfix_notation[i]=="+"){
      int op_a = evalstack.top(); evalstack.pop();
      int op_b = evalstack.top(); evalstack.pop();
      evalstack.push(op_a+op_b);
    } else if(postfix_notation[i]=="-"){
      int op_a = evalstack.top(); evalstack.pop();
      int op_b = evalstack.top(); evalstack.pop();
      evalstack.push(op_b-op_a);
    } else if(postfix_notation[i]=="*"){
      int op_a = evalstack.top(); evalstack.pop();
      int op_b = evalstack.top(); evalstack.pop();
      evalstack.push(op_a*op_b);
    } else if(postfix_notation[i]=="/"){
      int op_a = evalstack.top(); evalstack.pop();
      int op_b = evalstack.top(); evalstack.pop();
      evalstack.push(op_b/op_a);
    } else if(postfix_notation[i]=="^"){
      int op_a = evalstack.top(); evalstack.pop();
      int op_b = evalstack.top(); evalstack.pop();
      evalstack.push(pow(op_b, op_a));
    } else {
      evalstack.push(stoi(postfix_notation[i]));
    }
  }
  
  return evalstack.top();
}

string subsMathExp(string line, vector<Parameter> params){
  string buffer = ":";
  buffer.append(line);
  buffer = matchParameters(buffer, params);
  return buffer.substr(1, buffer.length());
}

//Given a line in pli file, matches parameters for modular style programming
string matchParameters(string line, vector<Parameter> params){
  vector<Parameter> param_needed;
  vector<Parameter> param_provided;
  for(int i=0;i<params.size();i++){
    Parameter p_need;
    Parameter p_provide;
    p_need.label = params[i].label;
    p_provide.value = params[i].value;
    param_needed.push_back(p_need);
    param_provided.push_back(p_provide);
  }
  return matchParameters(line, param_needed, param_provided);
}

//Overloaded method
string matchParameters(string line, vector<Parameter> param_needed, vector<Parameter> param_provided){
  if(param_needed.size()!=param_provided.size()){
    cout << "Parameters do not match: " << line << endl;
    return "";
  }
  
  stringstream sstream;
  bool after_colon = false;
  stack<char> expression_stack;
  stringstream buffer;
  int colon_index = line.find(':');
  
  for(int iter=0;iter<line.length();iter++){
    switch(line.at(iter)){
      case ':':
        expression_stack.push(':');
        sstream << ':';
        break;
      case '{':
        expression_stack.push('{');
        sstream << '{';
        break;
      case ')':
        if(!expression_stack.empty() && expression_stack.top() == 'a'){
          expression_stack.pop();
          goto marker;
        } else {
          sstream << ')';
          break;
        }
      case '}':
        expression_stack.pop();
        goto marker;
      case '*':
        if(line.at(iter) == '*' && line.at(iter+1) == '(' && line.at(iter-1) == 'a'){
          expression_stack.push('a');
          sstream << "*(";
          iter++;
          break;
        }
      case '+': case '-': case '/': case '>': 
      case '<': case '=': case ' ': case ',':
        marker:
        if(buffer.rdbuf()->in_avail()>0){
          bool param_match = false;
          for(int params_iter=0;params_iter<param_needed.size();params_iter++){
            if(buffer.str()==param_needed[params_iter].label){
              sstream << to_string(param_provided[params_iter].value);
              param_match = true;
            }
          }
          if(!param_match){
            sstream << buffer.str();
          }
          buffer.str("");
          buffer.clear();
        }
        sstream << line.at(iter);
        break;
      default:
        if(expression_stack.empty()){
          sstream << line.at(iter);
        } else{
          buffer << line.at(iter);
        }
        break;
    }
  }
  if(buffer.rdbuf()->in_avail()>0){
    bool param_match = false;
    for(int params_iter=0;params_iter<param_needed.size();params_iter++){
      if(buffer.str()==param_needed[params_iter].label){
        sstream << to_string(param_provided[params_iter].value);
        param_match = true;
      }
    }
    if(!param_match){
      sstream << buffer.str();
    }
    buffer.str("");
    buffer.clear();
  }

  return(sstream.str());
}

//Identify range of variables given
void identifyRanges(string entry, TreeNode& root){
  const int lesseq = 1, eqless = 2, less = 3;
  vector<string> comma_split = split(entry, ",");
  vector<Range> ranges;
  vector<Range> exceptions;
  for(int i=0;i<comma_split.size();i++){
    Range range;
    int delim1 = -1;
    int delim1_mode = -1; 
    //CASE <=
    if(comma_split[i].find("<=")!=string::npos){
      delim1 = comma_split[i].find("<=");
      delim1_mode = lesseq;
    }
    //CASE =<
    else if(comma_split[i].find("=<")!=string::npos){
      delim1 = comma_split[i].find("=<");
      delim1_mode = eqless;
    }
    //CASE <>
    else if(comma_split[i].find("<>")){
      delim1 = comma_split[i].find("<>");
      range.x1 = trim(comma_split[i].substr(0, delim1));
      range.x2 = trim(comma_split[i].substr(delim1+2, comma_split[i].length()));
      exceptions.push_back(range);
      continue;
    }
    //CASE <
    else if(comma_split[i].find("<")!= string::npos){
      delim1 = comma_split[i].find("<");
      delim1_mode = less;
    }
    string substr = comma_split[i].substr(0, delim1);
    range.x1 = trim(substr);
    if(delim1_mode == less){
      substr = comma_split[i].substr(delim1+1, comma_split[i].length());
      range.inclusive_x1 = false;
    }
    else if(delim1_mode == lesseq || delim1_mode == eqless){
      substr = comma_split[i].substr(delim1+2, comma_split[i].length());
      range.inclusive_x1 = true;
    }

    if(substr.find("<=")!=string::npos){
      delim1 = substr.find("<=");
      delim1_mode = lesseq;

    }
    else if(substr.find("=<")!=string::npos){
      delim1 = substr.find("=<");
      delim1_mode = eqless;
    }
    else if(substr.find("<")!= string::npos){
      delim1 = substr.find("<");
      delim1_mode = less;
    }
    
    range.label = trim(substr.substr(0, delim1));

    if(delim1_mode == less){
      substr = substr.substr(delim1+1, substr.length());
      range.inclusive_x2 = false;
    }
    else if(delim1_mode == lesseq || delim1_mode == eqless){
      substr = substr.substr(delim1+2, substr.length());
      range.inclusive_x2 = true;
    }

    range.x2 = trim(substr);
    ranges.push_back(range);
  }

  stack<int> range_stack;
  vector<Range> dummy;
  for(int i=ranges.size()-1;i>=0;i--){
    if(i==0){
      recursiveBranching(root, ranges[i], exceptions);
    } else {
      recursiveBranching(root, ranges[i], dummy);
    }
  }
}

//Recursive branching out of Range tree
void recursiveBranching(TreeNode& node, Range range, vector<Range> exceptions){
  if(node.children.empty()){
    TreeNode new_node;
    int x1;
    int x2;
    
    TreeNode *curr = &node; 
    vector<Parameter> params;
    while(curr->label!="root"){
      Parameter param;
      param.label = curr->label;
      param.value = curr->value;
      params.push_back(param);
      curr = curr->parent;
    }

    //Left hand side is a constant && Right hand side is a constant
    if(is_number(range.x1) && is_number(range.x2)){
      x1 = stoi(range.x1);
      x2 = stoi(range.x2);
    } 
    //Left hand side is a constant && Right hand side is not
    else if(is_number(range.x1) && !is_number(range.x2)){
      x1 = stoi(range.x1);
      string match = subsMathExp(match, params);
      x2 = evalMathExp(match);
    }
    //Left hand side is not a constant && Right hand side is a constant
    else if(!is_number(range.x1) && is_number(range.x2)){
      x2 = stoi(range.x2);
      string match = subsMathExp(match, params);
      x1 = evalMathExp(match);
    }
    else{
      string match_x1 = subsMathExp(match_x1, params);
      string match_x2 = subsMathExp(match_x2, params);
      x1 = evalMathExp(match_x1);
      x2 = evalMathExp(match_x2);
    }
    if(!range.inclusive_x1) x1++;
    if(range.inclusive_x2) x2++;
    for(x1;x1<x2;x1++){
      bool will_add = true;;
      vector<Parameter> prms = params;
      Parameter n_prm;
      n_prm.label = range.label;
      n_prm.value = x1;
      prms.push_back(n_prm);
      for(int i=0;i<exceptions.size();i++){
        string ex_1 = subsMathExp(exceptions[i].x1, prms);
        string ex_2 = subsMathExp(exceptions[i].x2, prms);
        int ex_1val = evalMathExp(ex_1);
        int ex_2val = evalMathExp(ex_2);
        if(ex_1val == ex_2val) will_add = false;
      }

      if(will_add){
        TreeNode new_node;
        new_node.label = range.label;
        new_node.value = x1;
        new_node.parent = &node;
        node.children.push_back(new_node);
      }
    }
  }
  else{
    for(int i=0;i<node.children.size();i++){
      recursiveBranching(node.children[i], range, exceptions);
    }
  }
}

//Splits string using whitespace as a delimiter
vector<string> whitespace_split(string tosplit){
  istringstream iss(tosplit);
  vector<string> tokens;
  copy(istream_iterator<string>(iss),
      istream_iterator<string>(),
      back_inserter(tokens));
  return tokens;
}

//Splits string given a delimiter as a parameter
vector<string> split(string tosplit, string delimiter){
  int string_len = tosplit.length();
  vector<string> retval;

  int curr = 0;
  int nextToken = 0;
  while((nextToken = tosplit.find(delimiter)) != string::npos){
    if(nextToken > 0){
      retval.push_back(trim(tosplit.substr(0, nextToken)));
    }
    tosplit.erase(0, nextToken+1);
  }
  if(tosplit.length()>0){
    retval.push_back(trim(tosplit));
  }
  return retval;
}

//Removes leading and trailing whitespaces
string trim(string entry){
  string whitespace = " \t";
  if(entry.length()<=0){
    return "";
  }
  int beginn = entry.find_first_not_of(whitespace);
  int end = entry.find_last_not_of(whitespace);
  int range = end - beginn + 1;
  return entry.substr(beginn, range);
}

//Simply get method label (main, init_snp, etc..)
string getMethodName(string buffer){
  if(buffer.find("def")!=string::npos&&buffer.find("call")!=string::npos){
    return "";
  }
  vector<string> w_split = whitespace_split(buffer);
  int sub_end = w_split[1].find("(");
  return w_split[1].substr(0, sub_end);
}

//Get Method Parameters needed from def
vector<Parameter> getDefParameters(string buffer){
  vector<Parameter> new_param;
  if(buffer.find("def")==string::npos){
    return new_param;
  }
  vector<string> w_split = whitespace_split(buffer);
  int params_start = w_split[1].find("(");
  string substr = w_split[1].substr(params_start, w_split[1].length()-1);
  substr.erase(remove(substr.begin(), substr.end(), '('), substr.end());
  substr.erase(remove(substr.begin(), substr.end(), ')'), substr.end());
  substr.erase(remove(substr.begin(), substr.end(), '{'), substr.end());
  vector<string> splitparameters = split(substr, ",");
  for(int i=0;i<splitparameters.size();i++){
    Parameter param;
    param.label = splitparameters[i];
    param.value = -1;
    new_param.push_back(param);
  }
  return new_param;
}

//Get Method Parameters provided from call
vector<Parameter> getCallParameters(string buffer){
  vector<Parameter> new_param;
  if(buffer.find("call")==string::npos){
    return new_param;
  }
  vector<string> w_split = whitespace_split(buffer);
  int params_start = w_split[1].find("(");
  string substr = w_split[1].substr(params_start, w_split[1].length()-1);
  substr.erase(remove(substr.begin(), substr.end(), '('), substr.end());
  substr.erase(remove(substr.begin(), substr.end(), ')'), substr.end());
  substr.erase(remove(substr.begin(), substr.end(), '{'), substr.end());
  vector<string> splitparameters = split(substr, ",");
  for(int i=0;i<splitparameters.size();i++){
    Parameter param;
    param.label = "";
    param.value = stoi(splitparameters[i]);
    new_param.push_back(param);
  }
  return new_param;
}
//Given a line, check if it possibly contains a special keyword
//returns the index of the keyword if there is, else return -1
int checkLineSpecialKeyword(string query){
  vector<string> split = whitespace_split(query);
  for(int i=0;i<split.size();i++){
    int check = checkSpecialKeyword(split[i]);
    if(check>=0){
      return check;
    }
  }
  return -1;
}

//Given a line, check if it possibly contains a reserve keyword
//returns the index of the keyword if there is, else return -1
int checkLineReserveKeyword(string query){
  for(int i=0;i<RESERVE_KEYWORD_COUNT;i++){
    int find = query.find(RESERVE_KEYWORDS[i]);
    if(find!=string::npos){
      return i;
    }
  }
  return -1;
}

//Check if given word is in reserve keywords
//Returns -1 if no match, else returns the index of keyword match
int checkReserveKeyword(string query){
  int retval=0;
  for(int i=0;i<RESERVE_KEYWORD_COUNT;i++){
    if(query == RESERVE_KEYWORDS[i]){
      return i;
    }
  }
  return -1;
}

//Check if given word is in special keywords
//Returns -1 if no match, else returns the index of keyword match
int checkSpecialKeyword(string query){
  int retval=0;
  for(int i=0;i<SPECIAL_KEYWORD_COUNT;i++){
    if(query == SPECIAL_KEYWORDS[i]){
      return i;
    }
  }
  return retval;
}

//Find a char sequence in a string from a given index up until the end of string
int findFromIndex(string source, string tofind, int index){
  string buffer = source.substr(index, source.length());
  int find_val = buffer.find(tofind);
  if(find_val == string::npos){
    return -1;
  }
  else{
    return index + find_val;
  }
}

//Check if given string is a number
bool is_number(string s){
  bool is_number = true;
  for(int i=0;i<s.length();i++){
    is_number = is_number && isdigit(s.at(i));
  }
  return is_number;
  /**
    string::const_iterator it = s.begin();
    while (it != s.end() && isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
  **/
}
//Print MethodHolder. simple. duh
void printMethodHolder(MethodHolder method){
  cout << method.label << endl;
  for(int i=0;i<method.contents.size();i++){
    cout << method.contents[i] << endl;
  }
}

//Print Parameter. duh
void printParameter(Parameter param){
  cout << param.label << ":" << param.value << endl;
}

//Print SNP. duh
void printSNP(){
  cout << "Neurons:" << endl;
  for(int i=0;i<snpsystem.neurons.size();i++){
    cout << "\t" << snpsystem.neurons[i].label << "::" << snpsystem.neurons[i].spikes <<  endl;
  }
  cout << "Synapses:" << endl;
  for(int i=0;i<snpsystem.synapses.size();i++){
    cout << "\t" << snpsystem.synapses[i].from << ">>" << snpsystem.synapses[i].to << endl;
  }
}

void printRange(Range r){
  cout << r.x1;
  if(r.inclusive_x1){
    cout << ":<=:";
  }else{
    cout << ":<:";
  }
  cout << r.label;
  if(r.inclusive_x2){
    cout << ":<=:";
  }else{
    cout << ":<:";
  }
  cout << r.x2 << endl;
}

void printTraverseTree(TreeNode root){
  cout << "Tree:" << root.label << ":" << root.value << endl;
  for(int i=0;i<root.children.size();i++){
    printTraverseTree(root.children[i]);
  }
}

void check(string r){
  cout << "=================<" << r << ">" << endl;
}
void check(){
  cout << "=================" << endl;
}