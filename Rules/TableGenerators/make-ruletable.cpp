#include <math.h>
#include <time.h>

#include <iostream>
#include <set>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
using namespace std;

/*

Makes a rule-table for your transition function.

To use:
1) fill slowcalc with your own transition function.
2) set the parameters in main() at the bottom.
3) execute the program from the command line (no options taken)

For a 32-state 5-neighbourhood rule it took 16mins on a 2.2GHz machine.
For a 4-state 9-neighbourhood rule it took 4s.

No-change transitions are skipped - the engine should default to this. 

The merging is very simple - if a transition is compatible with the first rule, 
then merge the transition into the rule. If not, try the next rule. If there are
no more rules, add a new rule containing just the transition. 

=== Worked example: ===
Transitions: 
1,0,0->3
1,2,0->3
2,0,0->3
2,2,0->1
Start. First transition taken as first rule:
rule 1: 1,0,0->3
Next transition - is it compatible with rule 1? 
i.e. is 1,[0,2],0->3 valid for all permutations? Yes.
rule 1 now: 1,[0,2],0->3
Next transition - is it compatible with rule 1?
i.e. is [1,2],[0,2],0->3 valid for all permutations?
no - because 2,2,0 ->1, not ->3. so add the transition as a new rule:
rule 1 still: 1,[0,2],0 -> 3
rule 2 now : 2,0,0->3
Next transition - is it compatible with rule 1? no - output is different.
Is it compatible with rule 2? no - output is different.
Final output:
1,[0,2],0 -> 3
2,0,0 -> 3
2,2,0 -> 1
Written with variables:
var a={0,2}
1,a,0,3
2,0,0,3
2,2,0,1
===============

In the function produce_rule_table, the state space is exhaustively traversed.
If your transition function consists of transition rules already then you can
optimise by running through the transitions instead. You might also want to
turn off the optimisation in rule::can_merge, to see if it gives you better
compression.

Also note: I feel sure there are better ways to compress rule tables than this...

Contact: Tim Hutton <tim.hutton@gmail.com>

*/

// some typedefs and compile-time constants
typedef unsigned short state;
enum TSymm { none, rot4, rot8 };
static const string symmetry_strings[] = {"none","rot4","rot8"};

// fill in this function with your desired transition rules
// (for von Neumann neighbourhoods, just ignore the nw,se,sw,ne inputs)
state slowcalc(state nw,state n,state ne,state w,state c,state e,state sw,state s,state se)
{
	// wireworld:
	switch (c) 
	{
	  case 0: return 0 ;
	  case 1: return 2 ;
	  case 2: return 3 ;
	  case 3:
		  if ((((1+(nw==1)+(n==1)+(ne==1)+(w==1)+(e==1)+(sw==1)+
			  (s==1)+(se==1))) | 1) == 3)
			  return 1 ;
		  else
			  return 3 ;
	  default:
		  return 0 ; // should throw an error here
	}
}

vector<state> rotate_inputs(const vector<state>& inputs,int rot)
{
	vector<state> rotinp(inputs);
	rotate_copy(inputs.begin()+1,inputs.begin()+1+rot,inputs.end(),rotinp.begin()+1);
	return rotinp;
}

// simple rule structure, e.g. 1,2,[4,5],8,2 -> 0
class rule { 

public:
	set<state> inputs[9]; // c,n,ne,e,se,s,sw,w,nw  or  c,n,e,s,w
	state ns; // new state

	int n_inputs; // 5: von Neumann; 9: Moore
	TSymm symm;

public:
	// constructors
	rule(const rule& r) : ns(r.ns),n_inputs(r.n_inputs),symm(r.symm)
	{
		for(int i=0;i<n_inputs;i++)
			inputs[i]=r.inputs[i];
	}
	rule& operator=(const rule& r) 
	{
		n_inputs=r.n_inputs;
		symm = r.symm;
		ns = r.ns;
		for(int i=0;i<n_inputs;i++)
			inputs[i]=r.inputs[i];
		return *this;
	}
	rule(const vector<state>& inputs,int n_inputs,state ns1,TSymm symm1) 
		: ns(ns1),n_inputs(n_inputs),symm(symm1)
	{
		merge(inputs);
	}

	// if we merge the rule and the supplied transition, will the rule remain true for all cases?
	bool can_merge(const vector<state>& test_inputs,state ns1) const
	{
		if(ns1!=ns) return false; // can't merge if the outputs are different

		// if you are running through your own transitions, you might want to turn off this 
		// optimisation, to get better compression.
		{
			// optimisation: we skip this test if more than 2 entries are different, since we 
			// will have considered the relevant one-change rules before this. 
			int n_different=0;
			for(int i=0;i<n_inputs;i++)
				if(inputs[i].find(test_inputs[i])==inputs[i].end())
					if(++n_different>1)
						return false;
		}

		// just check the new permutations
		for(int i=0;i<n_inputs;i++)
		{
			if(inputs[i].find(test_inputs[i])==inputs[i].end())
			{
				rule r1(*this);
				r1.inputs[i].clear(); // (optimisation, since we don't need to re-test all the other permutations) 
				r1.inputs[i].insert(test_inputs[i]);
				if(!r1.all_true()) return false;
			}
		}
		return true;
	}
	// merge the inputs with this rule
	void merge(const vector<state>& new_inputs)
	{
		for(int i=0;i<n_inputs;i++)
			inputs[i].insert(new_inputs[i]); // may already exist, set ignores if so
	}

	// is this set of inputs a match for the rule, for the given symmetry?
	bool matches(const vector<state>& test_inputs) const
	{
		if(nosymm_matches(test_inputs))
			return true;
		if(symm==rot4 || symm==rot8)
		{
			if(n_inputs==5)
			{
				if(nosymm_matches(rotate_inputs(test_inputs,1)))
					return true;
				if(nosymm_matches(rotate_inputs(test_inputs,2)))
					return true;
				if(nosymm_matches(rotate_inputs(test_inputs,3)))
					return true;
			}
			else // n_inputs==9
			{
				if(nosymm_matches(rotate_inputs(test_inputs,2)))
					return true;
				if(nosymm_matches(rotate_inputs(test_inputs,4)))
					return true;
				if(nosymm_matches(rotate_inputs(test_inputs,6)))
					return true;
			}
		}
		if(symm==rot8) // n_inputs==9
		{
			if(nosymm_matches(rotate_inputs(test_inputs,1)))
				return true;
			if(nosymm_matches(rotate_inputs(test_inputs,3)))
				return true;
			if(nosymm_matches(rotate_inputs(test_inputs,5)))
				return true;
			if(nosymm_matches(rotate_inputs(test_inputs,7)))
				return true;
		}
		return false;
	}

protected:

	// ignoring symmetry, does this set of inputs match the rule?
	bool nosymm_matches(const vector<state>& test_inputs) const
	{
		for(int i=0;i<n_inputs;i++)
			if(inputs[i].find(test_inputs[i])==inputs[i].end())
				return false;
		return true;
	}

	// is the rule true in all permutations?
	bool all_true() const
	{
		set<state>::const_iterator c_it,n_it,ne_it,e_it,se_it,s_it,sw_it,w_it,nw_it;
		if(n_inputs==9)
		{
			for(c_it = inputs[0].begin();c_it!=inputs[0].end();c_it++)
				for(n_it = inputs[1].begin();n_it!=inputs[1].end();n_it++)
					for(ne_it = inputs[2].begin();ne_it!=inputs[2].end();ne_it++)
						for(e_it = inputs[3].begin();e_it!=inputs[3].end();e_it++)
							for(se_it = inputs[4].begin();se_it!=inputs[4].end();se_it++)
								for(s_it = inputs[5].begin();s_it!=inputs[5].end();s_it++)
									for(sw_it = inputs[6].begin();sw_it!=inputs[6].end();sw_it++)
										for(w_it = inputs[7].begin();w_it!=inputs[7].end();w_it++)
											for(nw_it = inputs[8].begin();nw_it!=inputs[8].end();nw_it++)
												if(slowcalc(*nw_it,*n_it,*ne_it,*w_it,*c_it,*e_it,*sw_it,*s_it,*se_it)!=ns)
													return false;
		}
		else
		{
			for(c_it = inputs[0].begin();c_it!=inputs[0].end();c_it++)
				for(n_it = inputs[1].begin();n_it!=inputs[1].end();n_it++)
					for(e_it = inputs[2].begin();e_it!=inputs[2].end();e_it++)
						for(s_it = inputs[3].begin();s_it!=inputs[3].end();s_it++)
							for(w_it = inputs[4].begin();w_it!=inputs[4].end();w_it++)
								if(slowcalc(0,*n_it,0,*w_it,*c_it,*e_it,0,*s_it,0)!=ns)
									return false;
		}
		return true;
	}
};

// makes a unique variable name for a given value
string get_variable_name(unsigned int iVar)
{
	const char VARS[52]={'a','b','c','d','e','f','g','h','i','j',
		'k','l','m','n','o','p','q','r','s','t','u','v','w','x',
		'y','z','A','B','C','D','E','F','G','H','I','J','K','L',
		'M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
	ostringstream oss;
	if(iVar<52)
		oss << VARS[iVar];
	else if(iVar<52*52)
		oss << VARS[(iVar-(iVar%52))/52 - 1] << VARS[iVar%52];
	else
		oss << "!"; // we have a 52*52 limit ("should be enough for anyone")
	return oss.str();
}

void print_rules(const vector<rule>& rules,ostream& out)
{
	// first collect all variables (makes reading easier)
	map<set<state>,string> vars;
	for(vector<rule>::const_iterator r_it=rules.begin();r_it!=rules.end();r_it++)
	{
		for(int i=0;i<r_it->n_inputs;i++)
		{
			if(r_it->inputs[i].size()>1 && vars.find(r_it->inputs[i])==vars.end())
			{
				string var = get_variable_name(vars.size());
				vars[r_it->inputs[i]] = var;
				// print it
				out << "var " << var << "={";
				set<state>::const_iterator it=r_it->inputs[i].begin();
				while(true)
				{
					out << (int)*it;
					it++;
					if(it!=r_it->inputs[i].end()) out << ",";
					else break;
				}
				out << "}\n";
			}
		}
	}
	for(vector<rule>::const_iterator r_it=rules.begin();r_it!=rules.end();r_it++)
	{
		for(int i=0;i<r_it->n_inputs;i++)
		{
			// if m[i] is a variable then use that variable instead
			map<set<state>,string>::const_iterator found = vars.find(r_it->inputs[i]);
			if(found!=vars.end())
			{
				// output the variable name
				out << found->second;
			}
			else if(r_it->inputs[i].size()==1)
			{
				// output the state
				out << *r_it->inputs[i].begin();
			}
			else
			{
				// internal error - should have replaced with a variable!
			}
			out << ",";
		}
		out << r_it->ns << "\n";
	}
}

void produce_rule_table(vector<rule>& rules,int N,int nhood_size,TSymm symm)
{
	state c,n,ne,nw,sw,s,se,e,w,ns;
	vector<rule>::iterator it;
	bool merged;
	for(c=0;c<N;c++)
	{
		cout << "\nProcessing for c=" << (int)c << ", " << rules.size() << " rules so far." << endl;

		if(nhood_size==9)
		{
			vector<state> inputs(9);
			inputs[0]=c;
			for(n=0;n<N;n++)
			{
				cout << ".";
				inputs[1]=n;
				for(ne=0;ne<N;ne++)
				{
					inputs[2]=ne;
					for(e=0;e<N;e++)
					{
						inputs[3]=e;
						for(se=0;se<N;se++)
						{
							inputs[4]=se;
							for(s=0;s<N;s++)
							{
								inputs[5]=s;
								for(sw=0;sw<N;sw++)
								{
									inputs[6]=sw;
									for(w=0;w<N;w++)
									{
										inputs[7]=w;
										for(nw=0;nw<N;nw++)
										{
											ns = slowcalc(nw,n,ne,w,c,e,sw,s,se);
											if(ns == c)
												continue; // we can ignore stasis transitions
											// can we merge this transition with any existing rule?
											inputs[8]=nw;
											merged = false;
											for(it=rules.begin();!merged && it!=rules.end();it++)
											{
												rule &r = *it;
												if(r.can_merge(inputs,ns))
												{
													r.merge(inputs);
													merged = true;
													//cout << "Expanded existing rule." << endl;
												}
												if(symm==rot4 || symm==rot8)
												{
													if(!merged && r.can_merge(rotate_inputs(inputs,2),ns))
													{
														r.merge(rotate_inputs(inputs,2));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
													if(!merged && r.can_merge(rotate_inputs(inputs,4),ns))
													{
														r.merge(rotate_inputs(inputs,4));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
													if(!merged && r.can_merge(rotate_inputs(inputs,6),ns))
													{
														r.merge(rotate_inputs(inputs,6));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
												}
												if(symm==rot8)
												{
													if(!merged && r.can_merge(rotate_inputs(inputs,1),ns))
													{
														r.merge(rotate_inputs(inputs,1));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
													if(!merged && r.can_merge(rotate_inputs(inputs,3),ns))
													{
														r.merge(rotate_inputs(inputs,3));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
													if(!merged && r.can_merge(rotate_inputs(inputs,5),ns))
													{
														r.merge(rotate_inputs(inputs,5));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
													if(!merged && r.can_merge(rotate_inputs(inputs,7),ns))
													{
														r.merge(rotate_inputs(inputs,7));
														merged = true;
														//cout << "Expanded existing rule." << endl;
													}
												}
											}
											if(!merged)
											{
												// need to make a new rule starting with this transition
												rule r(inputs,nhood_size,ns,symm);
												rules.push_back(r);
												//cout << "Started a new rule." << endl;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else // nhood_size==5
		{
			vector<state> inputs(5);
			inputs[0]=c;
			for(n=0;n<N;n++)
			{
				cout << ".";
				inputs[1]=n;
				for(e=0;e<N;e++)
				{
					inputs[2]=e;
					for(s=0;s<N;s++)
					{
						inputs[3]=s;
						for(w=0;w<N;w++)
						{
							ns = slowcalc(0,n,0,w,c,e,0,s,0);
							if(ns == c)
								continue; // we can ignore stasis transitions

							// can we merge this transition with any existing rule?
							inputs[4]=w;
							merged = false;
							for(it=rules.begin();!merged && it!=rules.end();it++)
							{
								rule &r = *it;
								if(r.can_merge(inputs,ns))
								{
									r.merge(inputs);
									merged = true;
									//cout << "Expanded existing rule." << endl;
								}
								if(symm==rot4)
								{
									if(!merged && r.can_merge(rotate_inputs(inputs,1),ns))
									{
										r.merge(rotate_inputs(inputs,1));
										merged = true;
										//cout << "Expanded existing rule." << endl;
									}
									if(!merged && r.can_merge(rotate_inputs(inputs,2),ns))
									{
										r.merge(rotate_inputs(inputs,2));
										merged = true;
										//cout << "Expanded existing rule." << endl;
									}
									if(!merged && r.can_merge(rotate_inputs(inputs,3),ns))
									{
										r.merge(rotate_inputs(inputs,3));
										merged = true;
										//cout << "Expanded existing rule." << endl;
									}
								}
							}
							if(!merged)
							{
								// need to make a new rule starting with this transition
								rule r(inputs,nhood_size,ns,symm);
								rules.push_back(r);
								//cout << "Started a new rule." << endl;
							}
						}
					}
				}
			}
		}
	}
}

// here we use the computed rule table as a replacement slowcalc, for checking correctness
state new_slowcalc(const vector<rule>& rules,const vector<state>& inputs) 
{
	for(vector<rule>::const_iterator it=rules.begin();it!=rules.end();it++)
		if(it->matches(inputs))
			return it->ns;
    return inputs[0]; // default: no change
}

bool is_correct(const vector<rule>&rules,int N,int neighbourhood_size)
{
	// exhaustive check
	state c,n,ne,nw,sw,s,se,e,w;
	if(neighbourhood_size==9)
	{
		vector<state> inputs(9);
		for(c=0;c<N;c++)
		{
			inputs[0]=c;
			for(n=0;n<N;n++)
			{
				inputs[1]=n;
				for(ne=0;ne<N;ne++)
				{
					inputs[2]=ne;
					for(e=0;e<N;e++)
					{
						inputs[3]=e;
						for(se=0;se<N;se++)
						{
							inputs[4]=se;
							for(s=0;s<N;s++)
							{
								inputs[5]=s;
								for(sw=0;sw<N;sw++)
								{
									inputs[6]=sw;
									for(w=0;w<N;w++)
									{
										inputs[7]=w;
										for(nw=0;nw<N;nw++)
										{
											inputs[8]=nw;
											if(new_slowcalc(rules,inputs) 
												!= slowcalc(nw,n,ne,w,c,e,sw,s,se))
													return false;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		vector<state> inputs(5);
		for(c=0;c<N;c++)
		{
			inputs[0]=c;
			for(n=0;n<N;n++)
			{
				inputs[1]=n;
				for(e=0;e<N;e++)
				{
					inputs[2]=e;
					for(s=0;s<N;s++)
					{
						inputs[3]=s;
						for(w=0;w<N;w++)
						{
							inputs[4]=w;
							if(new_slowcalc(rules,inputs) 
								!= slowcalc(0,n,0,w,c,e,0,s,0))
								return false;
						}
					}
				}
			}
		}
	}
	return true;
}

int main()
{
	// parameters for use:
	const int N_STATES = 4;
	const TSymm symmetry = rot8;
	const int nhood_size = 9;
	const string output_filename = "wireworld.table";

	vector<rule> rules;
	time_t t1,t2;
	time(&t1);
	produce_rule_table(rules,N_STATES,nhood_size,symmetry);
	time(&t2);
	int n_secs = (int)difftime(t2,t1);
	cout << "\nProcessing took: " << n_secs << " seconds." << endl;

	{
		ofstream out(output_filename.c_str());
		out << "# rules: " << rules.size() << "\n#";
		out << "\n# Golly rule-table format.\n# Each rule: C,";
		if(nhood_size==5)
			out << "N,E,S,W";
		else
			out << "N,NE,E,SE,S,SW,W,NW";
		out << ",C'";
		out << "\n# N.B. Where the same variable appears multiple times,\n# it can take different values each time.\n#";
		out << "\nn_states:" << N_STATES;
		out << "\nneighborhood_size:" << nhood_size;
		out << "\nsymmetries:" << symmetry_strings[symmetry] << "\n";
		print_rules(rules,out);
	}
	cout << rules.size() << " rules written." << endl;

	// optional: run through the entire state space, checking that new_slowcalc matches slowcalc
	cout << "Verifying is correct (can abort if you're confident)...";
	if(is_correct(rules,N_STATES,nhood_size))
		cout << "yes." << endl;
	else
		cout << "no! Either there's a bug in the code, or the transition function does not have the symmetry you selected." << endl;
}