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
void eval_mu(string line);
void recursiveCreateNeurons(TreeNode& root, vector<Neuron>& neurons, vector<Neuron>& temp);
void eval_ms(string line);
void eval_arcs(string line);
int findMethod(string query);
void identifyRanges(string entry, TreeNode& root);
void recursiveBranching(TreeNode& node, Range range);
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

void parseFile(char *filename){
  
  //Open and verify file integrity
  ifstream file (filename);
  if(!file.is_open()){
    cout << "File \"" << filename << "\" does not exist\n";
    return;
  }

  string buffer;
  getline(file, buffer);

  //Check header
  if(buffer != HEADER){
    cout << "SNP pli file must have \"" << HEADER << "\" as file header\n";
  }

  vector<MethodHolder> new_methods;

  while(getline(file, buffer)){
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
          eval_mu(lines[i]);
          break;
      }
    }


  }
}

void eval_mu(string line){
  string keyword = RESERVE_KEYWORDS[checkReserveKeyword("@mu")];
  int mu_index = line.find(keyword);
  string substr = line.substr(mu_index+keyword.length(), line.length());

  vector<string> colon_split = split(substr, ":");
  if(colon_split.size() > 1){
    TreeNode root;
    root.label = "root";
    root.value = -1;
    root.parent = NULL;
    
    identifyRanges(colon_split[1], root);

    //printTraverseTree(root);
    bool is_newrons = false;
    string buffer = trim(colon_split[0]);
    int delim = buffer.find("+=");
    if(delim==string::npos){
      is_newrons = true;
      buffer = trim(buffer.substr(1, buffer.length()));
    } else {
      buffer = trim(buffer.substr(2, buffer.length()));
    }

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
    if(is_newrons){
      snpsystem.neurons = new_rons;
    } else {
      for(int i=0;i<new_rons.size();i++){
        snpsystem.neurons.push_back(new_rons[i]);
      }
    }

  }else{
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
        new_rons.push_back(new_ron);
      }
      snpsystem.neurons = new_rons;
    }
  }
  printSNP();
}

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

void eval_ms(string line){
  string keyword = RESERVE_KEYWORDS[checkReserveKeyword("@ms")];
}

void eval_arcs(string line){

}


//Finds the index of the method given a string query
//returns -1 if method does not exist
int findMethod(string query){
  for(int i=0;i<methods.size();i++){
    if(methods[i].label == query){
      return i;
    }
  }
  return -1;
}

void identifyRanges(string entry, TreeNode& root){
  const int lesseq = 1, eqless = 2, less = 3;
  vector<string> comma_split = split(entry, ",");
  vector<Range> ranges;
  for(int i=0;i<comma_split.size();i++){
    Range range;
    int delim1 = -1;
    int delim1_mode = -1; 
    if(comma_split[i].find("<=")!=string::npos){
      delim1 = comma_split[i].find("<=");
      delim1_mode = lesseq;

    }
    else if(comma_split[i].find("=<")!=string::npos){
      delim1 = comma_split[i].find("=<");
      delim1_mode = eqless;
    }
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
  
  for(int i=ranges.size()-1;i>=0;i--){
    printRange(ranges[i]);
    recursiveBranching(root, ranges[i]);
  }
}

void recursiveBranching(TreeNode& node, Range range){
  if(node.children.empty()){
    TreeNode new_node;
    if(is_number(range.x1) && is_number(range.x2)){
      int x1 = stoi(range.x1);
      int x2 = stoi(range.x2);
      if(!range.inclusive_x1) x1++;
      if(range.inclusive_x2) x2++;
      for(x1;x1<x2;x1++){
        TreeNode new_node;
        new_node.label = range.label;
        new_node.value = x1;
        new_node.parent = &node;
        node.children.push_back(new_node);
      }
    } 
    else if(is_number(range.x1) && !is_number(range.x2)){
      int x1 = stoi(range.x1);
      int x2;
      bool halt = false;
      TreeNode *curr = &node;
      while(!halt){
        if(curr->label == range.x2){
          x2 = curr->value;
          halt = true;
        } else {
          curr = curr->parent;
        }
      }
      if(!range.inclusive_x1) x1++;
      if(range.inclusive_x2) x2++;
      for(x1;x1<x2;x1++){
        TreeNode new_node;
        new_node.label = range.label;
        new_node.value = x1;
        new_node.parent = &node;
        node.children.push_back(new_node);
      }
    }
    else if(!is_number(range.x1) && is_number(range.x2)){
      int x2 = stoi(range.x2);
      int x1;
      bool halt = false;
      TreeNode *curr = &node;
      while(!halt){
        if(curr->label == range.x1){
          x1 = curr->value;
          halt = true;
        } else {
          curr = curr->parent;
        }
      }
      if(!range.inclusive_x1) x1++;
      if(range.inclusive_x2) x2++;
      for(x1;x1<x2;x1++){
        TreeNode new_node;
        new_node.label = range.label;
        new_node.value = x1;
        new_node.parent = &node;
        node.children.push_back(new_node);
      }
    }
    else{
      int x1;
      int x2;
      bool halt = false;
      TreeNode *curr = &node;
      while(!halt){
        if(curr->label == range.x1){
          x1 = curr->value;
          halt = true;
        } else {
          curr = curr->parent;
        }
      }
      halt = false;
      curr = &node;
      while(!halt){
        if(curr->label == range.x2){
          x2 = curr->value;
          halt = true;
        } else {
          curr = curr->parent;
        }
      }
      if(!range.inclusive_x1) x1++;
      if(range.inclusive_x2) x2++;
      for(x1;x1<x2;x1++){
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
      recursiveBranching(node.children[i], range);
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
    cout << "\t" << snpsystem.neurons[i].label <<  endl;
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
  cout << "=================" << r << ":" << endl;
}
void check(){
  cout << "=================" << endl;
}