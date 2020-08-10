#include "ast.h"
#include <map>
#include <algorithm>

string DEFAULT_LOW = "0";
string DEFAULT_HIGH = "10000";

#define MIN_ACTION_VAL 1
#define MAX_ACTION_VAL 15
#define NORMAL_ACTION_VAL MIN_ACTION_VAL
#define DROP_ACTION_VAL   MAX_ACTION_VAL

// helper function: find+replace string in a string
void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

map <string, int> action_to_val = {{"FWD1", 1}, {"FWD2", 2}, {"FWD3", 3},
    {"FWD4", 4}, {"FWD5", 5}, {"FWD6", 6}, {"FWD7", 7}, {"FWD8", 8},
    {"FWD9", 9}, {"FWD10", 10}, {"FWD11", 11}, {"FWD12", 12}, {"FWD13", 13},
    {"FWD14", 14}, {"DROP", MAX_ACTION_VAL}};

int Program::parse_match_mon(MatchExpr *me, Check *check)
{
   string mvar = me->get_mvar();
   Monitor *mon = get_mon_by_name(mvar);
   ListPred *lp = mon->get_listpred();
   BinaryPred *bp = me->get_binarypred();
   cout << "processing a monitor check: " + mon->stringify() << endl;

   // TODO: hard-coded: monitor check always has highest priority
   check->if_action_val = NORMAL_ACTION_VAL;
   check->else_action_val = DROP_ACTION_VAL;
   check->priority = DROP_ACTION_VAL;
   check->type = CHECK_TYPE_MONITOR;
   check->header = lp->get_listvar();
   check->mon_name = mvar;

   if (bp) {
      cout << "monitor " << mvar << " has an extra check: " + bp->stringify() << endl;
      check->extra_check = 1;
      check->extra_header.push_back(bp->get_lhs());
      cout << "adding extra header " << bp->get_lhs() << endl;
      check->h_to_val[bp->get_lhs()] = bp->get_rhs();
   } else {
      check->extra_check = 0;
   }

   return 0;
}

int Program::parse_match_list(MatchExpr *me, Check *check)
{
   ListPred *lp = me->get_listpred();

   if (!me->get_ifthenelse()) {
      cout << "lp: " + lp->stringify() + "lacks an else branch." << endl;
      return -1;
   }

   cout << "processing list pred : " + lp->stringify() << endl;

   string listbodyvar = lp->get_listbodyvar();
   ListExpr *le = this->get_list_by_name(listbodyvar);
   check->header = lp->get_listvar();
   check->list = le->get_items();

   check->type = CHECK_TYPE_LIST;
   check->if_action_val = action_to_val[me->get_if_action()];
   check->else_action_val = action_to_val[me->get_else_action()];
   check->priority = check->if_action_val;
   if (check->else_action_val != NORMAL_ACTION_VAL ||
       check->if_action_val == check->else_action_val) {
      cout << "actions values do not meet requirements"  << endl;
      return -1;
   }

   check->table_size = (check->list).size();
   return 0;
}

int Program::parse_match_pred(MatchExpr *me, Check *check)
{
   BinaryPred *bp = me->get_binarypred();
   cout << "processing binary pd: " + bp->stringify() << endl;

   // we only support if-then-else
   if (!me->get_ifthenelse()) {
      cout << "pd: " + bp->stringify() + "lacks an else branch." << endl;
      return -1;
   }

   // fetch action values, we only support else action to be normal forward action
   check->if_action_val = action_to_val[me->get_if_action()];
   check->else_action_val = action_to_val[me->get_else_action()];
   check->priority = check->if_action_val;
   if (check->else_action_val != NORMAL_ACTION_VAL ||
       check->if_action_val == check->else_action_val) {
      cout << "actions values do not meet requirements"  << endl;
      return -1;
   }

   // parse match headers and values, we only support ==, < and >
   check->header = bp->get_lhs();
   if (bp->get_op() == "==") {
      check->type = CHECK_TYPE_EXACT;
      check->pred_hi = bp->get_rhs();
      check->pred_lo = bp->get_rhs();
   } else if (bp->get_op() == "<") {
      check->type = CHECK_TYPE_RANGE;
      check->pred_lo = DEFAULT_LOW;
      check->pred_hi  = bp->get_rhs();
   } else if (bp->get_op() == ">") {
      check->type = CHECK_TYPE_RANGE;
      check->pred_hi = DEFAULT_HIGH;
      check->pred_lo = bp->get_rhs();
   }
   return 0;
}

void Program::process_matchexprs()
{
   int i, ret;
   // process all match exprs one by one
   for (i = 0; i < matchexpr_v.size(); i ++) {
      Check check;
      MatchExpr *me = matchexpr_v[i];
      switch(me->get_type()) {
         case MATCH_MONITOR:
            ret = parse_match_mon(me, &check);
            assert(ret == 0);
            break;
         case MATCH_PRED:
            ret = parse_match_pred(me, &check);
            assert(ret == 0);
            break;
         case MATCH_LIST:
            ret = parse_match_list(me, &check);
            assert(ret == 0);
            break;
         default:
            fprintf(stderr, "unreconized me type %d", me->get_type());
            assert(0);
            break;
       }
       check.id = i;
       check.print();
       add_check(check);
   }
}

int Program::allocate_resources()
{
   int need;
   for (auto check = check_v.begin(); check != check_v.end(); check ++ ) {
      cout << "allocating resource for check no." << check->id << endl;
      switch(check->type) {
         case CHECK_TYPE_EXACT:
            need = NUM_STAGE_EXACT;
            break;
         case CHECK_TYPE_RANGE:
            need = NUM_STAGE_RANGE;
            break;
         case CHECK_TYPE_LIST:
            need = NUM_STAGE_RANGE;
            break;
         case CHECK_TYPE_MONITOR:
            need = NUM_STAGE_MONITOR;
            break;
         default:
            assert(0);
      }

      // move to next round of recirculation if necessary
      if (curr_stage + need > stages_per_rec) {
         curr_stage = 0;
         curr_rec += 1;
      }
      check->start_stage = curr_stage;
      check->stage_num = need;
      check->rec = curr_rec;
      curr_stage += need;
   }

   if (curr_rec >= max_rec_times) {
      cout << "resources not enough, curr_rec=" << curr_rec <<
              "curr_stage=" << curr_stage << endl;
      return -1;
   }

   return 0;
}

string Program::generate_header_def()
{
   string ret;
   vector<string> defs;
   ret += "header_type poise_h_t{\n"
          "   fields {\n";

   // TODO: support variable field length (i.e., list match)
   for (auto check = check_v.begin(); check != check_v.end(); check ++ ) {
      string rec = to_string(check->rec);
      string id  = to_string(check->id);
      string len = to_string(check->field_len);
      defs.push_back("      " + check->header + " : " + len + ";\n");
   }

   // remove duplicate header definitions
   sort(defs.begin(), defs.end());
   defs.erase(unique(defs.begin(), defs.end()), defs.end());
   for (auto def = defs.begin(); def != defs.end(); def ++) {
      ret = ret + *def;
   }

   ret += "   }\n}\n\n";
   return ret;
}

string Program::generate_table_def()
{
   string ret;
   string match_type;
   ifstream f("./templates/monitor_template.p4");
   string mon_template((std::istreambuf_iterator<char>(f)),
                       (std::istreambuf_iterator<char>()));
   for (auto check = check_v.begin(); check != check_v.end(); check ++ ) {
      string rec = to_string(check->rec);
      string id  = to_string(check->id);
      if (check->type == CHECK_TYPE_RANGE || check->type == CHECK_TYPE_EXACT ||
          check->type == CHECK_TYPE_LIST) {
         match_type = check->type == CHECK_TYPE_RANGE ? "range" : "exact";
         check->table_name = "eval_"+ check->header +"_tab_rec" + rec + "_id" + id;
         ret += "table " + check->table_name + " {\n"
                "   reads {\n"
                "      poise_h." + check->header + " : " + match_type + ";\n"
                "   }\n"
                "   actions {set_ctx_dec; nop;}\n"
                "   default_action: nop;\n"
                "   size: " + to_string(check->table_size) + ";\n"
                "}\n\n";
      } else if (check->type == CHECK_TYPE_MONITOR) {
         string mon = mon_template;
         check->table_name = "eval_"+ check->mon_name +"_tab_rec" + rec + "_id" + id;
         ReplaceStringInPlace(mon, "MONNAME", check->mon_name);
         ReplaceStringInPlace(mon, "HNAME", check->header);
         ReplaceStringInPlace(mon, "TIMEOUTVAL", to_string(check->timeout_val));
         ReplaceStringInPlace(mon, "RECNUM", to_string(check->rec));
         ReplaceStringInPlace(mon, "IDNUM", to_string(check->id));
         if (check->extra_check) {
            string extra = "";
            for (auto h: check->extra_header) {
               extra += "        poise_h." + h + " : exact;\n";
            }
            ReplaceStringInPlace(mon, "EXTRA_CHECK", extra);
         } else {
            ReplaceStringInPlace(mon, "EXTRA_CHECK", "");
         }
         // cout << mon << endl;
         ret += mon;
      } else {
         cout << "unrecognized check type" << endl;
         assert(0);
      }
   }

   return ret;
}

string Program::generate_eval_ctx()
{
   string ret;
   int prev_rec = 0;
   ret = "control EvaluateContext\n{\n";
   ret += "   if (log.recir_counter == 0) {\n";

   for (auto check = check_v.begin(); check != check_v.end(); check ++) {
      string rec = to_string(check->rec);
      string id  = to_string(check->id);
      string h   = check->header;
      if (check->rec != prev_rec) {
         ret += "   }\n"
                "   else if (log.recir_counter == " + rec + ") {\n";
         prev_rec = check->rec;
      }

      // for list
      if (check->type == CHECK_TYPE_EXACT || check->type == CHECK_TYPE_LIST ||
          check->type == CHECK_TYPE_RANGE) {
         ret += "      apply(" + check->table_name + ");\n";
      } else if (check->type == CHECK_TYPE_MONITOR) {
         string m = check->mon_name;
         ret += "      apply(monitor_" + m + "_tab) {\n"
                "         hit {\n"
                "            apply(update_" + m + "_tab);\n"
                "         }\n"
                "         miss {\n"
                "            apply(check_" + m + "_active_tab);\n"
                "         }\n"
                "      }\n";
         ret += "      apply(" + check->table_name + ");\n";
      } else {
         cout << "unrecognized check type" << endl;
         assert(0);
      }
   }

   ret += "   }\n\n"
          "   if (log.recir_counter < RECIR_TIMES) {\n"
          "      apply(do_recirculate_tab);\n"
          "   } else {\n"
          "      apply(drop1_tab);\n"
          "   }\n}\n";
   return ret;
}

int Program::generate_p4(string file_name)
{
   ofstream p4(file_name);
   int ret;

   // sort all checks by priority
   sort(check_v.begin(), check_v.end());

   // allocate stage resources for all checks
   ret = allocate_resources();
   if (ret) {
      cout << "failed to allocate resources" << endl;
      assert(0);
   }

   string p4_header = "#ifndef POLICY_P4\n"
                      "#define POLICY_P4\n"
                      "\n#define RECIR_TIMES " + to_string(curr_rec) + "\n\n";
   string p4_footer = "#endif";

   string header_def = generate_header_def();
   string table_def = generate_table_def();
   string eval_ctx = generate_eval_ctx();

   //cout << p4_header << header_def << table_def << eval_ctx << p4_footer << endl;
   p4 << p4_header << header_def << table_def << eval_ctx << p4_footer << endl;
   return 0;
}

int Program::generate_cmd(string file_name)
{
   ofstream cmd(file_name);
   string cmd_str;
   int ret = 0;

   for (auto check = check_v.begin(); check != check_v.end(); check ++) {
      // cout << "generating cmd for table" << check->table_name << endl;
      if (check->type == CHECK_TYPE_EXACT) {
         cmd_str += "pd " + check->table_name + " add_entry set_ctx_dec"
                    + " poise_h_" + check->header + " " + check->pred_hi
                    + " action_x " + to_string(check->if_action_val) + "\n";
      } else if (check->type == CHECK_TYPE_RANGE) {
         cmd_str += "pd " + check->table_name + " add_entry set_ctx_dec"
                    + " poise_h_" + check->header + "_start " + check->pred_lo
                    + " poise_h_" + check->header + "_end " + check->pred_hi
                    + " priority 0"
                    + " action_x " + to_string(check->if_action_val)
                    + " entry_hdl 1\n";
      } else if (check->type == CHECK_TYPE_LIST) {
         vector<int> l = check->list;
         for (auto e = l.begin(); e != l.end(); e ++) {
            cmd_str += "pd " + check->table_name + " add_entry set_ctx_dec"
                       + " poise_h_" + check->header + " " + to_string(*e)
                       + " action_x " + to_string(check->if_action_val) + "\n";
         }
      } else if (check->type == CHECK_TYPE_MONITOR) {
         string front = "pd " + check->table_name + " add_entry set_ctx_dec "
                        + check->mon_name + "_meta_active 0";
         string behind = " action_x " + to_string(check->else_action_val) + "\n";
         if (check->extra_check) {
            for (auto h: check->extra_header) {
               string val = check->h_to_val[h];
               string middle = " poise_h_" + h + " " + val;
               cmd_str += front + middle + behind;
               // cout << front + middle + behind;
            }
         } else {
            cmd_str += front + behind;
         }
      }
   }

   //cout << cmd_str << endl;
   cmd << cmd_str << endl;
   return ret;
}

// can merge list/exact matches with same priority.
int Program::merge_checks_per_header(vector<Check *> &checks)
{
   Check *first_check = *(checks.begin());
   string header = first_check->header;
   int if_action_val = first_check->if_action_val;
   int else_action_val = first_check->else_action_val;
   int prio = first_check->priority;
   int id = first_check->id;
   vector<int> list;

   for (auto i = checks.begin(); i != checks.end(); i ++) {
      Check *c = *i;
      if (c->priority != prio) {
         return -1;
      }
   }

   for (auto i = checks.begin(); i != checks.end();) {
      Check *c = *i;
      if (c->type != CHECK_TYPE_EXACT && c->type != CHECK_TYPE_LIST) {
         checks.erase(i);
      } else {
         i ++;
      }
   }

   if (checks.size() <= 1) {
      return -1;
   }

   for (auto i = checks.begin(); i != checks.end(); i ++) {
      Check *c = *i;
      if (c->type == CHECK_TYPE_EXACT) {
         std::string::size_type sz;
         list.push_back(stoi(c->pred_lo, &sz));
      } else {
         list.insert(list.end(), c->list.begin(), c->list.end());
      }
      c->type = CHECK_TYPE_INVALID;
      // cout << "setting check no." << c->id << " to invalid" << endl;
   }

   Check *nc = new Check();
   nc->type = CHECK_TYPE_LIST;
   nc->header = header;
   nc->if_action_val = if_action_val;
   nc->else_action_val = else_action_val;
   nc->priority = if_action_val;
   cout << "setting priority to " << nc->priority << endl;
   nc->list = list;
   nc->table_size = list.size();
   nc->id = id;

   check_v.push_back(*nc);
   return 0;
}

int Program::remove_invalid_checks()
{
   for (auto c = check_v.begin(); c != check_v.end();) {
      if (c->type == CHECK_TYPE_INVALID) {
         cout << "erase check no." << c->id << endl;
         check_v.erase(c);
      } else {
         c ++;
      }
   }
   return 0;
}

int Program::merge_same_headers()
{
   cout << check_v.size() << " checks exist before optimization" << endl;
   for (auto h = header_v.begin(); h != header_v.end(); h ++) {
      vector<Check *> checks;
      for (auto c = check_v.begin(); c != check_v.end(); c ++) {
         if (c->header == *h) {
            checks.push_back(&*c);
         }
      }
      if (checks.size() > 1) {
         if (merge_checks_per_header(checks) == 0) {
            cout << "composed header " << *h << endl;
         }
      }
   }
   remove_invalid_checks();
   cout << check_v.size() << " checks exist after optimization" << endl;
   return 0;
}

int Program::merge_monitors()
{
   for (auto m: monitor_v) {
      vector<Check *> checks;
      for (auto c = check_v.begin(); c != check_v.end(); c ++) {
         if (c->type == CHECK_TYPE_MONITOR && c->mon_name == m->get_mvar()) {
            checks.push_back(&*c);
         }
      }
      if (checks.size() <= 1) {
         continue;
      }
      cout << "trying to merge " << checks.size() <<
              " checks which access monitor:" << m->stringify() << endl;

      Check *first = *checks.begin();
      for (auto c = checks.begin() + 1; c != checks.end(); c ++) {
         for (auto h: (*c)->extra_header) {
            first->extra_header.push_back(h);
            first->h_to_val[h] = (*c)->h_to_val[h];
         }
         (*c)->type = CHECK_TYPE_INVALID;
      }
      first->table_size = first->extra_header.size();
   }

   remove_invalid_checks();
   return 0;
}

int Program::compile_tofino(string p4_file, string sw_file)
{
   cout << "\n========== Parsing match exprs ==========" << endl;
   cout << "the program has " + to_string(count_headers()) + " headers, "
           + to_string(count_monitors()) + " monitors, "
           + to_string(count_listexpr()) + " lists, "
           + to_string(count_matchexpr()) + " match exprs."
           << endl;

   // parse all match expressions
   process_matchexprs();
   cout << "all match expr prased, got " << check_v.size()
        << " sequential checks." << endl;

   // do optimization
   merge_same_headers();
   merge_monitors();

   // generate P4 code
   cout << "\n========== Generating P4 code ==========" << endl;
   generate_p4(p4_file);
   generate_cmd(sw_file);
   cout << "all done" << endl;

   return 0;
}
