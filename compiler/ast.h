#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <iostream>
#include <fstream>
#include <regex>

using namespace std;

/*===========================================*/
//ListExpr: The definition of a list
class ListExpr {

private:
  vector<int> items;    //content of the list

public:
  ListExpr (vector<int> theItems) {
    this->items = theItems;
  }

  // getter
  vector<int> get_items() { return this->items; }

  //printer
  string stringify() {
    string s = "List: ";
    for (int i = 0; i < this->items.size(); i ++) {
      s += " " + to_string(i);
    }
    return s;
  }
};

/*===========================================*/
//ListPred: A test on list membership
class ListPred {

private:
  string listvar;
  string listop; // in or notin?
  string listbodyvar;
  //ListExpr *listbody;

public:
  ListPred(string thelistvar, string thelistop, string thelistbodyvar) {
    this->listvar = thelistvar;
    this->listop = thelistop;
    this->listbodyvar = thelistbodyvar;
  }

  // print
  string stringify() {
    return "var=" + listvar + ", op=" + listop + ", list=" + listbodyvar;
  }

  // getters
  string get_listvar() { return this->listvar; }
  string get_listop() { return this->listop; }
  string get_listbodyvar() { return this->listbodyvar; }
};

//PredExpr: a predicate that evaluates to true or false.
class BinaryPred
{
private:
  string lhs;
  string rhs;
  string op;

public:
  BinaryPred (string thelhs, string theop, string therhs) {
    this->lhs = thelhs;
    this->op  = theop;
    this->rhs = therhs;
  }

  BinaryPred (string thelhs, string theop, int therhs) {
    this->lhs = thelhs;
    this->op  = theop;
    this->rhs = to_string(therhs);
  }

  // printer
  string stringify() {
    string res = this->lhs + " " + this->op  + " " + this->rhs;
    return res;
  }

  // printer
  string p4stringify() {
    regex reg("h([0-9]+)");
    string replacement = string("hdr.") + string("h") + "$1" + string(".val");
    string s = this->stringify();
    string res = regex_replace(s, reg, replacement);
    cout << "Replacing  " << s << " with " << res << endl;
    return res;
  }

  // getter
  string get_lhs () { return this->lhs; }
  string get_op () { return this->op; }
  string get_rhs() { return this->rhs; }
  bool is_exact_match() { return this->op == "==";}
};

class ArithExpr
{
private:
    //ArithExpr * Operator string
    ArithExpr *lhs;
    string lhs_str;
    string op;
    string rhs;
    string expr;

public:
    ArithExpr (string theexpr) {
      this->expr = theexpr;
    }

    ArithExpr (ArithExpr *thelhs, string theop, string therhs) {
      this->lhs = thelhs;
      this->op = theop;
      this->rhs = therhs;
    }

    ArithExpr (string thelhs, string theop, string therhs) {
      this->lhs_str = thelhs;
      this->op = theop;
      this->rhs = therhs;
    }

    // getters
    string get_expr () { return this->expr; }

    // print
    string stringify() {return lhs_str + " " + op + " " + rhs; }
};

#define MATCH_PRED 0
#define MATCH_LIST 1
#define MATCH_MONITOR 2

class MatchExpr
{
private:
  int type;

  // the predicate that we match on:
  ListPred *matchlist;
  BinaryPred *matchpred = NULL;
  // this is for monitors
  string matchmvar;

  // actions for the if-then-else branches
  string if_action;
  string else_action;
  int num_actions = 0;

public:

  // setters
  void set_if_action(string a) {
    assert(this->if_action.length() == 0);
    this->if_action = a;
    this->num_actions ++;
    cout << "MatchExpr has the if-action as: " + this->if_action << endl;
    cout << "action num is " << this->num_actions << endl;
  }

  void set_else_action(string a) {
    assert(this->else_action.length() == 0);
    this->else_action = a;
    this->num_actions ++;
    cout << "MatchExpr has the else-action as: " + this->else_action << endl;
    cout << "action num is " << this->num_actions << endl;
  }

  // getters
  int get_type () { return this->type; }

  int get_num_actions() { return this->num_actions; }

  bool get_ifthenelse () {
                         if (this->num_actions == 2)
                           return true;
                         else
                           return false;
                         }

  string get_if_action() {
                         assert(this->if_action.size() > 0);
                         return this->if_action;
                       }


  string get_else_action() {
                           assert(this->else_action.size() > 0);
                           return this->else_action;
                         }

  ListPred * get_listpred () {
    assert(type == MATCH_LIST);
    return this->matchlist;
  }

  BinaryPred *get_binarypred () {
    // assert(type == MATCH_PRED);
    return this->matchpred;
  }

  void set_binarypred (BinaryPred *bp) {
     this->matchpred = bp;
  }

  string get_mvar() {
    assert(type == MATCH_MONITOR);
    return this->matchmvar;
  }

  MatchExpr (ListPred *thematchlist) {
    assert(thematchlist);
    this->type = MATCH_LIST;
    this->matchlist = thematchlist;
  }

  MatchExpr (BinaryPred *thematchpred) {
    assert(thematchpred);
    this->type = MATCH_PRED;
    this->matchpred = thematchpred;
  }

  MatchExpr (string monitor) {
    this->type = MATCH_MONITOR;
    this->matchmvar = monitor;
  }

  string stringify() {
    string s;
    // the type
    if (type == MATCH_PRED) {
      s += "type: if-else match: ";
      // print BinaryPred
      s += matchpred->stringify();
    } else if (type == MATCH_LIST) {
      s += "type: list-membership match";
      // print ListPred
      s += matchlist->stringify();
    }
    return s;
  }
};


class Monitor
{
private:
  //the monitor variable
  string mvar;
  //the list expression; mvar counts how many packets for which "lp" holds true.
  ListPred *lp;

public:
  Monitor (string themvar, ListPred *thelp) {
    assert(thelp);
    cout << "Creating a monitor with variable " << mvar
         << " and the list expression is " << thelp->stringify() << endl;
    this->mvar = themvar;
    this->lp   = thelp;
  }

  string stringify() {
    string s = "monitor var=" + this->mvar + ", list: " + this->lp->stringify();
    return s;
  }

  ListPred * get_listpred() {
    assert(this->lp);
    return this->lp;
  }

  string get_mvar() {
    return this->mvar;
  }
};

#define CHECK_TYPE_INVALID   0
#define CHECK_TYPE_EXACT     1
#define CHECK_TYPE_RANGE     2
#define CHECK_TYPE_LIST      3
#define CHECK_TYPE_MONITOR   4

#define NUM_STAGE_EXACT 1
#define NUM_STAGE_RANGE 1
#define NUM_STAGE_LIST 1
#define NUM_STAGE_MONITOR 3

class Check
{
public:
   // resource usage
   int start_stage = 0;
   int stage_num = 0;
   int rec = 0;
   // table metadata
   int type = 0;
   int id = 0;
   int if_action_val = 0;
   int else_action_val = 0;
   int priority = 0;
   int table_size = 2;
   int field_len = 16;
   string header;
   // for range/exact match table
   string pred_hi;
   string pred_lo;
   // for list match table
   vector<int> list;

   // for monitors
   string mon_name;
   int extra_check = 0;
   vector<string> extra_header;
   map<string, string> h_to_val;
   int timeout_val = 100000;

   Check () {}
   string table_name;

   void print() {
      cout << "check no." << id << ", type=" << type << ", header=" << header
           << ", if-action-val=" << if_action_val
           << ", else-action-val=" << else_action_val
           << ", start_stage=" << start_stage << ", stage_num=" << stage_num
           << ", rec=" << rec << ", hi=" << pred_hi << ", lo=" << pred_lo
           << ", prio=" << priority
           << ", extra header num=" << extra_header.size()
           << endl;
   }

   bool operator < (const Check &c) const {
      return (priority < c.priority);
   }
};

class Program
{
private:
  vector<ListExpr*>   listexpr_v;  // def list
  vector<string>      listvar_v;   // names of lists
  vector<MatchExpr*>  matchexpr_v; // if match
  vector<Monitor*>    monitor_v;   // monitors
  vector<string>      header_v;    //all seen headers
  vector<Check>       check_v;     // all sequential checks

  map<string, ListExpr*> name2list;
  map<string, Monitor*> name2mon;

  // all tables that need to apply
  vector<string> tables_to_apply;
  void process_matchexprs();
  int process_monitors(string);
  int parse_match_mon(MatchExpr *, Check *);
  int parse_match_list(MatchExpr *, Check *);
  int parse_match_pred(MatchExpr *, Check *);

  // resource usage information
  const int max_rec_times = 8;
  const int stages_per_rec = 7;
  int curr_rec   = 0;    // range: 1 ~ max_rec_times
  int curr_stage = 0;    // range: 1 ~ checks_per_rec
  int num_stages = 0;    // range: 0 ~ max_rec_times * stages_per_rec

public:
  Program (vector<ListExpr*> the_listexpr_v, vector<MatchExpr*> the_matchexpr_v) {
    this->listexpr_v = the_listexpr_v;
    this->matchexpr_v = the_matchexpr_v;
  }

  Program (vector<MatchExpr*> the_matchexpr_v) {
    this->matchexpr_v = the_matchexpr_v;
  }

  Program () {}

  void add_check (Check check) {
     check_v.push_back(check);
  }

  // add helper functions
  int add_listexpr (string thelistvar, ListExpr *thele) {
    assert(thele);
    this->listvar_v.push_back(thelistvar);
    this->listexpr_v.push_back(thele);
    this->name2list[thelistvar] = thele;

    return this->listexpr_v.size();
  }

  ListExpr *get_list_by_name(string thelistvar)  {
    return this->name2list[thelistvar];
  }

  Monitor *get_mon_by_name(string mvar)  {
    return this->name2mon[mvar];
  }

  MatchExpr *get_latestmatchexpr () {
    assert(this->matchexpr_v.size() > 0);
    return this->matchexpr_v[ this->matchexpr_v.size()-1 ];
  }

  int add_matchexpr (MatchExpr *theme) {
    assert(theme);
    this->matchexpr_v.push_back(theme);
    return this->matchexpr_v.size();
  }

  int add_monitor (Monitor *them) {
    cout << "Adding a monitor to the program " + them->stringify() << endl;
    assert(them);
    string var = them->get_mvar();
    this->monitor_v.push_back(them);
    name2mon[var] = them;
    return this->monitor_v.size();
  }

  int add_header (string h) {
    this->header_v.push_back(h);
  }

  // counter
  int count_listexpr() { return this->listexpr_v.size(); }
  int count_matchexpr() { return this->matchexpr_v.size(); }

  int count_headers()
  {
    sort(header_v.begin(), header_v.end());
    header_v.erase(unique(header_v.begin(), header_v.end()), header_v.end());
    return header_v.size();
  }

  int count_monitors()
  {
    return this->monitor_v.size();
  }

  // print program
  void print() {
    for (int i = 0; i < listexpr_v.size(); i ++) {
      printf("== List %d ===\n", i);
      ListExpr *le = listexpr_v[i];
      vector<int> items = le->get_items();
      for (int j = 0; j < items.size(); j ++)
        printf("%d ", items[j]);
      printf("\n");
    }

    for (int i = 0; i < matchexpr_v.size(); i ++) {
      printf("== Match %d ===\n", i);
      MatchExpr *me = matchexpr_v[i];
      string mestr = me->stringify();
      cout << mestr << endl;
    }

    for (int i = 0; i < monitor_v.size(); i ++) {
      printf("== Monitor %d ===\n", i);
      Monitor *m = monitor_v[i];
      string mstr = m->stringify();
      cout << mstr << endl;
    }
  }

  int compile_tofino(string, string);
  int generate_p4(string);
  int generate_cmd(string);
  int allocate_resources();
  string generate_header_def();
  string generate_table_def();
  string generate_eval_ctx();
  int merge_same_headers();
  int merge_checks_per_header(vector<Check *> &checks);
  int merge_monitors();
  int remove_invalid_checks();
};
