/***************************************************************************
 *   Copyright (C) 2006 by BUI Quang Minh, Steffen Klaere, Arndt von Haeseler   *
 *   minh.bui@univie.ac.at   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iqtree_config.h>

//#include "Eigen/Core"
#include "phylotree.h"
#include <iostream>
#include <cstdlib>
#include <errno.h>
#include "greedy.h"
#include "pruning.h"
//#include "naivegreedy.h"
#include "splitgraph.h"
#include "circularnetwork.h"
#include "mtreeset.h"
#include "mexttree.h"
#include "ncl/ncl.h"
#include "msetsblock.h"
#include "myreader.h"
#include "phyloanalysis.h"
#include "matree.h"
#include "ngs.h"
#include "parsmultistate.h"
#include "gss.h"
#include "maalignment.h" //added by MA
#include "ncbitree.h"

using namespace std;



void generateRandomTree(Params &params)
{
	if (params.sub_size < 3) {
		outError(ERR_FEW_TAXA);
	}

	//cout << "Random number seed: " << params.ran_seed << endl << endl;

	SplitGraph sg;

	try {

		if (params.tree_gen == YULE_HARDING || params.tree_gen == CATERPILLAR ||
			params.tree_gen == BALANCED || params.tree_gen == UNIFORM || params.tree_gen == STAR_TREE) {
			if (!overwriteFile(params.user_file)) return;
			ofstream out;
			out.open(params.user_file);
			switch (params.tree_gen) {
			case YULE_HARDING:
				cout << "Generating random Yule-Harding tree..." << endl;
				break;
			case UNIFORM:
				cout << "Generating random uniform tree..." << endl;
				break;
			case CATERPILLAR:
				cout << "Generating random caterpillar tree..." << endl;
				break;
			case BALANCED:
				cout << "Generating random balanced tree..." << endl;
				break;
			case STAR_TREE:
				cout << "Generating star tree with random external branch lengths..." << endl;
				break;
			default: break;
			}
			ofstream out2;
			if (params.num_zero_len) {
				cout << "Setting " << params.num_zero_len << " internal branches to zero length..." << endl;
				string str = params.user_file;
				str += ".collapsed";
				out2.open(str.c_str());
			}
			for (int i = 0; i < params.repeated_time; i++) {
				MExtTree mtree;
				mtree.generateRandomTree(params.tree_gen, params);
				if (params.num_zero_len) {
					mtree.setZeroInternalBranches(params.num_zero_len);
					MExtTree collapsed_tree;
					collapsed_tree.copyTree(&mtree);
					collapsed_tree.collapseZeroBranches();
					collapsed_tree.printTree(out2);
					out2 << endl;
				}
				mtree.printTree(out);
				out << endl;
			}
			out.close();
			cout << params.repeated_time << " tree(s) printed to " << params.user_file << endl;
			if (params.num_zero_len) {
				out2.close();
				cout << params.repeated_time << " collapsed tree(s) printed to " << params.user_file << ".collapsed" << endl;
			}
		}
		// Generate random trees if optioned
		else if (params.tree_gen == CIRCULAR_SPLIT_GRAPH) {
			cout << "Generating random circular split network..." << endl;
			if (!overwriteFile(params.user_file)) return;
			sg.generateCircular(params);
		} else if (params.tree_gen == TAXA_SET) {
			sg.init(params);
			cout << "Generating random taxa set of size " << params.sub_size <<
				" overlap " << params.overlap << " with " << params.repeated_time << " times..." << endl;
			if (!overwriteFile(params.pdtaxa_file)) return;
			sg.generateTaxaSet(params.pdtaxa_file, params.sub_size, params.overlap, params.repeated_time);
		}
	} catch (bad_alloc) {
		outError(ERR_NO_MEMORY);
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, params.user_file);
	}

	// calculate the distance
	if (params.run_mode == CALC_DIST) {
		if (params.tree_gen == CIRCULAR_SPLIT_GRAPH) {
			cout << "Calculating distance matrix..." << endl;
			sg.calcDistance(params.dist_file);
			cout << "Distances printed to " << params.dist_file << endl;
		}// else {
			//mtree.calcDist(params.dist_file);
		//}
	}

}

inline void separator(ostream &out, int type = 0) {
	switch (type) {
	case 0:
		out << endl << "==============================================================================" << endl;
		break;
	case 1:
		out << endl << "-----------------------------------------------------------" << endl;
		break;
	default:
		break;
	}
}


void printCopyright(ostream &out) {
#ifdef IQ_TREE
 	out << "IQ-TREE beta version " << iqtree_VERSION_MAJOR << "." << iqtree_VERSION_MINOR << " built " << __DATE__;
#else
 	out << "PDA - Phylogenetic Diversity Analyzer version " << iqtree_VERSION_MAJOR << "." << iqtree_VERSION_MINOR << " built " << __DATE__;
#endif
#ifdef DEBUG
	out << " - debug mode";
#endif

#ifdef IQ_TREE
	out << endl << "Copyright (c) 2011 Nguyen Lam Tung, Bui Quang Minh, and Arndt von Haeseler." << endl << endl;
#else
	out << endl << "Copyright (c) 2006-2008 Bui Quang Minh, Steffen Klaere and Arndt von Haeseler." << endl << endl;
#endif
}

void printRunMode(ostream &out, RunMode run_mode) {
	switch (run_mode) {
		case DETECTED: out << "Detected"; break;
		case GREEDY: out << "Greedy"; break;
		case PRUNING: out << "Pruning"; break;
		case BOTH_ALG: out << "Greedy and Pruning"; break;
		case EXHAUSTIVE: out << "Exhaustive"; break;
		case DYNAMIC_PROGRAMMING: out << "Dynamic Programming"; break;
		case LINEAR_PROGRAMMING: out << "Linear Programming"; break;
		default: outError(ERR_INTERNAL);
	}
}

/**
	summarize the running with header
*/
void summarizeHeader(ostream &out, Params &params, bool budget_constraint, InputType analysis_type) {
	printCopyright(out);
	out << "Input file name: " << params.user_file << endl;
	out << "Input file format: " << ((params.intype == IN_NEWICK) ? "Newick" : ( (params.intype == IN_NEXUS) ? "Nexus" : "Unknown" )) << endl;
	if (params.initial_file != NULL)
		out << "Initial taxa file: " << params.initial_file << endl;
	if (params.param_file != NULL)
		out << "Parameter file: " << params.param_file << endl;
	out << endl;
	out << "Type of PD: " << ((params.root != NULL || params.is_rooted) ? "Rooted": "Unrooted");
	if (params.root != NULL) out << " at " << params.root;
	out << endl;
	if (params.run_mode != CALC_DIST && params.run_mode != PD_USER_SET) {
		out << "Search objective: " << ((params.find_pd_min) ? "Minimum" : "Maximum") << endl;
		out << "Search algorithm: ";
		printRunMode(out, params.run_mode);
		if (params.run_mode == DETECTED) {
			out << " -> ";
			printRunMode(out, params.detected_mode);
		}
		out << endl;
		out << "Search option: " << ((params.find_all) ? "Multiple optimal PD sets" : "Single optimal PD set") << endl;
	}
	out << endl;
	out << "Type of analysis: ";
	switch (params.run_mode) {
		case PD_USER_SET: out << "PD of user sets";
			if (params.pdtaxa_file) out << " (" << params.pdtaxa_file << ")"; break;
		case CALC_DIST: out << "Distance matrix computation"; break;
		default:
			out << ((budget_constraint) ? "Budget constraint " : "Subset size k ");
			if (params.intype == IN_NEWICK)
				out << ((analysis_type == IN_NEWICK) ? "on tree" : "on tree -> network");
			else
				out << "on network";
	}
	out << endl;
	//out << "Random number seed: " << params.ran_seed << endl;
}

void summarizeFooter(ostream &out, Params &params) {
	separator(out);
	time_t beginTime;
	time (&beginTime);
	char *date;
	date = ctime(&beginTime);

	out << "Time used: " << params.run_time  << " seconds." << endl;
	out << "Finished time: " << date << endl;
}


int getMaxNameLen(vector<string> &setName) {
	int len = 0;
	for (vector<string>::iterator it = setName.begin(); it != setName.end(); it++)
		if (len < (*it).length())
			len = (*it).length();
	return len;
}

void printPDUser(ostream &out, Params &params, PDRelatedMeasures &pd_more) {
	out << "List of user-defined sets of taxa with PD score computed" << endl << endl;
	int maxlen = getMaxNameLen(pd_more.setName)+2;
	out.width(maxlen);
	out << "Name" << "     PD";
	if (params.exclusive_pd) out << "   excl.-PD";
	if (params.endemic_pd) out << "   PD-Endem.";
	if (params.complement_area) out << "   PD-Compl. given area " << params.complement_area;
	out << endl;
	int cnt;
	for (cnt = 0; cnt < pd_more.setName.size(); cnt++) {
		out.width(maxlen);
		out << pd_more.setName[cnt] << " ";
		out.width(7);
		out << pd_more.PDScore[cnt] << "  ";
		if (params.exclusive_pd) {
			out.width(7);
			out << pd_more.exclusivePD[cnt] << "  ";
		}
		if (params.endemic_pd) {
			out.width(7);
			out << pd_more.PDEndemism[cnt] << "  ";
		}
		if (params.complement_area) {
			out.width(8);
			out << pd_more.PDComplementarity[cnt];
		}
		out << endl;
	}
	separator(out, 1);
}

void summarizeTree(Params &params, PDTree &tree, vector<PDTaxaSet> &taxa_set,
	PDRelatedMeasures &pd_more) {
	string filename;
	if (params.out_file == NULL) {
		filename = params.out_prefix;
		filename += ".pda";
	} else
		filename = params.out_file;

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename.c_str());

		summarizeHeader(out, params, false, IN_NEWICK);
		out << "Tree size: " << tree.leafNum-params.is_rooted << " taxa, " <<
			tree.nodeNum-1-params.is_rooted << " branches" << endl;
		separator(out);

		vector<PDTaxaSet>::iterator tid;

		if (params.run_mode == PD_USER_SET) {
			printPDUser(out, params, pd_more);
		}
		else if (taxa_set.size() > 1)
			out << "Optimal PD-sets with k = " << params.min_size-params.is_rooted <<
			" to " << params.sub_size-params.is_rooted << endl << endl;


		int subsize = params.min_size-params.is_rooted;
		if (params.run_mode == PD_USER_SET) subsize = 1;
		for (tid = taxa_set.begin(); tid != taxa_set.end(); tid++, subsize++) {
			if (tid != taxa_set.begin())
				separator(out, 1);
			if (params.run_mode == PD_USER_SET) {
				out << "Set " << subsize << " has PD score of " << tid->score << endl;
			}
			else {
				out << "For k = " << subsize << " the optimal PD score is " << (*tid).score << endl;
				out << "The optimal PD set has " << subsize << " taxa:" << endl;
			}
			for (NodeVector::iterator it = (*tid).begin(); it != (*tid).end(); it++)
				if ((*it)->name != ROOT_NAME){
					out << (*it)->name << endl;
				}
			if (!tid->tree_str.empty()) {
				out << endl << "Corresponding sub-tree: " << endl;
				out << tid->tree_str << endl;
			}
			tid->clear();
		}
		taxa_set.clear();

		summarizeFooter(out, params);
		out.close();
		cout << endl << "Results are summarized in " << filename << endl << endl;

	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}
}


void printTaxaSet(Params &params, vector<PDTaxaSet> &taxa_set, RunMode cur_mode) {
	int subsize = params.min_size-params.is_rooted;
	ofstream out;
	ofstream scoreout;
	string filename;
	filename = params.out_prefix;
	filename += ".score";
	scoreout.open(filename.c_str());
	if (!scoreout.is_open())
		outError(ERR_WRITE_OUTPUT, filename);
	cout << "PD scores printed to " << filename << endl;

	if (params.nr_output == 1) {
		filename = params.out_prefix;
		filename += ".pdtaxa";
		out.open(filename.c_str());
		if (!out.is_open())
			outError(ERR_WRITE_OUTPUT, filename);
	}
	for (vector<PDTaxaSet>::iterator tid = taxa_set.begin(); tid != taxa_set.end(); tid++, subsize++) {
		if (params.nr_output > 10) {
			filename = params.out_prefix;
			filename += ".";
			filename += subsize;
			if (params.run_mode == BOTH_ALG) {
				if (cur_mode == GREEDY)
					filename += ".greedy";
				else
					filename += ".pruning";
			} else {
				filename += ".pdtree";
			}
			(*tid).printTree((char*)filename.c_str());

			filename = params.out_prefix;
			filename += ".";
			filename += subsize;
			filename += ".pdtaxa";
			(*tid).printTaxa((char*)filename.c_str());
		} else {
			out << subsize << " " << (*tid).score << endl;
			scoreout << subsize << " " << (*tid).score << endl;
			(*tid).printTaxa(out);
		}
	}

	if (params.nr_output == 1) {
		out.close();
		cout << "All taxa list(s) printed to " << filename << endl;
	}

	scoreout.close();
}


/**
	run PD algorithm on trees
*/
void runPDTree(Params &params)
{

	if (params.run_mode == CALC_DIST) {
		bool is_rooted = false;
		MExtTree tree(params.user_file, is_rooted);
		cout << "Tree contains " << tree.leafNum << " taxa." << endl;
		cout << "Calculating distance matrix..." << endl;
		tree.calcDist(params.dist_file);
		cout << "Distances printed to " << params.dist_file << endl;
		return;
	}

	clock_t t_begin, t_end;
	//char filename[300];
	//int idx;

	vector<PDTaxaSet> taxa_set;

	if (params.run_mode == PD_USER_SET) {
		// compute score of user-defined sets
		t_begin = clock();
		cout << "Computing PD score for user-defined set of taxa..." << endl;
		PDTree tree(params);
		PDRelatedMeasures pd_more;
		tree.computePD(params, taxa_set, pd_more);

		if (params.endemic_pd)
			tree.calcPDEndemism(taxa_set, pd_more.PDEndemism);
		if (params.complement_area != NULL)
			tree.calcPDComplementarity(taxa_set, params.complement_area, pd_more.PDComplementarity);

		t_end = clock();
		params.run_time = (t_end-t_begin);
		summarizeTree(params, tree, taxa_set, pd_more);
		return;
	}


	/*********************************************
		run greedy algorithm
	*********************************************/

	if (params.sub_size < 2) {
		outError(ERR_NO_K);
	}

	bool detected_greedy = (params.run_mode != PRUNING);

	Greedy test_greedy;

	test_greedy.init(params);

	if (params.root == NULL && !params.is_rooted)
		cout << endl << "Running PD algorithm on UNROOTED tree..." << endl;
	else
		cout << endl << "Running PD algorithm on ROOTED tree..." << endl;

	if (verbose_mode >= VB_DEBUG)
		test_greedy.drawTree(cout, WT_INT_NODE + WT_BR_SCALE + WT_BR_LEN);

	if (params.run_mode == GREEDY || params.run_mode == BOTH_ALG ||
		(params.run_mode == DETECTED)) {

		if (params.run_mode == DETECTED && params.sub_size >= test_greedy.leafNum * 7 / 10
			&& params.min_size < 2)
			detected_greedy = false;

		if (detected_greedy) {
			params.detected_mode = GREEDY;
			t_begin=clock();
			cout << endl << "Greedy Algorithm..." << endl;

			taxa_set.clear();
			test_greedy.run(params, taxa_set);

			t_end=clock();
			params.run_time = (t_end-t_begin);
			printf("Time used: %8.6f seconds.\n", (double)params.run_time / CLOCKS_PER_SEC);
			if (params.min_size == params.sub_size)
				printf("Resulting tree length = %10.4f\n", taxa_set[0].score);

			if (params.nr_output > 0)
				printTaxaSet(params, taxa_set, GREEDY);

			PDRelatedMeasures pd_more;

			summarizeTree(params, test_greedy, taxa_set, pd_more);
		}
	}

	/*********************************************
		run pruning algorithm
	*********************************************/
	if (params.run_mode == PRUNING || params.run_mode == BOTH_ALG ||
		(params.run_mode == DETECTED)) {

		Pruning test_pruning;

		if (params.run_mode == PRUNING || params.run_mode == BOTH_ALG) {
			//Pruning test_pruning(params);
			test_pruning.init(params);
		} else if (!detected_greedy) {
			test_pruning.init(test_greedy);
		} else {
			return;
		}
		params.detected_mode = PRUNING;
		t_begin=clock();
		cout << endl << "Pruning Algorithm..." << endl;
		taxa_set.clear();
		test_pruning.run(params, taxa_set);

		t_end=clock();
		params.run_time = (t_end-t_begin) ;
		printf("Time used: %8.6f seconds.\n", (double)params.run_time / CLOCKS_PER_SEC);
		if (params.min_size == params.sub_size)
			printf("Resulting tree length = %10.4f\n", taxa_set[0].score);

		if (params.nr_output > 0)
			printTaxaSet(params, taxa_set, PRUNING);

		PDRelatedMeasures pd_more;

		summarizeTree(params, test_pruning, taxa_set, pd_more);

	}

}

void checkSplitDistance(ostream &out, PDNetwork &sg) {
	matrix(double) dist;
	sg.calcDistance(dist);
	int ntaxa = sg.getNTaxa();
	int i, j;
	bool found = false;
	for (i = 0; i < ntaxa-1; i++) {
		bool first = true;
		for (j = i+1; j < ntaxa; j++)
			if (abs(dist[i][j]) <= 1e-5) {
				if (!found) {
					out << "The following sets of taxa (each set in a line) have very small split-distance" << endl;
					out << "( <= 1e-5) as computed from the split system. To avoid a lot of multiple" << endl;
					out << "optimal PD sets to be reported, one should only keep one taxon from each set" << endl;
					out << "and exclude the rest from the analysis." << endl << endl;
				}
				if (first)
					out << sg.getTaxa()->GetTaxonLabel(i);
				found = true;
				first = false;
				out << ", " << sg.getTaxa()->GetTaxonLabel(j);
			}
		if (!first) out << endl;
	}
	if (found)
		separator(out);
}



/**
	check if the set are nested and there are no multiple optimal sets.
	If yes, return the ranking as could be produced by a greedy algorithm
*/
bool makeRanking(vector<SplitSet> &pd_set, IntVector &indices, IntVector &ranking) {
	vector<SplitSet>::iterator it;
	IntVector::iterator inti;
	ranking.clear();
	bool nested = true;
	Split *cur_sp = NULL;
	int id = 1;
	for (it = pd_set.begin(); it != pd_set.end(); it++) {
		if ((*it).empty()) continue;
		if ((*it).size() > 1) {
			nested = false;
			ranking.push_back(-10);
			indices.push_back(0);
		}
		Split *sp = (*it)[0];

		if (!cur_sp) {
			IntVector sp_tax;
			sp->getTaxaList(sp_tax);
			ranking.insert(ranking.end(), sp_tax.begin(), sp_tax.end());
			for (inti = sp_tax.begin(); inti != sp_tax.end(); inti++)
				indices.push_back(id++);
		} else {
			if ( !cur_sp->subsetOf(*sp)) {
				ranking.push_back(-1);
				indices.push_back(0);
				nested = false;
			}
			Split sp_diff(*sp);
			sp_diff -= *cur_sp;
			Split sp_diff2(*cur_sp);
			sp_diff2 -= *sp;
			IntVector sp_tax;
			sp_diff2.getTaxaList(sp_tax);
			ranking.insert(ranking.end(), sp_tax.begin(), sp_tax.end());
			for (inti = sp_tax.begin(); inti != sp_tax.end(); inti++)
				indices.push_back(-id);
			sp_diff.getTaxaList(sp_tax);
			ranking.insert(ranking.end(), sp_tax.begin(), sp_tax.end());
			for (inti = sp_tax.begin(); inti != sp_tax.end(); inti++)
				indices.push_back(id);
			if ( !cur_sp->subsetOf(*sp)) {
				ranking.push_back(-2);
				indices.push_back(0);
			}
			id++;
		}
		cur_sp = sp;
	}
	return nested;
}


void printNexusSets(const char *filename, PDNetwork &sg, vector<SplitSet> &pd_set) {
	try {
		ofstream out;
		out.open(filename);
		out << "#NEXUS" << endl << "BEGIN Sets;" << endl;
		vector<SplitSet>::iterator it;
		for (it = pd_set.begin(); it != pd_set.end(); it++) {
			int id = 1;
			for (SplitSet::iterator sit = (*it).begin(); sit != (*it).end(); sit++, id++) {
				IntVector taxa;
				(*sit)->getTaxaList(taxa);
				out << "   TAXSET Opt_" << taxa.size() << "_" << id << " =";
				for (IntVector::iterator iit = taxa.begin(); iit != taxa.end(); iit++) {
					if (sg.isPDArea())
						out << " '" << sg.getSetsBlock()->getSet(*iit)->name << "'";
					else
						out << " '" << sg.getTaxa()->GetTaxonLabel(*iit) << "'";
				}
				out << ";" << endl;
			}
		}
		out << "END; [Sets]" << endl;
		out.close();
		cout << endl << "Optimal sets are written to nexus file " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}

}



void computeTaxaFrequency(SplitSet &taxa_set, DoubleVector &freq) {
	assert(taxa_set.size());
	int ntaxa = taxa_set[0]->getNTaxa();
	int i;

	freq.resize(ntaxa, 0);
	for (SplitSet::iterator it2 = taxa_set.begin(); it2 != taxa_set.end(); it2++) {
		for ( i = 0; i < ntaxa; i++)
			if ((*it2)->containTaxon(i)) freq[i] += 1.0;
	}

	for ( i = 0; i < ntaxa; i++)
		freq[i] /= taxa_set.size();

}

/**
	summarize the running results
*/
void summarizeSplit(Params &params, PDNetwork &sg, vector<SplitSet> &pd_set, PDRelatedMeasures &pd_more, bool full_report) {
	int i;


	if (params.nexus_output) {
		string nex_file = params.out_prefix;
		nex_file += ".pdsets.nex";
		printNexusSets(nex_file.c_str(), sg, pd_set);
	}
	string filename;
	if (params.out_file == NULL) {
		filename = params.out_prefix;
		filename += ".pda";
	} else
		filename = params.out_file;

	try {
		ofstream out;
		out.open(filename.c_str());
		/****************************/
		/********** HEADER **********/
		/****************************/
		summarizeHeader(out, params, sg.isBudgetConstraint(), IN_NEXUS);

		out << "Network size: " << sg.getNTaxa()-params.is_rooted << " taxa, " <<
			sg.getNSplits()-params.is_rooted << " splits (of which " <<
			sg.getNTrivialSplits() << " are trivial splits)" << endl;
		out << "Network type: " << ((sg.isCircular()) ? "Circular" : "General") << endl;

		separator(out);

		checkSplitDistance(out, sg);

		int c_num = 0;
		//int subsize = (sg.isBudgetConstraint()) ? params.budget : (params.sub_size-params.is_rooted);
		//subsize -= pd_set.size()-1;
		int subsize = (sg.isBudgetConstraint()) ? params.min_budget : params.min_size-params.is_rooted;
		int stepsize = (sg.isBudgetConstraint()) ? params.step_budget : params.step_size;
		if (params.detected_mode != LINEAR_PROGRAMMING) stepsize = 1;
		vector<SplitSet>::iterator it;
		SplitSet::iterator it2;


		if (params.run_mode == PD_USER_SET) {
			printPDUser(out, params, pd_more);
		}

		/****************************/
		/********** SUMMARY *********/
		/****************************/

		if (params.run_mode != PD_USER_SET && !params.num_bootstrap_samples) {
			out << "Summary of the PD-score and the number of optimal PD-sets with the same " << endl << "optimal PD-score found." << endl;

			if (sg.isBudgetConstraint())
				out << endl << "Budget   PD-score   %PD-score   #PD-sets" << endl;
			else
				out << endl << "Size-k   PD-score   %PD-score   #PD-sets" << endl;

			int sizex = subsize;
			double total = sg.calcWeight();

			for (it = pd_set.begin(); it != pd_set.end(); it++, sizex+=stepsize) {
				out.width(6);
				out << right << sizex << " ";
				out.width(10);
				out << right << (*it).getWeight() << " ";
				out.width(10);
				out << right << ((*it).getWeight()/total)*100.0 << " ";
				out.width(6);
				out << right << (*it).size();
				out << endl;
			}

			out << endl;
			if (!params.find_all)
				out << "Note: You did not choose the option to find multiple optimal PD sets." << endl <<
					"That's why we only reported one PD-set per size-k or budget. If you want" << endl <<
					"to determine all multiple PD-sets, use the '-a' option.";
			else {
				out << "Note: The number of multiple optimal PD sets to be reported is limited to " << params.pd_limit << "." << endl <<
					"There might be cases where the actual #PD-sets exceeds that upper-limit but" << endl <<
					"won't be listed here. Please refer to the above list to identify such cases." << endl <<
					"To increase the upper-limit, use the '-lim <limit_number>' option.";
			}
			out << endl;
			separator(out);
		}

		if (!full_report) {
			out.close();
			return;
		}


		/****************************/
		/********* BOOTSTRAP ********/
		/****************************/
		if (params.run_mode != PD_USER_SET && params.num_bootstrap_samples) {
			out << "Summary of the bootstrap analysis " << endl;
			for (it = pd_set.begin(); it != pd_set.end(); it++) {
				DoubleVector freq;
				computeTaxaFrequency((*it), freq);
				out << "For k/budget = " << subsize << " the " << ((sg.isPDArea()) ? "areas" : "taxa")
					<< " supports are: " << endl;
				for (i = 0; i < freq.size(); i++)
					out << ((sg.isPDArea()) ? sg.getSetsBlock()->getSet(i)->name : sg.getTaxa()->GetTaxonLabel(i))
						<< "\t" << freq[i] << endl;
				if ((it+1) != pd_set.end()) separator(out, 1);
			}
			out << endl;
			separator(out);
		}

		/****************************/
		/********** RANKING *********/
		/****************************/

		if (params.run_mode != PD_USER_SET && !params.num_bootstrap_samples) {


			IntVector ranking;
			IntVector index;

			out << "Ranking based on the optimal sets" << endl;


			if (!makeRanking(pd_set, index, ranking)) {
				out << "WARNING: Optimal sets are not nested, so ranking should not be considered stable" << endl;
			}
			if (subsize > 1) {
				out << "WARNING: The first " << subsize << " ranks should be treated equal" << endl;
			}
			out << endl << "Rank*   ";
			if (!sg.isPDArea())
				out << "Taxon names" << endl;
			else
				out << "Area names" << endl;


			for (IntVector::iterator intv = ranking.begin(), intid = index.begin(); intv != ranking.end(); intv ++, intid++) {
				if (*intv == -10)
					out << "<--- multiple optimal set here --->" << endl;
				else if (*intv == -1)
					out << "<--- BEGIN: greedy does not work --->" << endl;
				else if (*intv == -2)
					out << "<--- END --->" << endl;
				else {
					out.width(5);
					out <<  right << *intid << "   ";
					if (sg.isPDArea())
						out << sg.getSetsBlock()->getSet(*intv)->name << endl;
					else
						out << sg.getTaxa()->GetTaxonLabel(*intv) << endl;
				}
			}
			out << endl;
			out <<  "(*) Negative ranks indicate the point at which the greedy algorithm" << endl <<
					"    does not work. In that case, the corresponding taxon/area names" << endl <<
					"    should be deleted from the optimal set of the same size" << endl;
			separator(out);
		}

		int max_len = sg.getTaxa()->GetMaxTaxonLabelLength();

		/****************************/
		/***** DETAILED SETS ********/
		/****************************/

		if (params.run_mode != PD_USER_SET)
			out << "Detailed information of all taxa found in the optimal PD-sets" << endl;

		if (pd_set.size() > 1) {
			if (sg.isBudgetConstraint())
				out << "with budget = " << params.min_budget <<
					" to " << params.budget << endl << endl;
			else
				out << "with k = " << params.min_size-params.is_rooted <<
					" to " << params.sub_size-params.is_rooted << endl << endl;
		}

		if (params.run_mode != PD_USER_SET)
			separator(out,1);

		for (it = pd_set.begin(); it != pd_set.end(); it++, subsize+=stepsize) {

			// check if the pd-sets are the same as previous one
			if (sg.isBudgetConstraint() && it != pd_set.begin()) {
				vector<SplitSet>::iterator prev, next;
				for (next=it, prev=it-1; next != pd_set.end() && next->getWeight() == (*prev).getWeight() &&
					next->size() == (*prev).size(); next++ ) ;
				if (next != it) {
					// found something in between!
					out << endl;
					//out << endl << "**************************************************************" << endl;
					out << "For budget = " << subsize << " -> " << subsize+(next-it-1)*stepsize <<
						" the optimal PD score and PD sets" << endl;
					out << "are identical to the case when budget = " << subsize-stepsize << endl;
					//out << "**************************************************************" << endl;
					subsize += (next-it)*stepsize;
					it = next;
					if (it == pd_set.end()) break;
				}
			}

			if (it != pd_set.begin()) separator(out, 1);

			int num_sets = (*it).size();
			double weight = (*it).getWeight();

			if (params.run_mode != PD_USER_SET) {
				out << "For " << ((sg.isBudgetConstraint()) ? "budget" : "k") << " = " << subsize;
				out << " the optimal PD score is " << weight << endl;

				if (num_sets == 1) {
					if (!sg.isBudgetConstraint())
						out << "The optimal PD set has " << (*it)[0]->countTaxa()-params.is_rooted <<
							((sg.isPDArea()) ? " areas" : " taxa");
					else
						out << "The optimal PD set has " << (*it)[0]->countTaxa()-params.is_rooted <<
						((sg.isPDArea()) ? " areas" : " taxa") << " and requires " << sg.calcCost(*(*it)[0]) << " budget";
					if (!sg.isPDArea()) out << " and covers " << sg.countSplits(*(*it)[0]) <<
						" splits (of which " << sg.countInternalSplits(*(*it)[0]) << " are internal splits)";
					out << endl;
				}
				else
					out << "Found " << num_sets << " PD sets with the same optimal score." << endl;
			}
			for (it2 = (*it).begin(), c_num=1; it2 != (*it).end(); it2++, c_num++){
				Split *this_set = *it2;

				if (params.run_mode == PD_USER_SET && it2 != (*it).begin())
					separator(out, 1);

				if (params.run_mode == PD_USER_SET) {
					if (!sg.isBudgetConstraint())
						out << "Set " << c_num << " has PD score of " << this_set->getWeight();
					else
						out << "Set " << c_num << " has PD score of " << this_set->getWeight() <<
						" and requires " << sg.calcCost(*this_set) << " budget";
				} else if (num_sets > 1) {
					if (!sg.isBudgetConstraint())
						out << endl << "PD set " << c_num;
					else
						out << endl << "PD set " << c_num << " has " << this_set->countTaxa()-params.is_rooted <<
						" taxa and requires " << sg.calcCost(*this_set) << " budget";
				}

				if (!sg.isPDArea() && (num_sets > 1 || params.run_mode == PD_USER_SET ))
					out << " and covers " << sg.countSplits(*(*it)[0]) << " splits (of which "
					<< sg.countInternalSplits(*(*it)[0]) << " are internal splits)";
				out << endl;

				if (params.run_mode != PD_USER_SET && sg.isPDArea()) {
					for (i = 0; i < sg.getSetsBlock()->getNSets(); i++)
						if (this_set->containTaxon(i)) {
							if (sg.isBudgetConstraint()) {
								out.width(max_len);
								out << left << sg.getSetsBlock()->getSet(i)->name << "\t";
								out.width(10);
								out << right << sg.getPdaBlock()->getCost(i);
								out << endl;

							} else {
								out << sg.getSetsBlock()->getSet(i)->name << endl;
							}
						}

					Split sp(sg.getNTaxa());
					for (i = 0; i < sg.getSetsBlock()->getNSets(); i++)
						if (this_set->containTaxon(i))
							sp += *(sg.area_taxa[i]);
					out << endl << "which contains " << sp.countTaxa() - params.is_rooted << " taxa: " << endl;
					for (i = 0; i < sg.getNTaxa(); i++)
						if (sg.getTaxa()->GetTaxonLabel(i) != ROOT_NAME && sp.containTaxon(i))
							out << sg.getTaxa()->GetTaxonLabel(i) << endl;

				} else
				for ( i = 0; i < sg.getNTaxa(); i++)
					if (sg.getTaxa()->GetTaxonLabel(i) != ROOT_NAME && this_set->containTaxon(i)) {
						if (sg.isBudgetConstraint()) {
							out.width(max_len);
							out << left << sg.getTaxa()->GetTaxonLabel(i) << "\t";
							out.width(10);
							out << right << sg.getPdaBlock()->getCost(i);
							out << endl;

						} else {
							out << sg.getTaxa()->GetTaxonLabel(i) << endl;
						}
					}
			}
		}

		/****************************/
		/********** FOOTER **********/
		/****************************/

		summarizeFooter(out, params);

		out.close();
		cout << endl << "Results are summarized in " << filename << endl << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}
}

void printGainMatrix(char *filename, matrix(double) &delta_gain, int start_k) {
	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename);
		int k = start_k;
		for (matrix(double)::iterator it = delta_gain.begin(); it != delta_gain.end(); it++, k++) {
			out << k;
			for (int i = 0; i < (*it).size(); i++)
				out << "  " << (*it)[i];
			out << endl;
		}
		out.close();
		cout << "PD gain matrix printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}
}

/**
	run PD algorithm on split networks
*/
void runPDSplit(Params &params) {

	cout << "Using NCL - Nexus Class Library" << endl << endl;

	// init a split graph class from the parameters
	CircularNetwork sg(params);
	int i;

	// this vector of SplitSet store all the optimal PD sets
	vector<SplitSet> pd_set;
	// this define an order of taxa (circular order in case of circular networks)
	vector<int> taxa_order;
	// this store a particular taxa set
	Split taxa_set;


	if (sg.isCircular()) {
		// is a circular network, get circular order
		for (i = 0; i < sg.getNTaxa(); i++)
			taxa_order.push_back(sg.getCircleId(i));
	} else
		// otherwise, get the incremental order
		for (i = 0; i < sg.getNTaxa(); i++)
			taxa_order.push_back(i);

	PDRelatedMeasures pd_more;

	// begining time of the algorithm run
	time_t time_begin = clock();
	time(&time_begin);
	// check parameters
	if (sg.isPDArea()) {
		if (sg.isBudgetConstraint()) {
			int budget = (params.budget >= 0) ? params.budget : sg.getPdaBlock()->getBudget();
			if (budget < 0 && params.pd_proportion == 0.0) params.run_mode = PD_USER_SET;
		} else {
			int sub_size = (params.sub_size >= 1) ? params.sub_size : sg.getPdaBlock()->getSubSize();
			if (sub_size < 1 && params.pd_proportion == 0.0) params.run_mode = PD_USER_SET;

		}
	}

	if (params.run_mode == PD_USER_SET) {
		// compute score of user-defined sets
		cout << "Computing PD score for user-defined set of taxa..." << endl;
		pd_set.resize(1);
		sg.computePD(params, pd_set[0], pd_more);
		if (params.endemic_pd)
			sg.calcPDEndemism(pd_set[0], pd_more.PDEndemism);

		if (params.complement_area != NULL)
			sg.calcPDComplementarity(pd_set[0], params.complement_area, pd_more.setName, pd_more.PDComplementarity);

	} else {
		// otherwise, call the main function
		if (params.num_bootstrap_samples) {
			cout << endl << "======= START BOOTSTRAP ANALYSIS =======" << endl;
			MTreeSet *mtrees = sg.getMTrees();
			if (mtrees->size() < 100)
				cout << "Warning: bootstrap may be unstable with less than 100 trees" << endl;
			vector<string> taxname;
			sg.getTaxaName(taxname);
			i = 1;
			for (MTreeSet::iterator it = mtrees->begin(); it != mtrees->end(); it++, i++) {
				cout << "---------- TREE " << i << " ----------" << endl;
				// convert tree into split sytem
				SplitGraph sg2;
				(*it)->convertSplits(taxname, sg2);
				// change the current split system
				for (SplitGraph::reverse_iterator it = sg.rbegin(); it != sg.rend(); it++) {
					delete *it;
				}
				sg.clear();
				sg.insert(sg.begin(), sg2.begin(), sg2.end());
				sg2.clear();

				// now findPD on the converted tree-split system
				sg.findPD(params, pd_set, taxa_order);
			}
			cout << "======= DONE BOOTSTRAP ANALYSIS =======" << endl << endl;
		} else {
			sg.findPD(params, pd_set, taxa_order);
		}
	}

	// ending time
	time_t time_end;
	time(&time_end);
	params.run_time = difftime(time_end, time_begin);

	cout << "Time used: " << (double) (params.run_time) << " seconds." << endl;

	if (verbose_mode >= VB_DEBUG && !sg.isPDArea()) {
		cout << "PD set(s) with score(s): " << endl;
		for (vector<SplitSet>::iterator it = pd_set.begin(); it != pd_set.end(); it++)
		for (SplitSet::iterator it2 = (*it).begin(); it2 != (*it).end(); it2++ ){
			//(*it)->report(cout);
			cout << "  " << (*it2)->getWeight() << "    ";
			for (i = 0; i < sg.getNTaxa(); i++)
				if ((*it2)->containTaxon(i))
				cout << sg.getTaxa()->GetTaxonLabel(i) << "  ";
			if (sg.isBudgetConstraint())
				cout << " (budget = " << sg.calcCost(*(*it2)) << ")";
			cout << endl;
		}
	}

	sg.printOutputSetScore(params, pd_set);


	summarizeSplit(params, sg, pd_set, pd_more, true);

	if (params.calc_pdgain) {
		matrix(double) delta_gain;
		sg.calcPDGain(pd_set, delta_gain);
		string filename = params.out_prefix;
		filename += ".pdgain";
		printGainMatrix((char*)filename.c_str(), delta_gain, pd_set.front().front()->countTaxa());
		//cout << delta_gain;
	}


	//for (i = pd_set.size()-1; i >= 0; i--)
	//	delete pd_set[i];

}

void printSplitSet(SplitGraph &sg, SplitIntMap &hash_ss) {
/*
	for (SplitIntMap::iterator it = hash_ss.begin(); it != hash_ss.end(); it++) {
		if ((*it)->getWeight() > 50 && (*it)->countTaxa() > 1)
		(*it)->report(cout);
	}*/
	sg.getTaxa()->Report(cout);
	for (SplitGraph::iterator it = sg.begin(); it != sg.end(); it++) {
		if ((*it)->getWeight() > 50 && (*it)->countTaxa() > 1)
		(*it)->report(cout);
	}
}

void readTaxaOrder(char *taxa_order_file, StrVector &taxa_order) {

}

void calcTreeCluster(Params &params) {
	assert(params.taxa_order_file);
	MExtTree tree(params.user_file, params.is_rooted);
//	StrVector taxa_order;
	//readTaxaOrder(params.taxa_order_file, taxa_order);
	NodeVector taxa;
	matrix(int) clusters;
	clusters.reserve(tree.leafNum - 3);
	tree.getTaxa(taxa);
	sort(taxa.begin(), taxa.end(), nodenamecmp);
	tree.createCluster(taxa, clusters);
	int cnt = 1;

	string treename = params.out_prefix;
	treename += ".clu-id";
	tree.printTree(treename.c_str());

	for (matrix(int)::iterator it = clusters.begin(); it != clusters.end(); it++, cnt++) {
		ostringstream filename;
		filename << params.out_prefix << "." << cnt << ".clu";
		ofstream out(filename.str().c_str());

		ostringstream filename2;
		filename2 << params.out_prefix << "." << cnt << ".name-clu";
		ofstream out2(filename2.str().c_str());

		out << "w" << endl << "c" << endl << "4" << endl << "b" << endl << "g" << endl << 4-params.is_rooted << endl;
		IntVector::iterator it2;
		NodeVector::iterator it3;
		for (it2 = (*it).begin(), it3 = taxa.begin(); it2 != (*it).end(); it2++, it3++)
			if ((*it3)->name != ROOT_NAME) {
				out << char((*it2)+'a') << endl;
				out2 << (*it3)->name << "  " << char((*it2)+'a') << endl;
			}
		out << "y" << endl;
		out.close();
		out2.close();
		cout << "Cluster " << cnt << " printed to " << filename << " and " << filename2 << endl;
	}
}


void printTaxa(Params &params) {
	MTree mytree(params.user_file, params.is_rooted);
	vector<string> taxname;
	taxname.resize(mytree.leafNum);
	mytree.getTaxaName(taxname);
	sort(taxname.begin(), taxname.end());

	string filename = params.out_prefix;
	filename += ".taxa";

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename.c_str());
		for (vector<string>::iterator it = taxname.begin(); it != taxname.end(); it++) {
			if ((*it) != ROOT_NAME) out << (*it);
			out << endl;
		}
		out.close();
		cout << "All taxa names printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}
}

void printAreaList(Params &params) {
	MSetsBlock *sets;
	sets = new MSetsBlock();
 	cout << "Reading input file " << params.user_file << "..." << endl;

	MyReader nexus(params.user_file);

	nexus.Add(sets);

	MyToken token(nexus.inf);
	nexus.Execute(token);

	//sets->Report(cout);

	TaxaSetNameVector *allsets = sets->getSets();

	string filename = params.out_prefix;
	filename += ".names";

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename.c_str());
		for (TaxaSetNameVector::iterator it = allsets->begin(); it != allsets->end(); it++) {
			out << (*it)->name;
			out << endl;
		}
		out.close();
		cout << "All area names printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}

	delete sets;
}

void scaleBranchLength(Params &params) {
	params.is_rooted = true;
	PDTree tree(params);
	if (params.run_mode == SCALE_BRANCH_LEN) {
		cout << "Scaling branch length with a factor of " << params.scaling_factor << " ..." << endl;
		tree.scaleLength(params.scaling_factor, false);
	} else {
		cout << "Scaling clade support with a factor of " << params.scaling_factor << " ..." << endl;
		tree.scaleCladeSupport(params.scaling_factor, false);
	}
	if (params.out_file != NULL)
		tree.printTree(params.out_file);
	else {
		tree.printTree(cout);
		cout << endl;
	}
}

void calcDistribution(Params &params) {

	PDTree mytree(params);

	string filename = params.out_prefix;
	filename += ".randompd";

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename.c_str());
		for (int size = params.min_size; size <= params.sub_size; size += params.step_size) {
			out << size;
			for (int sample = 0; sample < params.sample_size; sample++) {
				Split taxset(mytree.leafNum);
				taxset.randomize(size);
				mytree.calcPD(taxset);
				out << "  " << taxset.getWeight();
			}
			out << endl;
		}
		out.close();
		cout << "PD distribution is printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}
}

void printRFDist(ostream &out, int *rfdist, int n, int m, int rf_dist_mode) {
	int i, j;
	out << n << endl;
	if (rf_dist_mode == RF_ADJACENT_PAIR) {
		out << "XXX        ";
		for (i = 0; i < n; i++)
			out << " " << rfdist[i];
		out << endl;
	} else {
		// all pairs
		for (i = 0; i < n; i++)  {
			out << "Tree" << i << "      ";
			for (j = 0; j < m; j++)
				out << " " << rfdist[i*m+j];
			out << endl;
		}
	}
}

void computeRFDist(Params &params) {

	if (!params.user_file) outError("User tree file not provided");

	string filename = params.out_prefix;
	filename += ".rfdist";
	MTreeSet trees(params.user_file, params.is_rooted, params.tree_burnin);
	int n = trees.size(), m = trees.size();
	int *rfdist;
	int *incomp_splits = NULL;
	string infoname = params.out_prefix;
	infoname += ".rfinfo";
	string treename = params.out_prefix;
	treename += ".rftree";
	if (params.rf_dist_mode == RF_TWO_TREE_SETS) {
		MTreeSet treeset2(params.second_tree, params.is_rooted, params.tree_burnin);
		cout << "Computing Robinson-Foulds distances between two sets of trees" << endl;
		m = treeset2.size();
		rfdist = new int [n*m];
		memset(rfdist, 0, n*m* sizeof(int));
		if (verbose_mode >= VB_MAX) {
			incomp_splits = new int [n*m];
			memset(incomp_splits, 0, n*m* sizeof(int));
		}
		if (verbose_mode >= VB_MED)
			trees.computeRFDist(rfdist, &treeset2, infoname.c_str(),treename.c_str(), incomp_splits);
		else
			trees.computeRFDist(rfdist, &treeset2);
	} else {
		rfdist = new int [n*n];
		memset(rfdist, 0, n*n* sizeof(int));
		trees.computeRFDist(rfdist, params.rf_dist_mode, params.split_weight_threshold);
	}

	if (verbose_mode >= VB_MED) printRFDist(cout, rfdist, n, m, params.rf_dist_mode);

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename.c_str());
		printRFDist(out, rfdist, n, m, params.rf_dist_mode);
		out.close();
		cout << "Robinson-Foulds distances printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}

	if (incomp_splits)
	try {
		filename = params.out_prefix;
		filename += ".incomp";
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(filename.c_str());
		printRFDist(out, incomp_splits, n, m, params.rf_dist_mode);
		out.close();
		cout << "Number of incompatible splits in printed to " << filename << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, filename);
	}

	if (incomp_splits) delete [] incomp_splits;
	delete [] rfdist;
}


void testInputFile(Params &params) {
	SplitGraph sg(params);
	if (sg.isWeaklyCompatible())
		cout << "The split system is weakly compatible." << endl;
	else
		cout << "The split system is NOT weakly compatible." << endl;

}

/**MINH ANH: for some statistics about the branches on the input tree*/
void branchStats(Params &params){
	MaTree mytree(params.user_file, params.is_rooted);
	mytree.drawTree(cout,WT_TAXON_ID + WT_INT_NODE);
	//report to output file
	string output;
	if (params.out_file)
		output = params.out_file;
	else {
		if (params.out_prefix)
			output = params.out_prefix;
		else
			output = params.user_file;
		output += ".stats";
	}

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(output.c_str());
		mytree.printBrInfo(out);
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, output);
	}
	cout << "Information about branch lengths of the tree is printed to: " << output << endl;
	
	/***** Following added by BQM to print internal branch lengths */
	NodeVector nodes1, nodes2;
	mytree.getInternalBranches(nodes1, nodes2);
	output = params.out_prefix;
	output += ".inlen";
	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(output.c_str());
		for (int i = 0; i < nodes1.size(); i++)
			out << nodes1[i]->findNeighbor(nodes2[i])->length << " ";
		out << endl;
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, output);
	}
	cout << "Internal branch lengths printed to: " << output << endl;
}

/**MINH ANH: for comparison between the input tree and each tree in a given set of trees*/
void compare(Params &params){
	MaTree mytree(params.second_tree, params.is_rooted);
	//sort taxon names and update nodeID, to be consistent with MTreeSet
	NodeVector taxa;
	mytree.getTaxa(taxa);
	sort(taxa.begin(), taxa.end(), nodenamecmp);
	int i;
	NodeVector::iterator it;
	for (it = taxa.begin(), i = 0; it != taxa.end(); it++, i++)
			(*it)->id = i;

	string drawFile = params.second_tree;
	drawFile += ".draw";
	try {
		ofstream out1;
		out1.exceptions(ios::failbit | ios::badbit);
		out1.open(drawFile.c_str());
		mytree.drawTree(out1,WT_TAXON_ID + WT_INT_NODE);
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, drawFile);
	}
	cout << "Tree with branchID (nodeID) was printed to: " << drawFile << endl;


	MTreeSet trees(params.user_file,params.is_rooted, params.tree_burnin);
	DoubleMatrix brMatrix;
	DoubleVector BSDs;
	IntVector RFs;
	mytree.comparedTo(trees, brMatrix, RFs, BSDs);
	int numTree = trees.size();
	int numNode = mytree.nodeNum;

	string output;
	if (params.out_file)
		output = params.out_file;
	else {
		if (params.out_prefix)
			output = params.out_prefix;
		else
			output = params.user_file;
		output += ".compare";
	}

	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(output.c_str());
		//print the header
		out << "tree  " ;
		for (int nodeID = 0; nodeID < numNode; nodeID++ )
			if ( brMatrix[0][nodeID] != -2 )
				out << "br_" << nodeID << "  ";
		out << "RF  BSD" << endl;
		for ( int treeID = 0; treeID < numTree; treeID++ )
		{
			out << treeID << "  ";
			for (int nodeID = 0; nodeID < numNode; nodeID++ )
				if ( brMatrix[treeID][nodeID] != -2 )
					out << brMatrix[treeID][nodeID] << "  ";
			out << RFs[treeID] << "  " << BSDs[treeID] << endl;
		}
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, output);
	}
	cout << "Comparison with the given set of trees is printed to: " << output << endl;
}

/**MINH ANH: to compute 'guided bootstrap' alignment*/
void guidedBootstrap(Params &params)
{
	MaAlignment inputAlign(params.aln_file,params.sequence_type, params.intype);
	inputAlign.readLogLL(params.siteLL_file);

	string outFre_name = params.out_prefix;
    outFre_name += ".patInfo";

	inputAlign.printPatObsExpFre(outFre_name.c_str());

	string gboAln_name = params.out_prefix;
    gboAln_name += ".gbo";

	MaAlignment gboAlign;
	double prob;
	gboAlign.generateExpectedAlignment(&inputAlign, prob);
	gboAlign.printPhylip(gboAln_name.c_str());


	string outProb_name = params.out_prefix;
	outProb_name += ".gbo.logP";
	try {
		ofstream outProb;
		outProb.exceptions(ios::failbit | ios::badbit);
		outProb.open(outProb_name.c_str());
		outProb.precision(10);
		outProb << prob << endl;
		outProb.close();
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, outProb_name);
	}

	cout << "Information about patterns in the input alignment is printed to: " << outFre_name << endl;
	cout << "A 'guided bootstrap' alignment is printed to: " << gboAln_name << endl;
	cout << "Log of the probability of the new alignment is printed to: " << outProb_name << endl;
}

/**MINH ANH: to compute the probability of an alignment given the multinomial distribution of patterns frequencies derived from a reference alignment*/
void computeMulProb(Params &params)
{
	Alignment refAlign(params.second_align, params.sequence_type, params.intype);
	Alignment inputAlign(params.aln_file, params.sequence_type, params.intype);
	double prob;
	inputAlign.multinomialProb(refAlign,prob);
	//Printing
	string outProb_name = params.out_prefix;
	outProb_name += ".mprob";
	try {
		ofstream outProb;
		outProb.exceptions(ios::failbit | ios::badbit);
		outProb.open(outProb_name.c_str());
		outProb.precision(10);
		outProb << prob << endl;
		outProb.close();
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, outProb_name);
	}
	cout << "Probability of alignment " << params.aln_file << " given alignment " << params.second_align << " is: " << prob << endl;
	cout << "The probability is printed to: " << outProb_name << endl;
}

void processNCBITree(Params &params) {
	NCBITree tree;
	Node *dad = tree.readNCBITree(params.user_file, params.ncbi_taxid, params.ncbi_taxon_level, params.ncbi_ignore_level);
	if (params.ncbi_names_file) tree.readNCBINames(params.ncbi_names_file);

	cout << "Dad ID: " << dad->name << " Root ID: " << tree.root->name << endl;
	string str = params.user_file;
	str += ".tree";
	if (params.out_file) str = params.out_file;
	//tree.printTree(str.c_str(), WT_SORT_TAXA | WT_BR_LEN);
	cout << "NCBI tree printed to " << str << endl;
	try {
		ofstream out;
		out.exceptions(ios::failbit | ios::badbit);
		out.open(str.c_str());
		tree.printTree(out, WT_SORT_TAXA | WT_BR_LEN | WT_TAXON_ID, tree.root, dad);
		out << ";" << endl;
		out.close();
	} catch (ios::failure) {
		outError(ERR_WRITE_OUTPUT, str);
	}
}

/********************************************************
	main function
********************************************************/
int main(int argc, char *argv[])
{

/*	cout << "time_t: " << sizeof(time_t) << endl
		<< "clock_t: " << sizeof(clock_t) << endl
		<< "CLOCKS_PER_SEC: "<< CLOCKS_PER_SEC << endl;

	//test_eigen();

	srand(0);
	double a=(double)rand()/RAND_MAX, b=0, c=0;
	int maxj = 0;
	clock_t cbegin = clock();
	if (cbegin==0) { b=1.2345; maxj=1000000000;}
	//for (int i = 1; i < 100; i++)
	for (int j = 1; j < maxj; j++) {
		c+=a/b;
		//a=rand();
	}
	clock_t cend = clock();
	cout << cbegin << " a= " << a << " c= " << c << endl;
	cout << "time: " << double(cend-cbegin)/CLOCKS_PER_SEC << endl;
	return 0;*/

/*	stringstream ss("(T0:0.1,(T1:0.1,(T2:0.1,(T3:0.1,T4:0.1):0.1):0.1):0.1,T5:0.1);");
	MTree tree;
	bool is_rooted = false;
	tree.readTree(ss, is_rooted);
	tree.drawTree(cout);
	if (argc != 2) return 0;
	string tax_cov(argv[1]);
	for (string::iterator it = tax_cov.begin(); it != tax_cov.end(); it++)
		(*it) -= '0';
	MTree tree2;
	tree2.copyTree(&tree, tax_cov);
	cout << "new tree has " << tree2.leafNum << " leaves and " << tree2.nodeNum - tree2.leafNum << " internal nodes" << endl;
	tree2.drawTree(cout);
	tree2.printTree(cout);
	cout << endl;

	return 0;*/

	printCopyright(cout);

	Params params;
	parseArg(argc, argv, params);

	cout << "Running machine: ";
	cout.flush();
	system("hostname");
	cout << "Running arguments: " << endl;
	for (int i = 0; i < argc; i++)
		cout << " " << argv[i];
	cout << endl;

	cout << "Random number generator seed: " << params.ran_seed << endl << endl;
	srand(params.ran_seed);

	// call the main function
	if (params.tree_gen != NONE) {
		generateRandomTree(params);
	} else if (params.do_pars_multistate) {
		doParsMultiState(params);
	} else if (params.rf_dist_mode != 0) {
		computeRFDist(params);
	} else if (params.test_input != TEST_NONE) {
		params.intype = detectInputFile(params.user_file);
		testInputFile(params);
	} else if (params.run_mode == PRINT_TAXA) {
		printTaxa(params);
	} else if (params.run_mode == PRINT_AREA) {
		printAreaList(params);
	} else if (params.run_mode == SCALE_BRANCH_LEN || params.run_mode == SCALE_NODE_NAME) {
		scaleBranchLength(params);
	} else if (params.run_mode == PD_DISTRIBUTION) {
		calcDistribution(params);
	} else if (params.run_mode == STATS){ /**MINH ANH: for some statistics on the input tree*/
		branchStats(params); // MA
	} else if (params.branch_cluster > 0) {
		calcTreeCluster(params);
	} else if (params.ncbi_taxid) {
		processNCBITree(params);
	} else if (params.aln_file || params.partition_file) {
		if ((params.siteLL_file || params.second_align) && !params.gbo_replicates)
		{
			if (params.siteLL_file)
				guidedBootstrap(params);
			if (params.second_align)
				computeMulProb(params);
		} else
			runPhyloAnalysis(params);
	} else if (params.ngs_file || params.ngs_mapped_reads) {
		runNGSAnalysis(params);
	} else if (params.pdtaxa_file && params.gene_scale_factor >=0.0 && params.gene_pvalue_file) {
		runGSSAnalysis(params);
	} else if (params.consensus_type != CT_NONE) {
		MExtTree tree;
		switch (params.consensus_type) {
			case CT_CONSENSUS_TREE:
				computeConsensusTree(params.user_file, params.tree_burnin, params.split_threshold,
					params.split_weight_threshold, params.out_file, params.out_prefix, params.tree_weight_file);
				break;
			case CT_CONSENSUS_NETWORK:
				computeConsensusNetwork(params.user_file, params.tree_burnin, params.split_threshold,
					params.split_weight_threshold, params.out_file, params.out_prefix, params.tree_weight_file);
				break;
			case CT_ASSIGN_SUPPORT:
				assignBootstrapSupport(params.user_file, params.tree_burnin,
					params.second_tree, params.is_rooted, params.out_file,
					params.out_prefix, tree, params.tree_weight_file, &params);
				break;
			case CT_NONE: break;
			/**MINH ANH: for some comparison*/
			case COMPARE: compare(params); break; //MA
		}
	} else {
		params.intype = detectInputFile(params.user_file);
		if (params.intype == IN_NEWICK && params.pdtaxa_file && params.tree_gen == NONE) {
			if (params.budget_file) {
				//if (params.budget < 0) params.run_mode = PD_USER_SET;
			} else {
				if (params.sub_size < 1 && params.pd_proportion == 0.0)
					params.run_mode = PD_USER_SET;
			}
			// input is a tree, check if it is a reserve selection -> convert to splits
			if (params.run_mode != PD_USER_SET) params.multi_tree = true;
		}


		if (params.intype == IN_NEWICK && !params.find_all && params.budget_file == NULL &&
			params.find_pd_min == false && params.calc_pdgain == false &&
			params.run_mode != LINEAR_PROGRAMMING && params.multi_tree == false)
			runPDTree(params);
		else if (params.intype == IN_NEXUS || params.intype == IN_NEWICK) {
			if (params.run_mode == LINEAR_PROGRAMMING && params.find_pd_min)
				outError("Current linear programming does not support finding minimal PD sets!");
			if (params.find_all && params.run_mode == LINEAR_PROGRAMMING)
				params.binary_programming = true;
			runPDSplit(params);
		} else {
			outError("Unknown file input format");
		}
	}
	return EXIT_SUCCESS;
}
