%{
#include <cstdio>
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <regex>
#include <unistd.h>

#include "ast.h"

// the program AST
Program *prog = new Program();

// helper variables
ListPred *lp;
BinaryPred *bp;
string thebinaryop;

// ArithExpr stack:
vector<ArithExpr*> ae_stack;
string ao; //arithmetic operator
ArithExpr *ae = NULL;

// For parsing compound if-then-else statements.
// Nested statements not supported.
// If size is one, then only if-then, if size is two, then it's if-then-else
vector<string> action_v;

// for all context headers used in the policy programs
//vector<string> ctxt_v;

bool is_header_name(string s) {
  std::regex r ("(h)([0-9]*)");
  if (regex_match(s, r)) {
    cout << "Detected a header " + s << endl;
    return true;
  } else {
    cout << s + " is not a header!" << endl;
    return false;
  }
}

bool is_monitor_name(string s) {
  std::regex r ("(m)([0-9]*)");
  if (regex_match(s, r)) {
    cout << "Detected a monitor " + s << endl;
    return true;
  } else {
    cout << s + " is not a monitor !" << endl;
    return false;
  }
}

using namespace std;

// for parsing the LISTBODY
vector<int> list_items;

void print_list_items () {
  printf("List contains: ");
  for (int i = 0; i < list_items.size(); i ++) {
    printf(" %d ", list_items[i]);
  }
  printf("\n");
}

// stuff from flex that bison needs to know about:
extern "C" int yylex();
extern "C" int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);
%}

// Bison fundamentally works by asking flex to get the next token, which it
// returns as an object of type "yystype".  But tokens could be of any
// arbitrary data type!  So we deal with that in Bison by defining a C union
// holding each of the types of tokens that Flex could return, and have Bison
// use that union instead of "int" for the definition of "yystype":
%union {
	int ival;
	float fval;
	char *sval;
	int listval[100];
}

// define the "terminal symbol" token types I'm going to use (in CAPS
// by convention), and associate each with a field of the union:

%token IF THEN ELSE
%token MATCH
%token COUNT
%token MONITOR

// actions
%token DROP FWD1 FWD2 FWD3 FWD4 FWD5 FWD6 FWD7 FWD8 FWD9 FWD10 FWD11 FWD12 FWD13 FWD14

// delineators
%token LP RP
%token LB RB

// operators
%token LT GT GE LE EQ
%token PLUS MINUS MULTI MOD
%token AND OR NOT

// list operators
%token ASSIGN DEF IN NOTIN
%token <sval> LISTVAR

// base types
%token <ival> CONST
%token <sval> VAR

%%
// the first rule defined is the highest-level rule, which in our
// case is just the concept of a whole "snazzle file":
poise:
	statements {
                 cout << "\n====== Done Parsing a Poise program =======" << endl;
                 prog->print();
               }
	;
statements:
	statements statement
	| statement
	;

matchexpr:
	MATCH LP binaryexpr RP  {
                              assert(bp);
                              MatchExpr *theme = new MatchExpr (bp);
                              prog->add_matchexpr(theme);
                              bp = NULL;
                              cout << "matchexpr: expr is " << endl;
                            }
   | MATCH LP listexpr RP {
                             assert(lp);
                             MatchExpr *theme = new MatchExpr(lp);
                             prog->add_matchexpr(theme);
                             lp= NULL;
                             cout << "matchexpr: listexpr " << endl;
                           }
   | MATCH LP VAR RP {
                             // A monitor statement: match(m0)
                             assert(is_monitor_name($3));
                             string monitor_var = $3;
                             cout << "matchexpr: monitor " << endl;
                             MatchExpr *theme = new MatchExpr($3);
                             cout << "mvar: " + monitor_var << endl;
                             prog->add_matchexpr(theme);
                          }
   | MATCH LP VAR AND binaryexpr RP {
                             // A monitor statement with one more header
                             assert(bp);
                             assert(is_monitor_name($3));
                             string monitor_var = $3;
                             cout << "matchexpr: monitor " << endl;
                             MatchExpr *theme = new MatchExpr($3);
                             cout << "mvar: " + monitor_var << endl;
                             theme->set_binarypred(bp);
                             bp = NULL;
                             prog->add_matchexpr(theme);
                          }
	;

statement:
	IF matchexpr THEN action {
                               cout << "statement: matchexpr " << endl;
                             }
	| IF matchexpr THEN action ELSE action {
               cout << "\nstatement: if-then-else " << endl;
               int num_actions = action_v.size();
               for (int i = 0; i < action_v.size(); i ++) {
                 cout << i << "th action is " <<  action_v[i] << endl;
               }

               MatchExpr *theme = prog->get_latestmatchexpr();
               cout << "this statement has matchexpr: " + theme->stringify()
                    << endl;
               theme->set_if_action(action_v[0]);
               theme->set_else_action(action_v[1]);

               // clear actions for the next if-else statement
               action_v.clear();
            }

	| IF matchexpr THEN statement {cout << "statement: nested-if" << endl; }
	| VAR ASSIGN COUNT LP listexpr RP {
                // not supported
                assert(0);
                                     }
	| VAR ASSIGN MONITOR LP listexpr RP {
				                cout << "statement: monitor" << endl;
                            string monitor_var = $1;
                            cout << "var: " + monitor_var + " " << lp->stringify() << endl;
                            Monitor *m = new Monitor (monitor_var, lp);
                            prog->add_monitor(m);
                            assert(! is_header_name($1));
				                   }
	| DEF LISTVAR ASSIGN listbody {
                           cout << "statement: found a list" << endl;
                           // create a ListExpr
                           ListExpr *le = new ListExpr (list_items);
                           list_items.clear();
                           // we have an additional list
                           string listvar = $2;
                           // cout << "adding list=" << listvar << endl;
                           prog->add_listexpr(listvar, le);
                                 }
	;
listbody:
	LB listcontent RB {cout << "listbody"  << endl; }
	;
listcontent:
	CONST  {
		  cout << "base case listcontent " << $1 << endl;
        list_items.push_back($1);
		  print_list_items();
		}
	| listcontent CONST {
		  cout << " recursive listcontent " << $2 << endl;
        list_items.push_back($2);
		  print_list_items();
		}
;

action:
     DROP { action_v.push_back("DROP"); cout << "got action DROP" << endl;}
	| FWD1 { cout << "Action: FWD1" << endl; action_v.push_back("FWD1"); }
	| FWD2 { cout << "Action: FWD2" << endl; action_v.push_back("FWD2"); }
	| FWD3 { cout << "Action: FWD3" << endl; action_v.push_back("FWD3"); }
	| FWD4 { cout << "Action: FWD4" << endl; action_v.push_back("FWD4"); }
	| FWD5 { cout << "Action: FWD5" << endl; action_v.push_back("FWD5"); }
	| FWD6 { cout << "Action: FWD6" << endl; action_v.push_back("FWD6"); }
	| FWD7 { cout << "Action: FWD7" << endl; action_v.push_back("FWD7"); }
	| FWD8 { cout << "Action: FWD8" << endl; action_v.push_back("FWD8"); }
	| FWD9 { cout << "Action: FWD9" << endl; action_v.push_back("FWD9"); }
	| FWD10 { cout << "Action: FWD10" << endl; action_v.push_back("FWD10"); }
	| FWD11 { cout << "Action: FWD11" << endl; action_v.push_back("FWD11"); }
	| FWD12 { cout << "Action: FWD12" << endl; action_v.push_back("FWD12"); }
	| FWD13 { cout << "Action: FWD13" << endl; action_v.push_back("FWD13"); }
	| FWD14 { cout << "Action: FWD14" << endl; action_v.push_back("FWD14"); }
;

listexpr:
    VAR IN LISTVAR {
                     lp = new ListPred ($1, "in", $3);
                     cout << "listexpr: " << $1 << " in " << $3 <<endl;
                     // record used header.
                     assert(is_header_name($1));
                     prog->add_header($1);
                   }
    | VAR NOTIN LISTVAR {
                          lp = new ListPred($1, "notin", $3);
                          cout << "listexpr: " << $1 << " notin " << $3 <<endl;
                          // record used header.
                          assert(is_header_name($1));
                          prog->add_header($1);
                        }
    ;
binaryexpr:
	VAR binaryop CONST {
                         // BinaryPred in an if statement
                         bp = new BinaryPred($1, thebinaryop, $3);
                         cout << bp->stringify() << endl;
                         // record used header.
                         assert(is_header_name($1) || is_monitor_name($1));
                         prog->add_header($1);
                       }
	| VAR binaryop VAR {
                         // BinaryPred in an if statement
                         bp = new BinaryPred($1, thebinaryop, $3);
                         cout << bp->stringify() << endl;
                         // record used headers.
                         assert(is_header_name($1));
                         prog->add_header($1);
                         assert(is_header_name($3));
                         prog->add_header($3);
                       }
	| arithexpr binaryop CONST {
                                     //BinaryPred in an if statement
                                     ArithExpr * theae = ae_stack.back();
                                     ae_stack.pop_back();
                                     string ae_str = theae->stringify();
                                     bp = new BinaryPred(ae_str, thebinaryop, $3);
                                     cout << bp->stringify() << endl;
                                     cout << "Recursive case with the ArithExpr being " + theae->stringify() << endl;
                                   }
	;
arithexpr:
	VAR arithop VAR  {
                       string v1 = $1;
                       string v3 = $3;
                       ae = new ArithExpr(v1, ao, v3);
                       ae_stack.push_back(ae);
                       // record used headers.
                       assert(is_header_name($1));
                       prog->add_header($1);
                       assert(is_header_name($3));
                       prog->add_header($3);
                     }
	| arithexpr arithop VAR {
                              ArithExpr *theae = ae_stack.back();
                              ae_stack.pop_back();
                              string varstr = $3;
                              ae = new ArithExpr(theae, ao, varstr);
                              ae_stack.push_back(ae);
                              // record used headers.
                              assert(is_header_name($3));
                              prog->add_header($3);
                            }
	| arithexpr arithop CONST {
                                ArithExpr *theae = ae_stack.back();
                                ae_stack.pop_back();
                                string varstr = to_string($3);
                                ae = new ArithExpr(theae, ao, varstr);
                                ae_stack.push_back(ae);
								cout << "Complex arithexpr: " + ae->stringify() << endl;
                              }
	| CONST
	;
arithop:
	   PLUS  { ao = "+"; }
    | MINUS { ao = "-";}
    | MULTI {ao = "*";}
    | MOD   {ao = "%";}
	;
binaryop:
	   GT {thebinaryop = ">";}
    | LT {thebinaryop = "<"; }
    | LE {thebinaryop = "<="; }
    | GE {thebinaryop = ">=";}
    | EQ {thebinaryop = "==";}
	| binaryexpr boolop binaryexpr
	;
boolop:
	AND | OR | NOT
	;
%%

static void usage()
{
   fprintf(stderr, "Usage: ./poise-tofino -f <policy_file> -o <output_file>"
           " -s <sw_command_file>\n");
}

int main(int argc, char *argv[]) {
   int opt;
   char *policy_file = NULL;
   char *output_file = NULL;
   char *sw_file = NULL;

   // parse arguments
   while ((opt = getopt(argc, argv, "f:o:s:")) != -1) {
      switch (opt) {
         case 'f':
            policy_file = optarg;
            break;
         case 'o':
            output_file = optarg;
            break;
         case 's':
            sw_file = optarg;
            break;
         default:
            usage();
            exit(1);
      }
   }

   if (policy_file == NULL || output_file == NULL || sw_file == NULL) {
      usage();
      exit(1);
   }

	// open the policy file
	FILE *fp = fopen(policy_file, "r");
   if (fp == NULL) {
      fprintf(stderr, "failed to open policy file %s", policy_file);
      return 1;
   }

	// set flex to read from it instead of defaulting to STDIN:
	yyin = fp;

	// parse through the input until there is no more:
	do {
		yyparse();
	} while (!feof(yyin));

   // generate code
   prog->compile_tofino(output_file, sw_file);

   return 0;
}

void yyerror(const char *s)
{
	cout << "EEK, parse error!  Message: " << s << endl;
	// might as well halt now:
	exit(-1);
}
